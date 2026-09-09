#include "truthrange_dense_uncertainty_v0_3.h"
#include "uncertainty_runtime_v5_0g.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>

using namespace truthraw;

static int color_for_bggr(int y,int x){ static const int p[4]={2,1,1,0}; return p[(y&1)*2+(x&1)]; }
static int role_for_bggr(int y,int x){ static const int p[4]={3,1,2,0}; return p[(y&1)*2+(x&1)]; }

static DecodedDngFrame make_frame(){
    DecodedDngFrame f;
    f.meta.width=48; f.meta.height=40; f.meta.cfa=CfaPattern::BGGR;
    f.meta.whiteLevel=1023.f; f.meta.blackPhase={64.f,65.f,66.f,67.f};
    f.meta.hasGainField=true; f.meta.hasNoiseProfile=true;
    f.meta.noiseProfile={0.00028f,2e-7f,0.00031f,1.5e-7f,0.00029f,2.2e-7f};
    const std::size_t N=std::size_t(f.meta.width)*f.meta.height;
    f.raw.resize(N); f.gainField.resize(N);
    for(int y=0;y<f.meta.height;++y){
        for(int x=0;x<f.meta.width;++x){
            const std::size_t i=std::size_t(y)*f.meta.width+x;
            const int ph=(y&1)*2+(x&1);
            const float b=f.meta.blackPhase[ph];
            const float g=1.f+0.5f*float(x)/float(f.meta.width-1);
            f.gainField[i]=g;
            float n=0.004f+0.78f*float(x)/float(f.meta.width-1)+0.06f*std::sin(float(y)*0.19f)+0.025f*std::cos(float(x)*0.31f);
            if(x<2 && y<2)n=-0.006f;
            const float rawf=std::max(0.f,std::min(1023.f,b+n*(f.meta.whiteLevel-b)));
            f.raw[i]=static_cast<std::uint16_t>(std::lround(rawf));
        }
    }
    f.raw[std::size_t(12)*f.meta.width+30]=1023;
    f.raw[std::size_t(13)*f.meta.width+31]=1023;
    return f;
}

