#include "truthraw/core.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <limits>
#include <thread>

namespace truthraw {
namespace {
using Clock = std::chrono::steady_clock;
static inline double ms(Clock::time_point a, Clock::time_point b){return std::chrono::duration<double,std::milli>(b-a).count();}
static inline float clamp01(float x){return std::max(0.f,std::min(1.f,x));}
static inline float clampf(float x,float a,float b){return std::max(a,std::min(b,x));}
static inline float smoothstep01(float x){x=clamp01(x); return x*x*(3.f-2.f*x);}
template<class Fn> static void parallel_ranges(int count,int threads,Fn fn){threads=std::max(1,std::min(threads,count));if(threads<=1){fn(0,count);return;}std::vector<std::thread> pool;pool.reserve(threads);for(int t=0;t<threads;++t){int a=(count*t)/threads,b=(count*(t+1))/threads;pool.emplace_back([=,&fn]{fn(a,b);});}for(auto&th:pool)th.join();}

struct Hist {
    static constexpr int kBins=16384;
    std::vector<std::uint64_t> bins;
    float maxValue;
    std::uint64_t total=0;
    explicit Hist(float m):bins(kBins,0),maxValue(m){}
    void add(float x){x=std::max(0.f,std::min(maxValue,x));int i=int((x/maxValue)*float(kBins-1)+0.5f);bins[std::max(0,std::min(kBins-1,i))]++;total++;}
};

static float quantile_hist(const std::vector<std::uint64_t>& h,float maxValue,float q){
    std::uint64_t total=0; for(auto v:h) total+=v; if(total==0) return 0.f;
    const double target=q*double(total-1); std::uint64_t acc=0;
    for(std::size_t i=0;i<h.size();++i){auto next=acc+h[i];if(double(next)>target)return maxValue*float(i)/float(h.size()-1);acc=next;}return maxValue;
}

static inline int phase_index(int y,int x){return (y&1)*2+(x&1);}
static inline int color_for_phase(CfaPattern cfa,int phase){
    static const int BGGR[4]={2,1,1,0}; static const int RGGB[4]={0,1,1,2}; static const int GRBG[4]={1,0,2,1}; static const int GBRG[4]={1,2,0,1};
    const int* p=BGGR;if(cfa==CfaPattern::RGGB)p=RGGB;else if(cfa==CfaPattern::GRBG)p=GRBG;else if(cfa==CfaPattern::GBRG)p=GBRG;return p[phase];
}

static float sample_clamped(const float* a,int w,int h,int x,int y){x=std::max(0,std::min(w-1,x));y=std::max(0,std::min(h-1,y));return a[std::size_t(y)*w+x];}
static float avg_valid(const float* a,int w,int h,const int* xy,int n){float s=0.f;int c=0;for(int i=0;i<n;++i){int x=xy[2*i],y=xy[2*i+1];if(x>=0&&x<w&&y>=0&&y<h){s+=a[std::size_t(y)*w+x];c++;}}return c?s/float(c):0.f;}

static void stage2_tile(const DecodedDngFrame& f,const TileRect& t,std::vector<float>& out){
    const auto& m=f.meta;const int tw=t.hx1-t.hx0,th=t.hy1-t.hy0;out.resize(std::size_t(tw)*th);
    for(int yy=0;yy<th;++yy){int y=t.hy0+yy;for(int xx=0;xx<tw;++xx){int x=t.hx0+xx;std::size_t gi=std::size_t(y)*m.width+x;int ph=phase_index(y,x);float b=m.blackPhase[ph];if(m.hasResidualBlack)b+=f.rowBias[y]+f.colBias[x];float denom=std::max(m.whiteLevel-b,1.f);float s=(float(f.raw[gi])-b)/denom;if(m.hasGainField)s*=f.gainField[gi];out[std::size_t(yy)*tw+xx]=s;}}
}

static void camera_to_xyz(const float* cam,float* xyz,int n,const std::array<float,9>& M){for(int i=0;i<n;++i){float r=cam[3*i],g=cam[3*i+1],b=cam[3*i+2];xyz[3*i]=M[0]*r+M[1]*g+M[2]*b;xyz[3*i+1]=M[3]*r+M[4]*g+M[5]*b;xyz[3*i+2]=M[6]*r+M[7]*g+M[8]*b;}}

static void xyz_d50_to_linear_srgb(const float* xyz,float* rgb,int n){
    static const float M[9]={3.1338561f,-1.6168667f,-0.4906146f,-0.9787684f,1.9161415f,0.0334540f,0.0719453f,-0.2289914f,1.4052427f};
    for(int i=0;i<n;++i){float X=xyz[3*i],Y=xyz[3*i+1],Z=xyz[3*i+2];rgb[3*i]=M[0]*X+M[1]*Y+M[2]*Z;rgb[3*i+1]=M[3]*X+M[4]*Y+M[5]*Z;rgb[3*i+2]=M[6]*X+M[7]*Y+M[8]*Z;}
}

static float noise_sigma_2pct(const DngMetadata& m){if(!m.hasNoiseProfile)return 0.f;float v=0.f;for(int c=0;c<3;++c){float S=m.noiseProfile[2*c],O=m.noiseProfile[2*c+1];v+=std::max(S*0.02f+O,0.f);}return std::sqrt(v/3.f);}
static float lut_sample(const std::vector<float>& lut,float x){x=clamp01(x);float u=x*float(lut.size()-1);int i=int(u),j=std::min(i+1,int(lut.size()-1));float f=u-float(i);return lut[i]+(lut[j]-lut[i])*f;}

static inline float directional_blend(float a,float b,float ga,float gb){
    constexpr float e=1e-7f;if(ga<0.72f*gb)return a;if(gb<0.72f*ga)return b;float wa=1.f/(ga+e),wb=1.f/(gb+e);return (wa*a+wb*b)/(wa+wb);
}
static inline void directional_axis(const float* a,int w,int h,int x,int y,int dx,int dy,float measured,float& est,float& grad){
    float p=sample_clamped(a,w,h,x-dx,y-dy),q=sample_clamped(a,w,h,x+dx,y+dy);float c0=sample_clamped(a,w,h,x-2*dx,y-2*dy),c1=sample_clamped(a,w,h,x+2*dx,y+2*dy);
    float curvature=2.f*measured-c0-c1;est=0.5f*(p+q)+0.25f*curvature;grad=std::abs(p-q)+0.5f*std::abs(curvature);
}


static inline float support_limited(float estimate,const float* vals,int n){
    float lo=vals[0],hi=vals[0];
    for(int i=1;i<n;++i){lo=std::min(lo,vals[i]);hi=std::max(hi,vals[i]);}
    float margin=0.125f*(hi-lo)+1e-5f;
    return clampf(estimate,lo-margin,hi+margin);
}

struct Oklab{float L,a,b;};
static inline Oklab linear_srgb_to_oklab(float r,float g,float b){
    r=std::max(r,0.f);g=std::max(g,0.f);b=std::max(b,0.f);
    float l=0.4122214708f*r+0.5363325363f*g+0.0514459929f*b;
    float m=0.2119034982f*r+0.6806995451f*g+0.1073969566f*b;
    float s=0.0883024619f*r+0.2817188376f*g+0.6299787005f*b;
    float lp=std::cbrt(l),mp=std::cbrt(m),sp=std::cbrt(s);
    return {0.2104542553f*lp+0.7936177850f*mp-0.0040720468f*sp,1.9779984951f*lp-2.4285922050f*mp+0.4505937099f*sp,0.0259040371f*lp+0.7827717662f*mp-0.8086757660f*sp};
}
static inline void oklab_to_linear_srgb(const Oklab& q,float& r,float& g,float& b){
    float lp=q.L+0.3963377774f*q.a+0.2158037573f*q.b;float mp=q.L-0.1055613458f*q.a-0.0638541728f*q.b;float sp=q.L-0.0894841775f*q.a-1.2914855480f*q.b;
    float l=lp*lp*lp,m=mp*mp*mp,s=sp*sp*sp;r=4.0767416621f*l-3.3077115913f*m+0.2309699292f*s;g=-1.2684380046f*l+2.6097574011f*m-0.3413193965f*s;b=-0.0041960863f*l-0.7034186147f*m+1.7076147010f*s;
}

static inline float integral_box_mean(const std::vector<float>& integ,int stride,int w,int h,int x,int y,int rad){
    int x0=std::max(0,x-rad),x1=std::min(w-1,x+rad),y0=std::max(0,y-rad),y1=std::min(h-1,y+rad);int xa=x0,xb=x1+1,ya=y0,yb=y1+1;
    float sum=integ[std::size_t(yb)*stride+xb]-integ[std::size_t(ya)*stride+xb]-integ[std::size_t(yb)*stride+xa]+integ[std::size_t(ya)*stride+xa];return sum/float((x1-x0+1)*(y1-y0+1));
}

} // namespace

float luminance709(float r,float g,float b){return 0.2126f*r+0.7152f*g+0.0722f*b;}

std::vector<TileRect> make_tiles(int width,int height,const TilePolicy& p){std::vector<TileRect> out;if(width<=0||height<=0||p.core<=0||p.halo<0)return out;for(int y=0;y<height;y+=p.core)for(int x=0;x<width;x+=p.core){TileRect t;t.x0=x;t.y0=y;t.x1=std::min(x+p.core,width);t.y1=std::min(y+p.core,height);t.hx0=std::max(0,t.x0-p.halo);t.hy0=std::max(0,t.y0-p.halo);t.hx1=std::min(width,t.x1+p.halo);t.hy1=std::min(height,t.y1+p.halo);out.push_back(t);}return out;}

Status ReferenceMeasuredPreservingReconstruction::reconstructTile(const float* a,int w,int h,int ghx0,int ghy0,int coreX0,int coreY0,int coreW,int coreH,CfaPattern cfa,float* out){
    if(!a||!out||w<=0||h<=0)return Status::error(StatusCode::InvalidArgument,"invalid reconstruction tile");
    for(int cy=0;cy<coreH;++cy){int gy=coreY0+cy,ly=gy-ghy0;for(int cx=0;cx<coreW;++cx){int gx=coreX0+cx,lx=gx-ghx0;float rgb[3]={0,0,0};int ph=phase_index(gy,gx),c=color_for_phase(cfa,ph);float m=sample_clamped(a,w,h,lx,ly);rgb[c]=m;if(c==0){int gxy[]={lx-1,ly,lx+1,ly,lx,ly-1,lx,ly+1};rgb[1]=avg_valid(a,w,h,gxy,4);int bxy[]={lx-1,ly-1,lx+1,ly-1,lx-1,ly+1,lx+1,ly+1};rgb[2]=avg_valid(a,w,h,bxy,4);}else if(c==2){int gxy[]={lx-1,ly,lx+1,ly,lx,ly-1,lx,ly+1};rgb[1]=avg_valid(a,w,h,gxy,4);int rxy[]={lx-1,ly-1,lx+1,ly-1,lx-1,ly+1,lx+1,ly+1};rgb[0]=avg_valid(a,w,h,rxy,4);}else{int leftColor=color_for_phase(cfa,phase_index(gy,gx-1));if(leftColor==0){int rxy[]={lx-1,ly,lx+1,ly};int bxy[]={lx,ly-1,lx,ly+1};rgb[0]=avg_valid(a,w,h,rxy,2);rgb[2]=avg_valid(a,w,h,bxy,2);}else{int bxy[]={lx-1,ly,lx+1,ly};int rxy[]={lx,ly-1,lx,ly+1};rgb[2]=avg_valid(a,w,h,bxy,2);rgb[0]=avg_valid(a,w,h,rxy,2);}}std::size_t oi=std::size_t(cy)*coreW+cx;out[3*oi]=rgb[0];out[3*oi+1]=rgb[1];out[3*oi+2]=rgb[2];}}
    return Status::ok();
}

Status ResearchEdgeAwareMeasuredPreservingReconstruction::reconstructTile(const float* a,int w,int h,int ghx0,int ghy0,int coreX0,int coreY0,int coreW,int coreH,CfaPattern cfa,float* out){
    if(!a||!out||w<=0||h<=0)return Status::error(StatusCode::InvalidArgument,"invalid research reconstruction tile");
    // Pass 1: reconstruct a complete green plane. Measured green stays exact;
    // R/B sites use Hamilton-Adams-style directional interpolation.
    thread_local std::vector<float> greenScratch;
    greenScratch.resize(std::size_t(w)*h);
    auto& green=greenScratch;
    for(int ly=0;ly<h;++ly){
        int gy=ghy0+ly;
        for(int lx=0;lx<w;++lx){
            int gx=ghx0+lx,c=color_for_phase(cfa,phase_index(gy,gx));
            float m=sample_clamped(a,w,h,lx,ly);
            if(c==1){green[std::size_t(ly)*w+lx]=m;continue;}
            float eh,ev,gh,gv;
            directional_axis(a,w,h,lx,ly,1,0,m,eh,gh);
            directional_axis(a,w,h,lx,ly,0,1,m,ev,gv);
            float ge=directional_blend(eh,ev,gh,gv);
            float support[4]={sample_clamped(a,w,h,lx-1,ly),sample_clamped(a,w,h,lx+1,ly),sample_clamped(a,w,h,lx,ly-1),sample_clamped(a,w,h,lx,ly+1)};
            green[std::size_t(ly)*w+lx]=support_limited(ge,support,4);
        }
    }
    auto colorDiffEstimate=[&](int lx,int ly,int targetColor,const int* xy,int n)->float{
        float gc=green[std::size_t(ly)*w+lx],sum=0.f,ws=0.f;
        for(int i=0;i<n;++i){
            int xx=std::max(0,std::min(w-1,xy[2*i])),yy=std::max(0,std::min(h-1,xy[2*i+1]));
            int gx=ghx0+xx,gy=ghy0+yy;
            if(color_for_phase(cfa,phase_index(gy,gx))!=targetColor)continue;
            float gn=green[std::size_t(yy)*w+xx],cn=a[std::size_t(yy)*w+xx];
            float wt=1.f/(1e-4f+std::abs(gn-gc));sum+=wt*(cn-gn);ws+=wt;
        }
        float est=ws>0.f?gc+sum/ws:gc;
        float vals[4]={gc,gc,gc,gc}; int vc=0;
        for(int i=0;i<n&&vc<4;++i){
            int xx=std::max(0,std::min(w-1,xy[2*i])),yy=std::max(0,std::min(h-1,xy[2*i+1]));
            int gx=ghx0+xx,gy=ghy0+yy;
            if(color_for_phase(cfa,phase_index(gy,gx))==targetColor) vals[vc++]=a[std::size_t(yy)*w+xx];
        }
        return vc>0?support_limited(est,vals,vc):gc;
    };
    // Pass 2: reconstruct red/blue as locally smooth color differences relative
    // to the edge-aware green plane. The measured CFA component is reinjected exactly.
    for(int cy=0;cy<coreH;++cy){
        int gy=coreY0+cy,ly=gy-ghy0;
        for(int cx=0;cx<coreW;++cx){
            int gx=coreX0+cx,lx=gx-ghx0,c=color_for_phase(cfa,phase_index(gy,gx));
            float m=sample_clamped(a,w,h,lx,ly),rgb[3]={0,green[std::size_t(ly)*w+lx],0};
            if(c==0){
                rgb[0]=m;
                int diag[]={lx-1,ly-1,lx+1,ly-1,lx-1,ly+1,lx+1,ly+1};
                rgb[2]=colorDiffEstimate(lx,ly,2,diag,4);
            }else if(c==2){
                rgb[2]=m;
                int diag[]={lx-1,ly-1,lx+1,ly-1,lx-1,ly+1,lx+1,ly+1};
                rgb[0]=colorDiffEstimate(lx,ly,0,diag,4);
            }else{
                rgb[1]=m;
                int leftColor=color_for_phase(cfa,phase_index(gy,gx-1));
                if(leftColor==0){
                    int rxy[]={lx-1,ly,lx+1,ly};int bxy[]={lx,ly-1,lx,ly+1};
                    rgb[0]=colorDiffEstimate(lx,ly,0,rxy,2);rgb[2]=colorDiffEstimate(lx,ly,2,bxy,2);
                }else{
                    int bxy[]={lx-1,ly,lx+1,ly};int rxy[]={lx,ly-1,lx,ly+1};
                    rgb[2]=colorDiffEstimate(lx,ly,2,bxy,2);rgb[0]=colorDiffEstimate(lx,ly,0,rxy,2);
                }
            }
            std::size_t oi=std::size_t(cy)*coreW+cx;
            out[3*oi]=rgb[0];out[3*oi+1]=rgb[1];out[3*oi+2]=rgb[2];
        }
    }
    return Status::ok();
}

Status NeutralReferenceAppearance::applyTile(const float* a,int w,int h,int cx0,int cy0,int cw,int ch,const AppearanceContext&,float* out) const{
    if(!a||!out||w<=0||h<=0)return Status::error(StatusCode::InvalidArgument,"invalid neutral appearance tile");
    for(int y=0;y<ch;++y){
        for(int x=0;x<cw;++x){
            std::size_t si=std::size_t(cy0+y)*w+(cx0+x),oi=std::size_t(y)*cw+x;
            out[3*oi]=a[3*si];out[3*oi+1]=a[3*si+1];out[3*oi+2]=a[3*si+2];
        }
    }
    return Status::ok();
}

Status SkinSafeDetailedCrispAppearance::applyTile(const float* a,int w,int h,int cx0,int cy0,int cw,int ch,const AppearanceContext&,float* out) const{
    if(!a||!out||w<=0||h<=0||cx0<0||cy0<0||cx0+cw>w||cy0+ch>h)return Status::error(StatusCode::InvalidArgument,"invalid skin-safe appearance tile");
    const int stride=w+1;
    thread_local std::vector<float> integralScratch;
    integralScratch.assign(std::size_t(w+1)*(h+1),0.f);
    auto& integ=integralScratch;
    for(int y=0;y<h;++y){float row=0.f;for(int x=0;x<w;++x){std::size_t i=std::size_t(y)*w+x;float Y=std::max(luminance709(std::max(a[3*i],0.f),std::max(a[3*i+1],0.f),std::max(a[3*i+2],0.f)),0.f);row+=Y;integ[std::size_t(y+1)*stride+(x+1)]=integ[std::size_t(y)*stride+(x+1)]+row;}}
    for(int y=0;y<ch;++y)for(int x=0;x<cw;++x){int tx=cx0+x,ty=cy0+y;std::size_t si=std::size_t(ty)*w+tx,oi=std::size_t(y)*cw+x;float nr=std::max(a[3*si],0.f),ng=std::max(a[3*si+1],0.f),nb=std::max(a[3*si+2],0.f);float Y=std::max(luminance709(nr,ng,nb),0.f);float b1=integral_box_mean(integ,stride,w,h,tx,ty,1),b4=integral_box_mean(integ,stride,w,h,tx,ty,4);float mid=clamp01((Y-0.02f)/0.45f)*clamp01((1.05f-Y)/0.35f);float yd=std::max(0.f,Y+(0.19f*(Y-b1)+0.11f*(Y-b4))*mid);float sc=Y>1e-8f?yd/Y:1.f;float dr=nr*sc,dg=ng*sc,db=nb*sc;
        Oklab N=linear_srgb_to_oklab(nr,ng,nb),D=linear_srgb_to_oklab(dr,dg,db);float Cn=std::sqrt(N.a*N.a+N.b*N.b);float brightGate=clamp01((N.L-0.08f)/0.65f);float vivid=clamp01(Cn/0.16f);float boost=1.045f+0.06f*(1.f-vivid)*brightGate;D.a*=boost;D.b*=boost;float chromaGate=clamp01((Cn-0.055f)/0.11f);float alpha=0.34f+0.34f*chromaGate;float darkRelax=clamp01((0.12f-N.L)/0.10f);alpha=clampf(alpha+0.16f*darkRelax,0.34f,0.78f);Oklab G{D.L,N.a+alpha*(D.a-N.a),N.b+alpha*(D.b-N.b)};float rr,gg,bb;oklab_to_linear_srgb(G,rr,gg,bb);out[3*oi]=std::max(rr,0.f);out[3*oi+1]=std::max(gg,0.f);out[3*oi+2]=std::max(bb,0.f);
    }return Status::ok();
}


Status AdaptiveDetailedCrispAppearance::applyTile(const float* a,int w,int h,int cx0,int cy0,int cw,int ch,const AppearanceContext& ctx,float* out) const{
    if(!a||!out||w<=0||h<=0||cx0<0||cy0<0||cx0+cw>w||cy0+ch>h)return Status::error(StatusCode::InvalidArgument,"invalid adaptive detailed appearance tile");
    const std::size_t N=std::size_t(w)*h, IN=std::size_t(w+1)*(h+1); const int stride=w+1;
    thread_local std::vector<float> Y,grad,lap,iY,iGrad,iLap;
    Y.resize(N);grad.resize(N);lap.resize(N);iY.assign(IN,0.f);iGrad.assign(IN,0.f);iLap.assign(IN,0.f);
    for(int y=0;y<h;++y)for(int x=0;x<w;++x){std::size_t i=std::size_t(y)*w+x;Y[i]=std::max(luminance709(std::max(a[3*i],0.f),std::max(a[3*i+1],0.f),std::max(a[3*i+2],0.f)),0.f);}
    auto sy=[&](int x,int y){x=std::max(0,std::min(w-1,x));y=std::max(0,std::min(h-1,y));return Y[std::size_t(y)*w+x];};
    for(int y=0;y<h;++y)for(int x=0;x<w;++x){
        float c=sy(x,y),l=sy(x-1,y),r=sy(x+1,y),u=sy(x,y-1),d=sy(x,y+1);
        float gx=0.5f*(r-l),gy=0.5f*(d-u);std::size_t i=std::size_t(y)*w+x;
        grad[i]=std::sqrt(gx*gx+gy*gy);lap[i]=0.25f*std::abs(4.f*c-l-r-u-d);
    }
    auto buildIntegral=[&](const std::vector<float>&src,std::vector<float>&dst){for(int y=0;y<h;++y){float row=0.f;for(int x=0;x<w;++x){row+=src[std::size_t(y)*w+x];dst[std::size_t(y+1)*stride+(x+1)]=dst[std::size_t(y)*stride+(x+1)]+row;}}};
    buildIntegral(Y,iY);buildIntegral(grad,iGrad);buildIntegral(lap,iLap);
    auto meanFrom=[&](const std::vector<float>&in,int x,int y,int r){return integral_box_mean(in,stride,w,h,x,y,r);};
    float n2=ctx.noiseSigmaAt2Pct;float q=n2>0.f?1.f-smoothstep01((n2-0.00125f)/(0.00180f-0.00125f)):0.65f;
    float A=0.25f+0.45f*q,B=0.18f+0.26f*q,C=0.10f+0.12f*q;
    for(int y=0;y<ch;++y)for(int x=0;x<cw;++x){
        int tx=cx0+x,ty=cy0+y;std::size_t si=std::size_t(ty)*w+tx,oi=std::size_t(y)*cw+x;
        float nr=std::max(a[3*si],0.f),ng=std::max(a[3*si+1],0.f),nb=std::max(a[3*si+2],0.f),yy=Y[si];
        float b1=meanFrom(iY,tx,ty,1),b2=meanFrom(iY,tx,ty,2),b5=meanFrom(iY,tx,ty,5);
        float micro=yy-b1,fine=b1-b2,texture=b2-b5;
        float low=smoothstep01((yy-0.004f)/0.055f),high=1.f-smoothstep01((yy-0.88f)/0.30f),shadow=0.32f+0.68f*q,tone=high*(shadow+(1.f-shadow)*low);
        float sigmaProxy=(n2>0.f?n2:0.00145f)*std::sqrt(std::max(yy,0.005f)/0.02f);
        float conf=smoothstep01((std::abs(micro)/std::max(sigmaProxy,1e-6f)-0.80f)/1.60f);
        float microGate=0.20f+0.80f*conf,fineGate=0.42f+0.58f*conf,textureGate=0.70f+0.30f*conf;
        float activity=(std::abs(fine)+0.50f*std::abs(texture))/std::max(b5,0.03f);
        float activityGate=0.15f+0.85f*smoothstep01((activity-0.012f)/0.043f);
        microGate*=activityGate;fineGate*=0.50f+0.50f*activityGate;textureGate*=0.75f+0.25f*activityGate;
        // General hard-edge guard (no semantic/skin detection). A persistent step has a
        // strong normalized gradient but low oscillation; repeated fine texture has a
        // higher Laplacian/gradient ratio and is therefore preserved.
        float normalizedGrad=grad[si]/std::max(b5,0.03f);
        float meanGrad=meanFrom(iGrad,tx,ty,2),meanLap=meanFrom(iLap,tx,ty,2);
        float oscillation=meanLap/std::max(meanGrad,1e-6f);
        float hardEdge=smoothstep01((normalizedGrad-0.03f)/(0.20f-0.03f))*(1.f-smoothstep01((oscillation-0.30f)/(1.00f-0.30f)));
        float edgeGate=1.f-hardEdge;
        microGate*=edgeGate;fineGate*=0.40f+0.60f*edgeGate;textureGate*=0.75f+0.25f*edgeGate;
        float yd=yy+(A*micro*microGate+B*fine*fineGate+C*texture*textureGate)*tone;
        float lo=std::numeric_limits<float>::infinity(),hi=-std::numeric_limits<float>::infinity();
        for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx){float v=sy(tx+dx,ty+dy);lo=std::min(lo,v);hi=std::max(hi,v);}
        float range=std::max(hi-lo,1e-5f);float margin=((0.035f+0.015f*q)*(1.f-0.72f*hardEdge))*range+2e-5f;
        yd=clampf(yd,lo-margin,hi+margin);yd=std::max(yd,0.f);float sc=yy>1e-8f?yd/yy:1.f;
        out[3*oi]=nr*sc;out[3*oi+1]=ng*sc;out[3*oi+2]=nb*sc;
    }
    return Status::ok();
}

