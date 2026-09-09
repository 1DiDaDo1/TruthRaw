#include "truthrange_latent_v0_2.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace truthraw {
namespace {

static inline int phase_index_v02(int y,int x){ return (y&1)*2+(x&1); }
static inline int color_for_phase_v02(CfaPattern cfa,int phase){
    static const int BGGR[4]={2,1,1,0};
    static const int RGGB[4]={0,1,1,2};
    static const int GRBG[4]={1,0,2,1};
    static const int GBRG[4]={1,2,0,1};
    const int* p=BGGR;
    if(cfa==CfaPattern::RGGB)p=RGGB;
    else if(cfa==CfaPattern::GRBG)p=GRBG;
    else if(cfa==CfaPattern::GBRG)p=GBRG;
    return p[phase];
}

static double q_sorted(const std::vector<double>& v,double q){
    if(v.empty()) return std::numeric_limits<double>::quiet_NaN();
    if(q<=0.0) return v.front();
    if(q>=1.0) return v.back();
    const double u=q*double(v.size()-1);
    const std::size_t i=static_cast<std::size_t>(std::floor(u));
    const std::size_t j=std::min(i+1,v.size()-1);
    const double f=u-double(i);
    return v[i]+(v[j]-v[i])*f;
}

static double lower_ev_from_abs_band(float mu,float absBand,const TruthRangeGaugeV02& gauge){
    const double lo=double(mu)-double(absBand);
    if(!(lo>0.0)) return -std::numeric_limits<double>::infinity();
    return truthrange_ev_v0_2(lo,gauge);
}

static double upper_ev_from_abs_band(float mu,float absBand,const TruthRangeGaugeV02& gauge){
    const double hi=std::max(0.0,double(mu)+double(absBand));
    if(!(hi>0.0)) return -std::numeric_limits<double>::infinity();
    return truthrange_ev_v0_2(hi,gauge);
}

} // namespace

double truthrange_ev_v0_2(double positiveL,const TruthRangeGaugeV02& gauge){
    if(!(gauge.L0>0.0) || !std::isfinite(gauge.L0)) return std::numeric_limits<double>::quiet_NaN();
    if(!(positiveL>0.0)) return -std::numeric_limits<double>::infinity();
    return std::log2(positiveL/gauge.L0);
}

Status build_latent_camera_scene_v0_2(
    const DecodedDngFrame& frame,
    const TilePolicy& tile,
    IReconstructionBackend& reconstruction,
    const LatentSceneBindingV02& binding,
    LatentCameraSceneV02& out){

    const auto& m=frame.meta;
    const std::size_t N=std::size_t(m.width)*std::size_t(m.height);
    if(m.width<=1 || m.height<=1 || frame.raw.size()!=N)
        return Status::error(StatusCode::InvalidArgument,"invalid decoded frame");
    if(m.hasGainField && frame.gainField.size()!=N)
        return Status::error(StatusCode::InvalidArgument,"gain field size mismatch");
    if(m.hasResidualBlack && (frame.rowBias.size()!=std::size_t(m.height) || frame.colBias.size()!=std::size_t(m.width)))
        return Status::error(StatusCode::InvalidArgument,"residual black size mismatch");
    if(tile.halo<reconstruction.requiredHalo())
        return Status::error(StatusCode::InvalidArgument,"tile halo smaller than reconstruction requirement");
    if(!binding.gainMapAppliedExactlyOnce)
        return Status::error(StatusCode::InvalidArgument,"binding must assert GainMap exactly once");

    out=LatentCameraSceneV02{};
    out.width=m.width; out.height=m.height; out.cfa=m.cfa; out.binding=binding;
    out.stage2Cfa.resize(N);
    out.cameraRgb.resize(3*N);
    out.measuredChannel.resize(N);
    out.sourceHighCensor.resize(N);
    out.sourceHighCensorLower.resize(N);

    for(int y=0;y<m.height;++y){
        for(int x=0;x<m.width;++x){
            const std::size_t i=std::size_t(y)*m.width+x;
            const int ph=phase_index_v02(y,x);
            const float b0=m.blackPhase[ph];
            float b=b0;
            if(m.hasResidualBlack) b+=frame.rowBias[std::size_t(y)]+frame.colBias[std::size_t(x)];
            const float den=std::max(m.whiteLevel-b,1.f);
            const float g=m.hasGainField?frame.gainField[i]:1.f;
            const float s=((float(frame.raw[i])-b)/den)*g;
            out.stage2Cfa[i]=s;
            out.measuredChannel[i]=static_cast<std::uint8_t>(color_for_phase_v02(m.cfa,ph));
            out.sourceHighCensor[i]=float(frame.raw[i])>=m.whiteLevel?1u:0u;
            out.sourceHighCensorLower[i]=g; // raw==WhiteLevel -> normalized source lower bound times exactly-one GainMap
        }
    }

    // Use the actual backend on the complete signed Stage-2 field. The backend itself
    // clamps spatial lookups at outer image borders. No appearance/display path is involved.
    auto st=reconstruction.reconstructTile(
        out.stage2Cfa.data(),m.width,m.height,
        0,0,0,0,m.width,m.height,m.cfa,out.cameraRgb.data());
    if(!st) return st;

    return Status::ok();
}

