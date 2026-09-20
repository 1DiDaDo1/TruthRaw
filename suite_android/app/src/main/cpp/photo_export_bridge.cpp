#include <jni.h>

#include "adaptive_detail_v47j_adapter.h"
#include "dng_color_binding_producer_v0_2.h"
#include "full_frame_streaming_v0_1.h"
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
constexpr std::size_t kPacketLongs = 20u;
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

bool valid_orientation(truthraw::Orientation o) noexcept {
    return o == truthraw::Orientation::Normal ||
           o == truthraw::Orientation::Rotate180 ||
           o == truthraw::Orientation::Rotate90CW ||
           o == truthraw::Orientation::Rotate90CCW;
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
    FullResNv21Sink(int fd, jint flags) : fd_(fd), flags_(flags) {}

    std::size_t residentBytesUpperBound() const override {
        // Only one 128x128 tile plus temporary Y/VU rows are resident.
        return 1024u * 1024u;
    }

    StreamStatus beginFrame(
        int width, int height, truthraw::Orientation orientation,
        const truthraw::ExposurePlan& exposure, bool hdrEnabled, bool diagnosticsEnabled) override {
        if (begun_ || fd_ < 0 || width <= 0 || height <= 0 || !valid_orientation(orientation) ||
            diagnosticsEnabled) {
            return StreamStatus::error(StreamStatusCode::InvalidArgument, "invalid full-res NV21 begin frame");
        }
        sourceWidth_=width; sourceHeight_=height; orientation_=orientation; exposure_=exposure;
        hdrPipelineEnabled_=hdrEnabled;
        const bool rotated=orientation==truthraw::Orientation::Rotate90CW ||
                           orientation==truthraw::Orientation::Rotate90CCW;
        displayWidth_=rotated?height:width;
        displayHeight_=rotated?width:height;
        // Android NV21/JPEG path is 4:2:0 and therefore requires even dimensions.
        if ((displayWidth_&1)!=0 || (displayHeight_&1)!=0) {
            return StreamStatus::error(StreamStatusCode::UnsupportedExecution, "NV21 requires even full-resolution dimensions");
        }
        const std::uint64_t pixels=static_cast<std::uint64_t>(displayWidth_)*displayHeight_;
        if (pixels==0u || pixels > (std::numeric_limits<std::uint64_t>::max()/3u)*2u) {
            return StreamStatus::error(StreamStatusCode::InvalidArgument, "full-res NV21 dimensions overflow");
        }
        outputBytes_=pixels + pixels/2u;
        if (outputBytes_ > static_cast<std::uint64_t>(std::numeric_limits<off_t>::max()) ||
            ::ftruncate(fd_, static_cast<off_t>(outputBytes_)) != 0) {
            return StreamStatus::error(StreamStatusCode::SinkFailed, "NV21 staging allocation failed");
        }
        begun_=true;
        return StreamStatus::ok();
    }

    StreamStatus writeSdrTile(const TileRect& r, const float* rgb, std::size_t floatCount) override {
        if (!begun_ || finished_ || rgb==nullptr || r.x0<0 || r.y0<0 ||
            r.x1>sourceWidth_ || r.y1>sourceHeight_ || r.x0>=r.x1 || r.y0>=r.y1) {
            return StreamStatus::error(StreamStatusCode::SinkFailed, "invalid full-res SDR tile");
        }
        const int sw=r.x1-r.x0, sh=r.y1-r.y0;
        const std::size_t pixels=static_cast<std::size_t>(sw)*sh;
        if (floatCount != 3u*pixels) {
            return StreamStatus::error(StreamStatusCode::SinkFailed, "full-res SDR tile count mismatch");
        }
        const Rect dr=display_rect_for_source(r,sourceWidth_,sourceHeight_,orientation_);
        const int dw=dr.x1-dr.x0, dh=dr.y1-dr.y0;
        if (dw<=0 || dh<=0 || (dr.x0&1)!=0 || (dr.y0&1)!=0 || (dr.x1&1)!=0 || (dr.y1&1)!=0) {
            return StreamStatus::error(StreamStatusCode::UnsupportedExecution, "tile boundary is not NV21 2x2 aligned");
        }

        std::vector<std::uint8_t> yrow(static_cast<std::size_t>(dw));
        for (int dy=dr.y0; dy<dr.y1; ++dy) {
            for (int dx=dr.x0; dx<dr.x1; ++dx) {
                int sx=-1, sy=-1;
                display_to_source(dx,dy,sourceWidth_,sourceHeight_,orientation_,sx,sy);
                if (sx<r.x0 || sx>=r.x1 || sy<r.y0 || sy>=r.y1) {
                    return StreamStatus::error(StreamStatusCode::SinkFailed, "orientation mapping escaped tile");
                }
                const std::size_t i=static_cast<std::size_t>(sy-r.y0)*sw + static_cast<std::size_t>(sx-r.x0);
                float rr=std::max(rgb[3u*i],0.0f);
                float gg=std::max(rgb[3u*i+1u],0.0f);
                float bb=std::max(rgb[3u*i+2u],0.0f);
                if (!std::isfinite(rr)||!std::isfinite(gg)||!std::isfinite(bb)) rr=gg=bb=0.0f;
                if ((flags_&kFlagLight)!=0) {
                    const float lum=std::max(truthraw::luminance709(rr,gg,bb),0.0f);
                    const float darkGate=1.0f-smoothstep((lum-0.02f)/0.30f);
                    const float blackProtect=smoothstep(lum/0.025f);
                    const float strength=0.18f*std::clamp(exposure_.evidenceConfidence,0.0f,1.0f)*
                                         darkGate*blackProtect;
                    if (strength>1e-4f) {
                        const float sc=1.0f+strength; rr*=sc; gg*=sc; bb*=sc; ++lightAdjustedPixels_;
                    }
                }
                const auto R=linear_to_srgb(rr), G=linear_to_srgb(gg), B=linear_to_srgb(bb);
                std::uint8_t Y,U,V; rgb_to_yuv(R,G,B,Y,U,V); (void)U; (void)V;
                yrow[static_cast<std::size_t>(dx-dr.x0)]=Y;
            }
            const std::uint64_t off=static_cast<std::uint64_t>(dy)*displayWidth_ + dr.x0;
            if (!pwrite_all(fd_,off,yrow.data(),yrow.size())) {
                return StreamStatus::error(StreamStatusCode::SinkFailed, "NV21 Y write failed");
            }
        }

        std::vector<std::uint8_t> uvrow(static_cast<std::size_t>(dw));
        for (int dy=dr.y0; dy<dr.y1; dy+=2) {
            for (int dx=dr.x0; dx<dr.x1; dx+=2) {
                int rs=0,gs=0,bs=0;
                for (int oy=0;oy<2;++oy) for (int ox=0;ox<2;++ox) {
                    int sx=-1,sy=-1;
                    display_to_source(dx+ox,dy+oy,sourceWidth_,sourceHeight_,orientation_,sx,sy);
                    const std::size_t i=static_cast<std::size_t>(sy-r.y0)*sw + static_cast<std::size_t>(sx-r.x0);
                    float rr=std::max(rgb[3u*i],0.0f);
                    float gg=std::max(rgb[3u*i+1u],0.0f);
                    float bb=std::max(rgb[3u*i+2u],0.0f);
                    if (!std::isfinite(rr)||!std::isfinite(gg)||!std::isfinite(bb)) rr=gg=bb=0.0f;
                    if ((flags_&kFlagLight)!=0) {
                        const float lum=std::max(truthraw::luminance709(rr,gg,bb),0.0f);
                        const float darkGate=1.0f-smoothstep((lum-0.02f)/0.30f);
                        const float blackProtect=smoothstep(lum/0.025f);
                        const float strength=0.18f*std::clamp(exposure_.evidenceConfidence,0.0f,1.0f)*
                                             darkGate*blackProtect;
                        if (strength>1e-4f) { const float sc=1.0f+strength; rr*=sc; gg*=sc; bb*=sc; }
                    }
                    rs+=linear_to_srgb(rr); gs+=linear_to_srgb(gg); bs+=linear_to_srgb(bb);
                }
                const std::uint8_t R=static_cast<std::uint8_t>((rs+2)/4);
                const std::uint8_t G=static_cast<std::uint8_t>((gs+2)/4);
                const std::uint8_t B=static_cast<std::uint8_t>((bs+2)/4);
                std::uint8_t Y,U,V; rgb_to_yuv(R,G,B,Y,U,V); (void)Y;
                const std::size_t p=static_cast<std::size_t>(dx-dr.x0);
                uvrow[p]=V; uvrow[p+1u]=U;
            }
            const std::uint64_t yBytes=static_cast<std::uint64_t>(displayWidth_)*displayHeight_;
            const std::uint64_t off=yBytes + static_cast<std::uint64_t>(dy/2)*displayWidth_ + dr.x0;
            if (!pwrite_all(fd_,off,uvrow.data(),uvrow.size())) {
                return StreamStatus::error(StreamStatusCode::SinkFailed, "NV21 VU write failed");
            }
        }

        writtenPixels_ += pixels;
        return StreamStatus::ok();
    }

    StreamStatus writeHalfLogGainBlock(const HalfStateRect&, const float* gain, std::size_t count) override {
        if (!begun_ || finished_ || gain==nullptr) {
            return StreamStatus::error(StreamStatusCode::SinkFailed, "invalid HDR gain block");
        }
        if (hdrPipelineEnabled_) {
            for (std::size_t i=0;i<count;++i) {
                if (!std::isfinite(gain[i])) return StreamStatus::error(StreamStatusCode::SinkFailed, "non-finite HDR gain");
                if (gain[i] > 1e-6f) ++hdrPositiveGainSamples_;
            }
        }
        return StreamStatus::ok();
    }

    StreamStatus writeStage2DiagnosticTile(const TileRect&, const float*, std::size_t) override {
        return StreamStatus::error(StreamStatusCode::UnsupportedExecution, "diagnostics disabled for photo export");
    }

    StreamStatus finishFrame() override {
        const std::uint64_t expected=static_cast<std::uint64_t>(sourceWidth_)*sourceHeight_;
        if (!begun_ || finished_ || writtenPixels_!=expected || ::fsync(fd_)!=0) {
            return StreamStatus::error(StreamStatusCode::SinkFailed, "full-res NV21 frame incomplete");
        }
        finished_=true;
        return StreamStatus::ok();
    }

    int width() const noexcept { return displayWidth_; }
    int height() const noexcept { return displayHeight_; }
    std::uint64_t outputBytes() const noexcept { return outputBytes_; }
    std::uint64_t lightAdjustedPixels() const noexcept { return lightAdjustedPixels_; }
    std::uint64_t hdrPositiveGainSamples() const noexcept { return hdrPositiveGainSamples_; }

private:
    int fd_=-1;
    jint flags_=0;
    int sourceWidth_=0, sourceHeight_=0, displayWidth_=0, displayHeight_=0;
    truthraw::Orientation orientation_=truthraw::Orientation::Normal;
    truthraw::ExposurePlan exposure_{};
    bool hdrPipelineEnabled_=false, begun_=false, finished_=false;
    std::uint64_t outputBytes_=0, writtenPixels_=0, lightAdjustedPixels_=0, hdrPositiveGainSamples_=0;
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
    jint maxSourceResidentBytes, jint maxLogicalResidentBytes) {
    if (sourceFd<0 || outputFd<0 || maxSourceResidentBytes<=0 || maxLogicalResidentBytes<=0 ||
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

    FullResNv21Sink sink(static_cast<int>(outputFd),flags);
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
    v[16]=0; // HDR not baked into compatibility JPEG front.
    v[17]=0; // Restoration not baked into compatibility JPEG front.
    v[18]=stream.provenance.physicalFrameCount;
    v[19]=stream.provenance.independentEvidenceCount;
    auto out=env->NewLongArray(static_cast<jsize>(v.size()));
    if(out) env->SetLongArrayRegion(out,0,static_cast<jsize>(v.size()),v.data());
    return out;
}
