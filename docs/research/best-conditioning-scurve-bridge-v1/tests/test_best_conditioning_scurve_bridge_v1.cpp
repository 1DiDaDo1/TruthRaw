#include "best_conditioning_scurve_bridge_v1.h"
#include "manifold_conditioning_v1.h"
#include <bit>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>
using namespace truthraw::appearance::bridge::v1;
static void req(bool x,const char* m){if(!x){std::cerr<<"FAIL: "<<m<<"\n";std::exit(2);}}
static float Y(const RgbLinear& x){return .2126f*x.r+.7152f*x.g+.0722f*x.b;}
static EvidenceImage ev(const std::vector<float>& l,const std::vector<float>& c,const std::vector<std::uint8_t>& z,bool audit=true){return {l,c,z,audit,1,1};}
int main(){
 namespace mc=truthraw::conditioning::v1; mc::GaussianScalar gs{.013f,.004f}; mc::ConditioningGauge gauge; req(mc::choose_best_conditioning_gauge(gs,{},gauge)==mc::Status::Ok,"choose gauge"); mc::ConditionedScalar cs; req(mc::condition_exact(gs,gauge,cs)==mc::Status::Ok,"condition"); mc::GaussianScalar back; req(mc::decondition_exact(cs,back)==mc::Status::Ok,"decondition"); req(std::bit_cast<unsigned>(gs.mean)==std::bit_cast<unsigned>(back.mean),"mean bits"); req(std::bit_cast<unsigned>(gs.sigma)==std::bit_cast<unsigned>(back.sigma),"sigma bits"); req(mc::snr_abs(gs)==mc::snr_abs(cs.value),"snr invariant");
 constexpr std::size_t W=128,H=32,N=W*H; std::vector<std::uint8_t> cens(N,0); std::vector<float> cc(N,.5f),lc(N,.1f); std::vector<RgbLinear> flat(N,{.08f,.08f,.08f}); for(std::size_t y=0;y<H;y++)for(std::size_t x=W/2;x<W;x++)lc[y*W+x]=.9f;
 auto bad=apply_image(flat,W,H,ev(lc,cc,cens,false)); req(bad.status==Status::ConditioningAuditMissing,"audit fail closed"); for(std::size_t i=0;i<N;i++)req(bad.rgb[i].r==flat[i].r&&bad.rgb[i].g==flat[i].g&&bad.rgb[i].b==flat[i].b,"identity");
 auto a=apply_image(flat,W,H,ev(lc,cc,cens,true)); float maxg=0; for(std::size_t x=1;x<W;x++)maxg=std::max(maxg,std::fabs(Y(a.rgb[H/2*W+x])-Y(a.rgb[H/2*W+x-1]))); req(maxg<=1e-7f,"flat seam");
 std::vector<RgbLinear> ramp(N); lc.assign(N,.15f); for(std::size_t y=0;y<H;y++)for(std::size_t x=0;x<W;x++){float v=.005f+.495f*float(x)/(W-1);ramp[y*W+x]={v,v,v};if(x>=W/2)lc[y*W+x]=.9f;} auto r=apply_image(ramp,W,H,ev(lc,cc,cens,true)); float mind=1; for(std::size_t x=1;x<W;x++)mind=std::min(mind,Y(r.rgb[H/2*W+x])-Y(r.rgb[H/2*W+x-1])); req(mind>=-1e-7f,"monotonic ramp");
 std::vector<RgbLinear> edge(N,{.06f,.06f,.06f});lc.assign(N,.1f);for(std::size_t y=0;y<H;y++)for(std::size_t x=W/2;x<W;x++){edge[y*W+x]={.35f,.35f,.35f};lc[y*W+x]=.9f;}Config cfg;cfg.sigmaRange=.04f;auto eo=apply_image(edge,W,H,ev(lc,cc,cens,true),cfg);float ein=Y(edge[H/2*W+W/2])-Y(edge[H/2*W+W/2-1]),eout=Y(eo.rgb[H/2*W+W/2])-Y(eo.rgb[H/2*W+W/2-1]);req(eout/ein>.95f,"edge retention");
 std::vector<RgbLinear> hf(N);std::vector<float> low(N,.05f),high(N,.95f);for(std::size_t y=0;y<H;y++)for(std::size_t x=0;x<W;x++){float v=.07f+.008f*std::sin(2.0*3.14159265358979323846*double(x)/4.0);hf[y*W+x]={v,v,v};}auto lo=apply_image(hf,W,H,ev(low,cc,cens,true)),hi=apply_image(hf,W,H,ev(high,cc,cens,true));double lea=0,hea=0;for(std::size_t x=2;x<W-2;x++){float lm=0,hm=0;for(int k=-2;k<=2;k++){lm+=Y(lo.rgb[H/2*W+x+k]);hm+=Y(hi.rgb[H/2*W+x+k]);}lm/=5;hm/=5;lea+=std::pow(Y(lo.rgb[H/2*W+x])-lm,2);hea+=std::pow(Y(hi.rgb[H/2*W+x])-hm,2);}req(lea<hea,"low conf compresses more");
 std::vector<RgbLinear> sat(N,{1.0f,0.0f,0.0f});std::vector<float> oneSat(N,1.0f);std::vector<std::uint8_t> noCens(N,0);auto so=apply_image(sat,W,H,ev(oneSat,oneSat,noCens,true));req(so.status==Status::Applied,"saturated applied");req(so.chromaGain[N/2]<1.25f,"gamut headroom cap");req(so.rgb[N/2].r<=1.0f&&so.rgb[N/2].g>=0.0f&&so.rgb[N/2].b>=0.0f,"saturated gamut bound");
 std::vector<RgbLinear> color(N,{.3f,.15f,.08f});std::vector<float> one(N,1);cens.assign(N,1);auto co=apply_image(color,W,H,ev(one,one,cens,true));for(float g:co.chromaGain)req(g<=1.0f+1e-7f,"censored chroma");
 std::cout<<"flat_max_gradient="<<maxg<<"\n"<<"ramp_min_diff="<<mind<<"\n"<<"edge_retention="<<eout/ein<<"\n"<<"hf_energy_low_over_high="<<lea/hea<<"\nBEST_CONDITIONING_SCURVE_BRIDGE_V1_PASS\n";}
