#include "truthnegative_center_excluded_spatial_audit_v0_2_2.h"

#include "full_frame_streaming_v0_1_internal.h"

#include <algorithm>
#include <chrono>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

namespace truthraw::truthnegative_center_excluded_spatial_audit::v0_2_2 {

namespace v021 =
    truthraw::truthnegative_center_excluded_spatial_audit::v0_2_1;
using Digest = v021::Digest;
using Binding = v021::Binding;
using Metrics = v021::Metrics;
using TileMetrics = v021::TileMetrics;
using Result = v021::Result;

namespace {

using SteadyClock = std::chrono::steady_clock;

double elapsed_ms(
    SteadyClock::time_point started,
    SteadyClock::time_point finished) noexcept {
    return std::chrono::duration<double,std::milli>(
        finished-started).count();
}

namespace detail = truthraw::streaming_v0_1::detail;
namespace ce =
    truthraw::truthnegative_center_excluded_neighborhood::v0_2;
namespace ce =
    truthraw::truthnegative_center_excluded_neighborhood::v0_2;

constexpr std::array<int,3u> kRadii{2,4,8};
constexpr std::array<std::array<int,2u>,4u> kDirections{{
    {{1,0}},{{0,1}},{{1,1}},{{1,-1}}
}};
constexpr int kMaxRadius = 8;

bool nonzero(const Digest& d) noexcept {
    return std::any_of(
        d.begin(),d.end(),[](std::uint8_t v){return v!=0u;});
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

} // namespace

bool runSparseReference(
    stream::IRawTileSource& source,
    const Binding& binding,
    const v01::Result& referenceV01,
    Result& out,
    Diagnostics* diagnostics) noexcept {
    out={};
    if(diagnostics)*diagnostics=Diagnostics{};
    const auto totalStarted=SteadyClock::now();
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
           !referenceV01.correctedSampleCoordinatesComplete||
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

        if(referenceV01.correctedSampleCoordinates.size()!=
               referenceV01.audit.corrected){
            return false;
        }

        detail::Workspace workspace{};

        for(const auto& referenceTile:referenceV01.tiles){
            if(diagnostics)++diagnostics->tileCount;
            if(referenceTile.width==0u||referenceTile.height==0u){
                return false;
            }

            const std::uint64_t sparseBegin=
                referenceTile.correctedSampleOffset;
            const std::uint64_t sparseCount=
                referenceTile.correctedSampleCount;
            const std::uint64_t sparseEnd=sparseBegin+sparseCount;
            if(sparseEnd<sparseBegin||
               sparseEnd>referenceV01.correctedSampleCoordinates.size()||
               sparseCount!=referenceTile.audit.corrected){
                return false;
            }

            TileMetrics tile{};
            tile.x=referenceTile.x;
            tile.y=referenceTile.y;
            tile.width=referenceTile.width;
            tile.height=referenceTile.height;
            tile.metrics.sampled=referenceTile.sampled;
            tile.metrics.v01CandidateCenters=
                referenceTile.audit.corrected;

            if(referenceTile.audit.corrected>0u){
                if(diagnostics){
                    ++diagnostics->candidateTileCount;
                    diagnostics->candidateCenterCount+=
                        referenceTile.audit.corrected;
                }
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

                const auto fillStarted=SteadyClock::now();
                const auto filled=detail::fill_stage2(
                    source,supportTile,workspace);
                const auto fillFinished=SteadyClock::now();
                if(diagnostics){
                    diagnostics->fillStage2Ms+=
                        elapsed_ms(fillStarted,fillFinished);
                }
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

                const auto candidateLoopStarted=SteadyClock::now();
                for(std::uint64_t localSparse=0u;
                    localSparse<sparseCount;
                    ++localSparse){
                    const auto sparseIndex=sparseBegin+localSparse;
                    if(sparseIndex>=
                       referenceV01.correctedSampleCoordinates.size()){
                        return false;
                    }
                    const auto& point=
                        referenceV01.correctedSampleCoordinates[
                            static_cast<std::size_t>(sparseIndex)];
                    const int gx=static_cast<int>(point.x);
                    const int gy=static_cast<int>(point.y);
                    if(gx<static_cast<int>(referenceTile.x)||
                       gy<static_cast<int>(referenceTile.y)||
                       gx>=static_cast<int>(
                           referenceTile.x+referenceTile.width)||
                       gy>=static_cast<int>(
                           referenceTile.y+referenceTile.height)){
                        return false;
                    }

                    const int channel=measured_channel(md.cfa,gx,gy);
                    if(channel<0||channel>2)return false;

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
                        const auto predictorStarted=SteadyClock::now();
                        const auto predictorOk=ce::estimate(input,predictor);
                        const auto predictorFinished=SteadyClock::now();
                        if(diagnostics){
                            diagnostics->predictorEstimateMs+=
                                elapsed_ms(
                                    predictorStarted,
                                    predictorFinished);
                        }
                        if(!predictorOk||
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
                const auto candidateLoopFinished=SteadyClock::now();
                if(diagnostics){
                    diagnostics->candidateLoopMs+=
                        elapsed_ms(
                            candidateLoopStarted,
                            candidateLoopFinished);
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

        const auto hashStarted=SteadyClock::now();
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
        const auto hashFinished=SteadyClock::now();
        if(diagnostics){
            diagnostics->finalHashMs=
                elapsed_ms(hashStarted,hashFinished);
        }
        out.v01TileParityVerified=true;
        out.centerOnlySigmaPrimary=true;
        out.combinedSigmaDiagnosticOnly=true;
        out.noiseIndependenceAdmitted=false;
        out.candidateApplied=false;
        out.createsNewEvidence=false;
        out.scientificWritebackAllowed=false;

        if(diagnostics){
            diagnostics->totalMs=
                elapsed_ms(totalStarted,SteadyClock::now());
        }
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


} // namespace truthraw::truthnegative_center_excluded_spatial_audit::v0_2_2
