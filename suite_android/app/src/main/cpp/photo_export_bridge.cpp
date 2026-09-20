#include <jni.h>

#include "adaptive_detail_v47j_adapter.h"
#include "dng_color_binding_producer_v0_2.h"
#include "full_frame_streaming_v0_1.h"
#include "output_acutance_v0_81.h"
#include "raw_source_adapter_bridge_common.h"
#include "scientific_master_streaming_binding_v0_3.h"
#include "scientific_preview_source_binding_v0_1.h"
#include "scientific_preview_source_binding_v0_2.h"
#include "technical_backplane_phase2_v0_1.h"
#include "tile_native_dng_source_v0_1.h"
#include "truthraw/core.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>

namespace {

using truthraw::NeutralReferenceAppearance;
using truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction;
using truthraw::TileRect;
using truthraw::dng_color_binding_producer_v0_2::ProducerResult;
using truthraw::scientific_preview_binding_v0_1::SourceSeal;
using truthraw::scientific_preview_binding_v0_2::PreparedScientificPreviewSource;
using truthraw::streaming_v0_1::HalfStateRect;
using truthraw::streaming_v0_1::IStreamingSink;
using truthraw::streaming_v0_1::StreamStatus;
using truthraw::streaming_v0_1::StreamStatusCode;
using truthraw::streaming_v0_1::StreamingOptions;
using truthraw::streaming_v0_1::StreamingResult;
using truthraw::streaming_v0_1::StreamingTruthRawProcessor;
using truthraw::tile_dng_v0_1::PosixFdByteSource;

namespace adaptive_detail = truthraw::adaptive_detail_v47j_adapter;

constexpr jlong kMagic = 0x54524a50; // TRJP
constexpr std::size_t kPacketLongs = 22u;
constexpr jint kFlagLight = 1 << 0;
constexpr jint kFlagHdr = 1 << 1;
constexpr jint kFlagDetail = 1 << 2;
constexpr jint kFlagRestoration = 1 << 3;
constexpr jint kAllowedFlags = kFlagLight | kFlagHdr | kFlagDetail | kFlagRestoration;
constexpr int kTileCore = 128;
constexpr int kTileHalo = 16;

jlongArray packet(JNIEnv* env, jlong status) {
    std::array<jlong, kPacketLongs> v{};
    v[0] = kMagic;
    v[1] = status;
    auto out = env->NewLongArray(static_cast<jsize>(v.size()));
    if (out) env->SetLongArrayRegion(out, 0, static_cast<jsize>(v.size()), v.data());
    return out;
}

jlong binding_status(const truthraw::scientific_preview_binding_v0_1::BindingStatus& s) {
    return 2000 + static_cast<jlong>(s.code);
}
jlong producer_status(const truthraw::dng_color_binding_producer_v0_2::ProducerStatus& s) {
    return 2100 + static_cast<jlong>(s.code);
}
jlong adapter_status(const truthraw::multivendor_raw_source_adapter::v0_1::AdapterStatus& s) {
    return 7000 + static_cast<jlong>(s.code);
}
jlong science_status(const truthraw::scientific_master_streaming_binding::v0_3::Status& s) {
    return 8000 + static_cast<jlong>(s.code);
}
jlong phase2_status(const truthraw::technical_backplane_phase2::v0_1::Status& s) {
    return 9000 + static_cast<jlong>(s.code);
}
jlong stream_status(const StreamStatus& s) {
    return 10000 + static_cast<jlong>(s.code);
}

bool pwrite_all(int fd, std::uint64_t offset, const std::uint8_t* data, std::size_t size) noexcept {
    std::size_t done = 0u;
    while (done < size) {
        const auto pos = static_cast<off_t>(offset + done);
        const ssize_t n = ::pwrite(fd, data + done, size - done, pos);
        if (n <= 0) return false;
        done += static_cast<std::size_t>(n);
    }
    return true;
}

bool pread_all(int fd, std::uint64_t offset, std::uint8_t* data, std::size_t size) noexcept {
    std::size_t done = 0u;
    while (done < size) {
        const auto pos = static_cast<off_t>(offset + done);
        const ssize_t n = ::pread(fd, data + done, size - done, pos);
        if (n <= 0) return false;
        done += static_cast<std::size_t>(n);
    }
    return true;
}

bool valid_orientation(truthraw::Orientation o) noexcept {
    return o == truthraw::Orientation::Normal ||
           o == truthraw::Orientation::Rotate180 ||
           o == truthraw::Orientation::Rotate90CW ||
           o == truthraw::Orientation::Rotate90CCW;
}

int orientation_quarter_turns(truthraw::Orientation o) noexcept {
    switch (o) {
        case truthraw::Orientation::Normal: return 0;
        case truthraw::Orientation::Rotate90CW: return 1;
        case truthraw::Orientation::Rotate180: return 2;
        case truthraw::Orientation::Rotate90CCW: return 3;
    }
    return -1;
}

truthraw::Orientation orientation_from_quarter_turns(int turns) noexcept {
    switch (((turns % 4) + 4) % 4) {
        case 0: return truthraw::Orientation::Normal;
        case 1: return truthraw::Orientation::Rotate90CW;
        case 2: return truthraw::Orientation::Rotate180;
        default: return truthraw::Orientation::Rotate90CCW;
    }
}

truthraw::Orientation compose_orientation(
    truthraw::Orientation sourceOrientation,
    int userQuarterTurns) noexcept {
    const int sourceTurns = orientation_quarter_turns(sourceOrientation);
    if (sourceTurns < 0) return sourceOrientation;
    return orientation_from_quarter_turns(sourceTurns + userQuarterTurns);
}

struct Rect {
    int x0=0, y0=0, x1=0, y1=0;
};

Rect display_rect_for_source(const TileRect& r, int width, int height, truthraw::Orientation o) noexcept {
    switch (o) {
        case truthraw::Orientation::Normal:
            return {r.x0, r.y0, r.x1, r.y1};
        case truthraw::Orientation::Rotate180:
            return {width-r.x1, height-r.y1, width-r.x0, height-r.y0};
        case truthraw::Orientation::Rotate90CW:
            return {height-r.y1, r.x0, height-r.y0, r.x1};
        case truthraw::Orientation::Rotate90CCW:
            return {r.y0, width-r.x1, r.y1, width-r.x0};
    }
    return {};
}

void display_to_source(int dx, int dy, int width, int height, truthraw::Orientation o, int& sx, int& sy) noexcept {
    switch (o) {
        case truthraw::Orientation::Normal:
            sx=dx; sy=dy; return;
        case truthraw::Orientation::Rotate180:
            sx=width-1-dx; sy=height-1-dy; return;
        case truthraw::Orientation::Rotate90CW:
            sx=dy; sy=height-1-dx; return;
        case truthraw::Orientation::Rotate90CCW:
            sx=width-1-dy; sy=dx; return;
    }
    sx=-1; sy=-1;
}

std::uint8_t linear_to_srgb(float linear) noexcept {
    if (!std::isfinite(linear)) return 0u;
    const float x = std::clamp(linear, 0.0f, 1.0f);
    const float e = x <= 0.0031308f ? 12.92f*x : 1.055f*std::pow(x, 1.0f/2.4f)-0.055f;
    return static_cast<std::uint8_t>(std::clamp<long>(std::lround(e*255.0f), 0, 255));
}

void rgb_to_yuv(std::uint8_t r, std::uint8_t g, std::uint8_t b,
                std::uint8_t& y, std::uint8_t& u, std::uint8_t& v) noexcept {
    const int ri=r, gi=g, bi=b;
    const int yi=((66*ri + 129*gi + 25*bi + 128) >> 8) + 16;
    const int ui=((-38*ri - 74*gi + 112*bi + 128) >> 8) + 128;
    const int vi=((112*ri - 94*gi - 18*bi + 128) >> 8) + 128;
    y=static_cast<std::uint8_t>(std::clamp(yi,0,255));
    u=static_cast<std::uint8_t>(std::clamp(ui,0,255));
    v=static_cast<std::uint8_t>(std::clamp(vi,0,255));
}

float smoothstep(float x) noexcept {
    x=std::clamp(x,0.0f,1.0f);
    return x*x*(3.0f-2.0f*x);
}

class FullResNv21Sink final : public IStreamingSink {
public:
    FullResNv21Sink(
        int fd,
        jint flags,
        jint userQuarterTurns,
        truthraw::streaming_v0_1::IRawTileSource& source,
        float noiseSigmaAt2Pct)
        : fd_(fd),
          flags_(flags),
          userQuarterTurns_(userQuarterTurns),
          source_(source),
          noiseSigmaAt2Pct_(noiseSigmaAt2Pct),
          outputProfile_(
              (flags & kFlagDetail) != 0
                  ? truthraw_v47k::OutputProfile::AdaptiveDetail
                  : truthraw_v47k::OutputProfile::Neutral) {}