Status derive_self_gauge_v0_2(
    const LatentCameraSceneV02& scene,
    TruthRangeGaugeV02& gauge,
    double quantile,
    double borderFraction){

    const std::size_t N=std::size_t(scene.width)*std::size_t(scene.height);
    if(scene.width<=1 || scene.height<=1 || scene.stage2Cfa.size()!=N || scene.sourceHighCensor.size()!=N)
        return Status::error(StatusCode::InvalidArgument,"invalid latent scene");
    if(!(quantile>=0.0 && quantile<=1.0) || !(borderFraction>=0.0 && borderFraction<0.5))
        return Status::error(StatusCode::InvalidArgument,"invalid gauge quantile/border");

    const int bx=std::min(scene.width/2-1,std::max(0,int(std::floor(scene.width*borderFraction))));
    const int by=std::min(scene.height/2-1,std::max(0,int(std::floor(scene.height*borderFraction))));
    std::vector<double> positive;
    positive.reserve(N);
    for(int y=by;y<scene.height-by;++y){
        for(int x=bx;x<scene.width-bx;++x){
            const std::size_t i=std::size_t(y)*scene.width+x;
            const double v=scene.stage2Cfa[i];
            if(scene.sourceHighCensor[i]) continue;
            if(v>0.0 && std::isfinite(v)) positive.push_back(v);
        }
    }
    if(positive.empty()) return Status::error(StatusCode::BackendFailed,"no positive uncensored evidence for self gauge");
    std::sort(positive.begin(),positive.end());
    const double L0=q_sorted(positive,quantile);
    if(!(L0>0.0) || !std::isfinite(L0)) return Status::error(StatusCode::BackendFailed,"invalid derived self gauge");

    gauge=TruthRangeGaugeV02{};
    gauge.mode=TruthRangeGaugeModeV02::SelfGauge;
    gauge.L0=L0;
    gauge.gaugeId="SELF_GAUGE_STAGE2_Q"+std::to_string(quantile);
    gauge.crossSceneComparable=false;
    gauge.absolutePhysicalUnits=false;
    return Status::ok();
}

Status validate_gauge_for_scene_v0_2(
    const LatentCameraSceneV02& scene,
    const TruthRangeGaugeV02& gauge){
    if(!(gauge.L0>0.0) || !std::isfinite(gauge.L0))
        return Status::error(StatusCode::InvalidArgument,"gauge L0 must be finite and positive");
    if(gauge.mode==TruthRangeGaugeModeV02::SelfGauge){
        if(gauge.crossSceneComparable || gauge.absolutePhysicalUnits)
            return Status::error(StatusCode::InvalidArgument,"self gauge cannot claim cross-scene/absolute comparability");
        return Status::ok();
    }
    if(gauge.mode==TruthRangeGaugeModeV02::PhysicalAbsoluteGauge){
        if(!scene.binding.exposureNormalizedToCommonScene || !scene.binding.gainNormalizedToCommonScene)
            return Status::error(StatusCode::BackendFailed,"physical/common gauge requires exposure and gain normalization");
        if(!gauge.crossSceneComparable || !gauge.absolutePhysicalUnits)
            return Status::error(StatusCode::InvalidArgument,"physical gauge must declare cross-scene and absolute binding");
    }
    if(gauge.mode==TruthRangeGaugeModeV02::ExternalRelativeGauge && !gauge.crossSceneComparable)
        return Status::error(StatusCode::InvalidArgument,"external relative gauge must declare shared cross-scene binding");
    return Status::ok();
}

TruthRangeSampleV02 map_latent_channel_to_truthrange_v0_2(
    float muLinearSigned,
    const LatentUncertaintyV02& uncertainty,
    const TruthRangeGaugeV02& gauge,
    TruthRangeSupportV02 requestedReconstructionSupport,
    bool measuredChannel,
    bool sourceHighCensored,
    float sourceHighCensorLowerLinear){

    TruthRangeSampleV02 r;
    r.muLinearSigned=muLinearSigned;
    r.gaugeId=gauge.gaugeId;
    r.uncertaintySource=uncertainty.source;
    if(muLinearSigned>0.f && std::isfinite(muLinearSigned)){
        r.hasEstimate=true;
        r.estimateEv=truthrange_ev_v0_2(muLinearSigned,gauge);
    }

    if(uncertainty.valid && uncertainty.p50Abs>=0.f && uncertainty.p95Abs>=uncertainty.p50Abs){
        r.p50LowerEv=lower_ev_from_abs_band(muLinearSigned,uncertainty.p50Abs,gauge);
        r.p50UpperEv=upper_ev_from_abs_band(muLinearSigned,uncertainty.p50Abs,gauge);
        r.p95LowerEv=lower_ev_from_abs_band(muLinearSigned,uncertainty.p95Abs,gauge);
        r.p95UpperEv=upper_ev_from_abs_band(muLinearSigned,uncertainty.p95Abs,gauge);
        if(r.p95LowerEv==-std::numeric_limits<double>::infinity()) r.censor=TruthRangeCensorV02::DarkNoiseLimited;
    }

    if(measuredChannel){
        if(sourceHighCensored){
            r.support=TruthRangeSupportV02::MeasuredCensoredLowerBound;
            r.censor=TruthRangeCensorV02::HighClipped;
            r.evidenceLowerEv=truthrange_ev_v0_2(std::max(sourceHighCensorLowerLinear,0.f),gauge);
            r.evidenceUpperEv=std::numeric_limits<double>::infinity();
        }else{
            r.support=TruthRangeSupportV02::MeasuredUncensored;
            if(muLinearSigned>0.f){
                r.evidenceLowerEv=r.estimateEv;
                r.evidenceUpperEv=r.estimateEv;
            }else{
                r.evidenceLowerEv=-std::numeric_limits<double>::infinity();
                r.evidenceUpperEv=std::numeric_limits<double>::infinity();
            }
        }
    }else{
        r.support=requestedReconstructionSupport;
    }
    return r;
}

} // namespace truthraw
