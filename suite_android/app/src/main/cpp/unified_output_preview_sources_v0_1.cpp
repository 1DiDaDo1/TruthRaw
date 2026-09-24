#include "unified_output_preview_sources_v0_1.h"

#include "full_frame_streaming_v0_1_internal.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <new>

namespace truthraw::unified_output_preview_sources::v0_1 {

struct RandomAccessScientificMasterSource::Impl final {
    streaming_v0_1::detail::Workspace workspace{};
};

RandomAccessScientificMasterSource::RandomAccessScientificMasterSource(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction) noexcept
    : source_(source),
      reconstruction_(reconstruction),
      impl_(new (std::nothrow) Impl{}) {}

RandomAccessScientificMasterSource::~RandomAccessScientificMasterSource() = default;

std::size_t RandomAccessScientificMasterSource::residentBytesUpperBound() const noexcept {
    std::size_t total=source_.residentBytesUpperBound();
    if(!impl_) return total;
    const auto workspace=
        streaming_v0_1::detail::vector_bytes(impl_->workspace);
    if(total>std::numeric_limits<std::size_t>::max()-workspace)
        return std::numeric_limits<std::size_t>::max();
    return total+workspace;
}

float_dng::Status RandomAccessScientificMasterSource::readCameraNativeTile(
    std::uint32_t x,
    std::uint32_t y,
    std::uint32_t width,
    std::uint32_t height,
    float* rgb,
    std::size_t floatCount) noexcept {
    try{
        if(!impl_||rgb==nullptr||width==0u||height==0u)
            return float_dng::Status::error(
                float_dng::StatusCode::InvalidArgument,
                "invalid random-access Scientific Master request");

        const auto& m=source_.metadata();
        if(m.width<=1||m.height<=1||
           x>=static_cast<std::uint32_t>(m.width)||
           y>=static_cast<std::uint32_t>(m.height)||
           width>static_cast<std::uint32_t>(m.width)-x||
           height>static_cast<std::uint32_t>(m.height)-y)
            return float_dng::Status::error(
                float_dng::StatusCode::InvalidArgument,
                "random-access Scientific Master request outside source");

        const std::size_t pixels=
            static_cast<std::size_t>(width)*height;
        if(pixels>std::numeric_limits<std::size_t>::max()/3u||
           floatCount!=pixels*3u)
            return float_dng::Status::error(
                float_dng::StatusCode::InvalidArgument,
                "random-access Scientific Master output size mismatch");

        const int halo=reconstruction_.requiredHalo();
        if(halo<0)
            return float_dng::Status::error(
                float_dng::StatusCode::InvalidArgument,
                "reconstruction backend returned negative halo");

        TileRect tile{};
        tile.x0=static_cast<int>(x);
        tile.y0=static_cast<int>(y);
        tile.x1=static_cast<int>(x+width);
        tile.y1=static_cast<int>(y+height);
        tile.hx0=std::max(0,tile.x0-halo);
        tile.hy0=std::max(0,tile.y0-halo);
        tile.hx1=std::min(m.width,tile.x1+halo);
        tile.hy1=std::min(m.height,tile.y1+halo);

        auto& workspace=impl_->workspace;
        const auto filled=
            streaming_v0_1::detail::fill_stage2(
                source_,
                tile,
                workspace);
        if(!filled)
            return float_dng::Status::error(
                float_dng::StatusCode::SourceFailed,
                "preview Stage-2 source read failed: "+filled.message);

        workspace.cam.resize(floatCount);
        const int tileWidth=tile.hx1-tile.hx0;
        const int tileHeight=tile.hy1-tile.hy0;
        const auto reconstructed=reconstruction_.reconstructTile(
            workspace.stage2.data(),
            tileWidth,
            tileHeight,
            tile.hx0,
            tile.hy0,
            tile.x0,
            tile.y0,
            static_cast<int>(width),
            static_cast<int>(height),
            m.cfa,
            workspace.cam.data());
        if(!reconstructed)
            return float_dng::Status::error(
                float_dng::StatusCode::SourceFailed,
                "preview camera-native reconstruction failed: "+
                    reconstructed.message);

        std::copy(workspace.cam.begin(),workspace.cam.end(),rgb);
        return float_dng::Status::ok();
    }catch(const std::bad_alloc&){
        return float_dng::Status::error(
            float_dng::StatusCode::SizeOverflow,
            "preview random-access Scientific Master allocation failed");
    }catch(...){
        return float_dng::Status::error(
            float_dng::StatusCode::SourceFailed,
            "unexpected preview random-access Scientific Master failure");
    }
}

float_dng::Status BoundedU16PrimarySource::readCameraNativeTile(
    std::uint32_t x,
    std::uint32_t y,
    std::uint32_t width,
    std::uint32_t height,
    float* rgb,
    std::size_t floatCount) noexcept {
    const auto status=
        source_.readCameraNativeTile(
            x,y,width,height,rgb,floatCount);
    if(!status) return status;

    for(std::size_t i=0u;i<floatCount;++i){
        const float value=rgb[i];
        if(!std::isfinite(value))
            return float_dng::Status::error(
                float_dng::StatusCode::SourceFailed,
                "bounded U16 preview source received non-finite value");
        std::uint32_t q=0u;
        if(value<=0.0f){
            q=0u;
        }else if(value>=1.0f){
            q=65535u;
        }else{
            const double scaled=static_cast<double>(value)*65535.0;
            q=static_cast<std::uint32_t>(
                std::floor(scaled+0.5));
            q=std::min<std::uint32_t>(q,65535u);
        }
        rgb[i]=static_cast<float>(
            static_cast<double>(q)/65535.0);
    }
    return float_dng::Status::ok();
}

const char* schema_name() noexcept {
    return "TruthRawUnifiedOutputPreviewSources/0.1";
}

} // namespace truthraw::unified_output_preview_sources::v0_1