ExposurePlan choose_exposure_plan_from_histograms(const std::vector<std::uint64_t>& dh,const std::vector<std::uint64_t>& sh,float sceneMax,float noise,float clip,std::uint64_t over1){
    ExposurePlan p;float p1=quantile_hist(dh,1.f,.01f),p50=quantile_hist(dh,1.f,.50f),p75=quantile_hist(dh,1.f,.75f),p99=quantile_hist(dh,1.f,.99f);float sy50=quantile_hist(sh,sceneMax,.50f);float darkness=smoothstep01((0.11f-p50)/(0.11f-0.045f));float np=smoothstep01((noise-0.00130f)/(0.00175f-0.00130f));float cp=smoothstep01(clip/0.005f);float conf=clamp01(1.f-cp);p.noiseSigmaAt2Pct=noise;p.clipFraction=clip;p.evidenceConfidence=conf;p.stage2Over1Count=over1;p.blackFactor=std::max(.28f,std::min(.68f,.30f+.24f*darkness+.12f*np));p.midGain=std::max(1.f,std::min(1.18f,1.f+.14f*darkness+.04f*np));p.sdrHighlightGain=std::max(1.08f,std::min(1.48f,1.20f+.30f*darkness-.12f*cp));float y1=std::max(1e-6f,p1*p.blackFactor),y50=std::max(y1+1e-5f,std::min(.45f,p50*p.midGain)),y99=std::max(y50+1e-4f,std::min(.72f,p99*p.sdrHighlightGain));p.anchorsX={0.f,std::max(p1,1e-6f),std::max(p50,p1+1e-5f),std::max(p99,p50+1e-5f),1.f};for(int i=1;i<5;++i)if(p.anchorsX[i]<=p.anchorsX[i-1])p.anchorsX[i]=std::min(1.f,p.anchorsX[i-1]+1e-6f);p.anchorsY={0.f,y1,y50,y99,1.f};for(int i=1;i<5;++i)if(p.anchorsY[i]<=p.anchorsY[i-1])p.anchorsY[i]=std::min(1.f,p.anchorsY[i-1]+1e-6f);p.sceneToDisplayScalar=y50/std::max(sy50,1e-8f);p.hdrGateStartY=std::max(.10f,p75*p.midGain);p.hdrGateFullY=std::max(p.hdrGateStartY+1e-4f,y99);p.hdrMaxGain=std::max(1.05f,std::min(4.f,1.f+2.5f*conf+(over1?0.5f:0.f)));return p;
}

