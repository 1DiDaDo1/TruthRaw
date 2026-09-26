#include <jni.h>

#include "free_world_appearance_resolve_v0_7.h"
#include "free_world_scientific_open_scene_binding_v0_3.h"
#include "full_frame_streaming_v0_1_internal.h"
#include "truthnegative_center_excluded_neighborhood_v0_2.h"
#include "truthnegative_n2_cfa_audit_v0_1.h"
#include "truthnegative_n2_full_colour_candidate_v0_1.h"
#include "truthnegative_n2_risk_quality_audit_v0_1.h"
#include "truthnegative_pipeline_bridge_common.h"
#include "truthraw_sha256_v0_69.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace {

namespace pipeline = truthraw::android_truthnegative_pipeline::v0_1;
namespace appearance = truthraw::free_world_appearance_resolve::v0_7;
namespace binding = truthraw::free_world_scientific_open_scene_binding::v0_3;
namespace deep = truthraw::free_world_deep_scene_contribution::v0_4;
namespace free_world = truthraw::free_world_pixel_resolve_2d::v0_2;
namespace tn = truthraw::truthnegative_continuous::v0_5;
namespace n2 = truthraw::truthnegative_n2_cfa_audit::v0_1;
namespace n2ce = truthraw::truthnegative_center_excluded_neighborhood::v0_2;
namespace n2fc = truthraw::truthnegative_n2_full_colour_candidate::v0_1;
namespace n2qa = truthraw::truthnegative_n2_risk_quality_audit::v0_1;
namespace stream_detail = truthraw::streaming_v0_1::detail;
namespace sha = truthraw::sha256_v0_69;

constexpr jint kMagic = 0x31424132; // "2AB1" little-endian view.
constexpr std::size_t kCropCount = 3u;
constexpr std::size_t kCropMetaInts = 107u;
constexpr std::size_t kHeaderInts = 48u + kCropCount * kCropMetaInts;
constexpr std::uint32_t kCropEdge = 192u;
constexpr double kDeltaGain = 32.0;

jint clamp_metric(std::uint64_t value) noexcept {
    const auto cap=static_cast<std::uint64_t>(std::numeric_limits<jint>::max());
    return static_cast<jint>(std::min(value,cap));
}

jint scaled_metric(double value,double scale) noexcept {
    if(!std::isfinite(value)||value<=0.0||!std::isfinite(scale)||scale<=0.0)return 0;
    const double cap=static_cast<double>(std::numeric_limits<jint>::max());
    return static_cast<jint>(std::llround(std::min(value*scale,cap)));
}

jint distance_metric(double value) noexcept {
    if(!std::isfinite(value)||value<0.0)return -1;
    return scaled_metric(value,1000.0);
}

void write_quantiles(
    const n2qa::Quantiles& q,
    jint* out) noexcept {
    out[0]=scaled_metric(q.mean,1000000000.0);
    out[1]=scaled_metric(q.p50,1000000000.0);
    out[2]=scaled_metric(q.p95,1000000000.0);
    out[3]=scaled_metric(q.p99,1000000000.0);
    out[4]=scaled_metric(q.max,1000000000.0);
}

jintArray status_packet(JNIEnv* env,jint status) {
    std::array<jint,kHeaderInts> values{};
    values[0]=kMagic; values[1]=status;
    jintArray out=env->NewIntArray(static_cast<jsize>(values.size()));
    if(out) env->SetIntArrayRegion(out,0,static_cast<jsize>(values.size()),values.data());
    return out;
}

void digest_to_words(const sha::Digest& digest,jint* out) noexcept {
    for(std::size_t word=0u;word<8u;++word){
        const std::size_t i=word*4u;
        const std::uint32_t v=
            static_cast<std::uint32_t>(digest[i]) |
            (static_cast<std::uint32_t>(digest[i+1u])<<8u) |
            (static_cast<std::uint32_t>(digest[i+2u])<<16u) |
            (static_cast<std::uint32_t>(digest[i+3u])<<24u);
        out[word]=static_cast<jint>(v);
    }
}

sha::Digest labeled_digest(
    const char* label,
    const sha::Digest* parent=nullptr,
    const std::string* text=nullptr) noexcept {
    sha::Hasher h;
    const std::size_t n=std::char_traits<char>::length(label);
    h.update(reinterpret_cast<const std::uint8_t*>(label),n);
    if(parent)h.update(*parent);
    if(text)h.update(reinterpret_cast<const std::uint8_t*>(text->data()),text->size());
    return h.finalize();
}

sha::Digest candidate_scene_digest(
    const sha::Digest& scientificScene,
    const sha::Digest& gridSha,
    std::uint32_t gx,
    std::uint32_t gy,
    const std::array<double,3u>& rgb) noexcept {
    sha::Hasher h;
    constexpr char domain[]="D_RAW_TN_N2_1_TO_1_CROP_CANDIDATE_V0_1";
    h.update(reinterpret_cast<const std::uint8_t*>(domain),sizeof(domain)-1u);
    h.update(scientificScene);h.update(gridSha);
    const std::array<std::uint32_t,2u> xy{gx,gy};
    for(auto v:xy){
        std::array<std::uint8_t,4u> b{
            static_cast<std::uint8_t>(v),
            static_cast<std::uint8_t>(v>>8u),
            static_cast<std::uint8_t>(v>>16u),
            static_cast<std::uint8_t>(v>>24u)};
        h.update(b);
    }
    for(double v:rgb){
        const auto bits=std::bit_cast<std::uint64_t>(v);
        std::array<std::uint8_t,8u> b{};
        for(std::size_t i=0u;i<8u;++i)b[i]=static_cast<std::uint8_t>(bits>>(8u*i));
        h.update(b);
    }
    return h.finalize();
}

appearance::Matrix3 matrix_from_f32(const std::array<float,9u>& source) noexcept {
    appearance::Matrix3 out{};
    for(std::size_t i=0u;i<source.size();++i)out.m[i]=static_cast<double>(source[i]);
    return out;
}

appearance::Matrix3 xyz_d50_to_linear_srgb() noexcept {
    appearance::Matrix3 out{};
    out.m={3.1338561,-1.6168667,-0.4906146,
          -0.9787684,1.9161415,0.0334540,
           0.0719453,-0.2289914,1.4052427};
    return out;
}