    std::size_t residentBytesUpperBound() const override {
        // Finish-frame uses one bounded core tile plus 3-pixel support halo.
        return 4u * 1024u * 1024u;
    }

    StreamStatus beginFrame(
        int width,
        int height,
        truthraw::Orientation orientation,
        const truthraw::ExposurePlan& exposure,
        bool hdrEnabled,
        bool diagnosticsEnabled) override {
        if (begun_ || fd_ < 0 || width <= 0 || height <= 0 ||
            !valid_orientation(orientation) || diagnosticsEnabled ||
            !std::isfinite(noiseSigmaAt2Pct_) || noiseSigmaAt2Pct_ < 0.0f) {
            return StreamStatus::error(
                StreamStatusCode::InvalidArgument,
                "invalid full-res staged begin frame");
        }

        sourceWidth_=width;
        sourceHeight_=height;
        sourceOrientation_=orientation;
        orientation_=compose_orientation(orientation,userQuarterTurns_);
        exposure_=exposure;
        hdrPipelineEnabled_=hdrEnabled;

        const bool rotated=orientation_==truthraw::Orientation::Rotate90CW ||
                           orientation_==truthraw::Orientation::Rotate90CCW;
        displayWidth_=rotated?height:width;
        displayHeight_=rotated?width:height;
        if ((displayWidth_&1)!=0 || (displayHeight_&1)!=0 ||
            (sourceWidth_&1)!=0 || (sourceHeight_&1)!=0) {
            return StreamStatus::error(
                StreamStatusCode::UnsupportedExecution,
                "full-res NV21 route requires even source/display dimensions");
        }

        const std::uint64_t pixels=
            static_cast<std::uint64_t>(sourceWidth_)*static_cast<std::uint64_t>(sourceHeight_);
        if (pixels==0u) {
            return StreamStatus::error(StreamStatusCode::InvalidArgument,"zero-sized frame");
        }
        const std::uint64_t halfW=static_cast<std::uint64_t>((sourceWidth_+1)/2);
        const std::uint64_t halfH=static_cast<std::uint64_t>((sourceHeight_+1)/2);
        const std::uint64_t halfCells=halfW*halfH;

        nv21Bytes_=pixels + pixels/2u;
        scratchRgbOffset_=nv21Bytes_;
        scratchRgbBytes_=pixels*3u*sizeof(float);
        scratchGainOffset_=scratchRgbOffset_+scratchRgbBytes_;
        scratchGainBytes_=halfCells*sizeof(float);
        scratchMaskOffset_=scratchGainOffset_+scratchGainBytes_;
        scratchMaskBytes_=pixels;
        scratchTotalBytes_=scratchMaskOffset_+scratchMaskBytes_;

        if (scratchTotalBytes_ > static_cast<std::uint64_t>(std::numeric_limits<off_t>::max()) ||
            ::ftruncate(fd_,static_cast<off_t>(scratchTotalBytes_))!=0) {
            return StreamStatus::error(
                StreamStatusCode::SinkFailed,
                "full-res staged allocation failed");
        }

        halfWidth_=static_cast<int>(halfW);
        halfHeight_=static_cast<int>(halfH);
        begun_=true;
        return StreamStatus::ok();
    }

