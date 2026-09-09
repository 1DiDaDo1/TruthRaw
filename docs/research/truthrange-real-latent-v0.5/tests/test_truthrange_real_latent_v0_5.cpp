#include "truthrange_real_latent_v0_5.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace truthraw;
using namespace truthraw_v05;

static void req(bool v,const char* m){if(!v)throw std::runtime_error(m);}
static std::size_t i3(std::size_t i,int c){return 3*i+std::size_t(c);}

int main(){
    constexpr int W=96,H=96; const std::size_t N=std::size_t(W)*H;
    DecodedDngFrame f;f.meta.width=W;f.meta.height=H;f.meta.cfa=CfaPattern::BGGR;f.meta.whiteLevel=1023;f.meta.blackPhase={64,64,64,64};f.meta.noiseProfile={5e-5f,4e-7f,9e-5f,2e-8f,6e-5f,4e-7f};f.meta.hasNoiseProfile=true;f.meta.hasGainField=true;f.raw.resize(N);f.gainField.resize(N);
    LatentCameraSceneV02 s;s.width=W;s.height=H;s.cfa=CfaPattern::BGGR;s.stage2Cfa.resize(N);s.cameraRgb.resize(3*N);s.measuredChannel.resize(N);s.sourceHighCensor.assign(N,0);s.sourceHighCensorLower.resize(N,1.f);
    for(int y=0;y<H;++y)for(int x=0;x<W;++x){auto i=std::size_t(y)*W+x;float v=0.03f+0.0007f*x+0.0004f*y+0.002f*float((x*y)%7);s.stage2Cfa[i]=v;int ph=(y&1)*2+(x&1);int c=ph==0?2:(ph==3?0:1);s.measuredChannel[i]=std::uint8_t(c);for(int k=0;k<3;++k)s.cameraRgb[i3(i,k)]=v+0.01f*k; s.cameraRgb[i3(i,c)]=v;f.gainField[i]=1.f+0.0008f*x;f.raw[i]=std::uint16_t(std::lround(64.f+v*(1023.f-64.f)/f.gainField[i]));}
    TruthRangeGaugeV02 g;g.mode=TruthRangeGaugeModeV02::SelfGauge;g.L0=.08;g.gaugeId="synthetic";

    req(role_at_v0_5(0,0)==BayerRoleV05::B,"BGGR B role");
    req(role_at_v0_5(0,1)==BayerRoleV05::G1,"BGGR G1 role");
    req(role_at_v0_5(1,0)==BayerRoleV05::G2,"BGGR G2 role");
    req(role_at_v0_5(1,1)==BayerRoleV05::R,"BGGR R role");

    float pred=0,sig=0,snr=0;auto x=build_v5g_features_exact_v0_5(f,s,BayerRoleV05::R,41,41,pred,sig,snr);
    req(x[17]==1.f && x[14]==0.f,"frozen R one-hot must occupy historical column 17");
    auto before=x;auto i=std::size_t(41)*W+41;float old=s.stage2Cfa[i];s.stage2Cfa[i]=old+100.f;float p2=0,s2=0,n2=0;auto after=build_v5g_features_exact_v0_5(f,s,BayerRoleV05::R,41,41,p2,s2,n2);s.stage2Cfa[i]=old;
    for(int k=0;k<18;++k)req(before[k]==after[k],"anti-leak feature changed after central target change");
    req(pred==p2 && sig==s2 && snr==n2,"anti-leak predictor changed after central target change");
    auto ub=predict_uncertainty_v5_0g(before,role_runtime_code_v0_5(BayerRoleV05::R),snr);req(ub.p50>0&&ub.p95>ub.p50,"v5g runtime bands invalid");

    std::vector<BackendAnchorV05> anchors;
    for(int ry=0;ry<2;++ry)for(int rx=0;rx<2;++rx){
        for(int y=8+ry;y<H-8;y+=4)for(int xx=8+rx;xx<W-8;xx+=4){BayerRoleV05 r=role_at_v0_5(y,xx);BackendAnchorV05 a;a.y=y;a.x=xx;a.role=r;a.rgbChannel=role_rgb_channel_v0_5(r);a.p50Abs=.005f+0.001f*a.rgbChannel;a.p95Abs=.02f+0.002f*a.rgbChannel;anchors.push_back(a);}
    }
    DenseStreamSummaryV05 ds;auto st=stream_dense_truthrange_v0_5(f,s,g,anchors,12,32,ds);req(bool(st),st.message.c_str());req(ds.totalRgbEntries==3*N,"dense total mismatch");req(ds.measuredUncensored==N,"measured count mismatch");req(ds.measuredHighCensored==0,"unexpected censor");req(ds.reconstructedProxyValid>0,"no reconstructed proxy");req(ds.reconstructedProxyUnresolved==0,"synthetic reconstructed proxy unresolved");req(ds.truthrangeFiniteEstimate>0,"no finite TruthRange estimates");

    std::cout<<"TRUTHRANGE_V0_5_TEST PASS\n";
    std::cout<<"anti_leak_exact=1\n";
    std::cout<<"historical_role_onehot_quirk_preserved=1\n";
    std::cout<<"measured="<<ds.measuredUncensored<<" reconstructed="<<ds.reconstructedProxyValid<<"\n";
    return 0;
}
