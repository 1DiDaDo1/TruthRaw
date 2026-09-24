#include "truthnegative_dense_local_field_adapter_v0_4.h"

#include <algorithm>
#include <limits>

namespace truthraw::truthnegative_dense_local_field_adapter::v0_4 {

SourceFieldAdapter::SourceFieldAdapter(
    truthraw::streaming_v0_1::IRawTileSource& rawSource,
    float_dng::IScientificMasterTileSource& masterSource) noexcept
    : raw_(rawSource),master_(masterSource) {
    const auto& md=raw_.metadata();
    if(md.width>0&&md.height>0&&
       static_cast<std::uint64_t>(md.width)*local::kScale<=
           std::numeric_limits<std::uint32_t>::max()&&
       static_cast<std::uint64_t>(md.height)*local::kScale<=
           std::numeric_limits<std::uint32_t>::max()){
        geometry_.sourceWidth=static_cast<std::uint32_t>(md.width);
        geometry_.sourceHeight=static_cast<std::uint32_t>(md.height);
        geometry_.targetWidth=geometry_.sourceWidth*local::kScale;
        geometry_.targetHeight=geometry_.sourceHeight*local::kScale;
    }
}

local::Geometry SourceFieldAdapter::geometry() const noexcept {
    return geometry_;
}

bool SourceFieldAdapter::readSourceTile(
    std::uint32_t x,
    std::uint32_t y,
    std::uint32_t width,
    std::uint32_t height,
    field::ChannelRecord* out,
    std::size_t recordCount) noexcept {
    try{
        if(!out||width==0u||height==0u||
           geometry_.sourceWidth==0u||geometry_.sourceHeight==0u||
           x+width>geometry_.sourceWidth||y+height>geometry_.sourceHeight){
            return false;
        }
        const std::size_t pixels=static_cast<std::size_t>(width)*height;
        if(pixels>std::numeric_limits<std::size_t>::max()/3u||
           recordCount!=pixels*3u)return false;

        rgbScratch_.resize(pixels*3u);
        const auto masterStatus=master_.readCameraNativeTile(
            x,y,width,height,rgbScratch_.data(),rgbScratch_.size());
        if(!masterStatus)return false;

        rawScratch_.resize(pixels);
        const auto& md=raw_.metadata();
        if(md.hasGainField)gainScratch_.resize(pixels);else gainScratch_.clear();
        truthraw::TileRect rect{
            static_cast<int>(x),static_cast<int>(y),
            static_cast<int>(x+width),static_cast<int>(y+height),
            static_cast<int>(x),static_cast<int>(y),
            static_cast<int>(x+width),static_cast<int>(y+height)};
        const auto rawStatus=raw_.readRawTile(
            rect,
            rawScratch_.data(),rawScratch_.size(),
            md.hasGainField?gainScratch_.data():nullptr,
            md.hasGainField?gainScratch_.size():0u);
        if(!rawStatus)return false;

        if(!field::build_source_tile_records(
                md.cfa,x,y,width,height,rawScratch_,md.whiteLevel,
                rgbScratch_,fieldScratch_)||
           fieldScratch_.size()!=recordCount){
            return false;
        }
        std::copy(fieldScratch_.begin(),fieldScratch_.end(),out);
        return true;
    }catch(...){
        return false;
    }
}

bool compute_dense_local_authority_summary(
    truthraw::streaming_v0_1::IRawTileSource& rawSource,
    float_dng::IScientificMasterTileSource& masterSource,
    dense::DenseProjectionTileSource& denseSource,
    const field::Digest& projectedRasterSha256,
    std::uint32_t tileEdge,
    local::ProjectionSummary& out) noexcept {
    SourceFieldAdapter fieldSource(rawSource,masterSource);
    const auto fg=fieldSource.geometry();
    const auto dg=denseSource.geometry();
    if(fg.sourceWidth==0u||fg.sourceHeight==0u||
       fg.sourceWidth!=dg.sourceWidth||fg.sourceHeight!=dg.sourceHeight||
       fg.targetWidth!=dg.targetWidth||fg.targetHeight!=dg.targetHeight){
        out={};return false;
    }
    return local::summarize_full_projection(
        fieldSource,projectedRasterSha256,tileEdge,out);
}

const char* schema_name() noexcept {
    return "TruthNegativeDenseLocalFieldAdapter/0.4";
}

} // namespace truthraw::truthnegative_dense_local_field_adapter::v0_4
