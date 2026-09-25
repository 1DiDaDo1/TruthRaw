#include "truthnegative_n2_full_colour_candidate_v0_1.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <limits>

namespace truthraw::truthnegative_n2_full_colour_candidate::v0_1 {
namespace {

bool nonzero(const Digest& d) noexcept {
    return std::any_of(
        d.begin(),d.end(),[](std::uint8_t v){return v!=0u;});
}

void hash_u32(
    truthraw::sha256_v0_69::Hasher& h,
    std::uint32_t v) noexcept {
    const std::array<std::uint8_t,4u> b{
        static_cast<std::uint8_t>(v),
        static_cast<std::uint8_t>(v>>8u),
        static_cast<std::uint8_t>(v>>16u),
        static_cast<std::uint8_t>(v>>24u)};
    h.update(b);
}

void hash_u64(
    truthraw::sha256_v0_69::Hasher& h,
    std::uint64_t v) noexcept {
    std::array<std::uint8_t,8u> b{};
    for(std::size_t i=0u;i<8u;++i){
        b[i]=static_cast<std::uint8_t>(v>>(8u*i));
    }
    h.update(b);
}

void hash_f32(
    truthraw::sha256_v0_69::Hasher& h,
    float v) noexcept {
    hash_u32(h,std::bit_cast<std::uint32_t>(v));
}

bool finite_vector(const std::vector<float>& v) noexcept {
    return std::all_of(
        v.begin(),v.end(),
        [](float x){return std::isfinite(x);});
}

} // namespace

bool reconstruct(
    const Input& input,
    IReconstructionBackend& backend,
    Result& out) noexcept {
    out={};
    try{
        if(input.stage2==nullptr||
           input.tileWidth<=0||input.tileHeight<=0||
           input.coreWidth<=0||input.coreHeight<=0||
           input.n2Audit==nullptr){
            return false;
        }
        const auto& a=*input.n2Audit;
        if(!a.appearanceGridDerived||
           !nonzero(a.appearanceGridSha256)||
           a.samplingPeriod!=2u||
           a.regionX!=static_cast<std::uint32_t>(input.globalHx0)||
           a.regionY!=static_cast<std::uint32_t>(input.globalHy0)||
           a.regionWidth!=static_cast<std::uint32_t>(input.tileWidth)||
           a.regionHeight!=static_cast<std::uint32_t>(input.tileHeight)||
           a.appearanceGridWidth!=static_cast<std::uint32_t>(input.tileWidth)||
           a.appearanceGridHeight!=static_cast<std::uint32_t>(input.tileHeight)||
           a.appearanceGrid.size()!=
               static_cast<std::size_t>(input.tileWidth)*
               static_cast<std::size_t>(input.tileHeight)||
           a.sourceValuesModified||
           a.truthNegativeModified||
           a.createsNewEvidence||
           a.scientificWritebackAllowed){
            return false;
        }

        const std::size_t tilePixels=
            static_cast<std::size_t>(input.tileWidth)*
            static_cast<std::size_t>(input.tileHeight);
        if(tilePixels>
           std::numeric_limits<std::size_t>::max()/sizeof(float)){
            return false;
        }
        std::vector<float> candidateStage2(
            input.stage2,input.stage2+tilePixels);
        if(!finite_vector(candidateStage2))return false;

        for(std::size_t i=0u;i<tilePixels;++i){
            const auto& bin=a.appearanceGrid[i];
            std::uint32_t sampledTotal=0u;
            int measuredChannel=-1;
            for(std::size_t cc=0u;cc<3u;++cc){
                sampledTotal+=bin.sampled[cc];
                if(bin.sampled[cc]>0u){
                    if(measuredChannel!=-1)return false;
                    measuredChannel=static_cast<int>(cc);
                }
                if(bin.corrected[cc]+bin.protectedCount[cc]!=bin.sampled[cc]){
                    return false;
                }
                if(!std::isfinite(bin.correctionSum[cc]))return false;
            }
            if(sampledTotal!=1u||measuredChannel<0)return false;
            const auto cc=static_cast<std::size_t>(measuredChannel);
            if(bin.corrected[cc]>0u){
                const double correction=
                    bin.correctionSum[cc]/
                    static_cast<double>(bin.sampled[cc]);
                if(!std::isfinite(correction))return false;
                const double candidate=
                    static_cast<double>(candidateStage2[i])+correction;
                if(!std::isfinite(candidate)||
                   candidate>
                       static_cast<double>(
                           std::numeric_limits<float>::max())||
                   candidate<
                       -static_cast<double>(
                           std::numeric_limits<float>::max())){
                    return false;
                }
                candidateStage2[i]=static_cast<float>(candidate);
                ++out.correctedStage2Sites;
            }
        }

        const std::size_t corePixels=
            static_cast<std::size_t>(input.coreWidth)*
            static_cast<std::size_t>(input.coreHeight);
        if(corePixels>std::numeric_limits<std::size_t>::max()/3u)return false;
        const std::size_t rgbCount=corePixels*3u;
        out.baselineCameraRgb.resize(rgbCount);
        out.candidateCameraRgb.resize(rgbCount);

        const auto baselineStatus=backend.reconstructTile(
            input.stage2,
            input.tileWidth,
            input.tileHeight,
            input.globalHx0,
            input.globalHy0,
            input.coreX0,
            input.coreY0,
            input.coreWidth,
            input.coreHeight,
            input.cfa,
            out.baselineCameraRgb.data());
        if(!baselineStatus)return false;

        const auto candidateStatus=backend.reconstructTile(
            candidateStage2.data(),
            input.tileWidth,
            input.tileHeight,
            input.globalHx0,
            input.globalHy0,
            input.coreX0,
            input.coreY0,
            input.coreWidth,
            input.coreHeight,
            input.cfa,
            out.candidateCameraRgb.data());
        if(!candidateStatus)return false;
        if(!finite_vector(out.baselineCameraRgb)||
           !finite_vector(out.candidateCameraRgb)){
            return false;
        }

        for(std::size_t i=0u;i<rgbCount;++i){
            const double d=std::abs(
                static_cast<double>(out.candidateCameraRgb[i])-
                static_cast<double>(out.baselineCameraRgb[i]));
            if(!std::isfinite(d))return false;
            if(d>0.0)++out.changedRgbChannels;
            out.maxAbsRgbDelta=std::max(out.maxAbsRgbDelta,d);
        }

        truthraw::sha256_v0_69::Hasher hasher;
        constexpr char domain[]=
            "D_RAW_TN_N2_FULL_COLOUR_CANDIDATE_V0_1";
        hasher.update(
            reinterpret_cast<const std::uint8_t*>(domain),
            sizeof(domain)-1u);
        hasher.update(a.appearanceGridSha256);
        hash_u32(hasher,static_cast<std::uint32_t>(input.globalHx0));
        hash_u32(hasher,static_cast<std::uint32_t>(input.globalHy0));
        hash_u32(hasher,static_cast<std::uint32_t>(input.tileWidth));
        hash_u32(hasher,static_cast<std::uint32_t>(input.tileHeight));
        hash_u32(hasher,static_cast<std::uint32_t>(input.coreX0));
        hash_u32(hasher,static_cast<std::uint32_t>(input.coreY0));
        hash_u32(hasher,static_cast<std::uint32_t>(input.coreWidth));
        hash_u32(hasher,static_cast<std::uint32_t>(input.coreHeight));
        hash_u64(hasher,out.correctedStage2Sites);
        hash_u64(hasher,out.changedRgbChannels);
        for(float v:out.candidateCameraRgb)hash_f32(hasher,v);
        out.candidateIdentitySha256=hasher.finalize();

        out.sourceStage2Modified=false;
        out.scientificWritebackAllowed=false;
        out.createsNewEvidence=false;
        return nonzero(out.candidateIdentitySha256);
    }catch(...){
        out={};
        return false;
    }
}

} // namespace truthraw::truthnegative_n2_full_colour_candidate::v0_1
