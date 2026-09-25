#include "truthnegative_n2_risk_quality_audit_v0_1.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <limits>
#include <vector>

namespace truthraw::truthnegative_n2_risk_quality_audit::v0_1 {
namespace {

bool nonzero(const Digest& d) noexcept {
    return std::any_of(
        d.begin(),d.end(),[](std::uint8_t v){return v!=0u;});
}

double quantile_sorted(
    const std::vector<double>& sorted,
    double q) noexcept {
    if(sorted.empty())return 0.0;
    if(sorted.size()==1u)return sorted.front();
    const double pos=
        std::clamp(q,0.0,1.0)*
        static_cast<double>(sorted.size()-1u);
    const auto lo=static_cast<std::size_t>(std::floor(pos));
    const auto hi=static_cast<std::size_t>(std::ceil(pos));
    const double t=pos-static_cast<double>(lo);
    return sorted[lo]*(1.0-t)+sorted[hi]*t;
}

bool summarize(
    std::vector<double> values,
    Quantiles& out) noexcept {
    if(values.empty())return false;
    double sum=0.0;
    for(double v:values){
        if(!std::isfinite(v)||v<0.0)return false;
        sum+=v;
        if(!std::isfinite(sum))return false;
    }
    std::sort(values.begin(),values.end());
    out.mean=sum/static_cast<double>(values.size());
    out.p50=quantile_sorted(values,0.50);
    out.p95=quantile_sorted(values,0.95);
    out.p99=quantile_sorted(values,0.99);
    out.max=values.back();
    return std::isfinite(out.mean)&&std::isfinite(out.max);
}

double luma(const double* rgb) noexcept {
    return 0.2126*rgb[0]+0.7152*rgb[1]+0.0722*rgb[2];
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

void hash_f64(
    truthraw::sha256_v0_69::Hasher& h,
    double v) noexcept {
    hash_u64(h,std::bit_cast<std::uint64_t>(v));
}

void hash_quantiles(
    truthraw::sha256_v0_69::Hasher& h,
    const Quantiles& q) noexcept {
    hash_f64(h,q.mean);
    hash_f64(h,q.p50);
    hash_f64(h,q.p95);
    hash_f64(h,q.p99);
    hash_f64(h,q.max);
}

double nearest_distance(
    std::uint32_t x,
    std::uint32_t y,
    std::uint32_t width,
    std::uint32_t height,
    const std::uint32_t* mask,
    std::uint32_t reasonBits) noexcept {
    double best2=std::numeric_limits<double>::infinity();
    for(std::uint32_t yy=0u;yy<height;++yy){
        for(std::uint32_t xx=0u;xx<width;++xx){
            const std::size_t i=
                static_cast<std::size_t>(yy)*width+xx;
            if((mask[i]&reasonBits)==0u)continue;
            const double dx=
                static_cast<double>(xx)-static_cast<double>(x);
            const double dy=
                static_cast<double>(yy)-static_cast<double>(y);
            best2=std::min(best2,dx*dx+dy*dy);
        }
    }
    return std::isfinite(best2)?std::sqrt(best2):-1.0;
}

} // namespace

bool evaluate(const Input& input,Result& out) noexcept {
    out={};
    try{
        if(input.aEncodedRgb==nullptr||
           input.bEncodedRgb==nullptr||
           input.preserveReasonMask==nullptr||
           input.width<2u||input.height<2u||
           !nonzero(input.candidateIdentitySha256)){
            return false;
        }
        const std::uint64_t pixelCount64=
            static_cast<std::uint64_t>(input.width)*input.height;
        if(pixelCount64==0u||
           pixelCount64>
               static_cast<std::uint64_t>(
                   std::numeric_limits<std::size_t>::max()/3u)){
            return false;
        }
        const std::size_t pixelCount=
            static_cast<std::size_t>(pixelCount64);

        std::vector<double> pixelDelta;
        std::array<std::vector<double>,3u> channelDelta;
        std::vector<double> lumaDelta;
        std::vector<double> chromaDelta;
        pixelDelta.reserve(pixelCount);
        lumaDelta.reserve(pixelCount);
        chromaDelta.reserve(pixelCount);
        for(auto& v:channelDelta)v.reserve(pixelCount);

        const auto structureBit=
            1u<<static_cast<std::uint32_t>(n2::PreserveReason::Structure);
        const auto censoredBit=
            1u<<static_cast<std::uint32_t>(n2::PreserveReason::Censored);
        const auto boundaryBit=
            1u<<static_cast<std::uint32_t>(n2::PreserveReason::CensorBoundary);
        const auto censorBits=censoredBit|boundaryBit;

        double maxPixel=-1.0;
        std::size_t maxIndex=0u;
        std::vector<double> lumaA(pixelCount,0.0);
        std::vector<double> lumaB(pixelCount,0.0);

        for(std::size_t i=0u;i<pixelCount;++i){
            const double* a=input.aEncodedRgb+i*3u;
            const double* b=input.bEncodedRgb+i*3u;
            std::array<double,3u> signedDelta{};
            double pixelMax=0.0;
            for(std::size_t cc=0u;cc<3u;++cc){
                if(!std::isfinite(a[cc])||!std::isfinite(b[cc])){
                    return false;
                }
                signedDelta[cc]=b[cc]-a[cc];
                const double d=std::abs(signedDelta[cc]);
                channelDelta[cc].push_back(d);
                pixelMax=std::max(pixelMax,d);
            }
            pixelDelta.push_back(pixelMax);
            if(pixelMax>0.0)++out.changedPixels;
            if(pixelMax>maxPixel){
                maxPixel=pixelMax;
                maxIndex=i;
            }

            lumaA[i]=luma(a);
            lumaB[i]=luma(b);
            if(!std::isfinite(lumaA[i])||!std::isfinite(lumaB[i])){
                return false;
            }
            const double dy=std::abs(lumaB[i]-lumaA[i]);
            lumaDelta.push_back(dy);

            const double signedLuma=
                0.2126*signedDelta[0]+
                0.7152*signedDelta[1]+
                0.0722*signedDelta[2];
            const double cr=signedDelta[0]-signedLuma;
            const double cg=signedDelta[1]-signedLuma;
            const double cb=signedDelta[2]-signedLuma;
            const double chroma=
                std::sqrt((cr*cr+cg*cg+cb*cb)/3.0);
            if(!std::isfinite(chroma))return false;
            chromaDelta.push_back(chroma);

            if((input.preserveReasonMask[i]&structureBit)!=0u){
                ++out.structureMaskPixels;
            }
            if((input.preserveReasonMask[i]&censorBits)!=0u){
                ++out.censorMaskPixels;
            }
        }

        if(!summarize(pixelDelta,out.pixelMaxAbsDelta)||
           !summarize(lumaDelta,out.lumaAbsDelta)||
           !summarize(chromaDelta,out.chromaDelta)){
            return false;
        }
        for(std::size_t cc=0u;cc<3u;++cc){
            if(!summarize(channelDelta[cc],out.channelAbsDelta[cc])){
                return false;
            }
        }

        out.maxLocalX=static_cast<std::uint32_t>(
            maxIndex%input.width);
        out.maxLocalY=static_cast<std::uint32_t>(
            maxIndex/input.width);
        out.maxSourceX=input.sourceX+out.maxLocalX;
        out.maxSourceY=input.sourceY+out.maxLocalY;
        out.maxPreserveReasonMask=input.preserveReasonMask[maxIndex];
        out.distanceToStructurePx=nearest_distance(
            out.maxLocalX,out.maxLocalY,input.width,input.height,
            input.preserveReasonMask,structureBit);
        out.distanceToCensorOrBoundaryPx=nearest_distance(
            out.maxLocalX,out.maxLocalY,input.width,input.height,
            input.preserveReasonMask,censorBits);

        double edgeA=0.0;
        double edgeB=0.0;
        double edgeDiff=0.0;
        std::uint64_t edgeSamples=0u;
        for(std::uint32_t y=0u;y+1u<input.height;++y){
            for(std::uint32_t x=0u;x+1u<input.width;++x){
                const std::size_t i=
                    static_cast<std::size_t>(y)*input.width+x;
                const double ax=lumaA[i+1u]-lumaA[i];
                const double ay=
                    lumaA[i+input.width]-lumaA[i];
                const double bx=lumaB[i+1u]-lumaB[i];
                const double by=
                    lumaB[i+input.width]-lumaB[i];
                const double ga=std::hypot(ax,ay);
                const double gb=std::hypot(bx,by);
                if(!std::isfinite(ga)||!std::isfinite(gb))return false;
                edgeA+=ga;
                edgeB+=gb;
                edgeDiff+=std::abs(gb-ga);
                ++edgeSamples;
            }
        }
        if(edgeSamples==0u||
           !std::isfinite(edgeA)||
           !std::isfinite(edgeB)||
           !std::isfinite(edgeDiff)){
            return false;
        }
        out.edgeEnergyA=edgeA;
        out.edgeEnergyB=edgeB;
        out.edgeEnergyRatio=edgeA>0.0?edgeB/edgeA:1.0;
        out.meanAbsGradientDelta=
            edgeDiff/static_cast<double>(edgeSamples);
        if(!std::isfinite(out.edgeEnergyRatio)||
           !std::isfinite(out.meanAbsGradientDelta)){
            return false;
        }

        truthraw::sha256_v0_69::Hasher hasher;
        constexpr char domain[]=
            "D_RAW_TN_N2_RISK_QUALITY_AUDIT_V0_1";
        hasher.update(
            reinterpret_cast<const std::uint8_t*>(domain),
            sizeof(domain)-1u);
        hasher.update(input.candidateIdentitySha256);
        hash_u32(hasher,input.width);
        hash_u32(hasher,input.height);
        hash_u32(hasher,input.sourceX);
        hash_u32(hasher,input.sourceY);
        for(std::size_t i=0u;i<pixelCount*3u;++i){
            hash_f64(hasher,input.aEncodedRgb[i]);
            hash_f64(hasher,input.bEncodedRgb[i]);
        }
        for(std::size_t i=0u;i<pixelCount;++i){
            hash_u32(hasher,input.preserveReasonMask[i]);
        }
        hash_quantiles(hasher,out.pixelMaxAbsDelta);
        for(const auto& q:out.channelAbsDelta)hash_quantiles(hasher,q);
        hash_quantiles(hasher,out.lumaAbsDelta);
        hash_quantiles(hasher,out.chromaDelta);
        hash_u32(hasher,out.maxSourceX);
        hash_u32(hasher,out.maxSourceY);
        hash_u32(hasher,out.maxPreserveReasonMask);
        hash_f64(hasher,out.distanceToStructurePx);
        hash_f64(hasher,out.distanceToCensorOrBoundaryPx);
        hash_f64(hasher,out.edgeEnergyA);
        hash_f64(hasher,out.edgeEnergyB);
        hash_f64(hasher,out.edgeEnergyRatio);
        hash_f64(hasher,out.meanAbsGradientDelta);
        out.qualitySha256=hasher.finalize();
        out.createsNewEvidence=false;
        out.scientificWritebackAllowed=false;
        return nonzero(out.qualitySha256);
    }catch(...){
        out={};
        return false;
    }
}

} // namespace truthraw::truthnegative_n2_risk_quality_audit::v0_1
