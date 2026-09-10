#include "best_conditioning_scurve_bridge_v1.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace truthraw::appearance::bridge::v1 {
namespace {
constexpr float kWr=0.2126f,kWg=0.7152f,kWb=0.0722f,kEps=1.0e-8f;
float clamp01(float x) noexcept { return std::clamp(x,0.0f,1.0f); }
float smoothstep01(float x) noexcept { const float t=clamp01(x); return t*t*(3.0f-2.0f*t); }
float luminance709(const RgbLinear& x) noexcept { return kWr*x.r+kWg*x.g+kWb*x.b; }
float s_curve01(float x,float s) noexcept { const float t=clamp01(x); return clamp01(t+s*t*(1.0f-t)*(2.0f*t-1.0f)); }
bool finite_rgb(const RgbLinear& x) noexcept { return std::isfinite(x.r)&&std::isfinite(x.g)&&std::isfinite(x.b); }
std::size_t at(std::size_t x,std::size_t y,std::size_t w) noexcept { return y*w+x; }
std::size_t mirror(long v,std::size_t n) noexcept { if(n<=1)return 0; const long hi=static_cast<long>(n-1); while(v<0||v>hi){ if(v<0)v=-v; if(v>hi)v=2*hi-v; } return static_cast<std::size_t>(v); }
std::vector<float> joint_filter(std::span<const float> f,std::span<const RgbLinear> guide,std::size_t w,std::size_t h,const Config& c){
 std::vector<float> out(f.size()); const double ss2=2.0*c.sigmaSpatial*c.sigmaSpatial,sr2=2.0*c.sigmaRange*c.sigmaRange; const int r=c.filterRadius;
 for(std::size_t y=0;y<h;++y)for(std::size_t x=0;x<w;++x){ const auto& g0=guide[at(x,y,w)]; double acc=0,ws=0;
  for(int dy=-r;dy<=r;++dy)for(int dx=-r;dx<=r;++dx){ const auto xx=mirror(static_cast<long>(x)+dx,w),yy=mirror(static_cast<long>(y)+dy,h),j=at(xx,yy,w); const auto& gj=guide[j];
   const double dr=gj.r-g0.r,dg=gj.g-g0.g,db=gj.b-g0.b,d2=dr*dr+dg*dg+db*db; const double ww=std::exp(-(dx*dx+dy*dy)/ss2)*std::exp(-d2/sr2); acc+=ww*f[j]; ws+=ww; }
  out[at(x,y,w)]=static_cast<float>(acc/std::max(ws,1e-30)); }
 return out; }
}

bool validate_config(const Config& c) noexcept {
 return std::isfinite(c.globalCurveStrength)&&c.globalCurveStrength>=0&&c.globalCurveStrength<=0.8f&&
 std::isfinite(c.shadowPivot)&&c.shadowPivot>0&&c.shadowPivot<=0.5f&&
 std::isfinite(c.maxResidualCompression)&&c.maxResidualCompression>=0&&c.maxResidualCompression<=0.6f&&
 std::isfinite(c.lowConfidenceChromaGain)&&c.lowConfidenceChromaGain>=0&&c.lowConfidenceChromaGain<=1&&
 std::isfinite(c.highConfidenceChromaGain)&&c.highConfidenceChromaGain>=1&&c.highConfidenceChromaGain<=1.5f&&
 std::isfinite(c.censoredMaxChromaGain)&&c.censoredMaxChromaGain>=0&&c.censoredMaxChromaGain<=1&&
 c.filterRadius>=1&&c.filterRadius<=12&&std::isfinite(c.sigmaSpatial)&&c.sigmaSpatial>0&&std::isfinite(c.sigmaRange)&&c.sigmaRange>0;
}

Result apply_image(std::span<const RgbLinear> rgb,std::size_t w,std::size_t h,const EvidenceImage& e,const Config& c){
 Result out; out.rgb.assign(rgb.begin(),rgb.end()); out.scientificMasterModified=false;
 if(!validate_config(c)){out.status=Status::InvalidConfig;return out;}
 if(w==0||h==0||w>std::numeric_limits<std::size_t>::max()/h||w*h!=rgb.size()||e.lumaConfidence.size()!=rgb.size()||e.chromaConfidence.size()!=rgb.size()||e.sourceHighCensored.size()!=rgb.size()){out.status=Status::InvalidInput;return out;}
 if(!e.conditioningAuditPassed||e.physicalFrameCount!=1||e.independentEvidenceCount!=1){out.status=Status::ConditioningAuditMissing;return out;}
 for(std::size_t i=0;i<rgb.size();++i){ if(!finite_rgb(rgb[i])||rgb[i].r<0||rgb[i].r>1||rgb[i].g<0||rgb[i].g>1||rgb[i].b<0||rgb[i].b>1||!std::isfinite(e.lumaConfidence[i])||e.lumaConfidence[i]<0||e.lumaConfidence[i]>1||!std::isfinite(e.chromaConfidence[i])||e.chromaConfidence[i]<0||e.chromaConfidence[i]>1||e.sourceHighCensored[i]>1){out.status=Status::InvalidInput;return out;} }
 out.regularizedLumaConfidence=joint_filter(e.lumaConfidence,rgb,w,h,c); out.regularizedChromaConfidence=joint_filter(e.chromaConfidence,rgb,w,h,c);
 std::vector<float> y(rgb.size()),yt(rgb.size()); std::vector<RgbLinear> tone(rgb.size());
 for(std::size_t i=0;i<rgb.size();++i){ y[i]=clamp01(luminance709(rgb[i])); yt[i]=s_curve01(y[i],c.globalCurveStrength); const float s=y[i]>kEps?yt[i]/y[i]:0; tone[i]={rgb[i].r*s,rgb[i].g*s,rgb[i].b*s}; }
 const auto base=joint_filter(yt,tone,w,h,c); out.residualGain.resize(rgb.size()); out.chromaGain.resize(rgb.size()); out.rgb.resize(rgb.size());
 for(std::size_t i=0;i<rgb.size();++i){ const float sw=1.0f-smoothstep01(y[i]/c.shadowPivot); const float rg=clamp01(1.0f-c.maxResidualCompression*sw*(1.0f-out.regularizedLumaConfidence[i])); const float yl=clamp01(base[i]+rg*(yt[i]-base[i])); const float ls=yt[i]>kEps?yl/yt[i]:0; const RgbLinear local{tone[i].r*ls,tone[i].g*ls,tone[i].b*ls}; const float q=smoothstep01(out.regularizedChromaConfidence[i]); float cg=c.lowConfidenceChromaGain+q*(c.highConfidenceChromaGain-c.lowConfidenceChromaGain); if(e.sourceHighCensored[i])cg=std::min(cg,c.censoredMaxChromaGain); out.residualGain[i]=rg; out.chromaGain[i]=cg; out.rgb[i]={clamp01(yl+(local.r-yl)*cg),clamp01(yl+(local.g-yl)*cg),clamp01(yl+(local.b-yl)*cg)}; }
 out.status=Status::Applied; return out;
}
} // namespace truthraw::appearance::bridge::v1