std::uint32_t quantize_u8(double encoded) noexcept {
    if(!std::isfinite(encoded))return 0u;
    const long q=std::lround(std::clamp(encoded,0.0,1.0)*255.0);
    return static_cast<std::uint32_t>(std::clamp<long>(q,0l,255l));
}

std::uint32_t argb(
    const std::array<double,3u>& encoded) noexcept {
    const auto r=quantize_u8(encoded[0]);
    const auto g=quantize_u8(encoded[1]);
    const auto b=quantize_u8(encoded[2]);
    return 0xff000000u|(r<<16u)|(g<<8u)|b;
}

struct CropRect final {
    std::uint32_t type=0u;
    std::uint32_t x=0u,y=0u,width=0u,height=0u;
};

std::size_t best_candidate_tile(const n2::Result& audit) noexcept {
    std::size_t best=0u; double bestScore=-1e300;
    for(std::size_t i=0u;i<audit.tiles.size();++i){
        const auto& t=audit.tiles[i];
        if(t.sampled==0u)continue;
        const double corrected=static_cast<double>(t.audit.corrected);
        const double structure=static_cast<double>(t.audit.structureProtected);
        const double censor=static_cast<double>(
            t.audit.censoredProtected+t.audit.censorBoundaryProtected);
        const double score=corrected-1.5*structure-4.0*censor;
        if(score>bestScore){bestScore=score;best=i;}
    }
    return best;
}

std::size_t best_structure_tile(const n2::Result& audit) noexcept {
    std::size_t best=0u; std::uint64_t score=0u;
    for(std::size_t i=0u;i<audit.tiles.size();++i){
        const auto s=audit.tiles[i].audit.structureProtected;
        if(s>score){score=s;best=i;}
    }
    return best;
}

std::size_t best_censor_tile(const n2::Result& audit) noexcept {
    std::size_t best=0u; std::uint64_t score=0u;
    for(std::size_t i=0u;i<audit.tiles.size();++i){
        const auto s=audit.tiles[i].audit.censoredProtected+
                     audit.tiles[i].audit.censorBoundaryProtected;
        if(s>score){score=s;best=i;}
    }
    return best;
}

CropRect crop_around(
    const n2::TileAudit& tile,
    std::uint32_t type,
    std::uint32_t sourceWidth,
    std::uint32_t sourceHeight) noexcept {
    const std::uint32_t edge=std::min(
        kCropEdge,std::min(sourceWidth,sourceHeight));
    const std::uint32_t cx=tile.x+tile.width/2u;
    const std::uint32_t cy=tile.y+tile.height/2u;
    const std::uint32_t maxX=sourceWidth-edge;
    const std::uint32_t maxY=sourceHeight-edge;
    const std::uint32_t x=std::min(
        maxX,cx>edge/2u?cx-edge/2u:0u);
    const std::uint32_t y=std::min(
        maxY,cy>edge/2u?cy-edge/2u:0u);
    return {type,x,y,edge,edge};
}

struct CropMetrics final {
    std::uint64_t changedPixels=0u;
    std::uint64_t adjustedChannels=0u;
    std::uint64_t clampA=0u;
    std::uint64_t clampB=0u;
    std::uint64_t baselineRgbMismatches=0u;
    std::uint64_t candidateStage2Sites=0u;
    double absDeltaSum=0.0;
    double maxAbsDelta=0.0;
};

struct CenterExcludedAuditMetrics final {
    std::uint64_t v01CandidateCenters=0u;
    std::uint64_t predictorValid=0u;
    std::uint64_t predictorInvalid=0u;
    std::uint64_t symmetricPairsConsidered=0u;
    std::uint64_t symmetricPairsAccepted=0u;
    std::uint64_t symmetricPairsRejected=0u;
    std::uint64_t scalesConsidered=0u;
    std::uint64_t scalesAccepted=0u;
    std::uint64_t scalesRejected=0u;
    std::uint64_t residualWithin1Sigma=0u;
    std::uint64_t residualBetween1And2Sigma=0u;
    std::uint64_t residualAbove2Sigma=0u;
    double absResidualSum=0.0;
    double maxAbsResidual=0.0;
    double maxDirectionalDisagreementSigma=0.0;
    double maxCrossScaleDisagreementSigma=0.0;
};

int n2_v02_measured_channel(
    truthraw::CfaPattern cfa,
    int x,
    int y) noexcept {
    const int phase=(y&1)*2+(x&1);
    static constexpr int bggr[4]={2,1,1,0};
    static constexpr int rggb[4]={0,1,1,2};
    static constexpr int grbg[4]={1,0,2,1};
    static constexpr int gbrg[4]={1,2,0,1};
    const int* map=bggr;
    switch(cfa){
        case truthraw::CfaPattern::RGGB:map=rggb;break;
        case truthraw::CfaPattern::GRBG:map=grbg;break;
        case truthraw::CfaPattern::GBRG:map=gbrg;break;
        case truthraw::CfaPattern::BGGR:map=bggr;break;
    }
    return map[phase];
}

bool n2_v02_variance_for(
    const truthraw::DngMetadata& md,
    const stream_detail::Workspace& workspace,
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
    if(!std::isfinite(shot)||!std::isfinite(read)||shot<0.0||read<0.0){
        return false;
    }
    variance=gain*shot*std::max(mu,0.0)+gain*gain*read;
    return std::isfinite(variance)&&variance>0.0;
}

