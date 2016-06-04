#include <mbgl/storage/offline_download.hpp>
#include <mbgl/storage/offline_database.hpp>
#include <mbgl/storage/file_source.hpp>
#include <mbgl/storage/resource.hpp>
#include <mbgl/storage/response.hpp>
#include <mbgl/style/parser.hpp>
#include <mbgl/style/source_impl.hpp>
#include <mbgl/text/glyph.hpp>
#include <mbgl/util/tile_cover.hpp>
#include <mbgl/util/mapbox.hpp>
#include <mbgl/util/run_loop.hpp>

#include <set>

namespace mbgl {

OfflineDownload::OfflineDownload(int64_t id_,
                                 OfflineRegionDefinition&& definition_,
                                 OfflineDatabase& offlineDatabase_,
                                 FileSource& onlineFileSource_)
    : id(id_),
      definition(definition_),
      offlineDatabase(offlineDatabase_),
      onlineFileSource(onlineFileSource_) {
    setObserver(nullptr);
}

OfflineDownload::~OfflineDownload() = default;

void OfflineDownload::setObserver(std::unique_ptr<OfflineRegionObserver> observer_) {
    observer = observer_ ? std::move(observer_) : std::make_unique<OfflineRegionObserver>();
}

void OfflineDownload::setState(OfflineRegionDownloadState state) {
    if (status.downloadState == state) {
        return;
    }

    status.downloadState = state;

    if (status.downloadState == OfflineRegionDownloadState::Active) {
        activateDownload();
    } else {
        deactivateDownload();
    }

    observer->statusChanged(status);
}

std::vector<Resource> OfflineDownload::spriteResources(const style::Parser& parser) const {
    std::vector<Resource> result;

    if (!parser.spriteURL.empty()) {
        result.push_back(Resource::spriteImage(parser.spriteURL, definition.pixelRatio));
        result.push_back(Resource::spriteJSON(parser.spriteURL, definition.pixelRatio));
    }

    return result;
}

std::vector<Resource> OfflineDownload::glyphResources(const style::Parser& parser) const {
    std::vector<Resource> result;

    if (!parser.glyphURL.empty()) {
        for (const auto& fontStack : parser.fontStacks()) {
            for (uint32_t i = 0; i < 256; i++) {
                result.push_back(Resource::glyphs(parser.glyphURL, fontStack, getGlyphRange(i * 256)));
            }
        }
    }

    return result;
}

std::vector<Resource> OfflineDownload::tileResources(SourceType type, uint16_t tileSize, const Tileset& tileset) const {
    std::vector<Resource> result;

    for (const auto& tile : definition.tileCover(type, tileSize, tileset.zoomRange)) {
        result.push_back(Resource::tile(tileset.tiles[0], definition.pixelRatio, tile.x, tile.y, tile.z));
    }

    return result;
}

OfflineRegionStatus OfflineDownload::getStatus() const {
    if (status.downloadState == OfflineRegionDownloadState::Active) {
        return status;
    }

    OfflineRegionStatus result = offlineDatabase.getRegionCompletedStatus(id);

    result.requiredResourceCount++;
    optional<Response> styleResponse = offlineDatabase.get(Resource::style(definition.styleURL));
    if (!styleResponse) {
        return result;
    }

    style::Parser parser;
    parser.parse(*styleResponse->data);

    result.requiredResourceCountIsPrecise = true;

    for (const auto& source : parser.sources) {
        switch (source->baseImpl->type) {
        case SourceType::Vector:
        case SourceType::Raster:
            if (source->baseImpl->getTileset()) {
                result.requiredResourceCount += tileResources(source->baseImpl->type, source->baseImpl->tileSize, *source->baseImpl->getTileset()).size();
            } else {
                result.requiredResourceCount += 1;
                optional<Response> sourceResponse = offlineDatabase.get(Resource::source(source->baseImpl->url));
                if (sourceResponse) {
                    result.requiredResourceCount += tileResources(source->baseImpl->type, source->baseImpl->tileSize,
                        *style::parseTileJSON(*sourceResponse->data, source->baseImpl->url, source->baseImpl->type, source->baseImpl->tileSize)).size();
                } else {
                    result.requiredResourceCountIsPrecise = false;
                }
            }
            break;

        case SourceType::GeoJSON:
            if (!source->baseImpl->url.empty()) {
                result.requiredResourceCount += 1;
            }
            break;

        case SourceType::Video:
        case SourceType::Annotations:
            break;
        }
    }

    result.requiredResourceCount += spriteResources(parser).size();
    result.requiredResourceCount += glyphResources(parser).size();

    return result;
}

void OfflineDownload::activateDownload() {
    status = OfflineRegionStatus();
    status.downloadState = OfflineRegionDownloadState::Active;

    requiredSourceURLs.clear();

    ensureResource(Resource::style(definition.styleURL), [&] (Response styleResponse) {
        status.requiredResourceCountIsPrecise = true;

        style::Parser parser;
        parser.parse(*styleResponse.data);

        for (const auto& source : parser.sources) {
            SourceType type = source->baseImpl->type;
            uint16_t tileSize = source->baseImpl->tileSize;
            std::string url = source->baseImpl->url;

            switch (type) {
            case SourceType::Vector:
            case SourceType::Raster:
                if (source->baseImpl->getTileset()) {
                    ensureTiles(type, tileSize, *source->baseImpl->getTileset());
                } else {
                    status.requiredResourceCountIsPrecise = false;
                    requiredSourceURLs.insert(url);

                    ensureResource(Resource::source(url), [=] (Response sourceResponse) {
                        ensureTiles(type, tileSize, *style::parseTileJSON(*sourceResponse.data, url, type, tileSize));

                        requiredSourceURLs.erase(url);
                        if (requiredSourceURLs.empty()) {
                            status.requiredResourceCountIsPrecise = true;
                        }
                    });
                }
                break;

            case SourceType::GeoJSON:
                if (!source->baseImpl->url.empty()) {
                    ensureResource(Resource::source(source->baseImpl->url));
                }
                break;

            case SourceType::Video:
            case SourceType::Annotations:
                break;
            }
        }
        
        for (const auto& resource : spriteResources(parser)) {
            ensureResource(resource);
        }

        for (const auto& resource : glyphResources(parser)) {
            ensureResource(resource);
        }
    });
}

void OfflineDownload::deactivateDownload() {
    requests.clear();
}

void OfflineDownload::ensureTiles(SourceType type, uint16_t tileSize, const Tileset& info) {
    for (const auto& resource : tileResources(type, tileSize, info)) {
        ensureResource(resource);
    }
}

void OfflineDownload::ensureResource(const Resource& resource, std::function<void (Response)> callback) {
    status.requiredResourceCount++;

    auto workRequestsIt = requests.insert(requests.begin(), nullptr);
    *workRequestsIt = util::RunLoop::Get()->invokeCancellable([=] () {
        requests.erase(workRequestsIt);

        optional<std::pair<Response, uint64_t>> offlineResponse = offlineDatabase.getRegionResource(id, resource);
        if (offlineResponse) {
            if (callback) {
                callback(offlineResponse->first);
            }

            status.completedResourceCount++;
            status.completedResourceSize += offlineResponse->second;
            if (resource.kind == Resource::Kind::Tile) {
                status.completedTileSize += offlineResponse->second;
            }
            
            observer->statusChanged(status);
            
            if (status.complete()) {
                setState(OfflineRegionDownloadState::Inactive);
            }

            return;
        }
        
        if (checkTileCountLimit(resource)) {
            return;
        }

        auto fileRequestsIt = requests.insert(requests.begin(), nullptr);
        *fileRequestsIt = onlineFileSource.request(resource, [=] (Response onlineResponse) {
            if (onlineResponse.error) {
                observer->responseError(*onlineResponse.error);
                return;
            }

            requests.erase(fileRequestsIt);

            if (callback) {
                callback(onlineResponse);
            }

            status.completedResourceCount++;
            uint64_t resourceSize = offlineDatabase.putRegionResource(id, resource, onlineResponse);
            status.completedResourceSize += resourceSize;
            if (resource.kind == Resource::Kind::Tile) {
                status.completedTileSize += resourceSize;
            }

            observer->statusChanged(status);
            
            if (checkTileCountLimit(resource)) {
                return;
            }
            
            if (status.complete()) {
                setState(OfflineRegionDownloadState::Inactive);
            }
        });
    });
}

bool OfflineDownload::checkTileCountLimit(const Resource& resource) {
    if (resource.kind == Resource::Kind::Tile
        && util::mapbox::isMapboxURL(resource.url)
        && offlineDatabase.offlineMapboxTileCountLimitExceeded()) {
        observer->mapboxTileCountLimitExceeded(offlineDatabase.getOfflineMapboxTileCountLimit());
        setState(OfflineRegionDownloadState::Inactive);
        return true;
    }
    
    return false;
}

} // namespace mbgl