    StreamStatus writeSdrTile(
        const TileRect& r,
        const float* rgb,
        std::size_t floatCount) override {
        if (!begun_ || finished_ || rgb==nullptr ||
            r.x0<0 || r.y0<0 || r.x1>sourceWidth_ || r.y1>sourceHeight_ ||
            r.x0>=r.x1 || r.y0>=r.y1) {
            return StreamStatus::error(StreamStatusCode::SinkFailed,"invalid staged SDR tile");
        }
        const int w=r.x1-r.x0;
        const int h=r.y1-r.y0;
        const std::size_t pixels=static_cast<std::size_t>(w)*static_cast<std::size_t>(h);
        if(floatCount!=3u*pixels) {
            return StreamStatus::error(StreamStatusCode::SinkFailed,"staged SDR count mismatch");
        }

        for(int y=0;y<h;++y) {
            const std::uint64_t pixelIndex=
                static_cast<std::uint64_t>(r.y0+y)*sourceWidth_+r.x0;
            const auto* row=reinterpret_cast<const std::uint8_t*>(
                rgb+3u*static_cast<std::size_t>(y)*static_cast<std::size_t>(w));
            const std::size_t bytes=static_cast<std::size_t>(w)*3u*sizeof(float);
            if(!pwrite_all(fd_,scratchRgbOffset_+pixelIndex*3u*sizeof(float),row,bytes)) {
                return StreamStatus::error(StreamStatusCode::SinkFailed,"staged RGB write failed");
            }
        }

        TileRect rawRect{r.x0,r.y0,r.x1,r.y1,r.x0,r.y0,r.x1,r.y1};
        std::vector<std::uint16_t> raw(pixels);
        std::vector<float> gain;
        if(source_.metadata().hasGainField) gain.resize(pixels);
        const auto rawStatus=source_.readRawTile(
            rawRect,
            raw.data(),
            raw.size(),
            source_.metadata().hasGainField?gain.data():nullptr,
            source_.metadata().hasGainField?gain.size():0u);
        if(!rawStatus) return rawStatus;

        std::vector<std::uint8_t> maskRow(static_cast<std::size_t>(w));
        for(int y=0;y<h;++y) {
            for(int x=0;x<w;++x) {
                const std::size_t local=
                    static_cast<std::size_t>(y)*static_cast<std::size_t>(w)+
                    static_cast<std::size_t>(x);
                maskRow[static_cast<std::size_t>(x)] =
                    static_cast<float>(raw[local])>=source_.metadata().whiteLevel?1u:0u;
            }
            const std::uint64_t pixelIndex=
                static_cast<std::uint64_t>(r.y0+y)*sourceWidth_+r.x0;
            if(!pwrite_all(
                    fd_,
                    scratchMaskOffset_+pixelIndex,
                    maskRow.data(),
                    maskRow.size())) {
                return StreamStatus::error(StreamStatusCode::SinkFailed,"staged censor-mask write failed");
            }
        }

        stagedPixels_+=pixels;
        return StreamStatus::ok();
    }

    StreamStatus writeHalfLogGainBlock(
        const HalfStateRect& rect,
        const float* gain,
        std::size_t count) override {
        const int w=rect.x1-rect.x0;
        const int h=rect.y1-rect.y0;
        if(!begun_ || finished_ || gain==nullptr || w<=0 || h<=0 ||
            count!=static_cast<std::size_t>(w)*static_cast<std::size_t>(h) ||
            rect.x0<0 || rect.y0<0 || rect.x1>halfWidth_ || rect.y1>halfHeight_) {
            return StreamStatus::error(StreamStatusCode::SinkFailed,"invalid staged HDR gain block");
        }
        for(int y=0;y<h;++y) {
            const std::uint64_t qIndex=
                static_cast<std::uint64_t>(rect.y0+y)*halfWidth_+rect.x0;
            const auto* row=reinterpret_cast<const std::uint8_t*>(
                gain+static_cast<std::size_t>(y)*static_cast<std::size_t>(w));
            const std::size_t bytes=static_cast<std::size_t>(w)*sizeof(float);
            if(!pwrite_all(fd_,scratchGainOffset_+qIndex*sizeof(float),row,bytes)) {
                return StreamStatus::error(StreamStatusCode::SinkFailed,"staged HDR gain write failed");
            }
        }
        stagedGainCells_+=count;
        return StreamStatus::ok();
    }

