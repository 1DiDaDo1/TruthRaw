#include "truthrange_real_latent_v0_5.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <deque>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <utility>

namespace truthraw_v05 {
namespace {
constexpr int kStride=16;
constexpr int kMargin=8;
constexpr float kAbsP50=0.6744897501960817f;
constexpr float kAbsP95=1.959963984540054f;
constexpr float kNegInf=-std::numeric_limits<float>::infinity();

[[noreturn]] void fail(const char* s){ throw std::runtime_error(s); }
std::vector<std::uint8_t> load_bytes(const std::string& p){
    std::ifstream f(p,std::ios::binary); if(!f) fail("cannot open DNG");
    f.seekg(0,std::ios::end); auto n=f.tellg(); if(n<0) fail("cannot size DNG");
    std::vector<std::uint8_t>b(static_cast<std::size_t>(n)); f.seekg(0);
    f.read(reinterpret_cast<char*>(b.data()),static_cast<std::streamsize>(b.size())); if(!f) fail("cannot read DNG");
    return b;
}
std::uint16_t rd16(const std::vector<std::uint8_t>& b,std::size_t o,bool le){
    if(o+2>b.size()) fail("truncated u16");
    return le?std::uint16_t(b[o]|(std::uint16_t(b[o+1])<<8)):std::uint16_t((std::uint16_t(b[o])<<8)|b[o+1]);
}
std::uint32_t rd32(const std::vector<std::uint8_t>& b,std::size_t o,bool le){
    if(o+4>b.size()) fail("truncated u32");
    if(le) return std::uint32_t(b[o])|(std::uint32_t(b[o+1])<<8)|(std::uint32_t(b[o+2])<<16)|(std::uint32_t(b[o+3])<<24);
    return (std::uint32_t(b[o])<<24)|(std::uint32_t(b[o+1])<<16)|(std::uint32_t(b[o+2])<<8)|std::uint32_t(b[o+3]);
}
std::uint64_t rd64(const std::vector<std::uint8_t>& b,std::size_t o,bool le){
    if(o+8>b.size()) fail("truncated u64");
    std::uint64_t u=0;
    if(le){ for(int i=7;i>=0;--i) u=(u<<8)|b[o+std::size_t(i)]; }
    else { for(int i=0;i<8;++i) u=(u<<8)|b[o+std::size_t(i)]; }
    return u;
}
double rd64f(const std::vector<std::uint8_t>&b,std::size_t o,bool le){auto u=rd64(b,o,le);double d;std::memcpy(&d,&u,8);return d;}
std::size_t type_size(std::uint16_t t){switch(t){case 1:case 2:case 7:return 1;case 3:return 2;case 4:case 9:case 11:return 4;case 5:case 10:case 12:return 8;default:return 0;}}
struct Ent{std::uint16_t type=0;std::uint32_t count=0;std::vector<std::uint8_t> raw;};
struct Ifd{std::vector<std::pair<std::uint16_t,Ent>> entries;};
Ifd parse_ifd(const std::vector<std::uint8_t>& b,std::uint32_t off,bool le){
    if(std::size_t(off)+2>b.size()) fail("IFD OOB");
    auto n=rd16(b,off,le);
    auto base=std::size_t(off)+2;
    if(base+std::size_t(n)*12+4>b.size()) fail("truncated IFD");
    Ifd out;
    out.entries.reserve(n);
    for(std::uint16_t i=0;i<n;++i){auto q=base+std::size_t(i)*12;auto tag=rd16(b,q,le),typ=rd16(b,q+2,le);auto cnt=rd32(b,q+4,le);auto ts=type_size(typ);if(!ts)continue;auto sz=std::size_t(cnt)*ts;Ent e;e.type=typ;e.count=cnt;if(sz<=4)e.raw.assign(b.begin()+q+8,b.begin()+q+8+sz);else{auto p=rd32(b,q+8,le);if(std::size_t(p)+sz>b.size())fail("tag OOB");e.raw.assign(b.begin()+p,b.begin()+p+sz);}out.entries.push_back({tag,std::move(e)});}return out;
}
const Ent* find_ent(const Ifd& ifd,std::uint16_t tag){for(const auto& p:ifd.entries)if(p.first==tag)return &p.second;return nullptr;}
std::uint32_t ent_u32(const Ent& e,bool le){if(e.count!=1)fail("scalar expected");if(e.type==3)return rd16(e.raw,0,le);if(e.type==4)return rd32(e.raw,0,le);fail("integer type expected");}

int phase_index(int y,int x){return (y&1)*2+(x&1);}
std::size_t idx3(std::size_t i,int c){return 3*i+std::size_t(c);}

struct PredSupport {
    double pred=0.0;
    std::array<double,8> support{};
    std::array<double,4> pair{};
    double dha=0.0,dva=0.0;
};

double leakfree_green_at_rb(const std::vector<float>& s,int w,int y,int x,double& dh,double& dv){
    const double gl=s[std::size_t(y)*w+(x-1)], gr=s[std::size_t(y)*w+(x+1)];
    const double gu=s[std::size_t(y-1)*w+x], gd=s[std::size_t(y+1)*w+x];
    dh=std::abs(gl-gr); dv=std::abs(gu-gd); const double wh=1.0/(dh+1e-5),wv=1.0/(dv+1e-5);
    return (wh*0.5*(gl+gr)+wv*0.5*(gu+gd))/(wh+wv);
}
PredSupport predict_hidden(const truthraw::LatentCameraSceneV02& scene,BayerRoleV05 role,int y,int x){
    PredSupport r; const auto& s=scene.stage2Cfa; const int w=scene.width;
    if(role==BayerRoleV05::R || role==BayerRoleV05::B){
        double dh=0,dv=0; const double g0=leakfree_green_at_rb(s,w,y,x,dh,dv);r.dha=dh;r.dva=dv;
        constexpr int off[8][2]={{-2,0},{2,0},{0,-2},{0,2},{-2,-2},{-2,2},{2,-2},{2,2}};
        double sw=0,sd=0;
        for(int k=0;k<8;++k){int yy=y+off[k][0],xx=x+off[k][1];double ndh=0,ndv=0;double gn=leakfree_green_at_rb(s,w,yy,xx,ndh,ndv);double cn=s[std::size_t(yy)*w+xx];r.support[k]=cn;double ww=1.0/(std::abs(gn-g0)+1e-4);sw+=ww;sd+=ww*(cn-gn);}
        double pred=g0+sd/sw;auto mm=std::minmax_element(r.support.begin(),r.support.end());double lo=*mm.first,hi=*mm.second,rg=std::max(hi-lo,1e-6);pred=std::max(lo-.08*rg,std::min(hi+.08*rg,pred));r.pred=pred;
        r.pair={std::abs(r.support[0]-r.support[1]),std::abs(r.support[2]-r.support[3]),std::abs(r.support[4]-r.support[7]),std::abs(r.support[5]-r.support[6])};
        return r;
    }
    const int oy[8]={0,0,-2,2,-1,1,-1,1}; const int ox[8]={-2,2,0,0,-1,1,1,-1};
    for(int k=0;k<8;++k) r.support[k]=s[std::size_t(y+oy[k])*w+(x+ox[k])];
    const std::array<double,4> cand={.5*(r.support[0]+r.support[1]),.5*(r.support[2]+r.support[3]),.5*(r.support[4]+r.support[5]),.5*(r.support[6]+r.support[7])};
    r.pair={std::abs(r.support[0]-r.support[1]),std::abs(r.support[2]-r.support[3]),std::abs(r.support[4]-r.support[5]),std::abs(r.support[6]-r.support[7])};
    std::array<int,4> order={0,1,2,3};std::sort(order.begin(),order.end(),[&](int a,int b){return r.pair[a]<r.pair[b];});int i0=order[0],i1=order[1];double w0=1.0/(r.pair[i0]+1e-5),w1=1.0/(r.pair[i1]+1e-5);double pred=(w0*cand[i0]+w1*cand[i1])/(w0+w1);
    auto mm=std::minmax_element(r.support.begin(),r.support.end());double lo=*mm.first,hi=*mm.second,rg=std::max(hi-lo,1e-6);pred=std::max(lo-.06*rg,std::min(hi+.06*rg,pred));r.pred=pred;r.dha=r.pair[0];r.dva=r.pair[1];return r;
}

double std_pop(const std::array<double,8>& a){double m=0;for(double v:a)m+=v;m/=8.0;double s=0;for(double v:a){double d=v-m;s+=d*d;}return std::sqrt(s/8.0);}
double range8(const std::array<double,8>& a){auto mm=std::minmax_element(a.begin(),a.end());return *mm.second-*mm.first;}
double median4(std::array<double,4> a){std::sort(a.begin(),a.end());return .5*(a[1]+a[2]);}
int onehot_training_index(BayerRoleV05 r){switch(r){case BayerRoleV05::B:return 0;case BayerRoleV05::G1:return 1;case BayerRoleV05::G2:return 2;case BayerRoleV05::R:return 3;}return 0;}

void max_filter_1d(const float* src,float* dst,int n,int radius){
    std::deque<int> q;int add=0;
    for(int i=0;i<n;++i){int right=std::min(n-1,i+radius);while(add<=right){while(!q.empty()&&src[q.back()]<=src[add])q.pop_back();q.push_back(add++);}int left=i-radius;while(!q.empty()&&q.front()<left)q.pop_front();dst[i]=q.empty()?kNegInf:src[q.front()];}
}
void max_filter_square(const std::vector<float>& src,int w,int h,int radius,std::vector<float>& dst){
    std::vector<float> tmp(src.size()); dst.resize(src.size());
    for(int y=0;y<h;++y) max_filter_1d(src.data()+std::size_t(y)*w,tmp.data()+std::size_t(y)*w,w,radius);
    std::vector<float> colIn(static_cast<std::size_t>(h));
    std::vector<float> colOut(static_cast<std::size_t>(h));
    for(int x=0;x<w;++x){for(int y=0;y<h;++y)colIn[std::size_t(y)]=tmp[std::size_t(y)*w+x];max_filter_1d(colIn.data(),colOut.data(),h,radius);for(int y=0;y<h;++y)dst[std::size_t(y)*w+x]=colOut[std::size_t(y)];}
}

}

DngExtraV05 read_dng_extra_v0_5(const std::string& path){
    auto b=load_bytes(path);if(b.size()<8)fail("short TIFF");bool le;if(b[0]=='I'&&b[1]=='I')le=true;else if(b[0]=='M'&&b[1]=='M')le=false;else fail("bad TIFF endian");if(rd16(b,2,le)!=42)fail("classic TIFF required");
    auto main=parse_ifd(b,rd32(b,4,le),le);const Ent* np=find_ent(main,51041);const Ent* cp=find_ent(main,50710);const Ent* iso=find_ent(main,34855);
    if(!np||np->type!=12||np->count!=6) fail("six-double NoiseProfile required");
    if(!cp||cp->type!=1||cp->count!=3) fail("CFAPlaneColor required");
    DngExtraV05 o;for(int i=0;i<6;++i){double v=rd64f(np->raw,8ull*std::size_t(i),le);if(!(v>=0.0) || !std::isfinite(v))fail("invalid NoiseProfile");o.noiseProfileRgb[std::size_t(i)]=float(v);}std::copy(cp->raw.begin(),cp->raw.end(),o.cfaPlaneColor.begin());if(o.cfaPlaneColor!=std::array<std::uint8_t,3>{0,1,2})fail("v0.5 requires CFAPlaneColor RGB order 0,1,2");
    if(iso)o.iso=int(ent_u32(*iso,le));else if(const Ent* exif=find_ent(main,34665)){auto ex=parse_ifd(b,ent_u32(*exif,le),le);if(const Ent* eiso=find_ent(ex,34855))o.iso=int(ent_u32(*eiso,le));}
    return o;
}

truthraw::Status adapt_classic_dng_to_decoded_v0_5(const truthraw_v04::ClassicDng& dng,const DngExtraV05& extra,truthraw::DecodedDngFrame& out){
    if(dng.width<=1||dng.height<=1||dng.cfaPattern!=std::array<std::uint8_t,4>{2,1,1,0})return truthraw::Status::error(truthraw::StatusCode::UnsupportedTopology,"v0.5 requires BGGR classic DNG");
    const std::size_t N=std::size_t(dng.width)*dng.height; if(dng.raw.size()!=N)return truthraw::Status::error(truthraw::StatusCode::InvalidArgument,"raw size mismatch");
    out=truthraw::DecodedDngFrame{};out.meta.width=dng.width;out.meta.height=dng.height;out.meta.cfa=truthraw::CfaPattern::BGGR;out.meta.orientation=truthraw::Orientation::Normal;out.meta.whiteLevel=dng.whiteLevel;out.meta.blackPhase=dng.blackPhase;out.meta.noiseProfile=extra.noiseProfileRgb;out.meta.hasNoiseProfile=true;out.meta.hasGainField=true;out.meta.hasResidualBlack=false;out.raw=dng.raw;out.gainField.resize(N);
    try{for(int y=0;y<dng.height;++y)for(int x=0;x<dng.width;++x)out.gainField[std::size_t(y)*dng.width+x]=dng.gainAt(y,x);}catch(const std::exception& e){return truthraw::Status::error(truthraw::StatusCode::DecoderFailed,e.what());}
    return truthraw::Status::ok();
}

const char* role_name_v0_5(BayerRoleV05 r){switch(r){case BayerRoleV05::R:return "R";case BayerRoleV05::G1:return "G1";case BayerRoleV05::G2:return "G2";case BayerRoleV05::B:return "B";}return "?";}
int role_rgb_channel_v0_5(BayerRoleV05 r){return r==BayerRoleV05::R?0:(r==BayerRoleV05::B?2:1);}
int role_runtime_code_v0_5(BayerRoleV05 r){return int(r);}
BayerRoleV05 role_at_v0_5(int y,int x){int ph=phase_index(y,x);return ph==0?BayerRoleV05::B:(ph==1?BayerRoleV05::G1:(ph==2?BayerRoleV05::G2:BayerRoleV05::R));}

std::array<float,18> build_v5g_features_exact_v0_5(const truthraw::DecodedDngFrame& frame,const truthraw::LatentCameraSceneV02& scene,BayerRoleV05 role,int y,int x,float& predictedHidden,float& sigmaFeature,float& snrOut){
    if(y<kMargin||x<kMargin||y>=scene.height-kMargin||x>=scene.width-kMargin) fail("feature target inside margin");
    if(role_at_v0_5(y,x)!=role) fail("feature role/coordinate mismatch");
    auto pr=predict_hidden(scene,role,y,x);const std::size_t i=std::size_t(y)*scene.width+x;const double g=frame.gainField[i];const int c=role_rgb_channel_v0_5(role);const double S=frame.meta.noiseProfile[2*c],O=frame.meta.noiseProfile[2*c+1];const double sigma=std::sqrt(std::max(g*S*std::max(pr.pred,0.0)+g*g*O,1e-16));const double snr=std::abs(pr.pred)/std::max(sigma,1e-12);
    double lmin=std::numeric_limits<double>::infinity(),lmax=-std::numeric_limits<double>::infinity();int cens=0,nnei=0;for(int oy=-2;oy<=2;++oy)for(int ox=-2;ox<=2;++ox){if(oy==0&&ox==0)continue;const std::size_t j=std::size_t(y+oy)*scene.width+(x+ox);double v=scene.stage2Cfa[j];lmin=std::min(lmin,v);lmax=std::max(lmax,v);cens+=float(frame.raw[j])>=frame.meta.whiteLevel;++nnei;}
    double radial=std::sqrt(std::pow(((double(x)/(scene.width-1)-.5)/.5),2)+std::pow(((double(y)/(scene.height-1)-.5)/.5),2))/std::sqrt(2.0);std::array<float,18> f{};auto divsig=[&](double v){return v/std::max(sigma,1e-12);};
    f[0]=float(std::log1p(std::abs(pr.pred)));f[1]=float(std::log1p(sigma*1e4));f[2]=float(std::log1p(std::max(snr,0.0)));f[3]=float(std::log1p(divsig(std_pop(pr.support))));f[4]=float(std::log1p(divsig(range8(pr.support))));f[5]=float(std::log1p(divsig(*std::min_element(pr.pair.begin(),pr.pair.end()))));f[6]=float(std::log1p(divsig(median4(pr.pair))));f[7]=float(std::log1p(divsig(std::abs(pr.dha-pr.dva))));f[8]=float(std::log1p(divsig(lmax-lmin)));f[9]=float(g);f[10]=float(double(cens)/nnei);f[11]=float(radial);f[12]=pr.pred<0.0?1.f:0.f;f[13]=pr.pred>1.0?1.f:0.f;f[14+onehot_training_index(role)]=1.f;
    predictedHidden=float(pr.pred);sigmaFeature=float(sigma);snrOut=float(snr);return f;
}

truthraw::Status build_v5g_measured_role_anchors_v0_5(const truthraw::DecodedDngFrame& frame,const truthraw::LatentCameraSceneV02& scene,std::vector<BackendAnchorV05>& anchors,std::array<std::uint64_t,4>& countsByRole,std::uint64_t& skippedSourceClip){
    const std::size_t N=std::size_t(scene.width)*scene.height;if(frame.raw.size()!=N||frame.gainField.size()!=N||scene.stage2Cfa.size()!=N)return truthraw::Status::error(truthraw::StatusCode::InvalidArgument,"v0.5 anchor geometry mismatch");anchors.clear();countsByRole.fill(0);skippedSourceClip=0;
    const std::array<BayerRoleV05,4> order={BayerRoleV05::B,BayerRoleV05::G1,BayerRoleV05::G2,BayerRoleV05::R};
    for(int ri=0;ri<4;++ri){auto role=order[std::size_t(ri)];int dy=role==BayerRoleV05::R||role==BayerRoleV05::G2?1:0;int dx=role==BayerRoleV05::R||role==BayerRoleV05::G1?1:0;int sy=kMargin+((dy-kMargin)&1),sx=kMargin+((dx-kMargin)&1);int iy=0;for(int y=sy;y<scene.height-kMargin;y+=kStride,++iy){int ix=0;for(int x=sx;x<scene.width-kMargin;x+=kStride,++ix){if(((iy+2*ix+ri)&1)!=0)continue;const std::size_t i=std::size_t(y)*scene.width+x;if(float(frame.raw[i])>=frame.meta.whiteLevel){++skippedSourceClip;continue;}BackendAnchorV05 a;a.y=y;a.x=x;a.role=role;a.rgbChannel=role_rgb_channel_v0_5(role);try{a.features=build_v5g_features_exact_v0_5(frame,scene,role,y,x,a.predictedHidden,a.sigmaFeature,a.snr);}catch(const std::exception& e){return truthraw::Status::error(truthraw::StatusCode::BackendFailed,e.what());}auto b=truthraw::predict_uncertainty_v5_0g(a.features,role_runtime_code_v0_5(role),a.snr);a.p50Abs=b.p50;a.p95Abs=b.p95;if(!(a.p50Abs>=0.f&&a.p95Abs>=a.p50Abs&&std::isfinite(a.p95Abs)))return truthraw::Status::error(truthraw::StatusCode::BackendFailed,"invalid v5.0g anchor band");anchors.push_back(a);countsByRole[std::size_t(role_runtime_code_v0_5(role))]++;}}}
    return truthraw::Status::ok();
}

truthraw::Status stream_dense_truthrange_v0_5(const truthraw::DecodedDngFrame& frame,const truthraw::LatentCameraSceneV02& scene,const truthraw::TruthRangeGaugeV02& gauge,const std::vector<BackendAnchorV05>& anchors,int transportRadius,int tileSize,DenseStreamSummaryV05& out){
    if(transportRadius<1||tileSize<16) return truthraw::Status::error(truthraw::StatusCode::InvalidArgument,"invalid v0.5 tile policy");
    auto gs=truthraw::validate_gauge_for_scene_v0_2(scene,gauge);
    if(!gs) return gs;
    const std::size_t N=std::size_t(scene.width)*scene.height;
    if(frame.raw.size()!=N||frame.gainField.size()!=N||scene.cameraRgb.size()!=3*N) return truthraw::Status::error(truthraw::StatusCode::InvalidArgument,"v0.5 dense geometry mismatch");
    out=DenseStreamSummaryV05{};out.totalRgbEntries=3*N;bool evInit=false;
    for(int y0=0;y0<scene.height;y0+=tileSize)for(int x0=0;x0<scene.width;x0+=tileSize){int y1=std::min(scene.height,y0+tileSize),x1=std::min(scene.width,x0+tileSize);int ey0=std::max(0,y0-transportRadius),ex0=std::max(0,x0-transportRadius),ey1=std::min(scene.height,y1+transportRadius),ex1=std::min(scene.width,x1+transportRadius);int ew=ex1-ex0,eh=ey1-ey0;
        for(int c=0;c<3;++c){std::vector<float> a50(std::size_t(ew)*eh,kNegInf),a95(std::size_t(ew)*eh,kNegInf);for(const auto& a:anchors){if(a.rgbChannel!=c||a.y<ey0||a.y>=ey1||a.x<ex0||a.x>=ex1)continue;auto j=std::size_t(a.y-ey0)*ew+(a.x-ex0);a50[j]=std::max(a50[j],a.p50Abs);a95[j]=std::max(a95[j],a.p95Abs);}std::vector<float> p50,p95;max_filter_square(a50,ew,eh,transportRadius,p50);max_filter_square(a95,ew,eh,transportRadius,p95);
            for(int y=y0;y<y1;++y)for(int x=x0;x<x1;++x){std::size_t i=std::size_t(y)*scene.width+x;bool measured=int(scene.measuredChannel[i])==c;truthraw::LatentUncertaintyV02 u;truthraw::TruthRangeSupportV02 support=measured?truthraw::TruthRangeSupportV02::MeasuredUncensored:truthraw::TruthRangeSupportV02::ReconstructedWeak;bool hi=measured&&scene.sourceHighCensor[i]!=0;
                if(measured){if(hi){++out.measuredHighCensored;u.valid=false;u.source="MEASURED_HIGH_CENSORED";}else{++out.measuredUncensored;float g=frame.gainField[i],S=frame.meta.noiseProfile[2*c],O=frame.meta.noiseProfile[2*c+1],mu=scene.stage2Cfa[i];float sigma=std::sqrt(std::max(g*S*std::max(mu,0.f)+g*g*O,0.f));u.valid=true;u.p50Abs=kAbsP50*sigma;u.p95Abs=kAbsP95*sigma;u.source="DNG_NOISEPROFILE_GAUSSIAN_EQUIVALENT_STAGE2";}}
                else{auto j=std::size_t(y-ey0)*ew+(x-ex0);if(std::isfinite(p95[j])&&p95[j]>=0.f){++out.reconstructedProxyValid;u.valid=true;u.p50Abs=p50[j];u.p95Abs=p95[j];u.source="V5.0G_MEASURED_ROLE_ANCHORS_LOCAL_MAX_TRANSPORT_PROXY";}else{++out.reconstructedProxyUnresolved;u.valid=false;u.source="UNRESOLVED";}}
                auto tr=truthraw::map_latent_channel_to_truthrange_v0_2(scene.cameraRgb[idx3(i,c)],u,gauge,support,measured,hi,measured?scene.sourceHighCensorLower[i]:0.f);if(tr.hasEstimate&&std::isfinite(tr.estimateEv)){++out.truthrangeFiniteEstimate;if(!evInit){out.estimateEvMin=out.estimateEvMax=tr.estimateEv;evInit=true;}else{out.estimateEvMin=std::min(out.estimateEvMin,tr.estimateEv);out.estimateEvMax=std::max(out.estimateEvMax,tr.estimateEv);}}if(std::isinf(tr.p95LowerEv)&&tr.p95LowerEv<0)++out.darkP95LowerInfinity;if(std::isinf(tr.evidenceUpperEv)&&tr.evidenceUpperEv>0&&measured&&hi)++out.brightEvidenceUpperInfinity;
            }
        }
    }
    return truthraw::Status::ok();
}

truthraw::Status run_real_latent_bridge_v0_5(const std::string& dngPath,truthraw::IReconstructionBackend& reconstruction,RealLatentSummaryV05& summary,std::vector<BackendAnchorV05>* anchorsOut,int transportRadius,int tileSize){
    truthraw_v04::ClassicDng dng;DngExtraV05 extra;try{dng=truthraw_v04::readClassicDng(dngPath);extra=read_dng_extra_v0_5(dngPath);}catch(const std::exception& e){return truthraw::Status::error(truthraw::StatusCode::DecoderFailed,e.what());}
    truthraw::DecodedDngFrame frame;auto ad=adapt_classic_dng_to_decoded_v0_5(dng,extra,frame);if(!ad)return ad;truthraw::LatentSceneBindingV02 bind;bind.reconstructionBackend=reconstruction.name();bind.reconstructionCoreCppSha256="68f52c4896d47c4605adc1cf670e55f95d42a687e18c06a167028a64ce37b94c";bind.reconstructionCoreHSha256="b7f6e2189d6ecccfc5fda80084041990bd12757145c5ff2c40f2ae0d0046b167";bind.uncertaintyModelSha256="8831bee921999e823466cfc462812e40620b2834080c0f7d64c6f11d7ead626f";bind.uncertaintyBindingSha256="61c99b0e29316730ca1323fd3e91fd069ebbc50cba73127cd81b9803b10178a0";bind.sceneScaleId="DNG_STAGE2_BLACK_WHITE_GAINMAP_EXACTLY_ONCE";bind.gainMapAppliedExactlyOnce=true;
    truthraw::LatentCameraSceneV02 scene;truthraw::TilePolicy tile;tile.core=512;tile.halo=std::max(16,reconstruction.requiredHalo());auto st=truthraw::build_latent_camera_scene_v0_2(frame,tile,reconstruction,bind,scene);if(!st)return st;
    summary=RealLatentSummaryV05{};summary.width=scene.width;summary.height=scene.height;summary.iso=extra.iso;bool first=true;double maxerr=0,reinject=0;std::uint64_t bitsum=0;std::uint32_t bitxor=0;for(int y=0;y<scene.height;++y)for(int x=0;x<scene.width;++x){std::size_t i=std::size_t(y)*scene.width+x;float v4=dng.stage2At(y,x),v=scene.stage2Cfa[i];maxerr=std::max(maxerr,std::abs(double(v)-double(v4)));std::uint32_t bits=0;std::memcpy(&bits,&v,4);bitsum+=bits;bitxor^=bits;int c=int(scene.measuredChannel[i]);reinject=std::max(reinject,std::abs(double(scene.cameraRgb[idx3(i,c)])-double(v)));first=false;} (void)first;summary.stage2ParityMaxAbs=maxerr;summary.stage2FloatBitsSum=bitsum;summary.stage2FloatBitsXor=bitxor;summary.measuredCfaReinjectionMaxAbs=reinject;
    truthraw::TruthRangeGaugeV02 gauge;auto gst=truthraw::derive_self_gauge_v0_2(scene,gauge,0.5,0.10);if(!gst)return gst;summary.selfGaugeL0=gauge.L0;summary.selfGaugeId=gauge.gaugeId;
    std::vector<BackendAnchorV05> anchors;auto ast=build_v5g_measured_role_anchors_v0_5(frame,scene,anchors,summary.anchorsByRole,summary.anchorsSkippedSourceClip);if(!ast)return ast;if(!anchors.empty()){summary.anchorP50Min=summary.anchorP50Max=anchors[0].p50Abs;summary.anchorP95Min=summary.anchorP95Max=anchors[0].p95Abs;for(const auto&a:anchors){summary.anchorP50Min=std::min(summary.anchorP50Min,a.p50Abs);summary.anchorP50Max=std::max(summary.anchorP50Max,a.p50Abs);summary.anchorP95Min=std::min(summary.anchorP95Min,a.p95Abs);summary.anchorP95Max=std::max(summary.anchorP95Max,a.p95Abs);}}
    auto dst=stream_dense_truthrange_v0_5(frame,scene,gauge,anchors,transportRadius,tileSize,summary.dense);if(!dst)return dst;if(anchorsOut)*anchorsOut=std::move(anchors);return truthraw::Status::ok();
}

} // namespace truthraw_v05