std::vector<float> build_monotone_lut(const ExposurePlan& p,int n){n=std::max(n,2);float x[5],y[5],d[4],m[5];for(int i=0;i<5;++i){x[i]=p.anchorsX[i];y[i]=p.anchorsY[i];}for(int i=0;i<4;++i)d[i]=(y[i+1]-y[i])/std::max(x[i+1]-x[i],1e-8f);m[0]=d[0];m[4]=d[3];for(int i=1;i<4;++i)m[i]=(d[i-1]*d[i]<=0)?0.f:2.f*d[i-1]*d[i]/(d[i-1]+d[i]);for(int i=0;i<4;++i){if(d[i]==0){m[i]=m[i+1]=0;}else{float aa=m[i]/d[i],bb=m[i+1]/d[i],s=aa*aa+bb*bb;if(s>9){float t=3/std::sqrt(s);m[i]=t*aa*d[i];m[i+1]=t*bb*d[i];}}}std::vector<float> lut(n);int seg=0;float prev=0;for(int k=0;k<n;++k){float u=float(k)/float(n-1);while(seg<3&&u>x[seg+1])seg++;float hh=std::max(x[seg+1]-x[seg],1e-8f),t=clamp01((u-x[seg])/hh);float h00=2*t*t*t-3*t*t+1,h10=t*t*t-2*t*t+t,h01=-2*t*t*t+3*t*t,h11=t*t*t-t*t;float v=h00*y[seg]+h10*hh*m[seg]+h01*y[seg+1]+h11*hh*m[seg+1];v=clamp01(v);v=std::max(v,prev);lut[k]=v;prev=v;}lut.front()=0;lut.back()=1;return lut;}

