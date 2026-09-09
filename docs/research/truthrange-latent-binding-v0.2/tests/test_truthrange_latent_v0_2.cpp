#include "truthrange_latent_v0_2.h"
#include "uncertainty_runtime_v5_0g.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>

using namespace truthraw;

static int color_for_bggr(int y,int x){
    static const int p[4]={2,1,1,0};
    return p[(y&1)*2+(x&1)];
}

static DecodedDngFrame make_frame(){
    DecodedDngFrame f;
    f.meta.width=40; f.meta.height=32; f.meta.cfa=CfaPattern::BGGR;
    f.meta.whiteLevel=1023.f;
    f.meta.blackPhase={64.f,65.f,66.f,67.f};
    f.meta.hasGainField=true;
    f.meta.hasNoiseProfile=true;
    f.meta.noiseProfile={0.00028f,2e-7f,0.00031f,1.5e-7f,0.00029f,2.2e-7f};
    const std::size_t N=std::size_t(f.meta.width)*f.meta.height;
    f.raw.resize(N); f.gainField.resize(N);
    for(int y=0;y<f.meta.height;++y){
        for(int x=0;x<f.meta.width;++x){
            const std::size_t i=std::size_t(y)*f.meta.width+x;
            const int ph=(y&1)*2+(x&1);
            const float b=f.meta.blackPhase[ph];
            const float g=1.0f+0.55f*float(x)/float(f.meta.width-1);
            f.gainField[i]=g;
            float n=0.02f+0.90f*float(x)/float(f.meta.width-1)+0.03f*std::sin(float(y)*0.41f);
            if(x<2 && y<2) n=-0.01f; // signed Stage-2 evidence below black
            float rawf=b+n*(f.meta.whiteLevel-b);
            rawf=std::max(0.f,std::min(1023.f,rawf));
            f.raw[i]=static_cast<std::uint16_t>(std::lround(rawf));
        }
    }
    // Explicit clipped evidence; gain>1 makes the Stage-2 lower bound >1.
    f.raw[std::size_t(10)*f.meta.width+30]=1023;
    f.raw[std::size_t(11)*f.meta.width+31]=1023;
    return f;
}

