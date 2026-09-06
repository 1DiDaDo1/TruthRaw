#include "output_acutance.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace truthraw_v47k {
namespace {
inline float clamp01(float x){return std::max(0.0f,std::min(1.0f,x));}
inline float smoothstep01(float x){x=clamp01(x);return x*x*(3.0f-2.0f*x);}
inline int clampi(int x,int n){return std::max(0,std::min(n-1,x));}
inline float sample(const std::vector<float>& a,int w,int h,int x,int y){return a[std::size_t(clampi(y,h))*w+clampi(x,w)];}
}

float luminance709(float r,float g,float b){
    return 0.2126f*std::max(r,0.0f)+0.7152f*std::max(g,0.0f)+0.0722f*std::max(b,0.0f);
}

OutputAcutancePlan choose_output_acutance_plan(float n,float rr,OutputProfile profile){
    OutputAcutancePlan p;
    p.noiseSigmaAt2Pct=n;
    float q=n>0.0f?1.0f-smoothstep01((n-0.00125f)/(0.00180f-0.00125f)):0.65f;
    rr=std::max(rr,1.0f);
    float need=smoothstep01((rr-1.0f)/1.25f);
    float base=(0.050f+0.110f*need)*q;
    if(profile==OutputProfile::SkinSafe)base*=0.86f;
    else if(profile==OutputProfile::Neutral)base*=0.72f;
    p.noiseConfidence=q;p.resizeRatio=rr;p.resizeNeed=need;
    p.strength=std::max(0.012f,std::min(0.130f,base));
    p.deltaCap=0.0045f+0.0025f*q;
    return p;
}

bool apply_output_acutance(const float* rgb,int w,int h,const OutputAcutancePlan& p,float* out){
    if(!rgb||!out||w<=0||h<=0)return false;
    const std::size_t N=std::size_t(w)*h;
    std::vector<float> Y(N),tmp(N),blur(N),grad(N);
    for(std::size_t i=0;i<N;++i)Y[i]=luminance709(rgb[3*i],rgb[3*i+1],rgb[3*i+2]);
    // separable [0.20, 0.60, 0.20], edge clamp
    for(int y=0;y<h;++y)for(int x=0;x<w;++x){
        tmp[std::size_t(y)*w+x]=0.20f*sample(Y,w,h,x-1,y)+0.60f*sample(Y,w,h,x,y)+0.20f*sample(Y,w,h,x+1,y);
    }
    for(int y=0;y<h;++y)for(int x=0;x<w;++x){
        blur[std::size_t(y)*w+x]=0.20f*sample(tmp,w,h,x,y-1)+0.60f*sample(tmp,w,h,x,y)+0.20f*sample(tmp,w,h,x,y+1);
    }
    for(int y=0;y<h;++y)for(int x=0;x<w;++x){
        float p00=sample(Y,w,h,x-1,y-1),p01=sample(Y,w,h,x,y-1),p02=sample(Y,w,h,x+1,y-1);
        float p10=sample(Y,w,h,x-1,y),  p12=sample(Y,w,h,x+1,y);
        float p20=sample(Y,w,h,x-1,y+1),p21=sample(Y,w,h,x,y+1),p22=sample(Y,w,h,x+1,y+1);
        float gx=(-p00+p02-2.0f*p10+2.0f*p12-p20+p22)*0.25f;
        float gy=(-p00-2.0f*p01-p02+p20+2.0f*p21+p22)*0.25f;
        grad[std::size_t(y)*w+x]=std::sqrt(gx*gx+gy*gy);
    }
    const float n=p.noiseSigmaAt2Pct/std::sqrt(std::max(p.resizeRatio,1.0f));
    const float nf=std::max(n,2e-4f);
    for(std::size_t i=0;i<N;++i){
        float yy=Y[i],bl=blur[i],detail=yy-bl;
        float edgeNorm=grad[i]/std::max(bl,0.035f);
        float hard=smoothstep01((edgeNorm-p.hardEdgeNormStart)/std::max(p.hardEdgeNormFull-p.hardEdgeNormStart,1e-8f));
        float edgeGuard=1.0f-0.75f*hard;
        float shadowGate=0.18f+0.82f*smoothstep01((yy-p.shadowStart)/std::max(p.shadowFull-p.shadowStart,1e-8f));
        float highGate=1.0f-0.82f*smoothstep01((yy-p.highlightStart)/std::max(p.highlightFull-p.highlightStart,1e-8f));
        float dsnr=std::abs(detail)/nf;
        float textureConf=0.35f+0.65f*smoothstep01((dsnr-0.7f)/2.0f);
        float ad=std::abs(detail);
        float shrink=std::max(ad-0.35f*nf,0.0f)/std::max(ad,1e-8f);
        float delta=p.strength*(detail*shrink)*edgeGuard*shadowGate*highGate*textureConf;
        delta=std::max(-p.deltaCap,std::min(p.deltaCap,delta));
        float yo=std::max(yy+delta,0.0f);
        float sc=yy>1e-8f?yo/yy:1.0f;
        out[3*i]=std::max(rgb[3*i],0.0f)*sc;
        out[3*i+1]=std::max(rgb[3*i+1],0.0f)*sc;
        out[3*i+2]=std::max(rgb[3*i+2],0.0f)*sc;
    }
    return true;
}

} // namespace truthraw_v47k