int main(){
    auto frame=make_frame();
    auto backend=std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
    LatentSceneBindingV02 binding;
    binding.reconstructionBackend=backend->name();
    binding.gainMapAppliedExactlyOnce=true;
    binding.reconstructionCoreCppSha256="68f52c4896d47c4605adc1cf670e55f95d42a687e18c06a167028a64ce37b94c";
    binding.reconstructionCoreHSha256="b7f6e2189d6ecccfc5fda80084041990bd12757145c5ff2c40f2ae0d0046b167";
    binding.uncertaintyModelSha256="8831bee921999e823466cfc462812e40620b2834080c0f7d64c6f11d7ead626f";
    binding.uncertaintyBindingSha256="61c99b0e29316730ca1323fd3e91fd069ebbc50cba73127cd81b9803b10178a0";

    LatentCameraSceneV02 scene;
    TilePolicy tile; tile.core=32; tile.halo=4;
    auto st=build_latent_camera_scene_v0_2(frame,tile,*backend,binding,scene);
    if(!st){std::cerr<<st.message<<"\n";return 1;}

    DenseUncertaintyFieldV03 field;
    st=build_measured_noiseprofile_field_v0_3(frame,scene,field);
    if(!st){std::cerr<<st.message<<"\n";return 2;}

    const std::size_t N=std::size_t(scene.width)*scene.height;
    std::size_t measuredValid=0,clippedMeasured=0,reconstructedInitiallyUnresolved=0;
    double formulaMax=0.0;
    constexpr double k50=0.6744897501960817, k95=1.959963984540054;
    for(std::size_t i=0;i<N;++i){
        const int c=int(scene.measuredChannel[i]);
        for(int cc=0;cc<3;++cc){
            const auto& e=field.rgb[3*i+cc];
            if(cc==c){
                if(scene.sourceHighCensor[i]){
                    if(e.valid || e.source!=DenseUncertaintySourceV03::MeasuredHighCensored)return 3;
                    clippedMeasured++;
                    continue;
                }
                if(!e.valid || !e.topologyCertified)return 4;
                measuredValid++;
                const float g=frame.gainField[i];
                const float S=frame.meta.noiseProfile[2*c],O=frame.meta.noiseProfile[2*c+1];
                const double sig=std::sqrt(std::max(double(g*S*std::max(scene.stage2Cfa[i],0.f)+g*g*O),0.0));
                formulaMax=std::max(formulaMax,std::abs(double(e.p50Abs)-k50*sig));
                formulaMax=std::max(formulaMax,std::abs(double(e.p95Abs)-k95*sig));
            }else if(!e.valid){
                reconstructedInitiallyUnresolved++;
            }
        }
    }
    if(formulaMax>2e-8 || measuredValid+clippedMeasured!=N || reconstructedInitiallyUnresolved!=2*N)return 5;

    BackendAnchorFieldV03 anchors;
    anchors.width=scene.width;anchors.height=scene.height;
    anchors.binding="SYNTHETIC_V5G_RUNTIME_ANCHOR_MECHANICS";
    anchors.rgb.resize(3*N);
    for(int y=0;y<scene.height;++y){
        for(int x=0;x<scene.width;++x){
            const std::size_t i=std::size_t(y)*scene.width+x;
            const int c=color_for_bggr(y,x);
            const int role=role_for_bggr(y,x);
            const auto& me=field.rgb[3*i+c];
            if(!me.valid)continue;
            const float mu=scene.cameraRgb[3*i+c];
            const float snr=std::abs(mu)/std::max(me.sigmaEquivalent,1e-8f);
            std::array<float,18> fv{};
            fv[0]=std::log1p(std::abs(mu));
            fv[1]=std::log1p(me.sigmaEquivalent*1e4f);
            fv[2]=std::log1p(std::max(snr,0.f));
            fv[9]=frame.gainField[i];
            fv[11]=std::sqrt(std::pow((float(x)/(scene.width-1)-.5f)/.5f,2.f)+std::pow((float(y)/(scene.height-1)-.5f)/.5f,2.f))/std::sqrt(2.f);
            fv[12]=mu<0.f?1.f:0.f;
            fv[13]=mu>1.f?1.f:0.f;
            fv[14+role]=1.f;
            auto u=predict_uncertainty_v5_0g(fv,role,snr);
            auto& a=anchors.rgb[3*i+c];
            a.valid=true;a.p50Abs=u.p50;a.p95Abs=u.p95;
            a.source=DenseUncertaintySourceV03::V5GMeasuredRoleAnchor;
            a.topologyCertified=true;
        }
    }

    // Fail closed: an anchor on a channel not directly measured at that site is rejected.
    {
        BackendAnchorFieldV03 bad=anchors;
        const std::size_t bi=std::size_t(8)*scene.width+8;
        const int measured=int(scene.measuredChannel[bi]);
        const int wrong=(measured+1)%3;
        bad.rgb[3*bi+wrong].valid=true;
        bad.rgb[3*bi+wrong].p50Abs=0.001f;
        bad.rgb[3*bi+wrong].p95Abs=0.002f;
        DenseUncertaintyFieldV03 tmp=field;
        if(transport_backend_anchors_v0_3(scene,bad,3,tmp))return 6;
    }

    st=transport_backend_anchors_v0_3(scene,anchors,3,field);
    if(!st){std::cerr<<st.message<<"\n";return 7;}

    std::size_t reconProxy=0,badTopo=0,unresolved=0;
    for(std::size_t i=0;i<N;++i){
        const int m=int(scene.measuredChannel[i]);
        for(int c=0;c<3;++c){
            if(c==m)continue;
            const auto&e=field.rgb[3*i+c];
            if(e.valid){reconProxy++;if(e.topologyCertified)badTopo++;}
            else unresolved++;
        }
    }
    if(badTopo || unresolved || reconProxy!=2*N)return 8;

    TruthRangeGaugeV02 gauge;
    st=derive_self_gauge_v0_2(scene,gauge,0.5,0.05);
    if(!st)return 9;
    DenseTruthRangeFieldV03 tr;
    st=map_dense_uncertainty_to_truthrange_v0_3(scene,field,gauge,tr);
    if(!st){std::cerr<<st.message<<"\n";return 10;}
    if(tr.rgb.size()!=3*N)return 11;

    std::size_t finiteRecon=0,darkTail=0,brightInf=0;
    for(const auto&r:tr.rgb){
        if(r.support==TruthRangeSupportV02::ReconstructedWeak && r.hasEstimate && std::isfinite(r.p95UpperEv))finiteRecon++;
        if(std::isinf(r.p95LowerEv)&&r.p95LowerEv<0)darkTail++;
        if(std::isinf(r.evidenceUpperEv)&&r.evidenceUpperEv>0&&r.support==TruthRangeSupportV02::MeasuredCensoredLowerBound)brightInf++;
    }
    if(finiteRecon==0 || brightInf!=clippedMeasured)return 12;

    const std::size_t ti=3*(std::size_t(20)*scene.width+20)+0;
    const auto& e=field.rgb[ti];
    if(!e.valid)return 13;
    const float mu=scene.cameraRgb[ti];
    if(!(mu>0.f))return 14;
    LatentUncertaintyV02 u{true,e.p50Abs,e.p95Abs,"V03"};
    auto a=map_latent_channel_to_truthrange_v0_2(mu,u,gauge,TruthRangeSupportV02::ReconstructedWeak,false,false,0.f);
    auto g37=gauge;g37.L0*=37.0;
    LatentUncertaintyV02 u37{true,e.p50Abs*37.f,e.p95Abs*37.f,"V03"};
    auto b=map_latent_channel_to_truthrange_v0_2(mu*37.f,u37,g37,TruthRangeSupportV02::ReconstructedWeak,false,false,0.f);
    const double inv=std::max({
        std::abs(a.estimateEv-b.estimateEv),
        std::abs(a.p50LowerEv-b.p50LowerEv),
        std::abs(a.p50UpperEv-b.p50UpperEv),
        std::abs(a.p95LowerEv-b.p95LowerEv),
        std::abs(a.p95UpperEv-b.p95UpperEv)});
    if(inv>2e-6)return 15;

    std::cout<<"TruthRange dense uncertainty v0.3 contract: PASS\n";
    std::cout<<"measured_valid="<<measuredValid<<" measured_clipped="<<clippedMeasured<<" formula_max_abs="<<formulaMax<<"\n";
    std::cout<<"reconstructed_proxy="<<reconProxy<<" unresolved="<<unresolved<<" topology_certified_reconstructed="<<badTopo<<"\n";
    std::cout<<"truthrange_finite_reconstructed="<<finiteRecon<<" dark_lower_inf_count="<<darkTail<<" bright_evidence_upper_inf="<<brightInf<<"\n";
    std::cout<<"coordinate_rescale_interval_max_ev="<<inv<<"\n";
    return 0;
}
