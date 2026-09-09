#include "truthrange_dense_uncertainty_v0_3.h"
#include <algorithm>
#include <cmath>

namespace truthraw {
namespace {
constexpr float kGaussianAbsP50 = 0.6744897501960817f;
constexpr float kGaussianAbsP95 = 1.959963984540054f;

static inline std::size_t idx3(std::size_t i,int c){ return 3*i+std::size_t(c); }

static bool same_geometry(const LatentCameraSceneV02& scene,int w,int h,std::size_t n3){
    const std::size_t N=std::size_t(scene.width)*std::size_t(scene.height);
    return scene.width==w && scene.height==h && n3==3*N;
}

} // namespace

const char* dense_uncertainty_source_name_v0_3(DenseUncertaintySourceV03 s){
    switch(s){
        case DenseUncertaintySourceV03::MeasuredNoiseProfileGaussianEquivalent: return "MEASURED_NOISEPROFILE_GAUSSIAN_EQUIVALENT";
        case DenseUncertaintySourceV03::MeasuredHighCensored: return "MEASURED_HIGH_CENSORED";
        case DenseUncertaintySourceV03::V5GMeasuredRoleAnchor: return "V5G_MEASURED_ROLE_ANCHOR";
        case DenseUncertaintySourceV03::V5GLocalMaxTransportProxy: return "V5G_LOCAL_MAX_TRANSPORT_PROXY";
        default: return "UNRESOLVED";
    }
}

Status build_measured_noiseprofile_field_v0_3(
    const DecodedDngFrame& frame,
    const LatentCameraSceneV02& scene,
    DenseUncertaintyFieldV03& out){

    const std::size_t N=std::size_t(scene.width)*std::size_t(scene.height);
    if(scene.width<=1 || scene.height<=1 || frame.meta.width!=scene.width || frame.meta.height!=scene.height ||
       frame.raw.size()!=N || scene.stage2Cfa.size()!=N || scene.cameraRgb.size()!=3*N ||
       scene.measuredChannel.size()!=N || scene.sourceHighCensor.size()!=N)
        return Status::error(StatusCode::InvalidArgument,"dense v0.3 geometry mismatch");
    if(!frame.meta.hasNoiseProfile)
        return Status::error(StatusCode::InvalidArgument,"NoiseProfile required for measured-channel uncertainty");
    for(float v:frame.meta.noiseProfile) if(!(v>=0.f) || !std::isfinite(v))
        return Status::error(StatusCode::InvalidArgument,"invalid NoiseProfile coefficient");
    if(frame.meta.hasGainField && frame.gainField.size()!=N)
        return Status::error(StatusCode::InvalidArgument,"gain field size mismatch");

    out=DenseUncertaintyFieldV03{};
    out.width=scene.width; out.height=scene.height; out.rgb.resize(3*N);
    out.measuredModel="DNG_NOISEPROFILE_GAUSSIAN_EQUIVALENT_STAGE2";
    out.reconstructedModel="UNRESOLVED_WAITING_BACKEND_ANCHORS";
    out.covarianceStatus="MARGINAL_ONLY_OFF_DIAGONAL_UNRESOLVED";
    out.claimBoundary="Measured CFA bands are NoiseProfile Gaussian-equivalent marginal intervals in Stage-2 units; reconstructed channels remain unresolved until a separately bound backend anchor proxy is supplied. No independence/covariance claim.";

    for(std::size_t i=0;i<N;++i){
        const int c=int(scene.measuredChannel[i]);
        if(c<0 || c>2) return Status::error(StatusCode::BackendFailed,"invalid measured channel id");
        auto& e=out.rgb[idx3(i,c)];
        e.sourceHighCensored=scene.sourceHighCensor[i]!=0;
        if(e.sourceHighCensored){
            e.valid=false;
            e.source=DenseUncertaintySourceV03::MeasuredHighCensored;
            e.topologyCertified=true;
            continue;
        }
        const float g=frame.meta.hasGainField?frame.gainField[i]:1.f;
        if(!(g>0.f) || !std::isfinite(g)) return Status::error(StatusCode::BackendFailed,"invalid GainMap value");
        const float S=frame.meta.noiseProfile[2*c];
        const float O=frame.meta.noiseProfile[2*c+1];
        const float mu=scene.stage2Cfa[i];
        const float var=std::max(g*S*std::max(mu,0.f)+g*g*O,0.f);
        const float sigma=std::sqrt(var);
        e.valid=true;
        e.sigmaEquivalent=sigma;
        e.p50Abs=kGaussianAbsP50*sigma;
        e.p95Abs=kGaussianAbsP95*sigma;
        e.source=DenseUncertaintySourceV03::MeasuredNoiseProfileGaussianEquivalent;
        e.topologyCertified=true;
    }
    return Status::ok();
}

Status transport_backend_anchors_v0_3(
    const LatentCameraSceneV02& scene,
    const BackendAnchorFieldV03& anchors,
    int radius,
    DenseUncertaintyFieldV03& io){

    const std::size_t N=std::size_t(scene.width)*std::size_t(scene.height);
    if(radius<1 || !same_geometry(scene,anchors.width,anchors.height,anchors.rgb.size()) ||
       !same_geometry(scene,io.width,io.height,io.rgb.size()))
        return Status::error(StatusCode::InvalidArgument,"invalid dense v0.3 anchor field");

    // Validate anchor provenance: numeric anchors are accepted only at an actually measured
    // same-channel CFA site. This prevents accidental use of v5.0g on unvalidated topologies.
    for(std::size_t i=0;i<N;++i){
        for(int c=0;c<3;++c){
            const auto& a=anchors.rgb[idx3(i,c)];
            if(!a.valid) continue;
            if(int(scene.measuredChannel[i])!=c)
                return Status::error(StatusCode::InvalidArgument,"backend anchor exists on an unmeasured channel topology");
            if(!(a.p50Abs>=0.f && a.p95Abs>=a.p50Abs && std::isfinite(a.p95Abs)))
                return Status::error(StatusCode::InvalidArgument,"invalid backend anchor quantiles");
        }
    }

    for(int y=0;y<scene.height;++y){
        for(int x=0;x<scene.width;++x){
            const std::size_t i=std::size_t(y)*scene.width+x;
            const int measured=int(scene.measuredChannel[i]);
            for(int c=0;c<3;++c){
                if(c==measured) continue;
                float p50=-1.f,p95=-1.f;
                bool any=false;
                for(int yy=std::max(0,y-radius);yy<=std::min(scene.height-1,y+radius);++yy){
                    for(int xx=std::max(0,x-radius);xx<=std::min(scene.width-1,x+radius);++xx){
                        const std::size_t j=std::size_t(yy)*scene.width+xx;
                        if(int(scene.measuredChannel[j])!=c) continue;
                        const auto& a=anchors.rgb[idx3(j,c)];
                        if(!a.valid) continue;
                        p50=std::max(p50,a.p50Abs);
                        p95=std::max(p95,a.p95Abs);
                        any=true;
                    }
                }
                auto& e=io.rgb[idx3(i,c)];
                if(any){
                    e.valid=true;
                    e.p50Abs=p50;
                    e.p95Abs=p95;
                    e.sigmaEquivalent=std::numeric_limits<float>::quiet_NaN();
                    e.source=DenseUncertaintySourceV03::V5GLocalMaxTransportProxy;
                    e.topologyCertified=false;
                    e.sourceHighCensored=false;
                }
            }
        }
    }
    io.reconstructedModel="V5.0G_MEASURED_ROLE_ANCHORS_LOCAL_MAX_TRANSPORT_PROXY";
    io.claimBoundary="Measured CFA: source-bound NoiseProfile Gaussian-equivalent marginal bands. Reconstructed channels: local-max transport of v5.0g anchors evaluated only on calibrated measured-role topology. Transported values are numeric backend proxies, not topology-certified missing-channel coverage or co-sited RGB truth. Covariance remains unresolved.";
    return Status::ok();
}

Status map_dense_uncertainty_to_truthrange_v0_3(
    const LatentCameraSceneV02& scene,
    const DenseUncertaintyFieldV03& uncertainty,
    const TruthRangeGaugeV02& gauge,
    DenseTruthRangeFieldV03& out){

    const std::size_t N=std::size_t(scene.width)*std::size_t(scene.height);
    if(!same_geometry(scene,uncertainty.width,uncertainty.height,uncertainty.rgb.size()) || scene.cameraRgb.size()!=3*N)
        return Status::error(StatusCode::InvalidArgument,"dense TruthRange geometry mismatch");
    auto gst=validate_gauge_for_scene_v0_2(scene,gauge);
    if(!gst) return gst;

    out=DenseTruthRangeFieldV03{}; out.width=scene.width; out.height=scene.height; out.rgb.resize(3*N);
    for(std::size_t i=0;i<N;++i){
        const int measured=int(scene.measuredChannel[i]);
        for(int c=0;c<3;++c){
            const auto& e=uncertainty.rgb[idx3(i,c)];
            LatentUncertaintyV02 u;
            if(e.valid){u.valid=true;u.p50Abs=e.p50Abs;u.p95Abs=e.p95Abs;u.source=dense_uncertainty_source_name_v0_3(e.source);}
            else {u.valid=false;u.source=dense_uncertainty_source_name_v0_3(e.source);}
            const bool isMeasured=(c==measured);
            const bool hi=isMeasured && scene.sourceHighCensor[i]!=0;
            const float hiLower=isMeasured?scene.sourceHighCensorLower[i]:0.f;
            const auto support=isMeasured?TruthRangeSupportV02::MeasuredUncensored:TruthRangeSupportV02::ReconstructedWeak;
            out.rgb[idx3(i,c)]=map_latent_channel_to_truthrange_v0_2(
                scene.cameraRgb[idx3(i,c)],u,gauge,support,isMeasured,hi,hiLower);
        }
    }
    return Status::ok();
}

} // namespace truthraw