    StreamStatus writeStage2DiagnosticTile(
        const TileRect&, const float*, std::size_t) override {
        return StreamStatus::error(
            StreamStatusCode::UnsupportedExecution,
            "diagnostics disabled for full-res photo export");
    }

    StreamStatus finishFrame() override {
        if(!begun_ || finished_) {
            return StreamStatus::error(StreamStatusCode::SinkFailed,"invalid staged finish");
        }
        const std::uint64_t expectedPixels=
            static_cast<std::uint64_t>(sourceWidth_)*sourceHeight_;
        const std::uint64_t expectedGainCells=
            static_cast<std::uint64_t>(halfWidth_)*halfHeight_;
        if(stagedPixels_!=expectedPixels || stagedGainCells_!=expectedGainCells) {
            return StreamStatus::error(
                StreamStatusCode::SinkFailed,
                "full-res staged coverage incomplete");
        }

        const auto plan=truthraw_v47k::choose_output_acutance_plan(
            noiseSigmaAt2Pct_,1.0f,outputProfile_);

        for(int y0=0;y0<sourceHeight_;y0+=kTileCore) {
            const int y1=std::min(sourceHeight_,y0+kTileCore);
            for(int x0=0;x0<sourceWidth_;x0+=kTileCore) {
                const int x1=std::min(sourceWidth_,x0+kTileCore);
                const auto status=finalizeCoreTile(x0,y0,x1,y1,plan);
                if(!status) return status;
            }
        }

        if(writtenPixels_!=expectedPixels || ::fsync(fd_)!=0 ||
            ::ftruncate(fd_,static_cast<off_t>(nv21Bytes_))!=0 ||
            ::fsync(fd_)!=0) {
            return StreamStatus::error(
                StreamStatusCode::SinkFailed,
                "full-res NV21 finalization incomplete");
        }
        finished_=true;
        return StreamStatus::ok();
    }

    int width() const noexcept { return displayWidth_; }
    int height() const noexcept { return displayHeight_; }
    std::uint64_t outputBytes() const noexcept { return nv21Bytes_; }
    std::uint64_t lightAdjustedPixels() const noexcept { return lightAdjustedPixels_; }
    std::uint64_t hdrPositiveGainSamples() const noexcept { return hdrPositiveGainSamples_; }
    std::uint64_t restoredPixels() const noexcept { return restoredPixels_; }
    std::uint64_t censoredPixels() const noexcept { return censoredPixels_; }
    bool hdrBaked() const noexcept {
        return (flags_&kFlagHdr)!=0 &&
               hdrPipelineEnabled_ &&
               hdrPositiveGainSamples_>0u;
    }
    bool restorationBaked() const noexcept {
        return (flags_&kFlagRestoration)!=0 && restoredPixels_>0u;
    }

private:
    StreamStatus readRgbRect(
        int x0,int y0,int x1,int y1,
        std::vector<float>& out) const {
        const int w=x1-x0;
        const int h=y1-y0;
        if(w<=0||h<=0) return StreamStatus::error(StreamStatusCode::SinkFailed,"invalid RGB scratch rect");
        out.resize(3u*static_cast<std::size_t>(w)*static_cast<std::size_t>(h));
        for(int y=0;y<h;++y) {
            const std::uint64_t pixelIndex=
                static_cast<std::uint64_t>(y0+y)*sourceWidth_+x0;
            auto* row=reinterpret_cast<std::uint8_t*>(
                out.data()+3u*static_cast<std::size_t>(y)*static_cast<std::size_t>(w));
            const std::size_t bytes=static_cast<std::size_t>(w)*3u*sizeof(float);
            if(!pread_all(fd_,scratchRgbOffset_+pixelIndex*3u*sizeof(float),row,bytes)) {
                return StreamStatus::error(StreamStatusCode::SinkFailed,"RGB scratch read failed");
            }
        }
        return StreamStatus::ok();
    }

    StreamStatus readMaskRect(
        int x0,int y0,int x1,int y1,
        std::vector<std::uint8_t>& out) const {
        const int w=x1-x0;
        const int h=y1-y0;
        out.resize(static_cast<std::size_t>(w)*static_cast<std::size_t>(h));
        for(int y=0;y<h;++y) {
            const std::uint64_t pixelIndex=
                static_cast<std::uint64_t>(y0+y)*sourceWidth_+x0;
            auto* row=out.data()+static_cast<std::size_t>(y)*static_cast<std::size_t>(w);
            if(!pread_all(fd_,scratchMaskOffset_+pixelIndex,row,static_cast<std::size_t>(w))) {
                return StreamStatus::error(StreamStatusCode::SinkFailed,"mask scratch read failed");
            }
        }
        return StreamStatus::ok();
    }

    StreamStatus readGainRect(
        int x0,int y0,int x1,int y1,
        std::vector<float>& out) const {
        const int w=x1-x0;
        const int h=y1-y0;
        out.resize(static_cast<std::size_t>(w)*static_cast<std::size_t>(h));
        for(int y=0;y<h;++y) {
            const std::uint64_t qIndex=
                static_cast<std::uint64_t>(y0+y)*halfWidth_+x0;
            auto* row=reinterpret_cast<std::uint8_t*>(
                out.data()+static_cast<std::size_t>(y)*static_cast<std::size_t>(w));
            const std::size_t bytes=static_cast<std::size_t>(w)*sizeof(float);
            if(!pread_all(fd_,scratchGainOffset_+qIndex*sizeof(float),row,bytes)) {
                return StreamStatus::error(StreamStatusCode::SinkFailed,"gain scratch read failed");
            }
        }
        return StreamStatus::ok();
    }

