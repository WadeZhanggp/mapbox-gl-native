#pragma once

#include <mbgl/util/raster.hpp>

namespace mbgl {

class RasterRenderData {
public:
    inline RasterRenderData(gl::TexturePool& texturePool) : raster(texturePool) {
    }

    Raster raster;
};

} // namespace mbgl
