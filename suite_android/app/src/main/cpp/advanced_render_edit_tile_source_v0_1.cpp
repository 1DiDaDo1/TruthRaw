#include "advanced_render_edit_tile_source_v0_1.h"

#include "adaptive_detail_v47j_adapter.h"
#include "scientific_master_digest_v0_1.h"
#include "full_frame_streaming_v0_1_internal.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace truthraw::advanced_render_edit::v0_1 {
namespace {

namespace adaptive_detail = truthraw::adaptive_detail_v47j_adapter;
namespace digest = truthraw::scientific_master_digest::v0_1;

constexpr int kRestorationRadius = 2;
constexpr int kRestorationMinSupport = 3;

// This is the exact inverse (to the stored float precision below) of the
// byte-frozen D50-XYZ -> linear-sRGB matrix used by full-frame-streaming-v0.1.
constexpr std::array<float, 9> kLinearSrgbToXyzD50 = {
    0.43607472f, 0.38506492f, 0.14308038f,
    0.22250448f, 0.71687860f, 0.06061692f,
    0.01393217f, 0.09710452f, 0.71417328f,
};

constexpr std::array<float, 9> kXyzD50ToLinearSrgb = {
     3.1338561f, -1.6168667f, -0.4906146f,
    -0.9787684f,  1.9161415f,  0.0334540f,
     0.0719453f, -0.2289914f,  1.4052427f,
};

inline float smoothstep01(float x) noexcept {
    x = std::clamp(x, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

inline bool finite3(const float* p) noexcept {
    return p != nullptr &&
           std::isfinite(p[0]) &&
           std::isfinite(p[1]) &&
           std::isfinite(p[2]);
}

inline bool any_negative(const float* p) noexcept {
    return p[0] < 0.0f || p[1] < 0.0f || p[2] < 0.0f;
}

void camera_to_linear_srgb(
    const float* camera,
    float* linear,
    std::size_t pixels,
    const std::array<float, 9>& cameraToXyzD50) noexcept {
    for (std::size_t i = 0u; i < pixels; ++i) {
        const float cr = camera[3u * i + 0u];
        const float cg = camera[3u * i + 1u];
        const float cb = camera[3u * i + 2u];

        const float X =
            cameraToXyzD50[0] * cr +
            cameraToXyzD50[1] * cg +
            cameraToXyzD50[2] * cb;
        const float Y =
            cameraToXyzD50[3] * cr +
            cameraToXyzD50[4] * cg +
            cameraToXyzD50[5] * cb;
        const float Z =
            cameraToXyzD50[6] * cr +
            cameraToXyzD50[7] * cg +
            cameraToXyzD50[8] * cb;

        linear[3u * i + 0u] =
            kXyzD50ToLinearSrgb[0] * X +
            kXyzD50ToLinearSrgb[1] * Y +
            kXyzD50ToLinearSrgb[2] * Z;
        linear[3u * i + 1u] =
            kXyzD50ToLinearSrgb[3] * X +
            kXyzD50ToLinearSrgb[4] * Y +
            kXyzD50ToLinearSrgb[5] * Z;
        linear[3u * i + 2u] =
            kXyzD50ToLinearSrgb[6] * X +
            kXyzD50ToLinearSrgb[7] * Y +
            kXyzD50ToLinearSrgb[8] * Z;
    }
}

float_dng::Status source_error(std::string message) {
    return float_dng::Status::error(
        float_dng::StatusCode::SourceFailed,
        std::move(message));
}

}  // namespace

struct ExtendedLinearSrgbTileSource::Impl final {
    streaming_v0_1::IRawTileSource& source;
    IReconstructionBackend& reconstruction;
    std::array<float, 9> cameraToXyzD50{};
    std::uint32_t flags = 0u;
    ExposurePlan exposure{};
    float detailMix = 0.0f;
    float colorFullnessMix = 0.0f;
    streaming_v0_1::detail::Workspace reconstructionWorkspace{};
    adaptive_detail::AdaptiveDetailedCrispAppearanceV47j detail;

    std::vector<float> cameraExpanded;
    std::vector<float> linearExpanded;
    std::vector<float> developedSupport;
    std::vector<std::uint16_t> rawSupport;
    std::vector<float> gainScratch;

    Impl(
        streaming_v0_1::IRawTileSource& sourceIn,
        IReconstructionBackend& reconstructionIn,
        const std::array<float, 9>& matrix,
        std::uint32_t flagsIn,
        const ExposurePlan& exposureIn) noexcept
        : source(sourceIn),
          reconstruction(reconstructionIn),
          cameraToXyzD50(matrix),
          flags(flagsIn & kAllowedFlags),
          exposure(exposureIn),
          detailMix(controls::detail_mix(flags)),
          colorFullnessMix(controls::color_fullness_mix(flags)),
          detail(
              adaptive_detail::noise_sigma_2pct_from_metadata(sourceIn.metadata()),
              detailMix) {}
};

ExtendedLinearSrgbTileSource::ExtendedLinearSrgbTileSource(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const std::array<float, 9>& cameraToXyzD50,
    std::uint32_t flags,
    const ExposurePlan& exposure) noexcept
    : impl_(new (std::nothrow) Impl(
          source, reconstruction, cameraToXyzD50, flags, exposure)) {}

ExtendedLinearSrgbTileSource::~ExtendedLinearSrgbTileSource() = default;

std::size_t ExtendedLinearSrgbTileSource::residentBytesUpperBound() const noexcept {
    if (!impl_) return 0u;
    // Canonical DNG writes request 64x64 cores. Worst support expansion is
    // restoration(2) + detail(5) on every side. Keep a conservative bound that
    // also includes the underlying Scientific Master replay workspace.
    constexpr std::size_t kLocalBound = 2u * 1024u * 1024u;
    const std::size_t workspace =
        streaming_v0_1::detail::vector_bytes(
            impl_->reconstructionWorkspace);
    const std::size_t sourceBytes = impl_->source.residentBytesUpperBound();
    if (sourceBytes >
            std::numeric_limits<std::size_t>::max() - workspace ||
        sourceBytes + workspace >
            std::numeric_limits<std::size_t>::max() - kLocalBound) {
        return std::numeric_limits<std::size_t>::max();
    }
    return sourceBytes + workspace + kLocalBound;
}

std::uint32_t ExtendedLinearSrgbTileSource::flags() const noexcept {
    return impl_ ? impl_->flags : 0u;
}

bool ExtendedLinearSrgbTileSource::appearanceBakedIntoPrimary() const noexcept {
    if (!impl_) return false;
    return (impl_->flags & (kFlagLight | kFlagDetail | kFlagRestoration)) != 0u ||
           controls::color_fullness(impl_->flags) != 0;
}

bool ExtendedLinearSrgbTileSource::hdrBakedIntoPrimary() const noexcept {
    return false;
}

float_dng::Status ExtendedLinearSrgbTileSource::readCameraNativeTile(
    std::uint32_t x,
    std::uint32_t y,
    std::uint32_t width,
    std::uint32_t height,
    float* rgb,
    std::size_t floatCount) noexcept {
    if (!impl_ || rgb == nullptr || width == 0u || height == 0u ||
        floatCount != static_cast<std::size_t>(width) *
                          static_cast<std::size_t>(height) * 3u) {
        return source_error("Render/Edit derivative tile arguments are invalid");
    }

    try {
        const auto& m = impl_->source.metadata();
        if (x >= static_cast<std::uint32_t>(m.width) ||
            y >= static_cast<std::uint32_t>(m.height) ||
            width > static_cast<std::uint32_t>(m.width) - x ||
            height > static_cast<std::uint32_t>(m.height) - y) {
            return source_error("Render/Edit derivative tile escaped source bounds");
        }

        const bool useDetail = (impl_->flags & kFlagDetail) != 0u;
        const bool useRestoration = (impl_->flags & kFlagRestoration) != 0u;
        const bool useLight = (impl_->flags & kFlagLight) != 0u;
        const bool needsRawMask = useRestoration || useLight;

        const int coreX0 = static_cast<int>(x);
        const int coreY0 = static_cast<int>(y);
        const int coreX1 = coreX0 + static_cast<int>(width);
        const int coreY1 = coreY0 + static_cast<int>(height);

        const int restorationHalo = useRestoration ? kRestorationRadius : 0;
        const int supportX0 = std::max(0, coreX0 - restorationHalo);
        const int supportY0 = std::max(0, coreY0 - restorationHalo);
        const int supportX1 = std::min(m.width, coreX1 + restorationHalo);
        const int supportY1 = std::min(m.height, coreY1 + restorationHalo);
        const int supportW = supportX1 - supportX0;
        const int supportH = supportY1 - supportY0;

        const int detailHalo = useDetail ? impl_->detail.requiredHalo() : 0;
        const int expandedX0 = std::max(0, supportX0 - detailHalo);
        const int expandedY0 = std::max(0, supportY0 - detailHalo);
        const int expandedX1 = std::min(m.width, supportX1 + detailHalo);
        const int expandedY1 = std::min(m.height, supportY1 + detailHalo);
        const int expandedW = expandedX1 - expandedX0;
        const int expandedH = expandedY1 - expandedY0;
        const std::size_t expandedPixels =
            static_cast<std::size_t>(expandedW) *
            static_cast<std::size_t>(expandedH);

        const int reconstructionHalo =
            impl_->reconstruction.requiredHalo();
        if (reconstructionHalo < 0) {
            return source_error(
                "Render/Edit reconstruction backend returned negative halo");
        }

        ::truthraw::TileRect reconstructionTile{};
        reconstructionTile.x0 = expandedX0;
        reconstructionTile.y0 = expandedY0;
        reconstructionTile.x1 = expandedX1;
        reconstructionTile.y1 = expandedY1;
        reconstructionTile.hx0 =
            std::max(0, expandedX0 - reconstructionHalo);
        reconstructionTile.hy0 =
            std::max(0, expandedY0 - reconstructionHalo);
        reconstructionTile.hx1 =
            std::min(m.width, expandedX1 + reconstructionHalo);
        reconstructionTile.hy1 =
            std::min(m.height, expandedY1 + reconstructionHalo);

        auto fill = streaming_v0_1::detail::fill_stage2(
            impl_->source,
            reconstructionTile,
            impl_->reconstructionWorkspace);
        if (!fill) {
            return source_error(
                "Render/Edit Stage-2 source read failed: " + fill.message);
        }

        impl_->cameraExpanded.resize(expandedPixels * 3u);
        const auto reconstructionStatus =
            impl_->reconstruction.reconstructTile(
                impl_->reconstructionWorkspace.stage2.data(),
                reconstructionTile.hx1 - reconstructionTile.hx0,
                reconstructionTile.hy1 - reconstructionTile.hy0,
                reconstructionTile.hx0,
                reconstructionTile.hy0,
                expandedX0,
                expandedY0,
                expandedW,
                expandedH,
                m.cfa,
                impl_->cameraExpanded.data());
        if (!reconstructionStatus) {
            return source_error(
                "Render/Edit v4.7i reconstruction failed: " +
                    reconstructionStatus.message);
        }

        impl_->linearExpanded.resize(expandedPixels * 3u);
        camera_to_linear_srgb(
            impl_->cameraExpanded.data(),
            impl_->linearExpanded.data(),
            expandedPixels,
            impl_->cameraToXyzD50);

        for (std::size_t i = 0u; i < expandedPixels; ++i) {
            if (!finite3(impl_->linearExpanded.data() + 3u * i)) {
                return source_error(
                    "Render/Edit color conversion produced non-finite linear RGB");
            }
        }

        const std::size_t supportPixels =
            static_cast<std::size_t>(supportW) *
            static_cast<std::size_t>(supportH);
        impl_->developedSupport.resize(supportPixels * 3u);

        if (useDetail) {
            const auto detailStatus = impl_->detail.applyTile(
                impl_->linearExpanded.data(),
                expandedW,
                expandedH,
                supportX0 - expandedX0,
                supportY0 - expandedY0,
                supportW,
                supportH,
                impl_->developedSupport.data());
            if (!detailStatus) {
                return source_error(
                    "Render/Edit v4.7j Detail failed: " + detailStatus.message);
            }
        } else {
            for (int sy = supportY0; sy < supportY1; ++sy) {
                for (int sx = supportX0; sx < supportX1; ++sx) {
                    const std::size_t src =
                        (static_cast<std::size_t>(sy - expandedY0) *
                             static_cast<std::size_t>(expandedW) +
                         static_cast<std::size_t>(sx - expandedX0)) * 3u;
                    const std::size_t dst =
                        (static_cast<std::size_t>(sy - supportY0) *
                             static_cast<std::size_t>(supportW) +
                         static_cast<std::size_t>(sx - supportX0)) * 3u;
                    impl_->developedSupport[dst + 0u] =
                        impl_->linearExpanded[src + 0u];
                    impl_->developedSupport[dst + 1u] =
                        impl_->linearExpanded[src + 1u];
                    impl_->developedSupport[dst + 2u] =
                        impl_->linearExpanded[src + 2u];
                }
            }
        }

        // Negative extended-linear pixels belong to the edit headroom. v4.7j
        // internally clamps for its luminance analysis, so restore those pixels
        // exactly before any later appearance operation.
        for (int sy = supportY0; sy < supportY1; ++sy) {
            for (int sx = supportX0; sx < supportX1; ++sx) {
                const std::size_t src =
                    (static_cast<std::size_t>(sy - expandedY0) *
                         static_cast<std::size_t>(expandedW) +
                     static_cast<std::size_t>(sx - expandedX0)) * 3u;
                const std::size_t dst =
                    (static_cast<std::size_t>(sy - supportY0) *
                         static_cast<std::size_t>(supportW) +
                     static_cast<std::size_t>(sx - supportX0)) * 3u;
                const float* original = impl_->linearExpanded.data() + src;
                if (any_negative(original)) {
                    impl_->developedSupport[dst + 0u] = original[0];
                    impl_->developedSupport[dst + 1u] = original[1];
                    impl_->developedSupport[dst + 2u] = original[2];
                }
            }
        }

        if (needsRawMask) {
            impl_->rawSupport.resize(supportPixels);
            if (m.hasGainField) {
                impl_->gainScratch.resize(supportPixels);
            } else {
                impl_->gainScratch.clear();
            }
            TileRect rect{
                supportX0, supportY0, supportX1, supportY1,
                supportX0, supportY0, supportX1, supportY1};
            const auto rawStatus = impl_->source.readRawTile(
                rect,
                impl_->rawSupport.data(),
                impl_->rawSupport.size(),
                m.hasGainField ? impl_->gainScratch.data() : nullptr,
                m.hasGainField ? impl_->gainScratch.size() : 0u);
            if (!rawStatus) {
                return source_error(
                    "Render/Edit censor-mask read failed: " + rawStatus.message);
            }
        } else {
            impl_->rawSupport.clear();
            impl_->gainScratch.clear();
        }

        for (int cy = 0; cy < static_cast<int>(height); ++cy) {
            for (int cx = 0; cx < static_cast<int>(width); ++cx) {
                const int gx = coreX0 + cx;
                const int gy = coreY0 + cy;
                const std::size_t supportIndex =
                    static_cast<std::size_t>(gy - supportY0) *
                        static_cast<std::size_t>(supportW) +
                    static_cast<std::size_t>(gx - supportX0);
                const std::size_t expandedIndex =
                    static_cast<std::size_t>(gy - expandedY0) *
                        static_cast<std::size_t>(expandedW) +
                    static_cast<std::size_t>(gx - expandedX0);
                const std::size_t outIndex =
                    static_cast<std::size_t>(cy) *
                        static_cast<std::size_t>(width) +
                    static_cast<std::size_t>(cx);

                const float* original =
                    impl_->linearExpanded.data() + 3u * expandedIndex;
                float r = impl_->developedSupport[3u * supportIndex + 0u];
                float g = impl_->developedSupport[3u * supportIndex + 1u];
                float b = impl_->developedSupport[3u * supportIndex + 2u];

                if (any_negative(original)) {
                    rgb[3u * outIndex + 0u] = original[0];
                    rgb[3u * outIndex + 1u] = original[1];
                    rgb[3u * outIndex + 2u] = original[2];
                    continue;
                }

                const bool censored =
                    needsRawMask &&
                    static_cast<float>(impl_->rawSupport[supportIndex]) >=
                        m.whiteLevel;

                if (useRestoration && censored) {
                    double sumR = 0.0;
                    double sumG = 0.0;
                    double sumB = 0.0;
                    double sumW = 0.0;
                    int support = 0;
                    for (int radius = 1;
                         radius <= kRestorationRadius &&
                         support < kRestorationMinSupport;
                         ++radius) {
                        for (int dy = -radius; dy <= radius; ++dy) {
                            for (int dx = -radius; dx <= radius; ++dx) {
                                if (dx == 0 && dy == 0) continue;
                                if (std::max(std::abs(dx), std::abs(dy)) != radius) {
                                    continue;
                                }
                                const int nx = gx + dx;
                                const int ny = gy + dy;
                                if (nx < supportX0 || nx >= supportX1 ||
                                    ny < supportY0 || ny >= supportY1) {
                                    continue;
                                }
                                const std::size_t ni =
                                    static_cast<std::size_t>(ny - supportY0) *
                                        static_cast<std::size_t>(supportW) +
                                    static_cast<std::size_t>(nx - supportX0);
                                if (static_cast<float>(impl_->rawSupport[ni]) >=
                                    m.whiteLevel) {
                                    continue;
                                }
                                const float* neighbour =
                                    impl_->developedSupport.data() + 3u * ni;
                                if (!finite3(neighbour) || any_negative(neighbour)) {
                                    continue;
                                }
                                const double weight =
                                    1.0 /
                                    std::sqrt(
                                        static_cast<double>(dx * dx + dy * dy));
                                sumR += weight * neighbour[0];
                                sumG += weight * neighbour[1];
                                sumB += weight * neighbour[2];
                                sumW += weight;
                                ++support;
                            }
                        }
                    }
                    if (support >= kRestorationMinSupport && sumW > 0.0) {
                        r = static_cast<float>(sumR / sumW);
                        g = static_cast<float>(sumG / sumW);
                        b = static_cast<float>(sumB / sumW);
                    }
                }

                if (useLight && !censored) {
                    const float lum =
                        std::max(truthraw::luminance709(r, g, b), 0.0f);
                    const float darkGate =
                        1.0f - smoothstep01((lum - 0.02f) / 0.30f);
                    const float blackProtect = smoothstep01(lum / 0.025f);
                    const float strength =
                        0.18f *
                        std::clamp(
                            impl_->exposure.evidenceConfidence,
                            0.0f,
                            1.0f) *
                        darkGate *
                        blackProtect;
                    if (strength > 1.0e-4f) {
                        const float scale = 1.0f + strength;
                        r *= scale;
                        g *= scale;
                        b *= scale;
                    }
                }

                if (!controls::apply_color_fullness(
                        r, g, b, impl_->colorFullnessMix)) {
                    return source_error(
                        "Render/Edit color-fullness transform failed");
                }

                if (!std::isfinite(r) || !std::isfinite(g) || !std::isfinite(b)) {
                    return source_error(
                        "Render/Edit derivative produced non-finite output");
                }

                rgb[3u * outIndex + 0u] = r;
                rgb[3u * outIndex + 1u] = g;
                rgb[3u * outIndex + 2u] = b;
            }
        }

        return float_dng::Status::ok();
    } catch (const std::bad_alloc&) {
        return float_dng::Status::error(
            float_dng::StatusCode::SizeOverflow,
            "Render/Edit derivative allocation failed");
    } catch (...) {
        return source_error("Render/Edit derivative failed unexpectedly");
    }
}

const std::array<float, 9>& linear_srgb_to_xyz_d50_matrix() noexcept {
    return kLinearSrgbToXyzD50;
}

float_dng::Status compute_projected_raster_sha256(
    float_dng::IScientificMasterTileSource& source,
    std::uint32_t width,
    std::uint32_t height,
    float_dng::Hash256& out) noexcept {
    try {
        if (width == 0u || height == 0u) {
            return float_dng::Status::error(
                float_dng::StatusCode::InvalidArgument,
                "Render/Edit projected-raster dimensions are invalid");
        }

        digest::ScientificMasterDigestAccumulator accumulator(width, height);
        if (!accumulator.valid()) {
            return float_dng::Status::error(
                float_dng::StatusCode::DigestFailed,
                "Render/Edit projected-raster digest init failed: " +
                    accumulator.error());
        }

        std::vector<float> tile;
        for (std::uint32_t y = 0u;
             y < height;
             y += float_dng::kCanonicalTileEdge) {
            const std::uint32_t h =
                std::min(float_dng::kCanonicalTileEdge, height - y);
            for (std::uint32_t x = 0u;
                 x < width;
                 x += float_dng::kCanonicalTileEdge) {
                const std::uint32_t w =
                    std::min(float_dng::kCanonicalTileEdge, width - x);
                tile.resize(
                    static_cast<std::size_t>(w) *
                    static_cast<std::size_t>(h) * 3u);
                const auto read = source.readCameraNativeTile(
                    x, y, w, h, tile.data(), tile.size());
                if (!read) return read;

                digest::TileView view{};
                view.x = x;
                view.y = y;
                view.width = w;
                view.height = h;
                view.rgb = tile.data();
                view.rowStrideSamples = static_cast<std::size_t>(w) * 3u;
                if (!accumulator.add_tile(view)) {
                    return float_dng::Status::error(
                        float_dng::StatusCode::DigestFailed,
                        "Render/Edit projected-raster digest rejected tile: " +
                            accumulator.error());
                }
            }
        }

        if (!accumulator.finalize(out)) {
            return float_dng::Status::error(
                float_dng::StatusCode::DigestFailed,
                "Render/Edit projected-raster digest finalization failed: " +
                    accumulator.error());
        }
        return float_dng::Status::ok();
    } catch (const std::bad_alloc&) {
        return float_dng::Status::error(
            float_dng::StatusCode::SizeOverflow,
            "Render/Edit projected-raster digest allocation failed");
    } catch (...) {
        return float_dng::Status::error(
            float_dng::StatusCode::DigestFailed,
            "Render/Edit projected-raster digest failed unexpectedly");
    }
}

}  // namespace truthraw::advanced_render_edit::v0_1