bool run_center_excluded_audit(
    const truthraw::DngMetadata& md,
    const truthraw::TileRect& tile,
    const stream_detail::Workspace& workspace,
    const CropRect& crop,
    const n2::Result& v01Audit,
    CenterExcludedAuditMetrics& out) noexcept {
    out={};
    try{
        if(!v01Audit.appearanceGridDerived||
           v01Audit.appearanceGridWidth!=crop.width||
           v01Audit.appearanceGridHeight!=crop.height||
           v01Audit.appearanceGrid.size()!=
               static_cast<std::size_t>(crop.width)*crop.height){
            return false;
        }
        if(!md.hasNoiseProfile){
            return v01Audit.audit.corrected==0u;
        }
        const int tileWidth=tile.hx1-tile.hx0;
        const int tileHeight=tile.hy1-tile.hy0;
        if(tileWidth<=0||tileHeight<=0)return false;
        const std::size_t tilePixels=
            static_cast<std::size_t>(tileWidth)*tileHeight;
        if(workspace.stage2.size()!=tilePixels||
           workspace.raw.size()!=tilePixels||
           (md.hasGainField&&workspace.gain.size()!=tilePixels)){
            return false;
        }

        const auto indexOf=[&](int gx,int gy,std::size_t& index) noexcept {
            if(gx<tile.hx0||gy<tile.hy0||gx>=tile.hx1||gy>=tile.hy1){
                return false;
            }
            index=
                static_cast<std::size_t>(gy-tile.hy0)*
                    static_cast<std::size_t>(tileWidth)+
                static_cast<std::size_t>(gx-tile.hx0);
            return index<tilePixels;
        };

        const auto pathTouchesCensor=
            [&](int gx,int gy,int dx,int dy) noexcept {
                const int radius=std::max(std::abs(dx),std::abs(dy));
                if(radius<=0||(radius%2)!=0)return true;
                const int steps=radius/2;
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
                    if(static_cast<float>(workspace.raw[i])>=md.whiteLevel){
                        return true;
                    }
                }
                return false;
            };

        constexpr std::array<int,3u> radii{2,4,8};
        constexpr std::array<std::array<int,2u>,4u> directions{{
            {{1,0}},{{0,1}},{{1,1}},{{1,-1}}
        }};

        for(std::uint32_t ly=0u;ly<crop.height;++ly){
            for(std::uint32_t lx=0u;lx<crop.width;++lx){
                const std::uint32_t gxRaw=crop.x+lx;
                const std::uint32_t gyRaw=crop.y+ly;
                const int gx=static_cast<int>(gxRaw);
                const int gy=static_cast<int>(gyRaw);
                const std::size_t local=
                    static_cast<std::size_t>(ly)*crop.width+lx;
                const int channel=n2_v02_measured_channel(md.cfa,gx,gy);
                if(channel<0||channel>2)return false;
                const auto cc=static_cast<std::size_t>(channel);
                const auto& bin=v01Audit.appearanceGrid[local];
                if(bin.sampled[cc]!=1u)return false;
                if(bin.corrected[cc]==0u)continue;
                if(bin.corrected[cc]!=1u)return false;
                ++out.v01CandidateCenters;

                std::size_t centerIndex=0u;
                if(!indexOf(gx,gy,centerIndex))return false;
                if(static_cast<float>(workspace.raw[centerIndex])>=md.whiteLevel){
                    return false;
                }
                const double center=workspace.stage2[centerIndex];
                double centerVariance=0.0;
                if(!std::isfinite(center)||
                   !n2_v02_variance_for(
                       md,workspace,centerIndex,channel,center,centerVariance)){
                    return false;
                }

                n2ce::Input input{};
                input.neighbors.reserve(radii.size()*directions.size()*2u);
                for(const int radius:radii){
                    for(const auto& direction:directions){
                        for(const int side:{-1,1}){
                            const int dx=side*radius*direction[0];
                            const int dy=side*radius*direction[1];
                            const int nx=gx+dx;
                            const int ny=gy+dy;
                            std::size_t ni=0u;
                            if(!indexOf(nx,ny,ni))continue;
                            if(n2_v02_measured_channel(md.cfa,nx,ny)!=channel){
                                return false;
                            }
                            const double value=workspace.stage2[ni];
                            const bool censored=
                                static_cast<float>(workspace.raw[ni])>=
                                md.whiteLevel;
                            double variance=0.0;
                            const bool varianceKnown=
                                !censored&&
                                n2_v02_variance_for(
                                    md,workspace,ni,channel,value,variance);
                            n2ce::Sample sample{};
                            sample.value=value;
                            sample.variance=variance;
                            sample.dx=dx;
                            sample.dy=dy;
                            sample.authority=censored
                                ? n2ce::SampleAuthority::Censored
                                : n2ce::SampleAuthority::Measured;
                            sample.varianceKnown=varianceKnown;
                            sample.sameChannel=true;
                            sample.sameObject=true;
                            sample.objectIdentityKnown=false;
                            sample.censorBoundary=
                                pathTouchesCensor(gx,gy,dx,dy);
                            input.neighbors.push_back(sample);
                        }
                    }
                }

                n2ce::Result result{};
                if(!n2ce::estimate(input,result)||
                   !result.centerExcluded||
                   result.createsNewEvidence||
                   result.scientificWritebackAllowed){
                    return false;
                }
                out.symmetricPairsConsidered+=
                    result.symmetricPairsConsidered;
                out.symmetricPairsAccepted+=
                    result.symmetricPairsAccepted;
                out.symmetricPairsRejected+=
                    result.symmetricPairsRejected;
                out.scalesConsidered+=result.scalesConsidered;
                out.scalesAccepted+=result.scalesAccepted;
                out.scalesRejected+=result.scalesRejected;
                out.maxDirectionalDisagreementSigma=std::max(
                    out.maxDirectionalDisagreementSigma,
                    result.maxDirectionalDisagreementSigma);
                out.maxCrossScaleDisagreementSigma=std::max(
                    out.maxCrossScaleDisagreementSigma,
                    result.maxCrossScaleDisagreementSigma);

                if(!result.valid){
                    ++out.predictorInvalid;
                    continue;
                }
                ++out.predictorValid;
                const double absResidual=std::abs(center-result.estimate);
                if(!std::isfinite(absResidual))return false;
                out.absResidualSum+=absResidual;
                out.maxAbsResidual=std::max(
                    out.maxAbsResidual,absResidual);
                const double sigma=std::sqrt(centerVariance);
                if(!std::isfinite(sigma)||!(sigma>0.0))return false;
                const double z=absResidual/sigma;
                if(!std::isfinite(z))return false;
                if(z<=1.0){
                    ++out.residualWithin1Sigma;
                }else if(z<=2.0){
                    ++out.residualBetween1And2Sigma;
                }else{
                    ++out.residualAbove2Sigma;
                }
            }
        }

        return out.v01CandidateCenters==v01Audit.audit.corrected&&
               out.predictorValid+out.predictorInvalid==
                   out.v01CandidateCenters&&
               out.residualWithin1Sigma+
                   out.residualBetween1And2Sigma+
                   out.residualAbove2Sigma==
                   out.predictorValid&&
               std::isfinite(out.absResidualSum)&&
               std::isfinite(out.maxAbsResidual)&&
               std::isfinite(out.maxDirectionalDisagreementSigma)&&
               std::isfinite(out.maxCrossScaleDisagreementSigma);
    }catch(...){
        out={};
        return false;
    }
}