    StreamStatus finalizeCoreTile(
        int x0,int y0,int x1,int y1,
        const truthraw_v47k::OutputAcutancePlan& plan) {
        constexpr int kSupportHalo=3;
        const int sx0=std::max(0,x0-kSupportHalo);
        const int sy0=std::max(0,y0-kSupportHalo);
        const int sx1=std::min(sourceWidth_,x1+kSupportHalo);
        const int sy1=std::min(sourceHeight_,y1+kSupportHalo);
        const int sw=sx1-sx0;

        std::vector<float> supportRgb;
        std::vector<std::uint8_t> supportMask;
        auto st=readRgbRect(sx0,sy0,sx1,sy1,supportRgb);
        if(!st) return st;
        st=readMaskRect(sx0,sy0,sx1,sy1,supportMask);
        if(!st) return st;

        const int ax0=std::max(0,x0-1);
        const int ay0=std::max(0,y0-1);
        const int ax1=std::min(sourceWidth_,x1+1);
        const int ay1=std::min(sourceHeight_,y1+1);
        const int aw=ax1-ax0;
        const int ah=ay1-ay0;
        std::vector<float> preAcutance(
            3u*static_cast<std::size_t>(aw)*static_cast<std::size_t>(ah));

        for(int y=ay0;y<ay1;++y) {
            for(int x=ax0;x<ax1;++x) {
                const std::size_t si=
                    static_cast<std::size_t>(y-sy0)*sw+static_cast<std::size_t>(x-sx0);
                float r=std::max(supportRgb[3u*si],0.0f);
                float g=std::max(supportRgb[3u*si+1u],0.0f);
                float b=std::max(supportRgb[3u*si+2u],0.0f);
                if(!std::isfinite(r)||!std::isfinite(g)||!std::isfinite(b)) r=g=b=0.0f;
                const bool censored=supportMask[si]!=0u;

                bool restored=false;
                if((flags_&kFlagRestoration)!=0 && censored) {
                    double sumR=0.0,sumG=0.0,sumB=0.0,sumW=0.0;
                    int support=0;
                    for(int radius=1;radius<=2 && support<3;++radius) {
                        for(int dy=-radius;dy<=radius;++dy) {
                            for(int dx=-radius;dx<=radius;++dx) {
                                if(dx==0&&dy==0) continue;
                                if(std::max(std::abs(dx),std::abs(dy))!=radius) continue;
                                const int xx=x+dx, yy=y+dy;
                                if(xx<0||yy<0||xx>=sourceWidth_||yy>=sourceHeight_) continue;
                                if(xx<sx0||xx>=sx1||yy<sy0||yy>=sy1) continue;
                                const std::size_t ni=
                                    static_cast<std::size_t>(yy-sy0)*sw+
                                    static_cast<std::size_t>(xx-sx0);
                                if(supportMask[ni]!=0u) continue;
                                const float rr=supportRgb[3u*ni];
                                const float gg=supportRgb[3u*ni+1u];
                                const float bb=supportRgb[3u*ni+2u];
                                if(!std::isfinite(rr)||!std::isfinite(gg)||!std::isfinite(bb)) continue;
                                const double weight=
                                    1.0/std::sqrt(static_cast<double>(dx*dx+dy*dy));
                                sumR+=weight*rr; sumG+=weight*gg; sumB+=weight*bb;
                                sumW+=weight; ++support;
                            }
                        }
                    }
                    if(support>=3 && sumW>0.0) {
                        r=static_cast<float>(sumR/sumW);
                        g=static_cast<float>(sumG/sumW);
                        b=static_cast<float>(sumB/sumW);
                        restored=true;
                    }
                }

                if((flags_&kFlagLight)!=0 && !censored) {
                    const float lum=std::max(truthraw::luminance709(r,g,b),0.0f);
                    const float darkGate=1.0f-smoothstep((lum-0.02f)/0.30f);
                    const float blackProtect=smoothstep(lum/0.025f);
                    const float strength=
                        0.18f*std::clamp(exposure_.evidenceConfidence,0.0f,1.0f)*
                        darkGate*blackProtect;
                    if(strength>1e-4f) {
                        const float sc=1.0f+strength;
                        r*=sc; g*=sc; b*=sc;
                        if(x>=x0&&x<x1&&y>=y0&&y<y1) ++lightAdjustedPixels_;
                    }
                }

                if(x>=x0&&x<x1&&y>=y0&&y<y1) {
                    if(censored) ++censoredPixels_;
                    if(restored) ++restoredPixels_;
                }

                const std::size_t ai=
                    static_cast<std::size_t>(y-ay0)*aw+static_cast<std::size_t>(x-ax0);
                preAcutance[3u*ai]=r;
                preAcutance[3u*ai+1u]=g;
                preAcutance[3u*ai+2u]=b;
            }
        }

        std::vector<float> acutance(preAcutance.size());
        if(!truthraw_v47k::apply_output_acutance(
                preAcutance.data(),aw,ah,plan,acutance.data())) {
            return StreamStatus::error(StreamStatusCode::SinkFailed,"full-res output acutance failed");
        }

        const int qx0=x0/2;
        const int qy0=y0/2;
        const int qx1=(x1+1)/2;
        const int qy1=(y1+1)/2;
        const int qw=qx1-qx0;
        std::vector<float> gain;
        st=readGainRect(qx0,qy0,qx1,qy1,gain);
        if(!st) return st;

        const int cw=x1-x0;
        const int ch=y1-y0;
        std::vector<std::uint8_t> coreRgb(
            3u*static_cast<std::size_t>(cw)*static_cast<std::size_t>(ch));

        for(int y=y0;y<y1;++y) {
            for(int x=x0;x<x1;++x) {
                const std::size_t ai=
                    static_cast<std::size_t>(y-ay0)*aw+static_cast<std::size_t>(x-ax0);
                float r=acutance[3u*ai];
                float g=acutance[3u*ai+1u];
                float b=acutance[3u*ai+2u];
                const std::size_t si=
                    static_cast<std::size_t>(y-sy0)*sw+static_cast<std::size_t>(x-sx0);
                const bool censored=supportMask[si]!=0u;

                if((flags_&kFlagHdr)!=0 && hdrPipelineEnabled_ && !censored) {
                    const int qx=x/2-qx0;
                    const int qy=y/2-qy0;
                    const float halfLog=gain[
                        static_cast<std::size_t>(qy)*qw+static_cast<std::size_t>(qx)];
                    const float legacyGain=
                        truthraw::output_acutance_v0_81::legacy_display_gain_from_half_log(halfLog);
                    const float beforeY=truthraw_v47k::luminance709(
                        preAcutance[3u*ai],
                        preAcutance[3u*ai+1u],
                        preAcutance[3u*ai+2u]);
                    const float afterY=truthraw_v47k::luminance709(r,g,b);
                    const float target=beforeY*legacyGain;
                    float rebased=1.0f;
                    if(afterY>1e-8f && target>afterY) {
                        rebased=std::clamp(target/afterY,1.0f,3.04f);
                    }
                    if(!std::isfinite(rebased) || rebased<1.0f) {
                        return StreamStatus::error(StreamStatusCode::SinkFailed,"invalid full-res HDR rebase");
                    }
                    if(rebased>1.0f+1e-5f) {
                        r*=rebased; g*=rebased; b*=rebased;
                        ++hdrPositiveGainSamples_;
                    }
                }

                const float mx=std::max(r,std::max(g,b));
                if(mx>0.92f) {
                    const float shoulder=
                        0.92f+0.08f*(1.0f-std::exp(-3.0f*(mx-0.92f)));
                    const float sc=shoulder/std::max(mx,1e-8f);
                    r*=sc; g*=sc; b*=sc;
                }

                const std::size_t ci=
                    static_cast<std::size_t>(y-y0)*cw+static_cast<std::size_t>(x-x0);
                coreRgb[3u*ci]=linear_to_srgb(r);
                coreRgb[3u*ci+1u]=linear_to_srgb(g);
                coreRgb[3u*ci+2u]=linear_to_srgb(b);
            }
        }

        const TileRect coreRect{x0,y0,x1,y1,x0,y0,x1,y1};
        const Rect dr=display_rect_for_source(coreRect,sourceWidth_,sourceHeight_,orientation_);
        const int dw=dr.x1-dr.x0;
        const int dh=dr.y1-dr.y0;
        if(dw<=0||dh<=0||(dr.x0&1)!=0||(dr.y0&1)!=0||(dr.x1&1)!=0||(dr.y1&1)!=0) {
            return StreamStatus::error(
                StreamStatusCode::UnsupportedExecution,
                "full-res final tile is not NV21 2x2 aligned");
        }

        std::vector<std::uint8_t> yrow(static_cast<std::size_t>(dw));
        for(int dy=dr.y0;dy<dr.y1;++dy) {
            for(int dx=dr.x0;dx<dr.x1;++dx) {
                int sx=-1,sy=-1;
                display_to_source(dx,dy,sourceWidth_,sourceHeight_,orientation_,sx,sy);
                if(sx<x0||sx>=x1||sy<y0||sy>=y1) {
                    return StreamStatus::error(StreamStatusCode::SinkFailed,"final orientation escaped core");
                }
                const std::size_t ci=
                    static_cast<std::size_t>(sy-y0)*cw+static_cast<std::size_t>(sx-x0);
                std::uint8_t Y,U,V;
                rgb_to_yuv(coreRgb[3u*ci],coreRgb[3u*ci+1u],coreRgb[3u*ci+2u],Y,U,V);
                (void)U; (void)V;
                yrow[static_cast<std::size_t>(dx-dr.x0)]=Y;
            }
            const std::uint64_t off=
                static_cast<std::uint64_t>(dy)*displayWidth_+dr.x0;
            if(!pwrite_all(fd_,off,yrow.data(),yrow.size())) {
                return StreamStatus::error(StreamStatusCode::SinkFailed,"final NV21 Y write failed");
            }
        }

        std::vector<std::uint8_t> uvrow(static_cast<std::size_t>(dw));
        for(int dy=dr.y0;dy<dr.y1;dy+=2) {
            for(int dx=dr.x0;dx<dr.x1;dx+=2) {
                int rs=0,gs=0,bs=0;
                for(int oy=0;oy<2;++oy) for(int ox=0;ox<2;++ox) {
                    int sx=-1,sy=-1;
                    display_to_source(dx+ox,dy+oy,sourceWidth_,sourceHeight_,orientation_,sx,sy);
                    if(sx<x0||sx>=x1||sy<y0||sy>=y1) {
                        return StreamStatus::error(StreamStatusCode::SinkFailed,"final chroma orientation escaped core");
                    }
                    const std::size_t ci=
                        static_cast<std::size_t>(sy-y0)*cw+static_cast<std::size_t>(sx-x0);
                    rs+=coreRgb[3u*ci];
                    gs+=coreRgb[3u*ci+1u];
                    bs+=coreRgb[3u*ci+2u];
                }
                const std::uint8_t R=static_cast<std::uint8_t>((rs+2)/4);
                const std::uint8_t G=static_cast<std::uint8_t>((gs+2)/4);
                const std::uint8_t B=static_cast<std::uint8_t>((bs+2)/4);
                std::uint8_t Y,U,V;
                rgb_to_yuv(R,G,B,Y,U,V);
                (void)Y;
                const std::size_t p=static_cast<std::size_t>(dx-dr.x0);
                uvrow[p]=V;
                uvrow[p+1u]=U;
            }
            const std::uint64_t off=
                nv21LumaBytes_()+
                static_cast<std::uint64_t>(dy/2)*displayWidth_+dr.x0;
            if(!pwrite_all(fd_,off,uvrow.data(),uvrow.size())) {
                return StreamStatus::error(StreamStatusCode::SinkFailed,"final NV21 VU write failed");
            }
        }

        writtenPixels_+=static_cast<std::uint64_t>(cw)*ch;
        return StreamStatus::ok();
    }

