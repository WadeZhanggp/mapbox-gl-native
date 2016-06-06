#pragma once

#include <mbgl/renderer/bucket.hpp>
#include <mbgl/tile/geometry_tile.hpp>

#include <vector>
#include <memory>

namespace mbgl {

class OutlinePatternShader;
class PlainShader;
class PatternShader;
class OutlineShader;
class FillRenderData;

class FillBucket : public Bucket {
public:
    FillBucket();
    ~FillBucket() override;

    void upload(gl::ObjectStore&) override;
    void render(Painter&, const style::Layer&, const UnwrappedTileID&, const mat4&) override;
    bool hasData() const override;
    bool needsClipping() const override;

    void addGeometry(const GeometryCollection&);

    void drawElements(PlainShader&, gl::ObjectStore&);
    void drawElements(PatternShader&, gl::ObjectStore&);
    void drawVertices(OutlineShader&, gl::ObjectStore&);
    void drawVertices(OutlinePatternShader&, gl::ObjectStore&);

private:
    std::unique_ptr<FillRenderData> renderData;
};

} // namespace mbgl