free_world::ResolvedAuthority map_exact_authority(
    free_world::SourceAuthority authority) noexcept {
    switch(authority){
        case free_world::SourceAuthority::Censored:
            return free_world::ResolvedAuthority::Censored;
        case free_world::SourceAuthority::Unknown:
            return free_world::ResolvedAuthority::Unknown;
        case free_world::SourceAuthority::CalibratedEstimate:
        case free_world::SourceAuthority::Reconstructed:
            return free_world::ResolvedAuthority::Reconstructed;
    }
    return free_world::ResolvedAuthority::Unknown;
}

sha::Digest exact_master_scene_digest(
    const tn::State& state,
    const sha::Digest& authorityFieldSha,
    std::uint32_t x,
    std::uint32_t y,
    const free_world::SourcePixel& pixel) noexcept {
    sha::Hasher h;
    constexpr char domain[] =
        "D_RAW_TN_N2_EXACT_SCIENTIFIC_MASTER_PIXEL_V0_2";
    h.update(
        reinterpret_cast<const std::uint8_t*>(domain),
        sizeof(domain)-1u);
    h.update(state.stateSha256);
    h.update(authorityFieldSha);
    const std::array<std::uint32_t,2u> xy{x,y};
    for(auto v:xy){
        std::array<std::uint8_t,4u> b{
            static_cast<std::uint8_t>(v),
            static_cast<std::uint8_t>(v>>8u),
            static_cast<std::uint8_t>(v>>16u),
            static_cast<std::uint8_t>(v>>24u)};
        h.update(b);
    }
    for(const auto& ch:pixel.channel){
        const auto bits=std::bit_cast<std::uint64_t>(ch.value);
        std::array<std::uint8_t,8u> b{};
        for(std::size_t i=0u;i<8u;++i){
            b[i]=static_cast<std::uint8_t>(bits>>(8u*i));
        }
        h.update(b);
        const std::array<std::uint8_t,2u> tags{
            static_cast<std::uint8_t>(ch.role),
            static_cast<std::uint8_t>(ch.authority)};
        h.update(tags);
    }
    return h.finalize();
}

bool make_exact_scientific_master_view(
    const tn::State& state,
    const sha::Digest& authorityFieldSha,
    const free_world::SourcePixel& sourcePixel,
    std::uint32_t x,
    std::uint32_t y,
    const float* expectedBaselineRgb,
    deep::DeepResolvedPixel& out,
    std::uint64_t& mismatchCount) noexcept {
    out={};
    if(expectedBaselineRgb==nullptr)return false;
    for(std::size_t cc=0u;cc<3u;++cc){
        const double sourceValue=sourcePixel.channel[cc].value;
        if(!std::isfinite(sourceValue))return false;
        const float sourceF32=static_cast<float>(sourceValue);
        if(std::bit_cast<std::uint32_t>(sourceF32)!=
           std::bit_cast<std::uint32_t>(expectedBaselineRgb[cc])){
            ++mismatchCount;
            return false;
        }
        out.sceneLinearRgb[cc]=sourceValue;
        out.channelAuthority[cc]=
            map_exact_authority(sourcePixel.channel[cc].authority);
        out.uncertaintyKnown[cc]=
            sourcePixel.channel[cc].uncertaintyKnown;
        out.p95Uncertainty[cc]=
            sourcePixel.channel[cc].uncertaintyKnown
                ? sourcePixel.channel[cc].p95Uncertainty
                : 0.0;
    }

    out.visibility.evidenceWeight=1.0;
    out.visibility.inferredWeight=0.0;
    out.visibility.restorationWeight=0.0;
    out.visibility.counterfactualWeight=0.0;
    out.visibility.residualTransmittance=0.0;
    out.visibility.containsInferred=false;
    out.visibility.containsRestorationHypothesis=false;
    out.visibility.containsCounterfactual=false;
    out.visibility.scientificObservation=true;
    out.visibility.createsNewEvidence=false;
    out.visibility.scientificWritebackAllowed=false;

    out.sourcePacketSha256=exact_master_scene_digest(
        state,authorityFieldSha,x,y,sourcePixel);
    out.resolveMethodId=
        "EXACT_SCIENTIFIC_MASTER_SOURCE_PIXEL_V0_2";
    out.view=deep::ResolveView::ScientificView;
    out.appearanceApplied=false;
    out.displayEncoded=false;
    out.createsNewEvidence=false;
    out.scientificWritebackAllowed=false;
    out.physicalFrameCount=1u;
    out.independentEvidenceCount=1u;
    return std::any_of(
        out.sourcePacketSha256.begin(),
        out.sourcePacketSha256.end(),
        [](std::uint8_t v){return v!=0u;});
}

} // namespace