    std::uint64_t nv21LumaBytes_() const noexcept {
        return static_cast<std::uint64_t>(displayWidth_)*displayHeight_;
    }

    int fd_=-1;
    jint flags_=0;
    jint userQuarterTurns_=0;
    truthraw::streaming_v0_1::IRawTileSource& source_;
    float noiseSigmaAt2Pct_=0.0f;
    truthraw_v47k::OutputProfile outputProfile_=truthraw_v47k::OutputProfile::Neutral;

    int sourceWidth_=0;
    int sourceHeight_=0;
    int displayWidth_=0;
    int displayHeight_=0;
    int halfWidth_=0;
    int halfHeight_=0;
    truthraw::Orientation sourceOrientation_=truthraw::Orientation::Normal;
    truthraw::Orientation orientation_=truthraw::Orientation::Normal;
    truthraw::ExposurePlan exposure_{};
    bool hdrPipelineEnabled_=false;
    bool begun_=false;
    bool finished_=false;

    std::uint64_t nv21Bytes_=0;
    std::uint64_t scratchRgbOffset_=0;
    std::uint64_t scratchRgbBytes_=0;
    std::uint64_t scratchGainOffset_=0;
    std::uint64_t scratchGainBytes_=0;
    std::uint64_t scratchMaskOffset_=0;
    std::uint64_t scratchMaskBytes_=0;
    std::uint64_t scratchTotalBytes_=0;