TruthRawProcessor::TruthRawProcessor():reconstruction_(std::make_shared<ReferenceMeasuredPreservingReconstruction>()),appearance_(std::make_shared<NeutralReferenceAppearance>()){}
TruthRawProcessor::TruthRawProcessor(std::shared_ptr<IReconstructionBackend> r):reconstruction_(r?std::move(r):std::make_shared<ReferenceMeasuredPreservingReconstruction>()),appearance_(std::make_shared<NeutralReferenceAppearance>()){}
TruthRawProcessor::TruthRawProcessor(std::shared_ptr<IReconstructionBackend> r,std::shared_ptr<IAppearanceBackend> a):reconstruction_(r?std::move(r):std::make_shared<ReferenceMeasuredPreservingReconstruction>()),appearance_(a?std::move(a):std::make_shared<NeutralReferenceAppearance>()){}

Status TruthRawProcessor::processDng(const std::string& path,IDngDecoder& decoder,const ProcessOptions& o,ProcessResult& r) const{DecodedDngFrame f;auto s=decoder.decode(path,f);if(!s)return s;return processFrame(f,o,r);}

Status TruthRawProcessor::processFrame(const DecodedDngFrame& f,const ProcessOptions& o,ProcessResult& r) const{
    auto T0=Clock::now();auto keepSdr=std::move(r.sdrRgb),keepGain=std::move(r.halfLogGain),keepDiag=std::move(r.stage2Diagnostic);r=ProcessResult{};r.sdrRgb=std::move(keepSdr);r.halfLogGain=std::move(keepGain);r.stage2Diagnostic=std::move(keepDiag);const auto& m=f.meta;const std::size_t N=std::size_t(m.width)*m.height;
    if(m.width<=1||m.height<=1||f.raw.size()!=N)return r.status=Status::error(StatusCode::InvalidArgument,"invalid decoded frame dimensions/raw size");
    if(m.hasGainField&&f.gainField.size()!=N)return r.status=Status::error(StatusCode::InvalidArgument,"gain field size mismatch");
    if(m.hasResidualBlack&&(f.rowBias.size()!=std::size_t(m.height)||f.colBias.size()!=std::size_t(m.width)))return r.status=Status::error(StatusCode::InvalidArgument,"residual black row/column size mismatch");
    if(o.appearance!=appearance_->profile())return r.status=Status::error(StatusCode::BackendFailed,"requested appearance does not match installed appearance backend");
    r.width=m.width;r.height=m.height;r.orientation=m.orientation;r.provenance.reconstructionQuality=reconstruction_->quality();r.provenance.reconstructionBackend=reconstruction_->name();r.provenance.appearanceBackend=appearance_->name();r.provenance.colorFidelityPolicy=appearance_->colorFidelityPolicy();r.provenance.residualBlackApplied=m.hasResidualBlack;r.provenance.residualBlackStatus=m.hasResidualBlack?"validated_pack_supplied":"metadata_only_no_dark_pack";
    if(r.sdrRgb.size()!=3*N) r.sdrRgb.resize(3*N);
    if(o.keepScientificDiagnostics){
        if(r.stage2Diagnostic.size()!=N) r.stage2Diagnostic.resize(N);
    } else {
        r.stage2Diagnostic.clear();
    }
    const int hw=(m.width+1)/2,hh=(m.height+1)/2;std::vector<float> halfSceneMax(std::size_t(hw)*hh,0.f);std::vector<std::uint8_t> halfCensor(std::size_t(hw)*hh,0);
    auto tiles=make_tiles(m.width,m.height,o.tile);if(tiles.empty())return r.status=Status::error(StatusCode::InvalidArgument,"invalid tile policy");int required=reconstruction_->requiredHalo()+appearance_->requiredHalo();if(o.tile.halo<required)return r.status=Status::error(StatusCode::InvalidArgument,"tile halo smaller than reconstruction + appearance backend requirement");
    int workerCount=std::max(1,std::min(o.threads,int(tiles.size())));if(workerCount>1&&(o.tile.core&1))return r.status=Status::error(StatusCode::InvalidArgument,"parallel half-gain scheduling requires an even tile core");
    struct Worker{Hist display{1.f};Hist scene{4.f};std::vector<float>s2,cam,xyz,neutral,look;std::uint64_t over1=0,clipped=0;std::size_t peak=0;double appCpuMs=0;Status status=Status::ok();};std::vector<std::unique_ptr<Worker>> workers;for(int i=0;i<workerCount;++i)workers.emplace_back(std::make_unique<Worker>());std::atomic<std::size_t> nextTile{0};
    auto A=Clock::now();
    AppearanceContext appCtx;appCtx.noiseSigmaAt2Pct=noise_sigma_2pct(m);auto runWorker=[&](int wi){auto& w=*workers[wi];while(true){std::size_t ti=nextTile.fetch_add(1,std::memory_order_relaxed);if(ti>=tiles.size())break;const auto&t=tiles[ti];stage2_tile(f,t,w.s2);int tw=t.hx1-t.hx0,th=t.hy1-t.hy0,cw=t.x1-t.x0,ch=t.y1-t.y0,ah=appearance_->requiredHalo();int ax0=std::max(0,t.x0-ah),ay0=std::max(0,t.y0-ah),ax1=std::min(m.width,t.x1+ah),ay1=std::min(m.height,t.y1+ah),aw=ax1-ax0,ahh=ay1-ay0;w.cam.resize(3*std::size_t(aw)*ahh);w.xyz.resize(w.cam.size());w.neutral.resize(w.cam.size());w.look.resize(3*std::size_t(cw)*ch);auto st=reconstruction_->reconstructTile(w.s2.data(),tw,th,t.hx0,t.hy0,ax0,ay0,aw,ahh,m.cfa,w.cam.data());if(!st){w.status=st;break;}camera_to_xyz(w.cam.data(),w.xyz.data(),aw*ahh,m.cameraToXyzD50);xyz_d50_to_linear_srgb(w.xyz.data(),w.neutral.data(),aw*ahh);auto ap0=Clock::now();st=appearance_->applyTile(w.neutral.data(),aw,ahh,t.x0-ax0,t.y0-ay0,cw,ch,appCtx,w.look.data());auto ap1=Clock::now();w.appCpuMs+=ms(ap0,ap1);if(!st){w.status=st;break;}w.peak=std::max(w.peak,w.s2.size()*sizeof(float)+(w.cam.size()+w.xyz.size()+w.neutral.size()+w.look.size())*sizeof(float));
        for(int cy=0;cy<ch;++cy)for(int cx=0;cx<cw;++cx){int x=t.x0+cx,y=t.y0+cy;std::size_t li=std::size_t(cy)*cw+cx,ai=std::size_t(y-ay0)*aw+(x-ax0),gi=std::size_t(y)*m.width+x;r.sdrRgb[3*gi]=w.look[3*li];r.sdrRgb[3*gi+1]=w.look[3*li+1];r.sdrRgb[3*gi+2]=w.look[3*li+2];float nY=std::max(luminance709(w.neutral[3*ai],w.neutral[3*ai+1],w.neutral[3*ai+2]),0.f);w.display.add(nY);float sy=std::max(w.xyz[3*ai+1],0.f);w.scene.add(sy);int qi=(y/2)*hw+(x/2);halfSceneMax[qi]=std::max(halfSceneMax[qi],sy);int lx=x-t.hx0,ly=y-t.hy0;float s2core=w.s2[std::size_t(ly)*tw+lx];if(s2core>1.f)w.over1++;if(float(f.raw[gi])>=m.whiteLevel){w.clipped++;halfCensor[qi]=1;}if(o.keepScientificDiagnostics)r.stage2Diagnostic[gi]=s2core;}
    }};
    {std::vector<std::thread> pool;for(int i=0;i<workerCount;++i)pool.emplace_back([&,i]{runWorker(i);});for(auto&th:pool)th.join();}
    Hist displayHist(1.f),sceneHist(4.f);std::uint64_t totalOver1=0,totalClipped=0;std::size_t peakTile=0;double maxAppCpu=0;for(auto&wp:workers){auto&w=*wp;if(!w.status)return r.status=w.status;for(int i=0;i<Hist::kBins;++i){displayHist.bins[i]+=w.display.bins[i];sceneHist.bins[i]+=w.scene.bins[i];}displayHist.total+=w.display.total;sceneHist.total+=w.scene.total;totalOver1+=w.over1;totalClipped+=w.clipped;peakTile=std::max(peakTile,w.peak);maxAppCpu=std::max(maxAppCpu,w.appCpuMs);}auto B=Clock::now();double tileWall=ms(A,B);r.timing.appearanceMs=std::min(tileWall,maxAppCpu);r.timing.stage2ReconstructColorMs=std::max(0.0,tileWall-r.timing.appearanceMs);
    float noise=noise_sigma_2pct(m),clip=float(totalClipped)/float(N);r.exposure=choose_exposure_plan_from_histograms(displayHist.bins,sceneHist.bins,4.f,noise,clip,totalOver1);auto lut=build_monotone_lut(r.exposure,o.sdrLutSize);auto C=Clock::now();r.timing.exposurePlanMs=ms(B,C);
    parallel_ranges(m.height,std::max(1,o.threads),[&](int y0,int y1){for(int y=y0;y<y1;++y)for(int x=0;x<m.width;++x){std::size_t i=std::size_t(y)*m.width+x;float rr=r.sdrRgb[3*i],gg=r.sdrRgb[3*i+1],bb=r.sdrRgb[3*i+2];float Y=std::max(luminance709(rr,gg,bb),0.f),Yo=lut_sample(lut,Y),sc=Y>1e-8f?Yo/Y:0.f;rr=std::max(rr*sc,0.f);gg=std::max(gg*sc,0.f);bb=std::max(bb*sc,0.f);float mx=std::max(rr,std::max(gg,bb));if(mx>1){rr/=mx;gg/=mx;bb/=mx;}r.sdrRgb[3*i]=rr;r.sdrRgb[3*i+1]=gg;r.sdrRgb[3*i+2]=bb;}});auto D=Clock::now();r.timing.sdrApplyMs=ms(C,D);
    {auto base=halfCensor;for(int y=0;y<hh;++y)for(int x=0;x<hw;++x)if(base[std::size_t(y)*hw+x])for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx){int xx=x+dx,yy=y+dy;if(xx>=0&&xx<hw&&yy>=0&&yy<hh)halfCensor[std::size_t(yy)*hw+xx]=1;}}
    if(r.halfLogGain.size()!=std::size_t(hw)*hh) r.halfLogGain.resize(std::size_t(hw)*hh);
    if(o.hdrEnabled){
        parallel_ranges(hh,std::max(1,o.threads),[&](int qy0,int qy1){
            for(int qy=qy0;qy<qy1;++qy){
                for(int qx=0;qx<hw;++qx){
                    std::size_t qi=std::size_t(qy)*hw+qx;
                    if(halfCensor[qi]){r.halfLogGain[qi]=0;continue;}
                    float sdrSum=0;int cnt=0;
                    for(int dy=0;dy<2;++dy)for(int dx=0;dx<2;++dx){
                        int x=2*qx+dx,y=2*qy+dy;if(x>=m.width||y>=m.height)continue;
                        std::size_t gi=std::size_t(y)*m.width+x;
                        sdrSum+=std::max(luminance709(r.sdrRgb[3*gi],r.sdrRgb[3*gi+1],r.sdrRgb[3*gi+2]),0.f);cnt++;
                    }
                    float Yb=sdrSum/std::max(cnt,1),target=halfSceneMax[qi]*r.exposure.sceneToDisplayScalar,rawGain=Yb>1e-7f?target/Yb:1.f;
                    float tt=clamp01((Yb-r.exposure.hdrGateStartY)/std::max(r.exposure.hdrGateFullY-r.exposure.hdrGateStartY,1e-8f));
                    float gate=tt*tt*(3-2*tt);
                    float g=1.f+r.exposure.evidenceConfidence*gate*(std::max(1.f,std::min(r.exposure.hdrMaxGain,rawGain))-1.f);
                    r.halfLogGain[qi]=std::log2(std::max(g,1.f));
                }
            }
        });
    } else {
        std::fill(r.halfLogGain.begin(),r.halfLogGain.end(),0.f);
    }
    auto E=Clock::now();r.timing.gainMapMs=ms(D,E);r.timing.totalMs=ms(T0,E);r.memory.peakTileBytes=peakTile;r.memory.finalSdrBytes=r.sdrRgb.size()*sizeof(float);r.memory.halfGainBytes=r.halfLogGain.size()*sizeof(float);r.memory.diagnosticBytes=r.stage2Diagnostic.size()*sizeof(float);r.status=Status::ok();return r.status;
}

} // namespace truthraw
