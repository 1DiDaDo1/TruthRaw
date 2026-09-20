#include <jni.h>

#include "dng_color_binding_producer_v0_2.h"
#include "full_frame_streaming_v0_1.h"
#include "open_world_native_v03.h"
#include "open_scene_canonical_v0_70.h"
#include "open_scene_channel_authority_v0_78.h"
#include "bound_uncertainty_admission_v0_79.h"
#include "adaptive_detail_v47j_adapter.h"
#include "output_acutance_v0_81.h"
#include "illumination_state_v0_82.h"
#include "truthraw_sha256_v0_69.h"
#include "raw_source_adapter_bridge_common.h"
#include "scientific_master_streaming_binding_v0_2.h"
#include "scientific_preview_source_binding_v0_1.h"
#include "scientific_preview_source_binding_v0_2.h"
#include "technical_backplane_phase2_v0_1.h"
#include "technical_backplane_v0_1.h"
#include "tile_native_dng_source_v0_1.h"
#include "truthraw/core.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace {

using truthraw::NeutralReferenceAppearance;
using truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction;
using truthraw::TileRect;
using truthraw::dng_color_binding_producer_v0_2::ProducerResult;
using truthraw::scientific_preview_binding_v0_1::ColorClaimScope;
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

namespace canonical_scene = truthraw::open_scene_canonical::v0_70;
namespace channel_authority = truthraw::open_scene_channel_authority::v0_78;
namespace uncertainty_admission = truthraw::bound_uncertainty_admission::v0_79;
namespace adaptive_detail = truthraw::adaptive_detail_v47j_adapter;
namespace output_acutance = truthraw::output_acutance_v0_81;
namespace illumination_state = truthraw::illumination_state::v0_82;
namespace sha = truthraw::sha256_v0_69;

constexpr jint kMagic = 0x54524144; // TRAD
constexpr std::size_t kHeaderInts = 128u;
constexpr int kAbsoluteMaxPreviewEdge = 512;
constexpr int kTileCore = 128;
constexpr int kTileHalo = 16;
constexpr jint kFlagLight = 1 << 0;
constexpr jint kFlagHdr = 1 << 1;
constexpr jint kFlagDetail = 1 << 2;
constexpr jint kFlagRestoration = 1 << 3;
constexpr jint kAllowedFlags = kFlagLight | kFlagHdr | kFlagDetail | kFlagRestoration;

struct IntRect {
    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;
};

jint clamp_metric(std::uint64_t value) {
    const auto cap = static_cast<std::uint64_t>(std::numeric_limits<jint>::max());
    return static_cast<jint>(std::min(value, cap));
}

jint digest_word_le(const canonical_scene::Digest& digest, std::size_t word) noexcept {
    const std::size_t i = word * 4u;
    const std::uint32_t value =
        static_cast<std::uint32_t>(digest[i]) |
        (static_cast<std::uint32_t>(digest[i + 1u]) << 8u) |
        (static_cast<std::uint32_t>(digest[i + 2u]) << 16u) |
        (static_cast<std::uint32_t>(digest[i + 3u]) << 24u);
    return static_cast<jint>(value);
}

std::string hex_sha256(const std::array<std::uint8_t, 32>& bytes) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out(64u, '0');
    for (std::size_t i = 0u; i < bytes.size(); ++i) {
        out[2u * i] = kHex[(bytes[i] >> 4u) & 0x0fu];
        out[2u * i + 1u] = kHex[bytes[i] & 0x0fu];
    }
    return out;
}

canonical_scene::Digest build_adaptive_detail_binding(
    const canonical_scene::Digest& openSceneSha256,
    const canonical_scene::Digest& channelAuthoritySha256,
    const canonical_scene::Digest& uncertaintyAdmissionSha256,
    float noiseSigmaAt2Pct) {
    sha::Hasher h;
    constexpr char kDomain[] =
        "TruthRawAdaptiveDetailBinding/0.80\n"
        "canonical_algorithm=canonical/detail/v4.7j\n"
        "backend=adaptive_detailed_crisp_multiband_hard_edge_guard_v47j\n"
        "role=APPEARANCE_DETAIL_COMPENSATION_ONLY\n"
        "scientific_master_modified=0\n"
        "authority_modified=0\n"
        "creates_optical_or_sensor_evidence=0\n"
        "color_policy=luminance_only_rgb_direction_preserved_no_semantic_segmentation\n"
        "required_halo=5\n";
    h.update(
        reinterpret_cast<const std::uint8_t*>(kDomain),
        sizeof(kDomain) - 1u);
    h.update(openSceneSha256);
    h.update(channelAuthoritySha256);
    h.update(uncertaintyAdmissionSha256);
    const std::uint32_t bits =
        std::bit_cast<std::uint32_t>(noiseSigmaAt2Pct);
    const std::array<std::uint8_t, 4> little{
        static_cast<std::uint8_t>(bits),
        static_cast<std::uint8_t>(bits >> 8u),
        static_cast<std::uint8_t>(bits >> 16u),
        static_cast<std::uint8_t>(bits >> 24u),
    };
    h.update(little);
    return h.finalize();
}

canonical_scene::Digest build_output_acutance_binding(
    const canonical_scene::Digest& openSceneSha256,
    const canonical_scene::Digest& channelAuthoritySha256,
    const canonical_scene::Digest& uncertaintyAdmissionSha256,
    const canonical_scene::Digest& adaptiveDetailBindingSha256,
    const output_acutance::Result& result,
    int width,
    int height) {
    sha::Hasher h;
    constexpr char kDomain[] =
        "TruthRawOutputAcutanceBinding/0.81\n"
        "canonical_algorithm=canonical/output-acutance/v4.7k\n"
        "role=POST_FINAL_RESIZE_OUTPUT_ACUTANCE_ONLY\n"
        "order=FINAL_RESIZE_THEN_ACUTANCE_THEN_HDR_REBASE_THEN_OETF\n"
        "scientific_master_modified=0\n"
        "authority_modified=0\n"
        "creates_optical_or_sensor_evidence=0\n"
        "zero_upstream_hdr_gain_stays_unity=1\n"
        "censored_hdr_gain_stays_unity=1\n";
    h.update(
        reinterpret_cast<const std::uint8_t*>(kDomain),
        sizeof(kDomain) - 1u);
    h.update(openSceneSha256);
    h.update(channelAuthoritySha256);
    h.update(uncertaintyAdmissionSha256);
    h.update(adaptiveDetailBindingSha256);

    const auto put_u32 = [&](std::uint32_t value) {
        const std::array<std::uint8_t, 4> little{
            static_cast<std::uint8_t>(value),
            static_cast<std::uint8_t>(value >> 8u),
            static_cast<std::uint8_t>(value >> 16u),
            static_cast<std::uint8_t>(value >> 24u),
        };
        h.update(little);
    };
    const auto put_u64 = [&](std::uint64_t value) {
        std::array<std::uint8_t, 8> little{};
        for (std::size_t i = 0u; i < little.size(); ++i) {
            little[i] = static_cast<std::uint8_t>(value >> (8u * i));
        }
        h.update(little);
    };
    const auto put_f32 = [&](float value) {
        put_u32(std::bit_cast<std::uint32_t>(value));
    };

    put_u32(static_cast<std::uint32_t>(result.profile));
    put_u32(result.applied ? 1u : 0u);
    put_u32(result.hdrRebased ? 1u : 0u);
    put_u32(static_cast<std::uint32_t>(width));
    put_u32(static_cast<std::uint32_t>(height));
    put_f32(result.plan.noiseSigmaAt2Pct);
    put_f32(result.plan.resizeRatio);
    put_f32(result.plan.resizeNeed);
    put_f32(result.plan.strength);
    put_f32(result.plan.deltaCap);
    put_u64(result.changedPixels);
    put_u64(result.hdrRebasedPixels);
    put_f32(result.maxEffectiveHdrTargetAbsError);
    return h.finalize();
}