    std::uint64_t stagedPixels_=0;
    std::uint64_t stagedGainCells_=0;
    std::uint64_t writtenPixels_=0;
    std::uint64_t lightAdjustedPixels_=0;
    std::uint64_t hdrPositiveGainSamples_=0;
    std::uint64_t restoredPixels_=0;
    std::uint64_t censoredPixels_=0;
};

StreamingOptions photo_options(std::size_t memoryBudgetBytes, jint flags) {
    StreamingOptions o;
    o.tile={kTileCore,kTileHalo};
    o.workers=1;
    o.hdrEnabled=(flags&kFlagHdr)!=0;
    o.streamScientificDiagnostics=false;
    o.sdrLutSize=4096;
    o.memoryBudgetBytes=memoryBudgetBytes;
    return o;
}

} // namespace

extern "C" JNIEXPORT jlongArray JNICALL
Java_com_truthraw_adaptiveui_PhotoExportNativeBridge_renderFullResNv21(
    JNIEnv* env, jobject, jint sourceFd, jint outputFd, jint flags, jint sourceRouteCode,
    jint userQuarterTurns, jint maxSourceResidentBytes, jint maxLogicalResidentBytes) {
    if (sourceFd<0 || outputFd<0 || maxSourceResidentBytes<=0 || maxLogicalResidentBytes<=0 ||
        userQuarterTurns<0 || userQuarterTurns>3 ||
        (flags&~kAllowedFlags)!=0 || (sourceRouteCode!=0 && sourceRouteCode!=1)) {
        return packet(env,-1);
    }

    auto bytes=std::make_shared<PosixFdByteSource>(static_cast<int>(sourceFd));
    SourceSeal seal;
    const auto sealed=truthraw::scientific_preview_binding_v0_1::seal_source_sha256(*bytes,seal);
    if(!sealed) return packet(env,binding_status(sealed));

    ProducerResult produced;
    const auto color=truthraw::dng_color_binding_producer_v0_2::produce_source_metadata_color_binding(*bytes,seal,produced);
    if(!color) return packet(env,producer_status(color));

    PreparedScientificPreviewSource prepared;
    const auto prep=truthraw::scientific_preview_binding_v0_2::prepare_scientific_color_source(seal,produced.color,prepared);
    if(!prep) return packet(env,binding_status(prep));
    if(!prepared.mainHouseComputeAllowed || prepared.physicalFrameCount!=1u || prepared.independentEvidenceCount!=1u) {
        return packet(env,-2);
    }

    const auto pre=truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes,seal);
    if(!pre) return packet(env,binding_status(pre));

    auto openOptions=prepared.tileNativeOptions;
    openOptions.maxResidentBytes=static_cast<std::size_t>(maxSourceResidentBytes);
    truthraw::android_raw_adapter_bridge::v0_1::OpenedDngSource opened;
    const auto openedStatus=truthraw::android_raw_adapter_bridge::v0_1::openDngViaAdapter(bytes,seal,openOptions,opened);
    if(!openedStatus) return packet(env,adapter_status(openedStatus));
    auto& source=opened.source;
    if ((source->metadata().width&1)!=0 || (source->metadata().height&1)!=0) return packet(env,-3);

    auto reconstruction=std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();

    truthraw::scientific_master_streaming_binding::v0_3::Options sciOptions;
    sciOptions.memoryBudgetBytes=static_cast<std::size_t>(maxLogicalResidentBytes);
    truthraw::scientific_master_streaming_binding::v0_3::Result sci;
    const auto sciStatus=truthraw::scientific_master_streaming_binding::v0_3::bind_scientific_master_streaming(
        *source,*reconstruction,sciOptions,sci);
    if(!sciStatus) return packet(env,science_status(sciStatus));

    truthraw::technical_backplane_phase2::v0_1::Phase2Input phaseInput;
    phaseInput.prepared=prepared;
    phaseInput.scientificMasterHash=sci.scientificMasterHash;
    phaseInput.zeroLineGauge=sci.zeroLineGauge;
    phaseInput.sceneBinding=sci.sceneBinding;
    phaseInput.roomStatus.fill(truthraw::technical_backplane::v0_1::RoomStatus::ResearchOnly);
    phaseInput.claimStatus=truthraw::technical_backplane::v0_1::ClaimStatus::Candidate;
    truthraw::technical_backplane_phase2::v0_1::Phase2Result phase2;
    const auto phaseStatus=truthraw::technical_backplane_phase2::v0_1::finalize_phase2(phaseInput,phase2);
    if(!phaseStatus) return packet(env,phase2_status(phaseStatus));
    if(phase2.backplane.sourceEvidenceHash!=seal.sha256 ||
       phase2.backplane.scientificMasterHash!=sci.scientificMasterHash ||
       phase2.backplane.forbiddenFlags!=0u ||
       phase2.backplane.physicalFrameCount!=1u ||
       phase2.backplane.independentEvidenceCount!=1u) return packet(env,-4);

    std::shared_ptr<truthraw::IAppearanceBackend> appearance;
    const float noiseSigma=adaptive_detail::noise_sigma_2pct_from_metadata(source->metadata());
    if((flags&kFlagDetail)!=0) {
        appearance=std::make_shared<adaptive_detail::AdaptiveDetailedCrispAppearanceV47j>(noiseSigma);
    } else {
        appearance=std::make_shared<NeutralReferenceAppearance>();
    }

    FullResNv21Sink sink(
        static_cast<int>(outputFd),
        flags,
        userQuarterTurns,
        *source,
        noiseSigma);
    StreamingTruthRawProcessor processor(reconstruction,appearance);
    StreamingResult stream;
    const auto processed=processor.process(
        *source,sink,photo_options(static_cast<std::size_t>(maxLogicalResidentBytes),flags),stream);
    if(!processed) { (void)::ftruncate(outputFd,0); return packet(env,stream_status(processed)); }

    const auto post=truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes,seal);
    if(!post) { (void)::ftruncate(outputFd,0); return packet(env,binding_status(post)); }

    if(stream.provenance.physicalFrameCount!=1u || stream.provenance.independentEvidenceCount!=1u ||
       stream.provenance.scientificMasterModifiedByAppearance || stream.provenance.counterfactualObservationCreated ||
       stream.memory.adapterOwnsFullRawFrame || stream.memory.adapterOwnsFullSdrFrame) {
        (void)::ftruncate(outputFd,0); return packet(env,-5);
    }

    std::array<jlong,kPacketLongs> v{};
    v[0]=kMagic; v[1]=0; v[2]=sink.width(); v[3]=sink.height();
    v[4]=source->metadata().width; v[5]=source->metadata().height;
    v[6]=static_cast<jlong>(source->metadata().orientation);
    v[7]=static_cast<jlong>(sink.outputBytes());
    v[8]=flags;
    v[9]=(flags&kFlagDetail)!=0?1:0;
    v[10]=static_cast<jlong>(sink.lightAdjustedPixels());
    v[11]=static_cast<jlong>(sink.hdrPositiveGainSamples());
    v[12]=1; // Scientific Master digest bound and verified.
    v[13]=1; // Technical Backplane phase2 bound and verified.
    v[14]=1; // source pre/post SHA verified.
    v[15]=1; // full resolution, no preview downscale.
    v[16]=sink.hdrBaked()?1:0; // presentation HDR only; never scientific HDR authority.
    v[17]=sink.restorationBaked()?1:0; // aesthetic reintegration only; no scientific writeback.
    v[18]=stream.provenance.physicalFrameCount;
    v[19]=stream.provenance.independentEvidenceCount;
    v[20]=userQuarterTurns;
    v[21]=static_cast<jlong>(compose_orientation(source->metadata().orientation, userQuarterTurns));
    auto out=env->NewLongArray(static_cast<jsize>(v.size()));
    if(out) env->SetLongArrayRegion(out,0,static_cast<jsize>(v.size()),v.data());
    return out;
}
