#include "truthnegative_center_excluded_spatial_audit_v0_2_1.h"

#include "full_frame_streaming_v0_1_internal.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <utility>

namespace truthraw::truthnegative_center_excluded_spatial_audit::v0_2_1 {
namespace {

namespace detail = truthraw::streaming_v0_1::detail;

constexpr std::array<int,3u> kRadii{2,4,8};
constexpr std::array<std::array<int,2u>,4u> kDirections{{
    {{1,0}},{{0,1}},{{1,1}},{{1,-1}}
}};
constexpr int kMaxRadius = 8;

bool nonzero(const Digest& d) noexcept {
    return std::any_of(
        d.begin(),d.end(),[](std::uint8_t v){return v!=0u;});
}

std::string hex(const Digest& d) {
    static constexpr char kHex[]="0123456789abcdef";
    std::string out(d.size()*2u,'0');
    for(std::size_t i=0u;i<d.size();++i){
        out[2u*i]=kHex[d[i]>>4u];
        out[2u*i+1u]=kHex[d[i]&0x0fu];
    }
    return out;
}

void hash_u32(truthraw::sha256_v0_69::Hasher& h,std::uint32_t v) noexcept {
    const std::array<std::uint8_t,4u> b{
        static_cast<std::uint8_t>(v),
        static_cast<std::uint8_t>(v>>8u),
        static_cast<std::uint8_t>(v>>16u),
        static_cast<std::uint8_t>(v>>24u)};
    h.update(b);
}

void hash_u64(truthraw::sha256_v0_69::Hasher& h,std::uint64_t v) noexcept {
    std::array<std::uint8_t,8u> b{};
    for(std::size_t i=0u;i<8u;++i){
        b[i]=static_cast<std::uint8_t>(v>>(8u*i));
    }
    h.update(b);
}

void hash_f64(truthraw::sha256_v0_69::Hasher& h,double v) noexcept {
    hash_u64(h,std::bit_cast<std::uint64_t>(v));
}

bool same_f64(double a,double b) noexcept {
    return std::bit_cast<std::uint64_t>(a)==
           std::bit_cast<std::uint64_t>(b);
}

bool same_audit(
    const v01::n2::Audit& a,
    const v01::n2::Audit& b) noexcept {
    return
        a.total==b.total &&
        a.preserved==b.preserved &&
        a.eligible==b.eligible &&
        a.corrected==b.corrected &&
        a.censoredProtected==b.censoredProtected &&
        a.censorBoundaryProtected==b.censorBoundaryProtected &&
        a.unknownNoiseProtected==b.unknownNoiseProtected &&
        a.nonMeasuredProtected==b.nonMeasuredProtected &&
        a.weakRegistrationProtected==b.weakRegistrationProtected &&
        a.structureProtected==b.structureProtected &&
        a.noNeighborhoodProtected==b.noNeighborhoodProtected &&
        a.residualOutlierProtected==b.residualOutlierProtected &&
        same_f64(a.totalResidualEnergy,b.totalResidualEnergy) &&
        same_f64(a.removedResidualEnergy,b.removedResidualEnergy) &&
        same_f64(a.maxAbsCorrection,b.maxAbsCorrection) &&
        a.createsNewEvidence==b.createsNewEvidence &&
        a.scientificWritebackAllowed==b.scientificWritebackAllowed;
}

int measured_channel(CfaPattern cfa,int x,int y) noexcept {
    const int phase=(y&1)*2+(x&1);
    static constexpr int bggr[4]={2,1,1,0};
    static constexpr int rggb[4]={0,1,1,2};
    static constexpr int grbg[4]={1,0,2,1};
    static constexpr int gbrg[4]={1,2,0,1};
    const int* map=bggr;
    switch(cfa){
        case CfaPattern::RGGB:map=rggb;break;
        case CfaPattern::GRBG:map=grbg;break;
        case CfaPattern::GBRG:map=gbrg;break;
        case CfaPattern::BGGR:map=bggr;break;
    }
    return map[phase];
}

bool variance_for(
    const DngMetadata& md,
    const detail::Workspace& workspace,
    std::size_t i,
    int channel,
    double mu,
    double& variance) noexcept {
    variance=0.0;
    if(!md.hasNoiseProfile||channel<0||channel>2||
       i>=workspace.stage2.size()||!std::isfinite(mu)){
        return false;
    }
    const double gain=md.hasGainField
        ? (i<workspace.gain.size()
            ? static_cast<double>(workspace.gain[i])
            : std::numeric_limits<double>::quiet_NaN())
        : 1.0;
    if(!std::isfinite(gain)||!(gain>0.0))return false;
    const double shot=static_cast<double>(md.noiseProfile[2*channel]);
    const double read=static_cast<double>(md.noiseProfile[2*channel+1]);
    if(!std::isfinite(shot)||!std::isfinite(read)||
       shot<0.0||read<0.0){
        return false;
    }
    variance=gain*shot*std::max(mu,0.0)+gain*gain*read;
    return std::isfinite(variance)&&variance>0.0;
}

void add_counted_result(
    Metrics& m,
    const ce::Result& result) noexcept {
    m.symmetricPairsConsidered+=result.symmetricPairsConsidered;
    m.symmetricPairsAccepted+=result.symmetricPairsAccepted;
    m.symmetricPairsRejected+=result.symmetricPairsRejected;
    m.scalesConsidered+=result.scalesConsidered;
    m.scalesAccepted+=result.scalesAccepted;
    m.scalesRejected+=result.scalesRejected;
    m.maxDirectionalDisagreementSigma=std::max(
        m.maxDirectionalDisagreementSigma,
        result.maxDirectionalDisagreementSigma);
    m.maxCrossScaleDisagreementSigma=std::max(
        m.maxCrossScaleDisagreementSigma,
        result.maxCrossScaleDisagreementSigma);
}

void add_valid_result(
    Metrics& m,
    double absResidual,
    double centerVariance,
    double estimateVariance,
    double centerZ,
    double combinedZ,
    std::size_t phase) noexcept {
    ++m.predictorValid;
    ++m.predictorValidCfaPhase[phase];

    m.absResidualSum+=absResidual;
    m.maxAbsResidual=std::max(m.maxAbsResidual,absResidual);
    m.centerVarianceSum+=centerVariance;
    m.estimateVarianceSum+=estimateVariance;

    const double ratio=estimateVariance/centerVariance;
    m.estimateToCenterVarianceRatioSum+=ratio;
    m.maxEstimateToCenterVarianceRatio=std::max(
        m.maxEstimateToCenterVarianceRatio,ratio);

    if(centerZ<=1.0){
        ++m.centerResidualWithin1Sigma;
    }else if(centerZ<=2.0){
        ++m.centerResidualBetween1And2Sigma;
    }else{
        ++m.centerResidualAbove2Sigma;
    }

    if(combinedZ<=1.0){
        ++m.combinedResidualWithin1Sigma;
    }else if(combinedZ<=2.0){
        ++m.combinedResidualBetween1And2Sigma;
    }else{
        ++m.combinedResidualAbove2Sigma;
    }
}

void accumulate_metrics(Metrics& dst,const Metrics& src) noexcept {
    dst.sampled+=src.sampled;
    dst.v01CandidateCenters+=src.v01CandidateCenters;
    dst.predictorValid+=src.predictorValid;
    dst.predictorInvalid+=src.predictorInvalid;

    dst.symmetricPairsConsidered+=src.symmetricPairsConsidered;
    dst.symmetricPairsAccepted+=src.symmetricPairsAccepted;
    dst.symmetricPairsRejected+=src.symmetricPairsRejected;

    dst.scalesConsidered+=src.scalesConsidered;
    dst.scalesAccepted+=src.scalesAccepted;
    dst.scalesRejected+=src.scalesRejected;

    dst.centerResidualWithin1Sigma+=src.centerResidualWithin1Sigma;
    dst.centerResidualBetween1And2Sigma+=src.centerResidualBetween1And2Sigma;
    dst.centerResidualAbove2Sigma+=src.centerResidualAbove2Sigma;

    dst.combinedResidualWithin1Sigma+=src.combinedResidualWithin1Sigma;
    dst.combinedResidualBetween1And2Sigma+=src.combinedResidualBetween1And2Sigma;
    dst.combinedResidualAbove2Sigma+=src.combinedResidualAbove2Sigma;

    dst.absResidualSum+=src.absResidualSum;
    dst.maxAbsResidual=std::max(dst.maxAbsResidual,src.maxAbsResidual);
    dst.centerVarianceSum+=src.centerVarianceSum;
    dst.estimateVarianceSum+=src.estimateVarianceSum;
    dst.estimateToCenterVarianceRatioSum+=
        src.estimateToCenterVarianceRatioSum;
    dst.maxEstimateToCenterVarianceRatio=std::max(
        dst.maxEstimateToCenterVarianceRatio,
        src.maxEstimateToCenterVarianceRatio);
    dst.maxDirectionalDisagreementSigma=std::max(
        dst.maxDirectionalDisagreementSigma,
        src.maxDirectionalDisagreementSigma);
    dst.maxCrossScaleDisagreementSigma=std::max(
        dst.maxCrossScaleDisagreementSigma,
        src.maxCrossScaleDisagreementSigma);

    for(std::size_t i=0u;i<4u;++i){
        dst.v01CandidateCfaPhase[i]+=src.v01CandidateCfaPhase[i];
        dst.predictorValidCfaPhase[i]+=src.predictorValidCfaPhase[i];
    }
}

bool metrics_consistent(const Metrics& m) noexcept {
    const auto centerZ=
        m.centerResidualWithin1Sigma+
        m.centerResidualBetween1And2Sigma+
        m.centerResidualAbove2Sigma;
    const auto combinedZ=
        m.combinedResidualWithin1Sigma+
        m.combinedResidualBetween1And2Sigma+
        m.combinedResidualAbove2Sigma;
    std::uint64_t candidatePhases=0u;
    std::uint64_t validPhases=0u;
    for(std::size_t i=0u;i<4u;++i){
        candidatePhases+=m.v01CandidateCfaPhase[i];
        validPhases+=m.predictorValidCfaPhase[i];
    }
    return
        m.v01CandidateCenters<=m.sampled &&
        m.predictorValid+m.predictorInvalid==m.v01CandidateCenters &&
        centerZ==m.predictorValid &&
        combinedZ==m.predictorValid &&
        candidatePhases==m.v01CandidateCenters &&
        validPhases==m.predictorValid &&
        m.symmetricPairsAccepted+m.symmetricPairsRejected<=
            m.symmetricPairsConsidered &&
        m.scalesAccepted+m.scalesRejected==m.scalesConsidered &&
        std::isfinite(m.absResidualSum) &&
        std::isfinite(m.maxAbsResidual) &&
        std::isfinite(m.centerVarianceSum) &&
        std::isfinite(m.estimateVarianceSum) &&
        std::isfinite(m.estimateToCenterVarianceRatioSum) &&
        std::isfinite(m.maxEstimateToCenterVarianceRatio) &&
        std::isfinite(m.maxDirectionalDisagreementSigma) &&
        std::isfinite(m.maxCrossScaleDisagreementSigma) &&
        m.absResidualSum>=0.0 &&
        m.maxAbsResidual>=0.0 &&
        m.centerVarianceSum>=0.0 &&
        m.estimateVarianceSum>=0.0 &&
        m.estimateToCenterVarianceRatioSum>=0.0 &&
        m.maxEstimateToCenterVarianceRatio>=0.0;
}

void hash_metrics(
    truthraw::sha256_v0_69::Hasher& h,
    const Metrics& m) noexcept {
    hash_u64(h,m.sampled);
    hash_u64(h,m.v01CandidateCenters);
    hash_u64(h,m.predictorValid);
    hash_u64(h,m.predictorInvalid);
    hash_u64(h,m.symmetricPairsConsidered);
    hash_u64(h,m.symmetricPairsAccepted);
    hash_u64(h,m.symmetricPairsRejected);
    hash_u64(h,m.scalesConsidered);
    hash_u64(h,m.scalesAccepted);
    hash_u64(h,m.scalesRejected);
    hash_u64(h,m.centerResidualWithin1Sigma);
    hash_u64(h,m.centerResidualBetween1And2Sigma);
    hash_u64(h,m.centerResidualAbove2Sigma);
    hash_u64(h,m.combinedResidualWithin1Sigma);
    hash_u64(h,m.combinedResidualBetween1And2Sigma);
    hash_u64(h,m.combinedResidualAbove2Sigma);
    hash_f64(h,m.absResidualSum);
    hash_f64(h,m.maxAbsResidual);
    hash_f64(h,m.centerVarianceSum);
    hash_f64(h,m.estimateVarianceSum);
    hash_f64(h,m.estimateToCenterVarianceRatioSum);
    hash_f64(h,m.maxEstimateToCenterVarianceRatio);
    hash_f64(h,m.maxDirectionalDisagreementSigma);
    hash_f64(h,m.maxCrossScaleDisagreementSigma);
    for(auto v:m.v01CandidateCfaPhase)hash_u64(h,v);
    for(auto v:m.predictorValidCfaPhase)hash_u64(h,v);
}

double safe_mean(double sum,std::uint64_t n) noexcept {
    return n>0u ? sum/static_cast<double>(n) : 0.0;
}

void write_metrics(std::ostringstream& o,const Metrics& m) {
    o<<"\"sampled\":"<<m.sampled;
    o<<",\"v01_candidate_centers\":"<<m.v01CandidateCenters;
    o<<",\"predictor_valid\":"<<m.predictorValid;
    o<<",\"predictor_invalid\":"<<m.predictorInvalid;
    o<<",\"pairs_considered\":"<<m.symmetricPairsConsidered;
    o<<",\"pairs_accepted\":"<<m.symmetricPairsAccepted;
    o<<",\"pairs_rejected\":"<<m.symmetricPairsRejected;
    o<<",\"scales_considered\":"<<m.scalesConsidered;
    o<<",\"scales_accepted\":"<<m.scalesAccepted;
    o<<",\"scales_rejected\":"<<m.scalesRejected;
    o<<",\"center_z_le_1\":"<<m.centerResidualWithin1Sigma;
    o<<",\"center_z_1_to_2\":"<<m.centerResidualBetween1And2Sigma;
    o<<",\"center_z_gt_2\":"<<m.centerResidualAbove2Sigma;
    o<<",\"combined_z_le_1\":"<<m.combinedResidualWithin1Sigma;
    o<<",\"combined_z_1_to_2\":"<<m.combinedResidualBetween1And2Sigma;
    o<<",\"combined_z_gt_2\":"<<m.combinedResidualAbove2Sigma;
    o<<",\"mean_abs_residual\":"
     <<safe_mean(m.absResidualSum,m.predictorValid);
    o<<",\"max_abs_residual\":"<<m.maxAbsResidual;
    o<<",\"mean_center_variance\":"
     <<safe_mean(m.centerVarianceSum,m.predictorValid);
    o<<",\"mean_estimate_variance\":"
     <<safe_mean(m.estimateVarianceSum,m.predictorValid);
    o<<",\"mean_estimate_to_center_variance_ratio\":"
     <<safe_mean(
           m.estimateToCenterVarianceRatioSum,
           m.predictorValid);
    o<<",\"max_estimate_to_center_variance_ratio\":"
     <<m.maxEstimateToCenterVarianceRatio;
    o<<",\"max_directional_disagreement_sigma\":"
     <<m.maxDirectionalDisagreementSigma;
    o<<",\"max_cross_scale_disagreement_sigma\":"
     <<m.maxCrossScaleDisagreementSigma;
    o<<",\"v01_candidate_cfa_phase\":["
     <<m.v01CandidateCfaPhase[0]<<","
     <<m.v01CandidateCfaPhase[1]<<","
     <<m.v01CandidateCfaPhase[2]<<","
     <<m.v01CandidateCfaPhase[3]<<"]";
    o<<",\"predictor_valid_cfa_phase\":["
     <<m.predictorValidCfaPhase[0]<<","
     <<m.predictorValidCfaPhase[1]<<","
     <<m.predictorValidCfaPhase[2]<<","
     <<m.predictorValidCfaPhase[3]<<"]";
}

} // namespace

bool run(
    stream::IRawTileSource& source,
    const Binding& binding,
    const v01::Result& referenceV01,
    Result& out) noexcept {
    out={};
    try{
        const auto& md=source.metadata();
        if(md.width<=0||md.height<=0||
           !nonzero(binding.sourceEvidenceSha256)||
           !nonzero(binding.scientificMasterSha256)||
           !nonzero(binding.authorityFieldSha256)||
           !nonzero(binding.truthNegativeStateSha256)||
           !nonzero(binding.v01CandidateSha256)||
           !nonzero(binding.v01AuditSha256)||
           !nonzero(binding.v01SpatialSha256)||
           binding.v01CandidateSha256!=referenceV01.candidateSha256||
           binding.v01AuditSha256!=referenceV01.auditSha256||
           binding.v01SpatialSha256!=referenceV01.spatialSha256||
           referenceV01.sourceValuesModified||
           referenceV01.truthNegativeModified||
           referenceV01.createsNewEvidence||
           referenceV01.scientificWritebackAllowed||
           referenceV01.tiles.empty()||
           referenceV01.sampled==0u||
           referenceV01.tileEdge<8u||
           referenceV01.samplingPeriod<2u||
           (referenceV01.samplingPeriod&1u)!=0u||
           referenceV01.regionX!=0u||
           referenceV01.regionY!=0u||
           referenceV01.regionWidth!=static_cast<std::uint32_t>(md.width)||
           referenceV01.regionHeight!=static_cast<std::uint32_t>(md.height)){
            return false;
        }

        out.tileEdge=referenceV01.tileEdge;
        out.samplingPeriod=referenceV01.samplingPeriod;
        out.tiles.reserve(referenceV01.tiles.size());

        v01::Binding v01Binding{};
        v01Binding.sourceEvidenceSha256=binding.sourceEvidenceSha256;
        v01Binding.truthNegativeStateSha256=binding.truthNegativeStateSha256;

        detail::Workspace workspace{};

        for(const auto& referenceTile:referenceV01.tiles){
            if(referenceTile.width==0u||referenceTile.height==0u){
                return false;
            }

            v01::Options localOptions{};
            localOptions.tileEdge=referenceV01.tileEdge;
            localOptions.samplingPeriod=referenceV01.samplingPeriod;
            localOptions.appearanceGridWidth=referenceTile.width;
            localOptions.appearanceGridHeight=referenceTile.height;
            localOptions.regionX=referenceTile.x;
            localOptions.regionY=referenceTile.y;
            localOptions.regionWidth=referenceTile.width;
            localOptions.regionHeight=referenceTile.height;

            v01::Result localV01{};
            if(!v01::run(source,v01Binding,localOptions,localV01)||
               localV01.tiles.size()!=1u||
               localV01.sampled!=referenceTile.sampled||
               localV01.audit.total!=localV01.sampled||
               !same_audit(localV01.audit,referenceTile.audit)||
               localV01.cfaPhaseSamples!=referenceTile.cfaPhaseSamples||
               localV01.borderProtected!=referenceTile.borderProtected||
               !localV01.appearanceGridDerived||
               localV01.appearanceGridWidth!=referenceTile.width||
               localV01.appearanceGridHeight!=referenceTile.height){
                return false;
            }

            TileMetrics tile{};
            tile.x=referenceTile.x;
            tile.y=referenceTile.y;
            tile.width=referenceTile.width;
            tile.height=referenceTile.height;
            tile.metrics.sampled=localV01.sampled;
            tile.metrics.v01CandidateCenters=localV01.audit.corrected;

            if(localV01.audit.corrected>0u){
                TileRect supportTile{};
                supportTile.x0=static_cast<int>(referenceTile.x);
                supportTile.y0=static_cast<int>(referenceTile.y);
                supportTile.x1=static_cast<int>(
                    referenceTile.x+referenceTile.width);
                supportTile.y1=static_cast<int>(
                    referenceTile.y+referenceTile.height);
                supportTile.hx0=std::max(
                    0,supportTile.x0-kMaxRadius);
                supportTile.hy0=std::max(
                    0,supportTile.y0-kMaxRadius);
                supportTile.hx1=std::min(
                    md.width,supportTile.x1+kMaxRadius);
                supportTile.hy1=std::min(
                    md.height,supportTile.y1+kMaxRadius);

                const auto filled=detail::fill_stage2(
                    source,supportTile,workspace);
                if(!filled)return false;

                const int tw=supportTile.hx1-supportTile.hx0;
                const int th=supportTile.hy1-supportTile.hy0;
                const std::size_t expected=
                    static_cast<std::size_t>(tw)*
                    static_cast<std::size_t>(th);
                if(tw<=0||th<=0||
                   workspace.stage2.size()!=expected||
                   workspace.raw.size()!=expected||
                   (md.hasGainField&&workspace.gain.size()!=expected)){
                    return false;
                }

                const auto indexOf=
                    [&](int gx,int gy,std::size_t& index) noexcept {
                        if(gx<supportTile.hx0||
                           gy<supportTile.hy0||
                           gx>=supportTile.hx1||
                           gy>=supportTile.hy1){
                            return false;
                        }
                        index=
                            static_cast<std::size_t>(
                                gy-supportTile.hy0)*
                                static_cast<std::size_t>(tw)+
                            static_cast<std::size_t>(
                                gx-supportTile.hx0);
                        return index<expected;
                    };

                const auto pathTouchesCensor=
                    [&](int gx,int gy,int dx,int dy) noexcept {
                        const int radius=
                            std::max(std::abs(dx),std::abs(dy));
                        if(radius<=0||(radius%2)!=0)return true;
                        const int steps=radius/2;
                        if(steps<=0)return true;
                        const int stepX=dx/steps;
                        const int stepY=dy/steps;
                        for(int step=1;step<=steps;++step){
                            std::size_t i=0u;
                            if(!indexOf(
                                    gx+step*stepX,
                                    gy+step*stepY,
                                    i)){
                                return true;
                            }
                            if(static_cast<float>(workspace.raw[i])>=
                               md.whiteLevel){
                                return true;
                            }
                        }
                        return false;
                    };

                for(std::uint32_t ly=0u;ly<referenceTile.height;++ly){
                    for(std::uint32_t lx=0u;lx<referenceTile.width;++lx){
                        const std::size_t binIndex=
                            static_cast<std::size_t>(ly)*
                                referenceTile.width+lx;
                        if(binIndex>=localV01.appearanceGrid.size()){
                            return false;
                        }
                        const auto& bin=localV01.appearanceGrid[binIndex];
                        std::uint32_t correctedSum=0u;
                        for(auto v:bin.corrected)correctedSum+=v;
                        if(correctedSum==0u)continue;
                        if(correctedSum!=1u)return false;

                        const int gx=static_cast<int>(referenceTile.x+lx);
                        const int gy=static_cast<int>(referenceTile.y+ly);
                        const int channel=measured_channel(md.cfa,gx,gy);
                        if(channel<0||channel>2)return false;
                        const auto cc=static_cast<std::size_t>(channel);
                        if(bin.corrected[cc]!=1u)return false;

                        const auto phase=
                            static_cast<std::size_t>((gy&1)*2+(gx&1));
                        ++tile.metrics.v01CandidateCfaPhase[phase];

                        std::size_t centerIndex=0u;
                        if(!indexOf(gx,gy,centerIndex))return false;
                        if(static_cast<float>(workspace.raw[centerIndex])>=
                           md.whiteLevel){
                            return false;
                        }
                        const double center=workspace.stage2[centerIndex];
                        double centerVariance=0.0;
                        if(!std::isfinite(center)||
                           !variance_for(
                               md,workspace,centerIndex,
                               channel,center,centerVariance)){
                            return false;
                        }

                        ce::Input input{};
                        input.neighbors.reserve(
                            kRadii.size()*kDirections.size()*2u);
                        for(const int radius:kRadii){
                            for(const auto& direction:kDirections){
                                for(const int side:{-1,1}){
                                    const int dx=
                                        side*radius*direction[0];
                                    const int dy=
                                        side*radius*direction[1];
                                    const int nx=gx+dx;
                                    const int ny=gy+dy;
                                    std::size_t ni=0u;
                                    if(!indexOf(nx,ny,ni))continue;
                                    if(measured_channel(md.cfa,nx,ny)!=
                                       channel){
                                        return false;
                                    }

                                    const double value=workspace.stage2[ni];
                                    const bool censored=
                                        static_cast<float>(
                                            workspace.raw[ni])>=md.whiteLevel;
                                    double variance=0.0;
                                    const bool varianceKnown=
                                        !censored&&
                                        variance_for(
                                            md,workspace,ni,
                                            channel,value,variance);

                                    ce::Sample sample{};
                                    sample.value=value;
                                    sample.variance=variance;
                                    sample.dx=dx;
                                    sample.dy=dy;
                                    sample.authority=censored
                                        ? ce::SampleAuthority::Censored
                                        : ce::SampleAuthority::Measured;
                                    sample.varianceKnown=varianceKnown;
                                    sample.sameChannel=true;
                                    sample.sameObject=true;
                                    sample.objectIdentityKnown=false;
                                    sample.censorBoundary=
                                        pathTouchesCensor(
                                            gx,gy,dx,dy);
                                    input.neighbors.push_back(sample);
                                }
                            }
                        }

                        ce::Result predictor{};
                        if(!ce::estimate(input,predictor)||
                           !predictor.centerExcluded||
                           predictor.createsNewEvidence||
                           predictor.scientificWritebackAllowed){
                            return false;
                        }

                        add_counted_result(tile.metrics,predictor);
                        if(!predictor.valid){
                            ++tile.metrics.predictorInvalid;
                            continue;
                        }

                        if(!std::isfinite(predictor.estimate)||
                           !(predictor.estimateVariance>0.0)||
                           !std::isfinite(predictor.estimateVariance)){
                            return false;
                        }
                        const double absResidual=
                            std::abs(center-predictor.estimate);
                        const double centerSigma=
                            std::sqrt(centerVariance);
                        const double combinedVariance=
                            centerVariance+predictor.estimateVariance;
                        const double combinedSigma=
                            std::sqrt(combinedVariance);
                        if(!std::isfinite(absResidual)||
                           !(centerSigma>0.0)||
                           !std::isfinite(centerSigma)||
                           !(combinedSigma>0.0)||
                           !std::isfinite(combinedSigma)){
                            return false;
                        }

                        const double centerZ=absResidual/centerSigma;
                        const double combinedZ=absResidual/combinedSigma;
                        if(!std::isfinite(centerZ)||
                           !std::isfinite(combinedZ)){
                            return false;
                        }
                        add_valid_result(
                            tile.metrics,
                            absResidual,
                            centerVariance,
                            predictor.estimateVariance,
                            centerZ,
                            combinedZ,
                            phase);
                    }
                }
            }

            if(tile.metrics.v01CandidateCenters!=
                   referenceTile.audit.corrected ||
               tile.metrics.predictorValid+
                   tile.metrics.predictorInvalid!=
                   tile.metrics.v01CandidateCenters ||
               !metrics_consistent(tile.metrics)){
                return false;
            }

            accumulate_metrics(out.metrics,tile.metrics);
            out.tiles.push_back(tile);
        }

        if(out.tiles.size()!=referenceV01.tiles.size()||
           out.metrics.sampled!=referenceV01.sampled||
           out.metrics.v01CandidateCenters!=
               referenceV01.audit.corrected||
           !metrics_consistent(out.metrics)){
            return false;
        }

        truthraw::sha256_v0_69::Hasher hasher;
        constexpr char domain[]=
            "D_RAW_TN_N2_CENTER_EXCLUDED_SPATIAL_AUDIT_V0_2_1";
        hasher.update(
            reinterpret_cast<const std::uint8_t*>(domain),
            sizeof(domain)-1u);
        hasher.update(binding.sourceEvidenceSha256);
        hasher.update(binding.scientificMasterSha256);
        hasher.update(binding.authorityFieldSha256);
        hasher.update(binding.truthNegativeStateSha256);
        hasher.update(binding.v01CandidateSha256);
        hasher.update(binding.v01AuditSha256);
        hasher.update(binding.v01SpatialSha256);
        hash_u32(hasher,out.tileEdge);
        hash_u32(hasher,out.samplingPeriod);
        hash_metrics(hasher,out.metrics);
        for(const auto& tile:out.tiles){
            hash_u32(hasher,tile.x);
            hash_u32(hasher,tile.y);
            hash_u32(hasher,tile.width);
            hash_u32(hasher,tile.height);
            hash_metrics(hasher,tile.metrics);
        }
        out.auditSha256=hasher.finalize();
        out.v01TileParityVerified=true;
        out.centerOnlySigmaPrimary=true;
        out.combinedSigmaDiagnosticOnly=true;
        out.noiseIndependenceAdmitted=false;
        out.candidateApplied=false;
        out.createsNewEvidence=false;
        out.scientificWritebackAllowed=false;

        return nonzero(out.auditSha256)&&
               out.v01TileParityVerified&&
               out.centerOnlySigmaPrimary&&
               out.combinedSigmaDiagnosticOnly&&
               !out.noiseIndependenceAdmitted&&
               !out.candidateApplied&&
               !out.createsNewEvidence&&
               !out.scientificWritebackAllowed;
    }catch(...){
        out={};
        return false;
    }
}

bool encode(
    const Binding& binding,
    std::uint32_t sourceWidth,
    std::uint32_t sourceHeight,
    const Result& audit,
    Report& out) noexcept {
    out={};
    try{
        if(sourceWidth==0u||sourceHeight==0u||
           !nonzero(binding.sourceEvidenceSha256)||
           !nonzero(binding.scientificMasterSha256)||
           !nonzero(binding.authorityFieldSha256)||
           !nonzero(binding.truthNegativeStateSha256)||
           !nonzero(binding.v01CandidateSha256)||
           !nonzero(binding.v01AuditSha256)||
           !nonzero(binding.v01SpatialSha256)||
           !nonzero(audit.auditSha256)||
           audit.tiles.empty()||
           !metrics_consistent(audit.metrics)||
           !audit.v01TileParityVerified||
           !audit.centerOnlySigmaPrimary||
           !audit.combinedSigmaDiagnosticOnly||
           audit.noiseIndependenceAdmitted||
           audit.candidateApplied||
           audit.createsNewEvidence||
           audit.scientificWritebackAllowed){
            return false;
        }

        std::ostringstream o;
        o.setf(std::ios::fixed);
        o<<std::setprecision(12);
        o<<"{\n";
        o<<"  \"schema\":\""<<kSchemaName<<"\",\n";
        o<<"  \"source_width\":"<<sourceWidth<<",\n";
        o<<"  \"source_height\":"<<sourceHeight<<",\n";
        o<<"  \"tile_edge\":"<<audit.tileEdge<<",\n";
        o<<"  \"sampling_period\":"<<audit.samplingPeriod<<",\n";
        o<<"  \"source_sha256\":\""
         <<hex(binding.sourceEvidenceSha256)<<"\",\n";
        o<<"  \"scientific_master_sha256\":\""
         <<hex(binding.scientificMasterSha256)<<"\",\n";
        o<<"  \"authority_field_sha256\":\""
         <<hex(binding.authorityFieldSha256)<<"\",\n";
        o<<"  \"truthnegative_state_sha256\":\""
         <<hex(binding.truthNegativeStateSha256)<<"\",\n";
        o<<"  \"v01_candidate_sha256\":\""
         <<hex(binding.v01CandidateSha256)<<"\",\n";
        o<<"  \"v01_audit_sha256\":\""
         <<hex(binding.v01AuditSha256)<<"\",\n";
        o<<"  \"v01_spatial_sha256\":\""
         <<hex(binding.v01SpatialSha256)<<"\",\n";
        o<<"  \"center_excluded_audit_sha256\":\""
         <<hex(audit.auditSha256)<<"\",\n";
        o<<"  \"v01_tile_parity_verified\":true,\n";
        o<<"  \"center_only_sigma_primary\":true,\n";
        o<<"  \"combined_sigma_diagnostic_only\":true,\n";
        o<<"  \"noise_independence_admitted\":false,\n";
        o<<"  \"candidate_applied\":false,\n";
        o<<"  \"creates_new_evidence\":false,\n";
        o<<"  \"scientific_writeback_allowed\":false,\n";
        o<<"  \"global\":{";
        write_metrics(o,audit.metrics);
        o<<"},\n";
        o<<"  \"tiles\":[\n";
        for(std::size_t i=0u;i<audit.tiles.size();++i){
            const auto& tile=audit.tiles[i];
            o<<"    {\"x\":"<<tile.x
             <<",\"y\":"<<tile.y
             <<",\"width\":"<<tile.width
             <<",\"height\":"<<tile.height<<",";
            write_metrics(o,tile.metrics);
            o<<"}";
            if(i+1u<audit.tiles.size())o<<",";
            o<<"\n";
        }
        o<<"  ]\n";
        o<<"}\n";

        out.json=o.str();
        truthraw::sha256_v0_69::Hasher hasher;
        hasher.update(
            reinterpret_cast<const std::uint8_t*>(out.json.data()),
            out.json.size());
        out.jsonSha256=hasher.finalize();
        out.tileCount=audit.tiles.size();
        out.candidateApplied=false;
        out.createsNewEvidence=false;
        out.scientificWritebackAllowed=false;
        return !out.json.empty()&&nonzero(out.jsonSha256);
    }catch(...){
        out={};
        return false;
    }
}

} // namespace truthraw::truthnegative_center_excluded_spatial_audit::v0_2_1
