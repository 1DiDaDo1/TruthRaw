#include "advanced_render_edit_tile_source_v0_1.h"
#include "truthraw/core.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

namespace render_edit = truthraw::advanced_render_edit::v0_1;
namespace float_dng =
    truthraw::scientific_master_linear_dng_projection::v0_1;
namespace streaming = truthraw::streaming_v0_1;

namespace {

class SyntheticSource final : public streaming::IRawTileSource {
public:
    SyntheticSource() {
        metadata_.width = 130;
        metadata_.height = 98;
        metadata_.cfa = truthraw::CfaPattern::RGGB;
        metadata_.orientation = truthraw::Orientation::Normal;
        metadata_.whiteLevel = 1023.0f;
        metadata_.blackPhase = {64.0f, 64.0f, 64.0f, 64.0f};
        metadata_.hasNoiseProfile = true;
        metadata_.noiseProfile = {
            0.0009f, 1.0e-6f,
            0.0010f, 1.2e-6f,
            0.0011f, 1.4e-6f,
        };
        metadata_.hasGainField = false;
        metadata_.hasResidualBlack = false;

        // Intentionally identity-like test characterization. Combined with a
        // red-dominant synthetic CFA, D50 XYZ -> linear-sRGB creates negative
        // channels so the extended-linear preservation rule is exercised.
        metadata_.cameraToXyzD50 = {
            1.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 1.0f,
        };
        metadata_.sourceId = "truthraw-render-edit-partition-v0.1";
    }

    const truthraw::DngMetadata& metadata() const override {
        return metadata_;
    }

    std::size_t residentBytesUpperBound() const override {
        return 64u * 1024u;
    }

    streaming::StreamStatus readRawTile(
        const truthraw::TileRect& rect,
        std::uint16_t* rawOut,
        std::size_t rawCount,
        float* gainOut,
        std::size_t gainCount) override {
        if (rawOut == nullptr ||
            rect.hx0 < 0 || rect.hy0 < 0 ||
            rect.hx1 > metadata_.width ||
            rect.hy1 > metadata_.height ||
            rect.hx1 <= rect.hx0 ||
            rect.hy1 <= rect.hy0) {
            return streaming::StreamStatus::error(
                streaming::StreamStatusCode::InvalidArgument,
                "synthetic source invalid read rectangle");
        }
        const int w = rect.hx1 - rect.hx0;
        const int h = rect.hy1 - rect.hy0;
        const std::size_t count =
            static_cast<std::size_t>(w) * static_cast<std::size_t>(h);
        if (rawCount < count) {
            return streaming::StreamStatus::error(
                streaming::StreamStatusCode::InvalidArgument,
                "synthetic source raw buffer too small");
        }
        if (gainOut != nullptr && gainCount < count) {
            return streaming::StreamStatus::error(
                streaming::StreamStatusCode::InvalidArgument,
                "synthetic source gain buffer too small");
        }

        for (int yy = 0; yy < h; ++yy) {
            const int y = rect.hy0 + yy;
            for (int xx = 0; xx < w; ++xx) {
                const int x = rect.hx0 + xx;
                std::uint16_t value = 0u;
                const bool red = ((y & 1) == 0) && ((x & 1) == 0);
                const bool blue = ((y & 1) != 0) && ((x & 1) != 0);
                if (red) {
                    value = static_cast<std::uint16_t>(
                        870 + ((x * 7 + y * 3) % 90));
                } else if (blue) {
                    value = static_cast<std::uint16_t>(
                        105 + ((x * 5 + y * 11) % 55));
                } else {
                    value = static_cast<std::uint16_t>(
                        225 + ((x * 13 + y * 7) % 95));
                }

                // Sparse clipped evidence exercises aesthetic Restoration.
                if (((x * 17 + y * 29) % 257) == 0) {
                    value = 1023u;
                }

                rawOut[
                    static_cast<std::size_t>(yy) *
                        static_cast<std::size_t>(w) +
                    static_cast<std::size_t>(xx)] = value;
                if (gainOut != nullptr) {
                    gainOut[
                        static_cast<std::size_t>(yy) *
                            static_cast<std::size_t>(w) +
                        static_cast<std::size_t>(xx)] = 1.0f;
                }
            }
        }
        return streaming::StreamStatus::ok();
    }

    streaming::StreamStatus readRowBias(
        int y0,
        int y1,
        float* out,
        std::size_t count) override {
        if (y0 < 0 || y1 < y0 || y1 > metadata_.height ||
            count < static_cast<std::size_t>(y1 - y0)) {
            return streaming::StreamStatus::error(
                streaming::StreamStatusCode::InvalidArgument,
                "synthetic row bias request invalid");
        }
        if (out != nullptr) {
            std::fill(out, out + (y1 - y0), 0.0f);
        }
        return streaming::StreamStatus::ok();
    }

    streaming::StreamStatus readColBias(
        int x0,
        int x1,
        float* out,
        std::size_t count) override {
        if (x0 < 0 || x1 < x0 || x1 > metadata_.width ||
            count < static_cast<std::size_t>(x1 - x0)) {
            return streaming::StreamStatus::error(
                streaming::StreamStatusCode::InvalidArgument,
                "synthetic column bias request invalid");
        }
        if (out != nullptr) {
            std::fill(out, out + (x1 - x0), 0.0f);
        }
        return streaming::StreamStatus::ok();
    }

private:
    truthraw::DngMetadata metadata_{};
};

bool same_float_bits(float a, float b) {
    return std::bit_cast<std::uint32_t>(a) ==
           std::bit_cast<std::uint32_t>(b);
}

bool same_rgb_bits(
    const std::vector<float>& a,
    const std::vector<float>& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0u; i < a.size(); ++i) {
        if (!same_float_bits(a[i], b[i])) return false;
    }
    return true;
}