int main(){
    DecodedDngFrame frame=make_frame();
    auto backend=std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
    LatentSceneBindingV02 binding;
    binding.reconstructionBackend=backend->name();
    binding.reconstructionCoreCppSha256="68f52c4896d47c4605adc1cf670e55f95d42a687e18c06a167028a64ce37b94c";
    binding.reconstructionCoreHSha256="b7f6e2189d6ecccfc5fda80084041990bd12757145c5ff2c40f2ae0d0046b167";
    binding.uncertaintyModelSha256="8831bee921999e823466cfc462812e40620b2834080c0f7d64c6f11d7ead626f";
    binding.uncertaintyBindingSha256="61c99b0e29316730ca1323fd3e91fd069ebbc50cba73127cd81b9803b10178a0";
    binding.sceneScaleId="SYNTHETIC_STAGE2_TEST";

    LatentCameraSceneV02 scene;
    TilePolicy tile; tile.core=16; tile.halo=4;
    auto st=build_latent_camera_scene_v0_2(frame,tile,*backend,binding,scene);
    if(!st){ std::cerr<<st.message<<"\n"; return 1; }

    // Exact Stage-2 parity with v4.7i scientific diagnostic.
    TruthRawProcessor proc(backend);
    ProcessOptions po; po.tile=tile; po.threads=1; po.keepScientificDiagnostics=true;
    ProcessResult pr;
    st=proc.processFrame(frame,po,pr);
    if(!st){ std::cerr<<st.message<<"\n"; return 2; }
    double stage2Max=0.0;
    for(std::size_t i=0;i<scene.stage2Cfa.size();++i)
        stage2Max=std::max(stage2Max,std::abs(double(scene.stage2Cfa[i])-double(pr.stage2Diagnostic[i])));
    if(stage2Max!=0.0){ std::cerr<<"stage2 parity failure "<<stage2Max<<"\n"; return 3; }

    // v4.7i measured-channel reinjection must be exact.
    double measuredMax=0.0;
    for(int y=0;y<scene.height;++y)for(int x=0;x<scene.width;++x){
        const std::size_t i=std::size_t(y)*scene.width+x;
        const int c=color_for_bggr(y,x);
        measuredMax=std::max(measuredMax,std::abs(double(scene.cameraRgb[3*i+c])-double(scene.stage2Cfa[i])));
    }
    if(measuredMax!=0.0){ std::cerr<<"measured reinjection failure "<<measuredMax<<"\n"; return 4; }

    const auto mm=std::minmax_element(scene.stage2Cfa.begin(),scene.stage2Cfa.end());
    if(!(*mm.first<0.f)){ std::cerr<<"negative signed value not preserved\n"; return 5; }
    if(!(*mm.second>1.f)){ std::cerr<<"over-1 value not preserved\n"; return 6; }

    TruthRangeGaugeV02 g;
    st=derive_self_gauge_v0_2(scene,g,0.5,0.05);
    if(!st){ std::cerr<<st.message<<"\n"; return 7; }
    st=validate_gauge_for_scene_v0_2(scene,g);
    if(!st){ std::cerr<<st.message<<"\n"; return 8; }

    // Whole-master multiplicative rescale: self-gauge must cancel it.
    LatentCameraSceneV02 scaled=scene;
    for(float& v:scaled.stage2Cfa) v*=37.f;
    for(float& v:scaled.cameraRgb) v*=37.f;
    for(float& v:scaled.sourceHighCensorLower) v*=37.f;
    TruthRangeGaugeV02 g37;
    st=derive_self_gauge_v0_2(scaled,g37,0.5,0.05);
    if(!st){ std::cerr<<st.message<<"\n"; return 9; }
    double scaleMax=0.0;
    for(std::size_t i=0;i<scene.cameraRgb.size();++i){
        if(scene.cameraRgb[i]<=0.f || scaled.cameraRgb[i]<=0.f) continue;
        const double a=truthrange_ev_v0_2(scene.cameraRgb[i],g);
        const double b=truthrange_ev_v0_2(scaled.cameraRgb[i],g37);
        scaleMax=std::max(scaleMax,std::abs(a-b));
    }
    if(scaleMax>2e-6){ std::cerr<<"scale invariance failure "<<scaleMax<<"\n"; return 10; }

    // v5.0g absolute Stage-2 bands can be transformed directly to asymmetric EV intervals.
    std::array<float,18> x{};
    x[0]=std::log1p(0.04f); x[1]=std::log1p(0.001f*1e4f); x[2]=std::log1p(8.f); x[14]=1.f;
    auto ub=predict_uncertainty_v5_0g(x,0,8.f);
    if(!(ub.p50>0.f && ub.p95>ub.p50)){ std::cerr<<"v5g runtime invalid\n"; return 11; }
    LatentUncertaintyV02 u; u.valid=true; u.p50Abs=ub.p50; u.p95Abs=ub.p95; u.source="v5.0g";
    auto rs=map_latent_channel_to_truthrange_v0_2(0.04f,u,g,TruthRangeSupportV02::ReconstructedStrong,false,false,0.f);
    if(!(std::isfinite(rs.estimateEv) && rs.p95LowerEv<rs.estimateEv && rs.p95UpperEv>rs.estimateEv)){
        std::cerr<<"uncertainty transform failure\n"; return 12;
    }

    // Dark-side uncertainty crossing physical zero must retain -inf lower tail.
    LatentUncertaintyV02 darku; darku.valid=true; darku.p50Abs=0.0015f; darku.p95Abs=0.003f; darku.source="TEST";
    auto dark=map_latent_channel_to_truthrange_v0_2(0.001f,darku,g,TruthRangeSupportV02::ReconstructedWeak,false,false,0.f);
    if(!(std::isinf(dark.p95LowerEv) && dark.p95LowerEv<0.0 && dark.censor==TruthRangeCensorV02::DarkNoiseLimited)){
        std::cerr<<"dark lower tail failure\n"; return 13;
    }

    // Direct source clipping is an evidence lower bound with unbounded bright tail.
    const std::size_t ci=std::size_t(10)*scene.width+30;
    auto clipped=map_latent_channel_to_truthrange_v0_2(
        scene.cameraRgb[3*ci+scene.measuredChannel[ci]],{},g,TruthRangeSupportV02::Unknown,
        true,true,scene.sourceHighCensorLower[ci]);
    if(!(clipped.support==TruthRangeSupportV02::MeasuredCensoredLowerBound &&
         std::isfinite(clipped.evidenceLowerEv) && std::isinf(clipped.evidenceUpperEv) && clipped.evidenceUpperEv>0.0)){
        std::cerr<<"high censor failure\n"; return 14;
    }

    // Absolute/common mode fails closed unless the scene is normalized to a common physical scale.
    TruthRangeGaugeV02 pg; pg.mode=TruthRangeGaugeModeV02::PhysicalAbsoluteGauge; pg.L0=1.0;
    pg.gaugeId="PHYSICAL_TEST"; pg.crossSceneComparable=true; pg.absolutePhysicalUnits=true;
    if(validate_gauge_for_scene_v0_2(scene,pg)){
        std::cerr<<"physical gauge should have failed closed\n"; return 15;
    }

    std::cout<<"TruthRange latent binding v0.2: PASS\n";
    std::cout<<"stage2_parity_max_abs="<<stage2Max<<"\n";
    std::cout<<"measured_reinjection_max_abs="<<measuredMax<<"\n";
    std::cout<<"signed_stage2_min="<<*mm.first<<" over1_max="<<*mm.second<<"\n";
    std::cout<<"self_gauge_L0="<<g.L0<<" scaled_L0="<<g37.L0<<"\n";
    std::cout<<"self_gauge_scale_invariance_max_ev="<<scaleMax<<"\n";
    std::cout<<"v5g_p50="<<ub.p50<<" v5g_p95="<<ub.p95<<"\n";
    return 0;
}