jintArray status_packet(JNIEnv* env, jint status) {
    std::array<jint, kHeaderInts> header{};
    header[0] = kMagic;
    header[1] = status;
    auto out = env->NewIntArray(static_cast<jsize>(header.size()));
    if (out != nullptr) {
        env->SetIntArrayRegion(out, 0, static_cast<jsize>(header.size()), header.data());
    }
    return out;
}

jint binding_status(const truthraw::scientific_preview_binding_v0_1::BindingStatus& status) {
    return 2000 + static_cast<jint>(status.code);
}

jint producer_status(const truthraw::dng_color_binding_producer_v0_2::ProducerStatus& status) {
    return 2100 + static_cast<jint>(status.code);
}

jint adapter_status(const truthraw::multivendor_raw_source_adapter::v0_1::AdapterStatus& status) {
    return 7000 + static_cast<jint>(status.code);
}

jint science_status(const truthraw::scientific_master_streaming_binding::v0_2::Status& status) {
    return 8000 + static_cast<jint>(status.code);
}

jint phase2_status(const truthraw::technical_backplane_phase2::v0_1::Status& status) {
    return 9000 + static_cast<jint>(status.code);
}

jint stream_status(const truthraw::streaming_v0_1::StreamStatus& status) {
    return 4000 + static_cast<jint>(status.code);
}

int sample_axis(int p, int previewSize, int displaySize) {
    const double v = (static_cast<double>(p) + 0.5) *
        static_cast<double>(displaySize) / static_cast<double>(previewSize);
    return std::clamp(static_cast<int>(v), 0, displaySize - 1);
}

void preview_axis_bounds(int d0, int d1, int displaySize, int previewSize, int& p0, int& p1) {
    const double scale = static_cast<double>(previewSize) / static_cast<double>(displaySize);
    p0 = std::max(0, static_cast<int>(std::floor(static_cast<double>(d0) * scale)) - 2);
    p1 = std::min(previewSize, static_cast<int>(std::ceil(static_cast<double>(d1) * scale)) + 2);
}

IntRect display_rect_for_source(const TileRect& r, int width, int height, truthraw::Orientation o) {
    switch (o) {
        case truthraw::Orientation::Normal:
            return {r.x0, r.y0, r.x1, r.y1};
        case truthraw::Orientation::Rotate180:
            return {width - r.x1, height - r.y1, width - r.x0, height - r.y0};
        case truthraw::Orientation::Rotate90CW:
            return {height - r.y1, r.x0, height - r.y0, r.x1};
        case truthraw::Orientation::Rotate90CCW:
            return {r.y0, width - r.x1, r.y1, width - r.x0};
    }
    return {};
}

void display_to_source(int dx, int dy, int width, int height, truthraw::Orientation o, int& sx, int& sy) {
    switch (o) {
        case truthraw::Orientation::Normal:
            sx = dx; sy = dy; return;
        case truthraw::Orientation::Rotate180:
            sx = width - 1 - dx; sy = height - 1 - dy; return;
        case truthraw::Orientation::Rotate90CW:
            sx = dy; sy = height - 1 - dx; return;
        case truthraw::Orientation::Rotate90CCW:
            sx = width - 1 - dy; sy = dx; return;
    }
    sx = -1; sy = -1;
}

bool valid_orientation(truthraw::Orientation o) {
    return o == truthraw::Orientation::Normal ||
           o == truthraw::Orientation::Rotate180 ||
           o == truthraw::Orientation::Rotate90CW ||
           o == truthraw::Orientation::Rotate90CCW;
}