truthraw::ExposurePlan test_exposure() {
    truthraw::ExposurePlan p{};
    p.evidenceConfidence = 0.82f;
    p.sceneToDisplayScalar = 1.0f;
    p.midGain = 1.0f;
    p.blackFactor = 1.0f;
    p.sdrHighlightGain = 1.0f;
    return p;
}

} // namespace

int main() {
    constexpr std::uint32_t flags =
        render_edit::kFlagLight |
        render_edit::kFlagHdr |
        render_edit::kFlagDetail |
        render_edit::kFlagRestoration;

    // Partition invariance: one 128x64 request versus two adjacent 64x64
    // requests must produce identical Float32 bits.
    SyntheticSource wideSource;
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction wideRecon;
    render_edit::ExtendedLinearSrgbTileSource wide(
        wideSource,
        wideRecon,
        wideSource.metadata().cameraToXyzD50,
        flags,
        test_exposure());

    std::vector<float> widePixels(128u * 64u * 3u);
    auto s = wide.readCameraNativeTile(
        0u, 0u, 128u, 64u,
        widePixels.data(), widePixels.size());
    assert(s);

    SyntheticSource splitSource;
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction splitRecon;
    render_edit::ExtendedLinearSrgbTileSource split(
        splitSource,
        splitRecon,
        splitSource.metadata().cameraToXyzD50,
        flags,
        test_exposure());

    std::vector<float> left(64u * 64u * 3u);
    std::vector<float> right(64u * 64u * 3u);
    assert(split.readCameraNativeTile(
        0u, 0u, 64u, 64u, left.data(), left.size()));
    assert(split.readCameraNativeTile(
        64u, 0u, 64u, 64u, right.data(), right.size()));

    std::vector<float> joined(widePixels.size());
    for (std::size_t y = 0u; y < 64u; ++y) {
        for (std::size_t x = 0u; x < 128u; ++x) {
            const bool useLeft = x < 64u;
            const std::size_t sx = useLeft ? x : x - 64u;
            const auto& source = useLeft ? left : right;
            for (std::size_t ch = 0u; ch < 3u; ++ch) {
                joined[(y * 128u + x) * 3u + ch] =
                    source[(y * 64u + sx) * 3u + ch];
            }
        }
    }
    assert(same_rgb_bits(widePixels, joined));

    // Extended-linear negative input pixels must remain exactly unchanged by
    // Detail/Light/Restoration.
    SyntheticSource neutralSource;
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction neutralRecon;
    render_edit::ExtendedLinearSrgbTileSource neutral(
        neutralSource,
        neutralRecon,
        neutralSource.metadata().cameraToXyzD50,
        0u,
        test_exposure());

    std::vector<float> neutralPixels(128u * 64u * 3u);
    assert(neutral.readCameraNativeTile(
        0u, 0u, 128u, 64u,
        neutralPixels.data(), neutralPixels.size()));

    std::size_t negativePixels = 0u;
    for (std::size_t i = 0u; i < 128u * 64u; ++i) {
        const bool negative =
            neutralPixels[3u * i + 0u] < 0.0f ||
            neutralPixels[3u * i + 1u] < 0.0f ||
            neutralPixels[3u * i + 2u] < 0.0f;
        if (!negative) continue;
        ++negativePixels;
        for (std::size_t ch = 0u; ch < 3u; ++ch) {
            assert(same_float_bits(
                neutralPixels[3u * i + ch],
                widePixels[3u * i + ch]));
        }
    }
    assert(negativePixels > 0u);

    // Canonical projected-raster digest must be deterministic across complete
    // replay on fresh source/reconstruction state.
    float_dng::Hash256 hashA{};
    float_dng::Hash256 hashB{};

    SyntheticSource hashSourceA;
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction hashReconA;
    render_edit::ExtendedLinearSrgbTileSource hashTileSourceA(
        hashSourceA, hashReconA,
        hashSourceA.metadata().cameraToXyzD50,
        flags, test_exposure());
    assert(render_edit::compute_projected_raster_sha256(
        hashTileSourceA,
        static_cast<std::uint32_t>(hashSourceA.metadata().width),
        static_cast<std::uint32_t>(hashSourceA.metadata().height),
        hashA));

    SyntheticSource hashSourceB;
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction hashReconB;
    render_edit::ExtendedLinearSrgbTileSource hashTileSourceB(
        hashSourceB, hashReconB,
        hashSourceB.metadata().cameraToXyzD50,
        flags, test_exposure());
    assert(render_edit::compute_projected_raster_sha256(
        hashTileSourceB,
        static_cast<std::uint32_t>(hashSourceB.metadata().width),
        static_cast<std::uint32_t>(hashSourceB.metadata().height),
        hashB));

    assert(hashA == hashB);
    assert(!wide.hdrBakedIntoPrimary());
    assert(wide.appearanceBakedIntoPrimary());

    std::cout
        << "advanced_render_edit_partition_v0_1: PASS · negativePixels="
        << negativePixels << "\n";
    return 0;
}
