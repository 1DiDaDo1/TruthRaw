#pragma once

#include "full_frame_streaming_v0_1.h"

#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

using namespace truthraw;
using namespace truthraw::streaming_v0_1;

namespace {

static void require_active(bool ok, const char* expr, int line) {
    if (!ok) { std::cerr << "REQUIRE_FAIL line=" << line << " expr=" << expr << "\n"; std::exit(2); }
}
#define REQUIRE(expr) require_active(bool(expr), #expr, __LINE__)


class FrameSource final : public IRawTileSource {
public:
    explicit FrameSource(const DecodedDngFrame& f) : f_(f) {}
    const DngMetadata& metadata() const override { return f_.meta; }
    std::size_t residentBytesUpperBound() const override {
        return f_.raw.size()*sizeof(std::uint16_t)
             + f_.gainField.size()*sizeof(float)
             + f_.rowBias.size()*sizeof(float)
             + f_.colBias.size()*sizeof(float);
    }
    StreamStatus readRawTile(const TileRect& r, std::uint16_t* raw, std::size_t rawCount,
                             float* gain, std::size_t gainCount) override {
        const int w=r.hx1-r.hx0,h=r.hy1-r.hy0;
        const std::size_t n=std::size_t(w)*std::size_t(h);
        if(!raw || rawCount!=n) return StreamStatus::error(StreamStatusCode::SourceFailed,"raw count");
        if(f_.meta.hasGainField && (!gain || gainCount!=n)) return StreamStatus::error(StreamStatusCode::SourceFailed,"gain count");
        for(int y=0;y<h;++y) for(int x=0;x<w;++x){
            const std::size_t si=std::size_t(r.hy0+y)*std::size_t(f_.meta.width)+std::size_t(r.hx0+x);
            const std::size_t di=std::size_t(y)*std::size_t(w)+std::size_t(x);
            raw[di]=f_.raw[si];
            if(f_.meta.hasGainField) gain[di]=f_.gainField[si];
        }
        return StreamStatus::ok();
    }
    StreamStatus readRowBias(int y0,int y1,float* out,std::size_t count) override {
        if(!f_.meta.hasResidualBlack) return count==0?StreamStatus::ok():StreamStatus::error(StreamStatusCode::SourceFailed,"unexpected row bias");
        if(!out || count!=std::size_t(y1-y0)) return StreamStatus::error(StreamStatusCode::SourceFailed,"row bias count");
        std::copy(f_.rowBias.begin()+y0,f_.rowBias.begin()+y1,out); return StreamStatus::ok();
    }
    StreamStatus readColBias(int x0,int x1,float* out,std::size_t count) override {
        if(!f_.meta.hasResidualBlack) return count==0?StreamStatus::ok():StreamStatus::error(StreamStatusCode::SourceFailed,"unexpected col bias");
        if(!out || count!=std::size_t(x1-x0)) return StreamStatus::error(StreamStatusCode::SourceFailed,"col bias count");
        std::copy(f_.colBias.begin()+x0,f_.colBias.begin()+x1,out); return StreamStatus::ok();
    }
private:
    const DecodedDngFrame& f_;
};

class CollectSink final : public IStreamingSink {
public:
    CollectSink(int w,int h,bool diag):w_(w),h_(h),hw_((w+1)/2),hh_((h+1)/2),diag_(diag){
        sdr_.assign(3*std::size_t(w)*h,std::numeric_limits<float>::quiet_NaN());
        gain_.assign(std::size_t(hw_)*hh_,std::numeric_limits<float>::quiet_NaN());
        if(diag_) diagnostic_.assign(std::size_t(w)*h,std::numeric_limits<float>::quiet_NaN());
        sdrWrites_.assign(std::size_t(w)*h,0); gainWrites_.assign(std::size_t(hw_)*hh_,0);
    }
    std::size_t residentBytesUpperBound() const override {
        return (sdr_.size()+gain_.size()+diagnostic_.size())*sizeof(float)+sdrWrites_.size()+gainWrites_.size();
    }
    StreamStatus beginFrame(int w,int h,Orientation,const ExposurePlan&e,bool,bool d) override {
        if (w!=w_ || h!=h_ || d!=diag_)
            return StreamStatus::error(StreamStatusCode::SinkFailed,"begin mismatch");
        exposure_=e;
        begun_=true;
        return StreamStatus::ok();
    }
    StreamStatus writeSdrTile(const TileRect&r,const float*rgb,std::size_t n) override {
        const int cw=r.x1-r.x0,ch=r.y1-r.y0;if(!begun_||!rgb||n!=3*std::size_t(cw)*ch)return StreamStatus::error(StreamStatusCode::SinkFailed,"sdr tile");
        for(int y=0;y<ch;++y)for(int x=0;x<cw;++x){auto li=std::size_t(y)*cw+x,gi=std::size_t(r.y0+y)*w_+(r.x0+x);for(int c=0;c<3;++c)sdr_[3*gi+c]=rgb[3*li+c];sdrWrites_[gi]++;}
        return StreamStatus::ok();
    }
    StreamStatus writeHalfLogGainBlock(const HalfStateRect&r,const float*g,std::size_t n) override {
        int qw=r.x1-r.x0,qh=r.y1-r.y0;if(!g||n!=std::size_t(qw)*qh)return StreamStatus::error(StreamStatusCode::SinkFailed,"gain block");
        for(int y=0;y<qh;++y) for(int x=0;x<qw;++x) {
            auto li=std::size_t(y)*qw+x,gi=std::size_t(r.y0+y)*hw_+(r.x0+x);
            gain_[gi]=g[li];
            gainWrites_[gi]++;
        }
        return StreamStatus::ok();
    }
    StreamStatus writeStage2DiagnosticTile(const TileRect&r,const float*d,std::size_t n) override {
        int cw=r.x1-r.x0,ch=r.y1-r.y0;if(!diag_||!d||n!=std::size_t(cw)*ch)return StreamStatus::error(StreamStatusCode::SinkFailed,"diag tile");
        for(int y=0;y<ch;++y) for(int x=0;x<cw;++x) {
            auto li=std::size_t(y)*cw+x,gi=std::size_t(r.y0+y)*w_+(r.x0+x);
            diagnostic_[gi]=d[li];
        }
        return StreamStatus::ok();
    }
    StreamStatus finishFrame() override {
        if(std::find(sdrWrites_.begin(),sdrWrites_.end(),std::uint8_t(1))==sdrWrites_.end())return StreamStatus::error(StreamStatusCode::SinkFailed,"no sdr writes");
        for(auto v:sdrWrites_)if(v!=1)return StreamStatus::error(StreamStatusCode::SinkFailed,"sdr ownership");
        for(auto v:gainWrites_)if(v!=1)return StreamStatus::error(StreamStatusCode::SinkFailed,"gain ownership");
        finished_=true;return StreamStatus::ok();
    }
    const std::vector<float>& sdr()const{return sdr_;} const std::vector<float>& gain()const{return gain_;} const std::vector<float>& diagnostic()const{return diagnostic_;}
    bool finished()const{return finished_;}
private:
    int w_,h_,hw_,hh_;bool diag_,begun_=false,finished_=false;ExposurePlan exposure_{};
    std::vector<float>sdr_,gain_,diagnostic_;std::vector<std::uint8_t>sdrWrites_,gainWrites_;
};

static float max_abs_diff(const std::vector<float>&a,const std::vector<float>&b){
    if(a.size()!=b.size()) return std::numeric_limits<float>::infinity();
    float d=0;
    for(std::size_t i=0;i<a.size();++i) d=std::max(d,std::abs(a[i]-b[i]));
    return d;
}

static DecodedDngFrame make_frame(int w,int h){
    DecodedDngFrame f;f.meta.width=w;f.meta.height=h;f.meta.cfa=CfaPattern::BGGR;f.meta.orientation=Orientation::Normal;f.meta.whiteLevel=1023.f;
    f.meta.blackPhase={64.f,65.f,66.f,67.f};f.meta.hasNoiseProfile=true;f.meta.noiseProfile={0.0009f,1e-6f,0.0010f,1.2e-6f,0.0011f,1.4e-6f};
    f.meta.cameraToXyzD50={0.62f,0.21f,0.08f,0.18f,0.71f,0.07f,0.03f,0.12f,0.79f};f.meta.hasGainField=true;f.meta.hasResidualBlack=true;f.meta.sourceId="synthetic_streaming_equivalence";
    const std::size_t N=std::size_t(w)*h;f.raw.resize(N);f.gainField.resize(N);f.rowBias.resize(h);f.colBias.resize(w);
    for(int y=0;y<h;++y){f.rowBias[y]=0.05f*float((y%5)-2);for(int x=0;x<w;++x){if(y==0)f.colBias[x]=0.03f*float((x%7)-3);auto i=std::size_t(y)*w+x;int v=70+((x*37+y*53+x*y*3)%900);if((x+y)%97==0)v=1023;f.raw[i]=std::uint16_t(v);f.gainField[i]=0.94f+0.12f*float((x+2*y)%19)/18.f;}}
    return f;
}

static void compare_exposure(const ExposurePlan&a,const ExposurePlan&b){
    for(int i=0;i<5;++i){REQUIRE(std::abs(a.anchorsX[i]-b.anchorsX[i])<1e-7f);REQUIRE(std::abs(a.anchorsY[i]-b.anchorsY[i])<1e-7f);}REQUIRE(std::abs(a.noiseSigmaAt2Pct-b.noiseSigmaAt2Pct)<1e-7f);REQUIRE(std::abs(a.clipFraction-b.clipFraction)<1e-7f);REQUIRE(std::abs(a.sceneToDisplayScalar-b.sceneToDisplayScalar)<1e-6f);REQUIRE(a.stage2Over1Count==b.stage2Over1Count);
}

} // namespace