float smoothstep(float x) {
    x = std::clamp(x, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

std::uint8_t linear_to_srgb_u8(float linear) {
    if (!std::isfinite(linear)) return 0u;
    const float x = std::clamp(linear, 0.0f, 1.0f);
    const float encoded = x <= 0.0031308f
        ? 12.92f * x
        : 1.055f * std::pow(x, 1.0f / 2.4f) - 0.055f;
    return static_cast<std::uint8_t>(std::clamp<long>(
        std::lround(std::clamp(encoded, 0.0f, 1.0f) * 255.0f), 0, 255));
}

class AdvancedPreviewSink final : public IStreamingSink {
public:
    AdvancedPreviewSink(int maxEdge, jint flags, float noiseSigmaAt2Pct)
        : maxEdge_(maxEdge),
          flags_(flags),
          outputNoiseSigmaAt2Pct_(noiseSigmaAt2Pct),
          outputProfile_(
              (flags & kFlagDetail) != 0
                  ? truthraw_v47k::OutputProfile::AdaptiveDetail
                  : truthraw_v47k::OutputProfile::Neutral) {}

    std::size_t residentBytesUpperBound() const override {
        if (maxEdge_ <= 0) return 0u;
        const std::size_t pixels =
            static_cast<std::size_t>(maxEdge_) *
            static_cast<std::size_t>(maxEdge_);
        // Deterministic upper bound known before beginFrame. v0.81 accounts for
        // the final-resize SDR base, canonical acutance output, HDR-rebased gain,
        // masks/ownership and ARGB presentation surface.
        return
            (3u * pixels) * sizeof(float) +  // linear_
            pixels * sizeof(float) +         // halfLogGain_
            (3u * pixels) * sizeof(float) +  // preAcutanceBase_
            (3u * pixels) * sizeof(float) +  // acutanceBase_
            pixels * sizeof(float) +         // rebasedDisplayGain_
            pixels * sizeof(std::uint8_t) +  // owners_
            pixels * sizeof(std::uint8_t) +  // gainOwners_
            pixels * sizeof(std::uint8_t) +  // censorMask_
            pixels * sizeof(std::uint32_t);  // argb_
    }

    StreamStatus beginFrame(
        int width,
        int height,
        truthraw::Orientation orientation,
        const truthraw::ExposurePlan& exposure,
        bool hdrEnabled,
        bool diagnosticsEnabled) override {
        if (begun_ || width <= 0 || height <= 0 || maxEdge_ < 32 ||
            maxEdge_ > kAbsoluteMaxPreviewEdge || !valid_orientation(orientation) ||
            !std::isfinite(outputNoiseSigmaAt2Pct_) ||
            outputNoiseSigmaAt2Pct_ < 0.0f) {
            return StreamStatus::error(StreamStatusCode::InvalidArgument,
                                       "invalid Advanced preview begin-frame contract");
        }
        sourceWidth_ = width;
        sourceHeight_ = height;
        orientation_ = orientation;
        exposure_ = exposure;
        hdrPipelineEnabled_ = hdrEnabled;
        diagnosticsEnabled_ = diagnosticsEnabled;
        const bool rotated = orientation == truthraw::Orientation::Rotate90CW ||
                             orientation == truthraw::Orientation::Rotate90CCW;
        displayWidth_ = rotated ? height : width;
        displayHeight_ = rotated ? width : height;
        const int longEdge = std::max(displayWidth_, displayHeight_);
        const double scale = std::min(
            1.0, static_cast<double>(maxEdge_) / static_cast<double>(longEdge));
        previewWidth_ = std::max(
            1, static_cast<int>(std::lround(static_cast<double>(displayWidth_) * scale)));
        previewHeight_ = std::max(
            1, static_cast<int>(std::lround(static_cast<double>(displayHeight_) * scale)));
        const std::size_t pixels =
            static_cast<std::size_t>(previewWidth_) * static_cast<std::size_t>(previewHeight_);
        if (pixels == 0u ||
            pixels > static_cast<std::size_t>(kAbsoluteMaxPreviewEdge) *
                     static_cast<std::size_t>(kAbsoluteMaxPreviewEdge)) {
            return StreamStatus::error(StreamStatusCode::BudgetExceeded,
                                       "Advanced preview surface exceeds cap");
        }
        outputResizeRatio_ = std::max(
            1.0f,
            std::max(
                static_cast<float>(displayWidth_) / static_cast<float>(previewWidth_),
                static_cast<float>(displayHeight_) / static_cast<float>(previewHeight_)));

        linear_.assign(3u * pixels, 0.0f);
        halfLogGain_.assign(pixels, 0.0f);
        preAcutanceBase_.assign(3u * pixels, 0.0f);
        acutanceBase_.assign(3u * pixels, 0.0f);
        rebasedDisplayGain_.assign(pixels, 1.0f);
        owners_.assign(pixels, 0u);
        gainOwners_.assign(pixels, 0u);
        argb_.assign(pixels, 0xff000000u);
        censorMask_.assign(pixels, 0u);
        outputAcutanceResult_ = output_acutance::Result{};
        begun_ = true;
        return StreamStatus::ok();
    }

    StreamStatus writeSdrTile(
        const TileRect& r,
        const float* rgb,
        std::size_t floatCount) override {
        if (!begun_ || finished_ || rgb == nullptr || r.x0 < 0 || r.y0 < 0 ||
            r.x1 > sourceWidth_ || r.y1 > sourceHeight_ || r.x0 >= r.x1 || r.y0 >= r.y1) {
            return StreamStatus::error(StreamStatusCode::SinkFailed,
                                       "invalid SDR tile for Advanced preview");
        }
        const int coreW = r.x1 - r.x0;
        const int coreH = r.y1 - r.y0;
        const std::size_t corePixels =
            static_cast<std::size_t>(coreW) * static_cast<std::size_t>(coreH);
        if (floatCount != 3u * corePixels) {
            return StreamStatus::error(StreamStatusCode::SinkFailed,
                                       "Advanced SDR tile count mismatch");
        }

        const IntRect dr =
            display_rect_for_source(r, sourceWidth_, sourceHeight_, orientation_);
        int px0 = 0, px1 = 0, py0 = 0, py1 = 0;
        preview_axis_bounds(dr.x0, dr.x1, displayWidth_, previewWidth_, px0, px1);
        preview_axis_bounds(dr.y0, dr.y1, displayHeight_, previewHeight_, py0, py1);
        for (int py = py0; py < py1; ++py) {
            const int dy = sample_axis(py, previewHeight_, displayHeight_);
            if (dy < dr.y0 || dy >= dr.y1) continue;
            for (int px = px0; px < px1; ++px) {
                const int dx = sample_axis(px, previewWidth_, displayWidth_);
                if (dx < dr.x0 || dx >= dr.x1) continue;
                int sx = -1, sy = -1;
                display_to_source(
                    dx, dy, sourceWidth_, sourceHeight_, orientation_, sx, sy);
                if (sx < r.x0 || sx >= r.x1 || sy < r.y0 || sy >= r.y1) continue;
                const std::size_t outIndex =
                    static_cast<std::size_t>(py) * static_cast<std::size_t>(previewWidth_) +
                    static_cast<std::size_t>(px);
                if (owners_[outIndex] != 0u) {
                    return StreamStatus::error(StreamStatusCode::SinkFailed,
                                               "Advanced preview pixel overlap");
                }
                const std::size_t local =
                    static_cast<std::size_t>(sy - r.y0) * static_cast<std::size_t>(coreW) +
                    static_cast<std::size_t>(sx - r.x0);
                linear_[3u * outIndex] = rgb[3u * local];
                linear_[3u * outIndex + 1u] = rgb[3u * local + 1u];
                linear_[3u * outIndex + 2u] = rgb[3u * local + 2u];
                owners_[outIndex] = 1u;
                ++writtenPixels_;
            }
        }
        return StreamStatus::ok();
    }

    StreamStatus writeHalfLogGainBlock(
        const HalfStateRect& rect,
        const float* halfLogGain,
        std::size_t count) override {
        const int qw = rect.x1 - rect.x0;
        const int qh = rect.y1 - rect.y0;
        if (!begun_ || finished_ || halfLogGain == nullptr || qw <= 0 || qh <= 0 ||
            count != static_cast<std::size_t>(qw) * static_cast<std::size_t>(qh)) {
            return StreamStatus::error(StreamStatusCode::SinkFailed,
                                       "invalid half-gain block for Advanced preview");
        }

        TileRect sourceRect{};
        sourceRect.x0 = std::max(0, 2 * rect.x0);
        sourceRect.y0 = std::max(0, 2 * rect.y0);
        sourceRect.x1 = std::min(sourceWidth_, 2 * rect.x1);
        sourceRect.y1 = std::min(sourceHeight_, 2 * rect.y1);
        const IntRect dr =
            display_rect_for_source(sourceRect, sourceWidth_, sourceHeight_, orientation_);
        int px0 = 0, px1 = 0, py0 = 0, py1 = 0;
        preview_axis_bounds(dr.x0, dr.x1, displayWidth_, previewWidth_, px0, px1);
        preview_axis_bounds(dr.y0, dr.y1, displayHeight_, previewHeight_, py0, py1);

        for (int py = py0; py < py1; ++py) {
            const int dy = sample_axis(py, previewHeight_, displayHeight_);
            if (dy < dr.y0 || dy >= dr.y1) continue;
            for (int px = px0; px < px1; ++px) {
                const int dx = sample_axis(px, previewWidth_, displayWidth_);
                if (dx < dr.x0 || dx >= dr.x1) continue;
                int sx = -1, sy = -1;
                display_to_source(
                    dx, dy, sourceWidth_, sourceHeight_, orientation_, sx, sy);
                const int qx = sx / 2;
                const int qy = sy / 2;
                if (qx < rect.x0 || qx >= rect.x1 || qy < rect.y0 || qy >= rect.y1) {
                    continue;
                }
                const std::size_t outIndex =
                    static_cast<std::size_t>(py) * static_cast<std::size_t>(previewWidth_) +
                    static_cast<std::size_t>(px);
                const std::size_t local =
                    static_cast<std::size_t>(qy - rect.y0) * static_cast<std::size_t>(qw) +
                    static_cast<std::size_t>(qx - rect.x0);
                if (gainOwners_[outIndex] != 0u) {
                    return StreamStatus::error(StreamStatusCode::SinkFailed,
                                               "Advanced gain ownership overlap");
                }
                halfLogGain_[outIndex] = halfLogGain[local];
                gainOwners_[outIndex] = 1u;
            }
        }
        return StreamStatus::ok();
    }

    StreamStatus writeStage2DiagnosticTile(
        const TileRect&,
        const float*,
        std::size_t) override {
        if (!begun_ || finished_ || !diagnosticsEnabled_) {
            return StreamStatus::error(StreamStatusCode::SinkFailed,
                                       "unexpected Advanced diagnostic tile");
        }
        return StreamStatus::ok();
    }

    StreamStatus finishFrame() override {
        if (!begun_ || finished_ || writtenPixels_ != owners_.size()) {
            return StreamStatus::error(StreamStatusCode::SinkFailed,
                                       "Advanced preview surface incomplete");
        }
        for (const auto owner : owners_) {
            if (owner != 1u) {
                return StreamStatus::error(StreamStatusCode::SinkFailed,
                                           "Advanced preview ownership incomplete");
            }
        }
        if ((flags_ & kFlagHdr) != 0 && hdrPipelineEnabled_) {
            for (const auto owner : gainOwners_) {
                if (owner != 1u) {
                    return StreamStatus::error(StreamStatusCode::SinkFailed,
                                               "Advanced HDR gain coverage incomplete");
                }
            }
        }
        if (!render()) {
            return StreamStatus::error(
                StreamStatusCode::SinkFailed,
                "Advanced v0.81 output acutance/HDR rebase failed");
        }
        finished_ = true;
        return StreamStatus::ok();
    }

    bool sourceCoordinateForPreview(int px, int py, int& sx, int& sy) const {
        if (!begun_ || px < 0 || py < 0 || px >= previewWidth_ || py >= previewHeight_) {
            return false;
        }
        const int dx = sample_axis(px, previewWidth_, displayWidth_);
        const int dy = sample_axis(py, previewHeight_, displayHeight_);
        display_to_source(dx, dy, sourceWidth_, sourceHeight_, orientation_, sx, sy);
        return sx >= 0 && sy >= 0 && sx < sourceWidth_ && sy < sourceHeight_;
    }

    bool applyCensorMask(const std::vector<std::uint8_t>& mask) {
        if (!finished_ || mask.size() != owners_.size()) return false;
        censorMask_ = mask;
        censoredPreviewPixels_ = 0u;
        restoredPixels_ = 0u;
        for (const auto value : mask) if (value != 0u) ++censoredPreviewPixels_;
        if ((flags_ & kFlagRestoration) == 0 || censoredPreviewPixels_ == 0u) {
            return render();
        }

        const auto original = linear_;
        for (int py = 0; py < previewHeight_; ++py) {
            for (int px = 0; px < previewWidth_; ++px) {
                const std::size_t index =
                    static_cast<std::size_t>(py) * static_cast<std::size_t>(previewWidth_) +
                    static_cast<std::size_t>(px);
                if (mask[index] == 0u) continue;

                double sumR = 0.0, sumG = 0.0, sumB = 0.0, sumW = 0.0;
                int support = 0;
                for (int radius = 1; radius <= 2 && support < 3; ++radius) {
                    for (int dy = -radius; dy <= radius; ++dy) {
                        for (int dx = -radius; dx <= radius; ++dx) {
                            if (dx == 0 && dy == 0) continue;
                            if (std::max(std::abs(dx), std::abs(dy)) != radius) continue;
                            const int xx = px + dx;
                            const int yy = py + dy;
                            if (xx < 0 || yy < 0 || xx >= previewWidth_ || yy >= previewHeight_) {
                                continue;
                            }
                            const std::size_t ni =
                                static_cast<std::size_t>(yy) * static_cast<std::size_t>(previewWidth_) +
                                static_cast<std::size_t>(xx);
                            if (mask[ni] != 0u) continue;
                            const float rr = original[3u * ni];
                            const float gg = original[3u * ni + 1u];
                            const float bb = original[3u * ni + 2u];
                            if (!std::isfinite(rr) || !std::isfinite(gg) || !std::isfinite(bb)) continue;
                            const double weight = 1.0 / std::sqrt(static_cast<double>(dx * dx + dy * dy));
                            sumR += weight * static_cast<double>(rr);
                            sumG += weight * static_cast<double>(gg);
                            sumB += weight * static_cast<double>(bb);
                            sumW += weight;
                            ++support;
                        }
                    }
                }
                if (support >= 3 && sumW > 0.0) {
                    linear_[3u * index] = static_cast<float>(sumR / sumW);
                    linear_[3u * index + 1u] = static_cast<float>(sumG / sumW);
                    linear_[3u * index + 2u] = static_cast<float>(sumB / sumW);
                    ++restoredPixels_;
                }
            }
        }
        return render();
    }

    int width() const { return previewWidth_; }
    int height() const { return previewHeight_; }
    bool finished() const { return finished_; }
    std::size_t writtenPixels() const { return writtenPixels_; }
    std::uint64_t restoredPixels() const { return restoredPixels_; }
    std::uint64_t censoredPreviewPixels() const { return censoredPreviewPixels_; }
    std::uint64_t hdrGainPixels() const { return hdrGainPixels_; }
    std::uint64_t lightAdjustedPixels() const { return lightAdjustedPixels_; }
    const output_acutance::Result& outputAcutanceResult() const {
        return outputAcutanceResult_;
    }
    float outputResizeRatio() const { return outputResizeRatio_; }
    const std::vector<std::uint32_t>& argb8888() const { return argb_; }

private:
    bool render() {
        if (!begun_ ||
            owners_.empty() ||
            linear_.size() != 3u * owners_.size() ||
            halfLogGain_.size() != owners_.size() ||
            censorMask_.size() != owners_.size()) {
            return false;
        }

        hdrGainPixels_ = 0u;
        lightAdjustedPixels_ = 0u;

        // Build the actual final-resize SDR base first. Existing Light is an
        // APPEARANCE_ONLY adjustment and therefore belongs to the presentation
        // base before output acutance; it remains suppressed on censored support.
        preAcutanceBase_.resize(linear_.size());
        for (std::size_t i = 0u; i < owners_.size(); ++i) {
            float r = std::max(linear_[3u * i], 0.0f);
            float g = std::max(linear_[3u * i + 1u], 0.0f);
            float b = std::max(linear_[3u * i + 2u], 0.0f);
            if (!std::isfinite(r) || !std::isfinite(g) || !std::isfinite(b)) {
                r = g = b = 0.0f;
            }

            const bool censored = censorMask_[i] != 0u;
            if ((flags_ & kFlagLight) != 0 && !censored) {
                const float y = std::max(truthraw::luminance709(r, g, b), 0.0f);
                const float darkGate = 1.0f - smoothstep((y - 0.02f) / 0.30f);
                const float blackProtect = smoothstep(y / 0.025f);
                const float strength =
                    0.18f * std::clamp(exposure_.evidenceConfidence, 0.0f, 1.0f) *
                    darkGate * blackProtect;
                if (strength > 1e-4f) {
                    const float scale = 1.0f + strength;
                    r *= scale;
                    g *= scale;
                    b *= scale;
                    ++lightAdjustedPixels_;
                }
            }

            preAcutanceBase_[3u * i] = r;
            preAcutanceBase_[3u * i + 1u] = g;
            preAcutanceBase_[3u * i + 2u] = b;
        }

        const bool hdrEnabled =
            (flags_ & kFlagHdr) != 0 && hdrPipelineEnabled_;
        if (!output_acutance::apply_final_resize_acutance_and_rebase_hdr(
                preAcutanceBase_,
                previewWidth_,
                previewHeight_,
                outputNoiseSigmaAt2Pct_,
                outputResizeRatio_,
                outputProfile_,
                hdrEnabled,
                halfLogGain_,
                censorMask_,
                acutanceBase_,
                rebasedDisplayGain_,
                outputAcutanceResult_)) {
            return false;
        }

        if (!outputAcutanceResult_.applied ||
            outputAcutanceResult_.profile != outputProfile_ ||
            outputAcutanceResult_.hdrRebased != hdrEnabled ||
            !std::isfinite(outputAcutanceResult_.plan.strength) ||
            !std::isfinite(outputAcutanceResult_.plan.deltaCap)) {
            return false;
        }

        hdrGainPixels_ = outputAcutanceResult_.hdrRebasedPixels;

        for (std::size_t i = 0u; i < owners_.size(); ++i) {
            float r = acutanceBase_[3u * i];
            float g = acutanceBase_[3u * i + 1u];
            float b = acutanceBase_[3u * i + 2u];
            const bool censored = censorMask_[i] != 0u;

            // HDR is now expressed against the final acutance-adjusted SDR base.
            // The rebase module guarantees: no new gain where upstream gain was
            // unity, and no positive gain on censored support.
            if (hdrEnabled && !censored) {
                const float gain = rebasedDisplayGain_[i];
                if (!std::isfinite(gain) || gain < 1.0f) return false;
                if (gain > 1.0f + 1e-5f) {
                    r *= gain;
                    g *= gain;
                    b *= gain;
                }
            } else if (rebasedDisplayGain_[i] != 1.0f) {
                return false;
            }

            const float mx = std::max(r, std::max(g, b));
            if (mx > 0.92f) {
                const float shoulder =
                    0.92f + 0.08f * (1.0f - std::exp(-3.0f * (mx - 0.92f)));
                const float scale = shoulder / std::max(mx, 1e-8f);
                r *= scale;
                g *= scale;
                b *= scale;
            }

            const std::uint32_t rr = linear_to_srgb_u8(r);
            const std::uint32_t gg = linear_to_srgb_u8(g);
            const std::uint32_t bb = linear_to_srgb_u8(b);
            argb_[i] = 0xff000000u | (rr << 16u) | (gg << 8u) | bb;
        }
        return true;
    }

    int maxEdge_ = 0;
    jint flags_ = 0;
    int sourceWidth_ = 0;
    int sourceHeight_ = 0;
    int displayWidth_ = 0;
    int displayHeight_ = 0;
    int previewWidth_ = 0;
    int previewHeight_ = 0;
    truthraw::Orientation orientation_ = truthraw::Orientation::Normal;
    truthraw::ExposurePlan exposure_{};
    float outputNoiseSigmaAt2Pct_ = 0.0f;
    float outputResizeRatio_ = 1.0f;
    truthraw_v47k::OutputProfile outputProfile_ =
        truthraw_v47k::OutputProfile::Neutral;
    output_acutance::Result outputAcutanceResult_{};
    bool hdrPipelineEnabled_ = false;
    bool diagnosticsEnabled_ = false;
    bool begun_ = false;
    bool finished_ = false;
    std::size_t writtenPixels_ = 0u;
    std::uint64_t restoredPixels_ = 0u;
    std::uint64_t censoredPreviewPixels_ = 0u;
    std::uint64_t hdrGainPixels_ = 0u;
    std::uint64_t lightAdjustedPixels_ = 0u;
    std::vector<float> linear_;
    std::vector<float> halfLogGain_;
    std::vector<float> preAcutanceBase_;
    std::vector<float> acutanceBase_;
    std::vector<float> rebasedDisplayGain_;
    std::vector<std::uint8_t> owners_;
    std::vector<std::uint8_t> gainOwners_;
    std::vector<std::uint8_t> censorMask_;
    std::vector<std::uint32_t> argb_;
};

StreamingOptions advanced_options(std::size_t memoryBudgetBytes, jint flags) {
    StreamingOptions options;
    options.tile = {kTileCore, kTileHalo};
    options.workers = 1;
    options.hdrEnabled = (flags & kFlagHdr) != 0;
    options.streamScientificDiagnostics = false;
    options.sdrLutSize = 4096;
    options.memoryBudgetBytes = memoryBudgetBytes;
    return options;
}

StreamStatus build_censor_mask(
    truthraw::streaming_v0_1::IRawTileSource& source,
    const AdvancedPreviewSink& sink,
    std::vector<std::uint8_t>& mask) {
    const auto& meta = source.metadata();
    const int pw = sink.width();
    const int ph = sink.height();
    const std::size_t pixels =
        static_cast<std::size_t>(pw) * static_cast<std::size_t>(ph);
    mask.assign(pixels, 0u);

    std::vector<std::vector<std::pair<int, std::size_t>>> rows(
        static_cast<std::size_t>(meta.height));
    for (int py = 0; py < ph; ++py) {
        for (int px = 0; px < pw; ++px) {
            int sx = -1, sy = -1;
            if (!sink.sourceCoordinateForPreview(px, py, sx, sy)) {
                return StreamStatus::error(StreamStatusCode::SinkFailed,
                                           "Advanced censor-map coordinate failed");
            }
            const std::size_t index =
                static_cast<std::size_t>(py) * static_cast<std::size_t>(pw) +
                static_cast<std::size_t>(px);
            rows[static_cast<std::size_t>(sy)].push_back({sx, index});
        }
    }

    std::vector<std::uint16_t> raw;
    std::vector<float> gain;
    for (int sy = 0; sy < meta.height; ++sy) {
        auto& points = rows[static_cast<std::size_t>(sy)];
        if (points.empty()) continue;
        int minX = meta.width - 1;
        int maxX = 0;
        for (const auto& point : points) {
            minX = std::min(minX, point.first);
            maxX = std::max(maxX, point.first);
        }
        const std::size_t count = static_cast<std::size_t>(maxX - minX + 1);
        raw.resize(count);
        if (meta.hasGainField) gain.resize(count); else gain.clear();
        TileRect rect{minX, sy, maxX + 1, sy + 1, minX, sy, maxX + 1, sy + 1};
        const auto status = source.readRawTile(
            rect,
            raw.data(),
            raw.size(),
            meta.hasGainField ? gain.data() : nullptr,
            meta.hasGainField ? gain.size() : 0u);
        if (!status) return status;
        for (const auto& point : points) {
            const auto sample = raw[static_cast<std::size_t>(point.first - minX)];
            if (static_cast<float>(sample) >= meta.whiteLevel) {
                mask[point.second] = 1u;
            }
        }
    }
    return StreamStatus::ok();
}

} // namespace

extern "C" JNIEXPORT jintArray JNICALL
Java_com_truthraw_adaptiveui_NativeTilePreviewBridge_buildAdvancedDerivativePreview(
    JNIEnv* env,
    jobject,
    jint fd,
    jint requestedMaxEdge,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes,
    jint flags,
    jint sourceRouteCode) {
    if (fd < 0 || requestedMaxEdge < 32 ||
        maxSourceResidentBytes <= 0 || maxLogicalResidentBytes <= 0 ||
        (flags & ~kAllowedFlags) != 0 ||
        (sourceRouteCode != 0 && sourceRouteCode != 1)) {
        return status_packet(env, -1);
    }

    const int maxEdge = std::min(static_cast<int>(requestedMaxEdge), kAbsoluteMaxPreviewEdge);
    auto bytes = std::make_shared<PosixFdByteSource>(static_cast<int>(fd));

    SourceSeal sourceSeal;
    const auto sealed =
        truthraw::scientific_preview_binding_v0_1::seal_source_sha256(*bytes, sourceSeal);
    if (!sealed) return status_packet(env, binding_status(sealed));

    ProducerResult produced;
    const auto colorStatus =
        truthraw::dng_color_binding_producer_v0_2::produce_source_metadata_color_binding(
            *bytes, sourceSeal, produced);
    if (!colorStatus) return status_packet(env, producer_status(colorStatus));

    PreparedScientificPreviewSource prepared;
    const auto preparedStatus =
        truthraw::scientific_preview_binding_v0_2::prepare_scientific_color_source(
            sourceSeal, produced.color, prepared);
    if (!preparedStatus) return status_packet(env, binding_status(preparedStatus));

    if (!prepared.mainHouseComputeAllowed ||
        !prepared.sourceBoundAppearanceReleaseAllowed ||
        prepared.scientificPreviewReleaseAllowed ||
        prepared.scientificClaimAllowed ||
        prepared.physicalFrameCount != 1u ||
        prepared.independentEvidenceCount != 1u) {
        return status_packet(env, -2);
    }

    const auto preVerified =
        truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes, sourceSeal);
    if (!preVerified) return status_packet(env, binding_status(preVerified));

    auto openOptions = prepared.tileNativeOptions;
    openOptions.maxResidentBytes = static_cast<std::size_t>(maxSourceResidentBytes);

    truthraw::android_raw_adapter_bridge::v0_1::OpenedDngSource openedSource;
    const auto opened = truthraw::android_raw_adapter_bridge::v0_1::openDngViaAdapter(
        bytes, sourceSeal, openOptions, openedSource);
    if (!opened) return status_packet(env, adapter_status(opened));
    auto& source = openedSource.source;
    auto* dngSource = openedSource.dngAuditSource;
    if (dngSource == nullptr) return status_packet(env, -3);

    auto reconstruction =
        std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();

    truthraw::scientific_master_streaming_binding::v0_2::Options scientificOptions;
    scientificOptions.memoryBudgetBytes =
        static_cast<std::size_t>(maxLogicalResidentBytes);
    truthraw::scientific_master_streaming_binding::v0_2::Result scientific;
    const auto scientificStatus =
        truthraw::scientific_master_streaming_binding::v0_2::bind_scientific_master_streaming(
            *source, *reconstruction, scientificOptions, scientific);
    if (!scientificStatus) return status_packet(env, science_status(scientificStatus));

    truthraw::technical_backplane_phase2::v0_1::Phase2Input phaseInput;
    phaseInput.prepared = prepared;
    phaseInput.scientificMasterHash = scientific.scientificMasterHash;
    phaseInput.zeroLineGauge = scientific.zeroLineGauge;
    phaseInput.sceneBinding = scientific.sceneBinding;
    phaseInput.roomStatus.fill(
        truthraw::technical_backplane::v0_1::RoomStatus::ResearchOnly);
    phaseInput.claimStatus =
        truthraw::technical_backplane::v0_1::ClaimStatus::Candidate;

    truthraw::technical_backplane_phase2::v0_1::Phase2Result phase2;
    const auto finalized =
        truthraw::technical_backplane_phase2::v0_1::finalize_phase2(
            phaseInput, phase2);
    if (!finalized) return status_packet(env, phase2_status(finalized));

    if (phase2.admission.claimScope == ColorClaimScope::None ||
        phase2.backplane.sourceEvidenceHash != sourceSeal.sha256 ||
        phase2.backplane.scientificMasterHash != scientific.scientificMasterHash ||
        phase2.backplane.forbiddenFlags != 0u ||
        scientific.physicalFrameCount != 1u ||
        scientific.independentEvidenceCount != 1u) {
        return status_packet(env, -4);
    }

    canonical_scene::Binding openSceneBinding{};
    openSceneBinding.sourceEvidenceSha256 = sourceSeal.sha256;
    openSceneBinding.scientificMasterSha256 = scientific.scientificMasterHash;
    openSceneBinding.width = static_cast<std::uint32_t>(source->metadata().width);
    openSceneBinding.height = static_cast<std::uint32_t>(source->metadata().height);
    openSceneBinding.physicalFrameCount = scientific.physicalFrameCount;
    openSceneBinding.independentEvidenceCount = scientific.independentEvidenceCount;
    openSceneBinding.colourBindingId = produced.color.bindingId;
    canonical_scene::Summary openSceneSummary{};
    if (!canonical_scene::build_from_source(*source, openSceneBinding, openSceneSummary) ||
        openSceneSummary.counterfactualPixelCount != 0u ||
        openSceneSummary.scientificWritebackPixelCount != 0u ||
        openSceneSummary.createsNewEvidence ||
        openSceneSummary.chunkingChangesScientificIdentity ||
        openSceneSummary.pixelCount !=
            static_cast<std::uint64_t>(source->metadata().width) *
            static_cast<std::uint64_t>(source->metadata().height)) {
        return status_packet(env, -10);
    }

    // v0.79 decides whether an uncertainty model is actually admissible for
    // this source domain. IMPORTED_FILE has no source-class attestation in the
    // current product path, while CAMERA_CAPTURE is explicitly a Camera-5-derived
    // processing DNG and therefore outside the historical vendor-DNG v5.0g scope.
    uncertainty_admission::Candidate uncertaintyCandidate{};
    if (sourceRouteCode == 1) {
        uncertaintyCandidate =
            uncertainty_admission::make_current_camera5_derived_blocked_candidate(
                sourceSeal.sha256,
                static_cast<std::uint32_t>(source->metadata().width),
                static_cast<std::uint32_t>(source->metadata().height),
                static_cast<std::uint32_t>(source->metadata().cfa),
                source->metadata().whiteLevel,
                reconstruction->name());
    } else {
        uncertaintyCandidate.sourceDomain =
            uncertainty_admission::SourceDomain::Unattested;
        uncertaintyCandidate.sourceEvidenceSha256 = sourceSeal.sha256;
        uncertaintyCandidate.width =
            static_cast<std::uint32_t>(source->metadata().width);
        uncertaintyCandidate.height =
            static_cast<std::uint32_t>(source->metadata().height);
        uncertaintyCandidate.cfaCode =
            static_cast<std::uint32_t>(source->metadata().cfa);
        uncertaintyCandidate.whiteLevel = source->metadata().whiteLevel;
        uncertaintyCandidate.reconstructionBackendId = reconstruction->name();
    }

    const auto uncertaintyDecision =
        uncertainty_admission::evaluate(uncertaintyCandidate);
    if (uncertaintyDecision.reconstructedAuthorityAllowed ||
        uncertaintyDecision.code == uncertainty_admission::DecisionCode::Admitted) {
        // v0.79 has no accepted F64 trace certificate and no runtime p95 field
        // generator wired to Advanced yet. Any admitted result here would be an
        // unexpected authority promotion and must fail closed.
        return status_packet(env, -12);
    }

    // v0.78 is an immutable child of the exact v0.70 Open Scene artifact.
    // The v0.79 decision above remains blocked, therefore missing channels stay
    // UNKNOWN and RECONSTRUCTED authority remains exactly zero.
    channel_authority::Binding channelBinding{};
    channelBinding.sourceEvidenceSha256 = sourceSeal.sha256;
    channelBinding.scientificMasterSha256 = scientific.scientificMasterHash;
    channelBinding.zeroLineSha256 = phase2.zeroLineHash;
    channelBinding.sceneScaleSha256 = phase2.sceneScaleHash;
    channelBinding.parentOpenSceneV070Sha256 = openSceneSummary.artifactSha256;
    channelBinding.width = static_cast<std::uint32_t>(source->metadata().width);
    channelBinding.height = static_cast<std::uint32_t>(source->metadata().height);
    channelBinding.physicalFrameCount = scientific.physicalFrameCount;
    channelBinding.independentEvidenceCount = scientific.independentEvidenceCount;
    channelBinding.reconstructionBackendId = reconstruction->name();
    channelBinding.reconstructedAuthorityAllowed = false;

    channel_authority::Summary channelSummary{};
    if (!channel_authority::build_generic_fail_closed_from_source(
            *source, channelBinding, channelSummary) ||
        channelSummary.recordCount !=
            static_cast<std::uint64_t>(source->metadata().width) *
            static_cast<std::uint64_t>(source->metadata().height) * 3u ||
        channelSummary.authorityCounts[1] != 0u ||
        channelSummary.p95KnownCount != 0u ||
        channelSummary.createsNewEvidence ||
        channelSummary.scientificWritebackAllowed ||
        channelSummary.counterfactualAuthorityPresent ||
        channelSummary.chunkingChangesScientificIdentity) {
        return status_packet(env, -11);
    }

    illumination_state::Input illuminationInput{};
    illuminationInput.sourceEvidenceSha256 = sourceSeal.sha256;
    illuminationInput.scientificMasterSha256 = scientific.scientificMasterHash;
    illuminationInput.canonicalOpenSceneSha256 = openSceneSummary.artifactSha256;
    illuminationInput.sourceEvidenceId = sourceSeal.sourceEvidenceId;
    illuminationInput.colourBindingId = produced.color.bindingId;
    illuminationInput.dualCalibrationUsed = produced.audit.dualIlluminantUsed;
    illuminationInput.profileCalibrationIlluminant1 =
        produced.audit.calibrationIlluminant1;
    illuminationInput.profileCalibrationIlluminant2 =
        produced.audit.calibrationIlluminant2;
    illuminationInput.resolvedWhiteAvailable =
        produced.audit.dualIlluminantUsed &&
        std::isfinite(produced.audit.resolvedWhiteX) &&
        std::isfinite(produced.audit.resolvedWhiteY) &&
        std::isfinite(produced.audit.resolvedWhiteTemperatureK) &&
        produced.audit.resolvedWhiteX > 0.0 &&
        produced.audit.resolvedWhiteY > 0.0 &&
        produced.audit.resolvedWhiteTemperatureK > 0.0;
    illuminationInput.resolvedWhiteX = produced.audit.resolvedWhiteX;
    illuminationInput.resolvedWhiteY = produced.audit.resolvedWhiteY;
    illuminationInput.resolvedWhiteCctK =
        produced.audit.resolvedWhiteTemperatureK;
    illuminationInput.physicalFrameCount = scientific.physicalFrameCount;
    illuminationInput.independentEvidenceCount =
        scientific.independentEvidenceCount;

    illumination_state::State illuminationState{};
    if (!illumination_state::build(illuminationInput, illuminationState) ||
        illuminationState.sceneLightKind !=
            illumination_state::SceneLightKind::Unknown ||
        illuminationState.spectrumAuthority !=
            illumination_state::SpectrumAuthority::Unknown ||
        illuminationState.directionAuthority !=
            illumination_state::SpatialAuthority::Unknown ||
        illuminationState.spatialExtentAuthority !=
            illumination_state::SpatialAuthority::Unknown ||
        illuminationState.temporalModulationAuthority !=
            illumination_state::TemporalAuthority::Unknown ||
        illuminationState.cctIsSpdProof ||
        illuminationState.calibrationIlluminantsAreSceneLightProof ||
        illuminationState.createsNewEvidence ||
        illuminationState.scientificMasterModified ||
        illuminationState.channelAuthorityModified ||
        illuminationState.counterfactual ||
        illuminationState.physicalFrameCount != 1u ||
        illuminationState.independentEvidenceCount != 1u) {
        return status_packet(env, -14);
    }

    // Restore the Open-World authority corridor as a runtime gate. With only one
    // admitted frame, illumination inferred from that frame may constrain an
    // appearance derivative but may not become another measured exposure or
    // modify the Scientific Master.
    truthraw::open_world::v0_3::IlluminationBinding illumination{};
    illumination.present = true;
    illumination.authority =
        truthraw::open_world::v0_3::IlluminationAuthority::Inferred;
    illumination.recordId = "OPEN_WORLD_SINGLE_FRAME_SCENE_V0_66";
    illumination.spatialScope = "OPEN_SCENE_GLOBAL_NO_FIXED_WORLD_BOUNDARY";
    illumination.provenanceSha256 = hex_sha256(sourceSeal.sha256);
    illumination.inferenceMethod =
        "SOURCE_BOUND_SINGLE_FRAME_SCENE_STATE_NO_PHYSICAL_RELIGHT_CLAIM";
    if (truthraw::open_world::v0_3::validate_illumination_binding(illumination) !=
        truthraw::open_world::v0_3::Status::Ok) {
        return status_packet(env, -9);
    }

    const float adaptiveDetailNoiseSigmaAt2Pct =
        adaptive_detail::noise_sigma_2pct_from_metadata(source->metadata());

    std::shared_ptr<truthraw::IAppearanceBackend> appearance;
    if ((flags & kFlagDetail) != 0) {
        appearance =
            std::make_shared<adaptive_detail::AdaptiveDetailedCrispAppearanceV47j>(
                adaptiveDetailNoiseSigmaAt2Pct);
    } else {
        appearance = std::make_shared<NeutralReferenceAppearance>();
    }

    AdvancedPreviewSink sink(maxEdge, flags, adaptiveDetailNoiseSigmaAt2Pct);
    StreamingTruthRawProcessor processor(reconstruction, appearance);
    StreamingResult streaming;
    const auto processed = processor.process(
        *source,
        sink,
        advanced_options(static_cast<std::size_t>(maxLogicalResidentBytes), flags),
        streaming);
    if (!processed) return status_packet(env, stream_status(processed));

    const bool adaptiveDetailEnabled = (flags & kFlagDetail) != 0;
    const char* expectedAppearanceBackend = adaptiveDetailEnabled
        ? "adaptive_detailed_crisp_multiband_hard_edge_guard_v47j"
        : "neutral_reference_cpu";

    if (!sink.finished() ||
        streaming.provenance.scientificMasterModifiedByAppearance ||
        streaming.provenance.counterfactualObservationCreated ||
        streaming.provenance.physicalFrameCount != 1u ||
        streaming.provenance.independentEvidenceCount != 1u ||
        streaming.provenance.appearanceBackend != expectedAppearanceBackend ||
        streaming.memory.adapterOwnsFullRawFrame ||
        streaming.memory.adapterOwnsFullSdrFrame ||
        streaming.memory.adapterOwnsFullHalfGainFrame ||
        streaming.memory.adapterOwnsFullDiagnosticFrame) {
        return status_packet(env, -13);
    }

    // Dynamic Authority is always evaluated for Advanced. Restoration is only
    // one downstream consumer of the resulting CENSORED mask.
    std::vector<std::uint8_t> censorMask;
    const auto maskStatus = build_censor_mask(*source, sink, censorMask);
    if (!maskStatus) return status_packet(env, stream_status(maskStatus));
    if (!sink.applyCensorMask(censorMask)) return status_packet(env, -6);

    const auto& outputAcutanceResult = sink.outputAcutanceResult();
    const auto expectedOutputProfile = adaptiveDetailEnabled
        ? truthraw_v47k::OutputProfile::AdaptiveDetail
        : truthraw_v47k::OutputProfile::Neutral;
    const bool expectedHdrRebase = (flags & kFlagHdr) != 0;
    if (!outputAcutanceResult.applied ||
        outputAcutanceResult.profile != expectedOutputProfile ||
        outputAcutanceResult.hdrRebased != expectedHdrRebase ||
        !std::isfinite(outputAcutanceResult.plan.noiseSigmaAt2Pct) ||
        outputAcutanceResult.plan.noiseSigmaAt2Pct < 0.0f ||
        !std::isfinite(outputAcutanceResult.plan.resizeRatio) ||
        outputAcutanceResult.plan.resizeRatio < 1.0f ||
        !std::isfinite(outputAcutanceResult.plan.strength) ||
        outputAcutanceResult.plan.strength < 0.012f ||
        outputAcutanceResult.plan.strength > 0.130001f ||
        !std::isfinite(outputAcutanceResult.plan.deltaCap) ||
        outputAcutanceResult.plan.deltaCap < 0.0045f ||
        outputAcutanceResult.plan.deltaCap > 0.007001f ||
        !std::isfinite(outputAcutanceResult.maxEffectiveHdrTargetAbsError)) {
        return status_packet(env, -14);
    }

    const auto postVerified =
        truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(
            *bytes, sourceSeal);
    if (!postVerified) return status_packet(env, binding_status(postVerified));

    const auto& audit = dngSource->audit();
    if (audit.fullRawMaterialized || audit.fullFileMaterialized) {
        return status_packet(env, -7);
    }

    const auto& pixels = sink.argb8888();
    const std::size_t expectedPixels =
        static_cast<std::size_t>(sink.width()) * static_cast<std::size_t>(sink.height());
    if (pixels.size() != expectedPixels || sink.writtenPixels() != expectedPixels) {
        return status_packet(env, -8);
    }

    const std::uint64_t logicalResidentUpperBound = std::max<std::uint64_t>(
        static_cast<std::uint64_t>(streaming.memory.logicalResidentUpperBound),
        static_cast<std::uint64_t>(scientific.logicalResidentUpperBound));

    std::vector<jint> out(kHeaderInts + pixels.size(), 0);
    out[0] = kMagic;
    out[1] = 0;
    out[2] = sink.width();
    out[3] = sink.height();
    out[4] = source->metadata().width;
    out[5] = source->metadata().height;
    out[6] = flags;
    out[7] = clamp_metric(source->residentBytesUpperBound());
    out[8] = clamp_metric(audit.rawPayloadBytesRead);
    out[9] = clamp_metric(audit.metadataBytesRead + produced.audit.metadataBytesRead);
    out[10] = clamp_metric(audit.tileReadCalls);
    out[11] = source->metadata().hasGainField ? 1 : 0;
    out[12] = static_cast<jint>(source->metadata().orientation);
    out[13] = static_cast<jint>(streaming.provenance.physicalFrameCount);
    out[14] = static_cast<jint>(streaming.provenance.independentEvidenceCount);
    out[15] = clamp_metric(logicalResidentUpperBound);
    out[16] = clamp_metric(streaming.tilesProcessedPass1);
    out[17] = clamp_metric(streaming.tilesProcessedPass2);
    out[18] = prepared.sourceBoundAppearanceReleaseAllowed ? 1 : 0;
    out[19] = prepared.scientificClaimAllowed ? 1 : 0;
    out[20] = static_cast<jint>(phase2.admission.claimScope);
    out[21] = produced.audit.usedForwardMatrix ? 1 : 0;
    out[22] = produced.audit.cameraCalibrationApplied ? 1 : 0;
    out[23] = clamp_metric(streaming.clippedCount);
    out[24] = clamp_metric(streaming.stage2Over1Count);
    out[25] = clamp_metric(sink.restoredPixels());
    out[26] = clamp_metric(sink.censoredPreviewPixels());
    out[27] = clamp_metric(sink.hdrGainPixels());
    out[28] = clamp_metric(sink.lightAdjustedPixels());
    out[29] = (flags & kFlagDetail) != 0 ? 1 : 0;
    out[30] = (flags & kFlagRestoration) != 0 ? 1 : 0;
    out[31] = 3; // ADVANCED_CANONICAL_OPEN_SCENE_V0_71
    out[32] = 1; // canonical Open Scene v0.70 artifact binding validated
    out[33] = static_cast<jint>(
        truthraw::open_world::v0_3::IlluminationAuthority::Inferred);
    out[34] = static_cast<jint>(
        truthraw::open_world::v0_3::OutputAuthority::AppearanceOnly);
    out[35] = clamp_metric(
        static_cast<std::uint64_t>(expectedPixels) - sink.censoredPreviewPixels());
    out[36] = 0; // generic RECONSTRUCTED authority requires admitted uncertainty
    out[37] = clamp_metric(sink.censoredPreviewPixels());
    out[38] = clamp_metric(2u * static_cast<std::uint64_t>(expectedPixels));
    out[39] = 1; // restoration/relight remain derivative; no scientific writeback
    out[56] = static_cast<jint>(uncertaintyDecision.code);
    out[57] = uncertaintyDecision.reconstructedAuthorityAllowed ? 1 : 0;

    canonical_scene::Digest adaptiveDetailBindingSha256{};
    if (adaptiveDetailEnabled) {
        adaptiveDetailBindingSha256 = build_adaptive_detail_binding(
            openSceneSummary.artifactSha256,
            channelSummary.artifactSha256,
            uncertaintyDecision.decisionSha256,
            adaptiveDetailNoiseSigmaAt2Pct);
    }
    out[66] = adaptiveDetailEnabled ? 1 : 0;
    out[67] = adaptiveDetailEnabled
        ? static_cast<jint>(
            std::bit_cast<std::uint32_t>(adaptiveDetailNoiseSigmaAt2Pct))
        : 0;

    const canonical_scene::Digest outputAcutanceBindingSha256 =
        build_output_acutance_binding(
            openSceneSummary.artifactSha256,
            channelSummary.artifactSha256,
            uncertaintyDecision.decisionSha256,
            adaptiveDetailBindingSha256,
            outputAcutanceResult,
            sink.width(),
            sink.height());

    out[76] = outputAcutanceResult.applied ? 1 : 0;
    out[77] = static_cast<jint>(outputAcutanceResult.profile);
    out[78] = static_cast<jint>(
        std::bit_cast<std::uint32_t>(outputAcutanceResult.plan.noiseSigmaAt2Pct));
    out[79] = static_cast<jint>(
        std::bit_cast<std::uint32_t>(outputAcutanceResult.plan.resizeRatio));
    out[80] = static_cast<jint>(
        std::bit_cast<std::uint32_t>(outputAcutanceResult.plan.strength));
    out[81] = static_cast<jint>(
        std::bit_cast<std::uint32_t>(outputAcutanceResult.plan.deltaCap));
    out[82] = outputAcutanceResult.hdrRebased ? 1 : 0;
    out[83] = clamp_metric(outputAcutanceResult.changedPixels);
    out[84] = clamp_metric(outputAcutanceResult.hdrRebasedPixels);
    out[85] = static_cast<jint>(
        std::bit_cast<std::uint32_t>(
            outputAcutanceResult.maxEffectiveHdrTargetAbsError));
    out[86] = 1; // FINAL_RESIZE -> ACUTANCE -> HDR_REBASE -> OETF
    out[87] = 0; // output acutance never changes scientific authority

    out[96] = static_cast<jint>(illuminationState.whitePointAuthority);
    out[97] = illuminationState.whitePointKnown ? 1 : 0;
    out[98] = illuminationState.whitePointKnown
        ? static_cast<jint>(std::bit_cast<std::uint32_t>(
            static_cast<float>(illuminationState.correlatedColorTemperatureK)))
        : 0;
    out[99] = illuminationState.whitePointKnown
        ? static_cast<jint>(std::bit_cast<std::uint32_t>(
            static_cast<float>(illuminationState.duv1960PolylineEstimate)))
        : 0;
    out[100] = illuminationState.whitePointKnown
        ? static_cast<jint>(std::bit_cast<std::uint32_t>(
            static_cast<float>(illuminationState.whiteX)))
        : 0;
    out[101] = illuminationState.whitePointKnown
        ? static_cast<jint>(std::bit_cast<std::uint32_t>(
            static_cast<float>(illuminationState.whiteY)))
        : 0;
    out[102] = static_cast<jint>(illuminationState.sceneLightKind);
    out[103] = static_cast<jint>(illuminationState.spectrumAuthority);
    out[104] = static_cast<jint>(illuminationState.directionAuthority);
    out[105] = static_cast<jint>(illuminationState.spatialExtentAuthority);
    out[106] =
        static_cast<jint>(illuminationState.temporalModulationAuthority);
    out[107] = illuminationState.dualCalibrationUsed ? 1 : 0;
    out[108] =
        static_cast<jint>(illuminationState.profileCalibrationIlluminant1);
    out[109] =
        static_cast<jint>(illuminationState.profileCalibrationIlluminant2);
    out[110] = illuminationState.cctIsSpdProof ? 1 : 0;
    out[111] =
        illuminationState.calibrationIlluminantsAreSceneLightProof ? 1 : 0;
    out[112] = illuminationState.createsNewEvidence ? 1 : 0;
    out[113] = illuminationState.scientificMasterModified ? 1 : 0;
    out[114] = illuminationState.channelAuthorityModified ? 1 : 0;
    out[115] = illuminationState.counterfactual ? 1 : 0;
    out[116] = static_cast<jint>(illuminationState.physicalFrameCount);
    out[117] =
        static_cast<jint>(illuminationState.independentEvidenceCount);
    out[126] = 0;
    out[127] = 0;

    for (std::size_t word = 0u; word < 8u; ++word) {
        out[40u + word] = digest_word_le(openSceneSummary.artifactSha256, word);
        out[48u + word] = digest_word_le(channelSummary.artifactSha256, word);
        out[58u + word] = digest_word_le(uncertaintyDecision.decisionSha256, word);
        out[68u + word] = adaptiveDetailEnabled
            ? digest_word_le(adaptiveDetailBindingSha256, word)
            : 0;
        out[88u + word] =
            digest_word_le(outputAcutanceBindingSha256, word);
        out[118u + word] =
            digest_word_le(illuminationState.stateSha256, word);
    }

    for (std::size_t i = 0; i < pixels.size(); ++i) {
        out[kHeaderInts + i] = static_cast<jint>(pixels[i]);
    }

    auto result = env->NewIntArray(static_cast<jsize>(out.size()));
    if (result != nullptr) {
        env->SetIntArrayRegion(result, 0, static_cast<jsize>(out.size()), out.data());
    }
    return result;
}