extern "C" JNIEXPORT jintArray JNICALL
Java_com_truthraw_adaptiveui_TruthNegativeN2CropAbNativeBridge_buildDiagnosticCrops(
    JNIEnv* env,
    jobject,
    jint sourceFd,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    if(sourceFd<0||maxSourceResidentBytes<=0||maxLogicalResidentBytes<=0){
        return status_packet(env,-1);
    }

    pipeline::Context ctx{};
    const auto prepared=pipeline::prepare(
        static_cast<int>(sourceFd),
        static_cast<std::size_t>(maxSourceResidentBytes),
        static_cast<std::size_t>(maxLogicalResidentBytes),
        ctx);
    if(!prepared)return status_packet(env,prepared.code);
    if(ctx.width<32u||ctx.height<32u)return status_packet(env,-2);

    binding::BoundScenePlane scene(
        *ctx.masterSource,*ctx.fieldSource,ctx.width,ctx.height);
    if(!scene.valid())return status_packet(env,-3);

    n2::Binding coarseBinding{};
    coarseBinding.sourceEvidenceSha256=ctx.sourceSeal.sha256;
    coarseBinding.truthNegativeStateSha256=ctx.truthNegativeState.stateSha256;
    n2::Options coarseOptions{};
    coarseOptions.tileEdge=64u;
    coarseOptions.samplingPeriod=8u;
    n2::Result coarse{};
    if(!n2::run(*ctx.openedSource.source,coarseBinding,coarseOptions,coarse)||
       coarse.tiles.empty()||coarse.sourceValuesModified||
       coarse.truthNegativeModified||coarse.createsNewEvidence||
       coarse.scientificWritebackAllowed){
        return status_packet(env,-4);
    }

    const std::array<std::size_t,kCropCount> selected{
        best_candidate_tile(coarse),
        best_structure_tile(coarse),
        best_censor_tile(coarse)};
    const std::array<std::uint32_t,kCropCount> types{1u,2u,3u};
    std::array<CropRect,kCropCount> crops{};
    for(std::size_t i=0u;i<kCropCount;++i){
        crops[i]=crop_around(
            coarse.tiles[selected[i]],types[i],ctx.width,ctx.height);
    }

    appearance::SceneColorimetry sceneColor{};
    sceneColor.rgbToXyz=matrix_from_f32(ctx.produced.color.cameraToXyzD50);
    sceneColor.referenceWhiteXyz={0.96422,1.0,0.82521};
    sceneColor.sceneReferenceWhiteNits=100.0;
    sceneColor.identitySha256=labeled_digest(
        "D_RAW_TN_N2_CROP_SCENE_COLOR",
        &ctx.truthNegativeState.stateSha256,
        &ctx.produced.color.bindingId);

    appearance::ViewingConditions viewing{};
    viewing.adaptingWhiteXyz={0.95047,1.0,1.08883};
    viewing.adaptingLuminanceNits=20.0;
    viewing.backgroundLuminanceNits=20.0;
    viewing.surround=appearance::Surround::Average;
    viewing.viewingDistanceMeters=0.5;
    viewing.identitySha256=labeled_digest(
        "D_RAW_TN_N2_CROP_VIEW_AVERAGE_20_NIT");

    appearance::DisplayTarget display{};
    display.xyzToRgb=xyz_d50_to_linear_srgb();
    display.whitePointXyz={0.95047,1.0,1.08883};
    display.referenceWhiteNits=100.0;
    display.peakLuminanceNits=100.0;
    display.blackLuminanceNits=0.0;
    display.transfer=appearance::TransferFunction::Srgb;
    display.identitySha256=labeled_digest(
        "D_RAW_TN_N2_CROP_SRGB_100_NIT");

    appearance::AppearancePolicy policy{};
    policy.exposureEv=0.0;
    policy.colorfulnessScale=1.0;
    policy.highlightCompression=1.0;
    policy.identitySha256=labeled_digest(
        "D_RAW_TN_N2_CROP_NEUTRAL_APPEARANCE");

    const std::uint32_t edge=crops[0].width;
    const std::size_t cropPixels=
        static_cast<std::size_t>(edge)*edge;
    const std::size_t payloadPixels=
        cropPixels*kCropCount*3u;
    if(payloadPixels>
       static_cast<std::size_t>(std::numeric_limits<jsize>::max())-kHeaderInts){
        return status_packet(env,-6);
    }

    std::vector<jint> packet(kHeaderInts+payloadPixels,0);
    packet[0]=kMagic;packet[1]=0;
    packet[2]=static_cast<jint>(kCropCount);
    packet[3]=static_cast<jint>(edge);
    packet[4]=static_cast<jint>(ctx.width);
    packet[5]=static_cast<jint>(ctx.height);
    packet[6]=static_cast<jint>(ctx.openedSource.source->metadata().orientation);
    packet[7]=1;packet[8]=1;
    packet[9]=0;packet[10]=0;packet[11]=0;
    packet[12]=static_cast<jint>(kDeltaGain);
    packet[13]=1; // full-colour candidate reconstruction enabled
    digest_to_words(ctx.truthNegativeState.stateSha256,packet.data()+14u);
    digest_to_words(ctx.authorityField.contentSha256,packet.data()+22u);
    digest_to_words(coarse.auditSha256,packet.data()+30u);
    digest_to_words(coarse.spatialSha256,packet.data()+38u);

    const std::size_t rasterBase=kHeaderInts;
    for(std::size_t cropIndex=0u;cropIndex<kCropCount;++cropIndex){
        const auto& crop=crops[cropIndex];

        n2::Options cropOptions{};
        cropOptions.tileEdge=64u;
        cropOptions.samplingPeriod=2u;
        cropOptions.regionX=crop.x;
        cropOptions.regionY=crop.y;
        cropOptions.regionWidth=crop.width;
        cropOptions.regionHeight=crop.height;
        cropOptions.appearanceGridWidth=crop.width;
        cropOptions.appearanceGridHeight=crop.height;

        n2::Result audit{};
        if(!n2::run(
                *ctx.openedSource.source,
                coarseBinding,
                cropOptions,
                audit)||
           !audit.appearanceGridDerived||
           audit.appearanceGrid.size()!=cropPixels||
           audit.sampled!=cropPixels||
           audit.sourceValuesModified||
           audit.truthNegativeModified||
           audit.createsNewEvidence||
           audit.scientificWritebackAllowed){
            return status_packet(env,-7);
        }

        // Parallel v0.2 research audit. This never changes the current v0.1
        // candidate: it only asks whether a center-excluded H/V/diagonal,
        // multiscale predictor can explain the same v0.1 candidate centers.
        constexpr int centerExcludedHalo=8;
        truthraw::TileRect centerExcludedTile{};
        centerExcludedTile.x0=static_cast<int>(crop.x);
        centerExcludedTile.y0=static_cast<int>(crop.y);
        centerExcludedTile.x1=static_cast<int>(crop.x+crop.width);
        centerExcludedTile.y1=static_cast<int>(crop.y+crop.height);
        centerExcludedTile.hx0=std::max(
            0,centerExcludedTile.x0-centerExcludedHalo);
        centerExcludedTile.hy0=std::max(
            0,centerExcludedTile.y0-centerExcludedHalo);
        centerExcludedTile.hx1=std::min(
            static_cast<int>(ctx.width),
            centerExcludedTile.x1+centerExcludedHalo);
        centerExcludedTile.hy1=std::min(
            static_cast<int>(ctx.height),
            centerExcludedTile.y1+centerExcludedHalo);

        stream_detail::Workspace centerExcludedWorkspace{};
        const auto centerExcludedStatus=stream_detail::fill_stage2(
            *ctx.openedSource.source,
            centerExcludedTile,
            centerExcludedWorkspace);
        if(!centerExcludedStatus)return status_packet(env,-20);

        CenterExcludedAuditMetrics centerExcludedAudit{};
        if(!run_center_excluded_audit(
                ctx.openedSource.source->metadata(),
                centerExcludedTile,
                centerExcludedWorkspace,
                crop,
                audit,
                centerExcludedAudit)){
            return status_packet(env,-20);
        }

        // Build a private Stage-2 tile with the exact halo required by the
        // existing measured-preserving reconstruction. N2 is evaluated on the
        // complete halo tile so colour reconstruction can propagate admitted
        // CFA corrections without inventing un-audited neighbours.
        const int reconstructionHalo=ctx.reconstruction->requiredHalo();
        if(reconstructionHalo<0)return status_packet(env,-15);
        truthraw::TileRect reconstructionTile{};
        reconstructionTile.x0=static_cast<int>(crop.x);
        reconstructionTile.y0=static_cast<int>(crop.y);
        reconstructionTile.x1=static_cast<int>(crop.x+crop.width);
        reconstructionTile.y1=static_cast<int>(crop.y+crop.height);
        reconstructionTile.hx0=std::max(
            0,reconstructionTile.x0-reconstructionHalo);
        reconstructionTile.hy0=std::max(
            0,reconstructionTile.y0-reconstructionHalo);
        reconstructionTile.hx1=std::min(
            static_cast<int>(ctx.width),
            reconstructionTile.x1+reconstructionHalo);
        reconstructionTile.hy1=std::min(
            static_cast<int>(ctx.height),
            reconstructionTile.y1+reconstructionHalo);

        stream_detail::Workspace reconstructionWorkspace{};
        const auto stage2Status=stream_detail::fill_stage2(
            *ctx.openedSource.source,
            reconstructionTile,
            reconstructionWorkspace);
        if(!stage2Status)return status_packet(env,-15);

        const int tileWidth=
            reconstructionTile.hx1-reconstructionTile.hx0;
        const int tileHeight=
            reconstructionTile.hy1-reconstructionTile.hy0;
        if(tileWidth<=0||tileHeight<=0||
           reconstructionWorkspace.stage2.size()!=
               static_cast<std::size_t>(tileWidth)*
               static_cast<std::size_t>(tileHeight)){
            return status_packet(env,-15);
        }

        n2::Options reconstructionAuditOptions{};
        reconstructionAuditOptions.tileEdge=64u;
        reconstructionAuditOptions.samplingPeriod=2u;
        reconstructionAuditOptions.regionX=
            static_cast<std::uint32_t>(reconstructionTile.hx0);
        reconstructionAuditOptions.regionY=
            static_cast<std::uint32_t>(reconstructionTile.hy0);
        reconstructionAuditOptions.regionWidth=
            static_cast<std::uint32_t>(tileWidth);
        reconstructionAuditOptions.regionHeight=
            static_cast<std::uint32_t>(tileHeight);
        reconstructionAuditOptions.appearanceGridWidth=
            static_cast<std::uint32_t>(tileWidth);
        reconstructionAuditOptions.appearanceGridHeight=
            static_cast<std::uint32_t>(tileHeight);

        n2::Result reconstructionAudit{};
        if(!n2::run(
                *ctx.openedSource.source,
                coarseBinding,
                reconstructionAuditOptions,
                reconstructionAudit)||
           !reconstructionAudit.appearanceGridDerived||
           reconstructionAudit.sampled!=
               static_cast<std::uint64_t>(tileWidth)*
               static_cast<std::uint64_t>(tileHeight)||
           reconstructionAudit.sourceValuesModified||
           reconstructionAudit.truthNegativeModified||
           reconstructionAudit.createsNewEvidence||
           reconstructionAudit.scientificWritebackAllowed){
            return status_packet(env,-16);
        }

        n2fc::Input fullColourInput{};
        fullColourInput.stage2=reconstructionWorkspace.stage2.data();
        fullColourInput.tileWidth=tileWidth;
        fullColourInput.tileHeight=tileHeight;
        fullColourInput.globalHx0=reconstructionTile.hx0;
        fullColourInput.globalHy0=reconstructionTile.hy0;
        fullColourInput.coreX0=reconstructionTile.x0;
        fullColourInput.coreY0=reconstructionTile.y0;
        fullColourInput.coreWidth=static_cast<int>(crop.width);
        fullColourInput.coreHeight=static_cast<int>(crop.height);
        fullColourInput.cfa=ctx.openedSource.source->metadata().cfa;
        fullColourInput.n2Audit=&reconstructionAudit;
        fullColourInput.reconstructionInfluenceRadius=
            reconstructionHalo;
        fullColourInput.closeProtectionOverReconstructionSupport=true;

        n2fc::Result fullColour{};
        if(!n2fc::reconstruct(
                fullColourInput,
                *ctx.reconstruction,
                fullColour)||
           fullColour.sourceStage2Modified||
           fullColour.createsNewEvidence||
           fullColour.scientificWritebackAllowed||
           !fullColour.supportGuardApplied||
           fullColour.protectedCoreChangedRgbChannels!=0u||
           fullColour.baselineCameraRgb.size()!=cropPixels*3u||
           fullColour.candidateCameraRgb.size()!=cropPixels*3u){
            return status_packet(env,-17);
        }

        CropMetrics metrics{};
        metrics.adjustedChannels=fullColour.changedRgbChannels;
        metrics.candidateStage2Sites=fullColour.correctedStage2Sites;
        std::vector<double> qaA(cropPixels*3u,0.0);
        std::vector<double> qaB(cropPixels*3u,0.0);
        std::vector<std::uint32_t> qaReasonMask(cropPixels,0u);
        const std::size_t aBase=
            rasterBase+(cropIndex*3u+0u)*cropPixels;
        const std::size_t bBase=
            rasterBase+(cropIndex*3u+1u)*cropPixels;
        const std::size_t dBase=
            rasterBase+(cropIndex*3u+2u)*cropPixels;

        for(std::uint32_t ly=0u;ly<crop.height;++ly){
            for(std::uint32_t lx=0u;lx<crop.width;++lx){
                const std::uint32_t gx=crop.x+lx;
                const std::uint32_t gy=crop.y+ly;
                const std::size_t local=
                    static_cast<std::size_t>(ly)*crop.width+lx;

                free_world::SourcePixel exactSourcePixel{};
                if(!scene.readPixel(gx,gy,exactSourcePixel)){
                    return status_packet(env,-8);
                }

                const std::size_t rgbBase=local*3u;
                deep::DeepResolvedPixel scientificView{};
                if(!make_exact_scientific_master_view(
                        ctx.truthNegativeState,
                        ctx.authorityField.contentSha256,
                        exactSourcePixel,
                        gx,
                        gy,
                        fullColour.baselineCameraRgb.data()+rgbBase,
                        scientificView,
                        metrics.baselineRgbMismatches)){
                    return status_packet(env,-18);
                }

                appearance::AppearanceInput aInput{};
                aInput.scene=scientificView;
                aInput.sceneColorimetry=sceneColor;
                aInput.viewing=viewing;
                aInput.display=display;
                aInput.policy=policy;
                appearance::AppearanceResolvedPixel aVisible{};
                if(!appearance::resolveAppearance(aInput,aVisible)||
                   aVisible.sourceSceneMutated||
                   aVisible.createsNewEvidence||
                   aVisible.scientificWritebackAllowed){
                    return status_packet(env,-10);
                }
                if(aVisible.gamutOrDisplayClampApplied)++metrics.clampA;

                auto candidateScene=scientificView;
                candidateScene.sceneLinearRgb={
                    static_cast<double>(
                        fullColour.candidateCameraRgb[rgbBase+0u]),
                    static_cast<double>(
                        fullColour.candidateCameraRgb[rgbBase+1u]),
                    static_cast<double>(
                        fullColour.candidateCameraRgb[rgbBase+2u])};
                candidateScene.channelAuthority.fill(
                    free_world::ResolvedAuthority::Unknown);
                candidateScene.uncertaintyKnown.fill(false);
                candidateScene.p95Uncertainty.fill(0.0);
                candidateScene.visibility={};
                candidateScene.appearanceApplied=false;
                candidateScene.displayEncoded=false;
                candidateScene.sourcePacketSha256=candidate_scene_digest(
                    scientificView.sourcePacketSha256,
                    fullColour.candidateIdentitySha256,
                    gx,gy,candidateScene.sceneLinearRgb);
                candidateScene.createsNewEvidence=false;
                candidateScene.scientificWritebackAllowed=false;

                appearance::AppearanceInput bInput{};
                bInput.scene=candidateScene;
                bInput.sceneColorimetry=sceneColor;
                bInput.viewing=viewing;
                bInput.display=display;
                bInput.policy=policy;
                appearance::AppearanceResolvedPixel bVisible{};
                if(!appearance::resolveAppearance(bInput,bVisible)||
                   bVisible.sourceSceneMutated||
                   bVisible.createsNewEvidence||
                   bVisible.scientificWritebackAllowed){
                    return status_packet(env,-12);
                }
                if(bVisible.gamutOrDisplayClampApplied)++metrics.clampB;

                for(std::size_t cc=0u;cc<3u;++cc){
                    qaA[rgbBase+cc]=aVisible.encodedRgb[cc];
                    qaB[rgbBase+cc]=bVisible.encodedRgb[cc];
                }
                qaReasonMask[local]=
                    audit.appearanceGrid[local].preserveReasonMask;

                const auto aArgb=argb(aVisible.encodedRgb);
                const auto bArgb=argb(bVisible.encodedRgb);
                packet[aBase+local]=static_cast<jint>(aArgb);
                packet[bBase+local]=static_cast<jint>(bArgb);

                std::array<double,3u> delta{};
                bool displayChanged=false;
                for(std::size_t cc=0u;cc<3u;++cc){
                    const double d=std::abs(
                        bVisible.encodedRgb[cc]-aVisible.encodedRgb[cc]);
                    if(!std::isfinite(d))return status_packet(env,-13);
                    metrics.absDeltaSum+=d;
                    metrics.maxAbsDelta=std::max(metrics.maxAbsDelta,d);
                    delta[cc]=std::clamp(d*kDeltaGain,0.0,1.0);
                    displayChanged=displayChanged||
                        quantize_u8(aVisible.encodedRgb[cc])!=
                        quantize_u8(bVisible.encodedRgb[cc]);
                }
                if(displayChanged)++metrics.changedPixels;
                packet[dBase+local]=static_cast<jint>(argb(delta));
            }
        }

        n2qa::Input qaInput{};
        qaInput.aEncodedRgb=qaA.data();
        qaInput.bEncodedRgb=qaB.data();
        qaInput.preserveReasonMask=qaReasonMask.data();
        qaInput.width=crop.width;
        qaInput.height=crop.height;
        qaInput.sourceX=crop.x;
        qaInput.sourceY=crop.y;
        qaInput.candidateIdentitySha256=
            fullColour.candidateIdentitySha256;
        n2qa::Result quality{};
        if(!n2qa::evaluate(qaInput,quality)||
           quality.createsNewEvidence||
           quality.scientificWritebackAllowed){
            return status_packet(env,-19);
        }

        const std::size_t m=48u+cropIndex*kCropMetaInts;
        packet[m+0u]=static_cast<jint>(crop.type);
        packet[m+1u]=static_cast<jint>(crop.x);
        packet[m+2u]=static_cast<jint>(crop.y);
        packet[m+3u]=static_cast<jint>(crop.width);
        packet[m+4u]=static_cast<jint>(crop.height);
        packet[m+5u]=clamp_metric(audit.sampled);
        packet[m+6u]=clamp_metric(audit.audit.corrected);
        packet[m+7u]=clamp_metric(audit.audit.preserved);
        packet[m+8u]=clamp_metric(audit.audit.structureProtected);
        packet[m+9u]=clamp_metric(audit.audit.censoredProtected);
        packet[m+10u]=clamp_metric(audit.audit.censorBoundaryProtected);
        packet[m+11u]=clamp_metric(audit.audit.residualOutlierProtected);
        packet[m+12u]=clamp_metric(audit.audit.noNeighborhoodProtected);
        packet[m+13u]=clamp_metric(metrics.changedPixels);
        packet[m+14u]=clamp_metric(metrics.adjustedChannels);
        packet[m+15u]=scaled_metric(
            metrics.absDeltaSum/(static_cast<double>(cropPixels)*3.0),
            1000000000.0);
        packet[m+16u]=scaled_metric(metrics.maxAbsDelta,1000000000.0);
        const double removedFraction=
            audit.audit.totalResidualEnergy>0.0
                ? audit.audit.removedResidualEnergy/audit.audit.totalResidualEnergy
                : 0.0;
        packet[m+17u]=scaled_metric(
            std::clamp(removedFraction,0.0,1.0),1000000.0);
        packet[m+18u]=scaled_metric(
            audit.audit.maxAbsCorrection,1000000000.0);
        packet[m+19u]=clamp_metric(metrics.clampA);
        packet[m+20u]=clamp_metric(metrics.clampB);
        packet[m+21u]=clamp_metric(metrics.baselineRgbMismatches);
        digest_to_words(
            fullColour.candidateIdentitySha256,
            packet.data()+m+22u);
        packet[m+30u]=clamp_metric(metrics.candidateStage2Sites);
        packet[m+31u]=1; // full-colour measured-preserving reconstruction

        write_quantiles(quality.pixelMaxAbsDelta,packet.data()+m+32u);
        write_quantiles(quality.channelAbsDelta[0],packet.data()+m+37u);
        write_quantiles(quality.channelAbsDelta[1],packet.data()+m+42u);
        write_quantiles(quality.channelAbsDelta[2],packet.data()+m+47u);
        write_quantiles(quality.lumaAbsDelta,packet.data()+m+52u);
        write_quantiles(quality.chromaDelta,packet.data()+m+57u);
        packet[m+62u]=static_cast<jint>(quality.maxSourceX);
        packet[m+63u]=static_cast<jint>(quality.maxSourceY);
        packet[m+64u]=static_cast<jint>(quality.maxPreserveReasonMask);
        packet[m+65u]=distance_metric(quality.distanceToStructurePx);
        packet[m+66u]=distance_metric(
            quality.distanceToCensorOrBoundaryPx);
        packet[m+67u]=scaled_metric(quality.edgeEnergyA,10000.0);
        packet[m+68u]=scaled_metric(quality.edgeEnergyB,10000.0);
        packet[m+69u]=scaled_metric(quality.edgeEnergyRatio,1000000.0);
        packet[m+70u]=scaled_metric(
            quality.meanAbsGradientDelta,1000000000.0);
        packet[m+71u]=clamp_metric(quality.structureMaskPixels);
        packet[m+72u]=clamp_metric(quality.censorMaskPixels);
        packet[m+73u]=clamp_metric(quality.changedPixels);
        digest_to_words(quality.qualitySha256,packet.data()+m+74u);
        packet[m+82u]=0; // quality createsNewEvidence
        packet[m+83u]=0; // quality scientificWritebackAllowed
        packet[m+84u]=clamp_metric(
            fullColour.supportGuardSuppressedStage2Sites);
        packet[m+85u]=clamp_metric(fullColour.protectedCorePixels);
        packet[m+86u]=clamp_metric(
            fullColour.protectedCoreChangedRgbChannels);
        packet[m+87u]=static_cast<jint>(reconstructionHalo);
        packet[m+88u]=clamp_metric(
            centerExcludedAudit.v01CandidateCenters);
        packet[m+89u]=clamp_metric(
            centerExcludedAudit.predictorValid);
        packet[m+90u]=clamp_metric(
            centerExcludedAudit.predictorInvalid);
        packet[m+91u]=clamp_metric(
            centerExcludedAudit.symmetricPairsConsidered);
        packet[m+92u]=clamp_metric(
            centerExcludedAudit.symmetricPairsAccepted);
        packet[m+93u]=clamp_metric(
            centerExcludedAudit.symmetricPairsRejected);
        packet[m+94u]=clamp_metric(
            centerExcludedAudit.scalesConsidered);
        packet[m+95u]=clamp_metric(
            centerExcludedAudit.scalesAccepted);
        packet[m+96u]=clamp_metric(
            centerExcludedAudit.scalesRejected);
        packet[m+97u]=clamp_metric(
            centerExcludedAudit.residualWithin1Sigma);
        packet[m+98u]=clamp_metric(
            centerExcludedAudit.residualBetween1And2Sigma);
        packet[m+99u]=clamp_metric(
            centerExcludedAudit.residualAbove2Sigma);
        const double centerExcludedMeanResidual=
            centerExcludedAudit.predictorValid>0u
                ? centerExcludedAudit.absResidualSum/
                    static_cast<double>(
                        centerExcludedAudit.predictorValid)
                : 0.0;
        packet[m+100u]=scaled_metric(
            centerExcludedMeanResidual,1000000000.0);
        packet[m+101u]=scaled_metric(
            centerExcludedAudit.maxAbsResidual,1000000000.0);
        packet[m+102u]=1; // predictor centerExcluded
        packet[m+103u]=0; // createsNewEvidence
        packet[m+104u]=0; // scientificWritebackAllowed
        packet[m+105u]=scaled_metric(
            centerExcludedAudit.maxDirectionalDisagreementSigma,
            1000000.0);
        packet[m+106u]=scaled_metric(
            centerExcludedAudit.maxCrossScaleDisagreementSigma,
            1000000.0);
    }

    const auto report=scene.report();
    if(!report.fieldSchemaValidated||
       !report.valueIdentityVerifiedForLoadedRecords||
       report.masterFieldValueBitMismatches!=0u||
       report.createsNewEvidence||report.scientificWritebackAllowed||
       report.physicalFrameCount!=1u||report.independentEvidenceCount!=1u||
       !pipeline::reverify(ctx)){
        return status_packet(env,-14);
    }

    jintArray out=env->NewIntArray(static_cast<jsize>(packet.size()));
    if(!out)return nullptr;
    env->SetIntArrayRegion(
        out,0,static_cast<jsize>(packet.size()),packet.data());
    return out;
}
