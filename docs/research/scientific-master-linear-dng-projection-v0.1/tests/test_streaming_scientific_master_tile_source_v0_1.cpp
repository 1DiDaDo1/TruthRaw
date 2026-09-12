#include "streaming_scientific_master_tile_source_v0_1.h"
#include "scientific_master_streaming_binding_v0_2.h"
#include "scientific_master_digest_v0_1.h"
#include "streaming_test_support_v0_1.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

namespace projection = truthraw::scientific_master_linear_dng_projection::v0_1;
namespace master_binding = truthraw::scientific_master_streaming_binding::v0_2;
namespace master_digest = truthraw::scientific_master_digest::v0_1;

int main() {
    constexpr int width = 130;
    constexpr int height = 70;
    auto frame = make_frame(width, height);

    FrameSource bindingSource(frame);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction bindingReconstruction;
    master_binding::Options bindingOptions{};
    master_binding::Result canonical{};
    const auto bindingStatus = master_binding::bind_scientific_master_streaming(
        bindingSource, bindingReconstruction, bindingOptions, canonical);
    if (!bindingStatus) {
        std::cerr << "canonical binding failed: " << bindingStatus.message << '\n';
        return 2;
    }

    FrameSource adapterSource(frame);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction adapterReconstruction;
    projection::StreamingScientificMasterTileSource adapter(
        adapterSource, adapterReconstruction);

    master_digest::ScientificMasterDigestAccumulator digest(
        static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
    if (!digest.valid()) {
        std::cerr << "digest init failed: " << digest.error() << '\n';
        return 2;
    }

    std::vector<float> tile;
    std::size_t tileCount = 0u;
    for (std::uint32_t y = 0u; y < static_cast<std::uint32_t>(height);
         y += projection::kCanonicalTileEdge) {
        const auto h = std::min(
            projection::kCanonicalTileEdge,
            static_cast<std::uint32_t>(height) - y);
        for (std::uint32_t x = 0u; x < static_cast<std::uint32_t>(width);
             x += projection::kCanonicalTileEdge) {
            const auto w = std::min(
                projection::kCanonicalTileEdge,
                static_cast<std::uint32_t>(width) - x);
            tile.resize(static_cast<std::size_t>(w) * h * 3u);
            const auto read = adapter.readCameraNativeTile(
                x, y, w, h, tile.data(), tile.size());
            if (!read) {
                std::cerr << "adapter read failed: " << read.message << '\n';
                return 2;
            }

            master_digest::TileView view{};
            view.x = x;
            view.y = y;
            view.width = w;
            view.height = h;
            view.rgb = tile.data();
            view.rowStrideSamples = static_cast<std::size_t>(w) * 3u;
            if (!digest.add_tile(view)) {
                std::cerr << "digest tile failed: " << digest.error() << '\n';
                return 2;
            }
            ++tileCount;
        }
    }

    projection::Hash256 replayed{};
    if (!digest.finalize(replayed)) {
        std::cerr << "digest finalize failed: " << digest.error() << '\n';
        return 2;
    }
    if (replayed != canonical.scientificMasterHash) {
        std::cerr << "adapter digest differs from canonical Scientific Master binding\n";
        return 2;
    }

    std::vector<float> invalidTile(3u * projection::kCanonicalTileEdge *
                                   projection::kCanonicalTileEdge);
    const auto nonCanonical = adapter.readCameraNativeTile(
        1u, 0u, projection::kCanonicalTileEdge,
        projection::kCanonicalTileEdge,
        invalidTile.data(), invalidTile.size());
    if (nonCanonical || nonCanonical.code != projection::StatusCode::InvalidArgument) {
        std::cerr << "non-canonical tile request was not rejected\n";
        return 2;
    }

    if (tileCount != canonical.masterTilesProcessed) {
        std::cerr << "canonical tile count mismatch\n";
        return 2;
    }
    if (adapter.residentBytesUpperBound() < adapterSource.residentBytesUpperBound()) {
        std::cerr << "adapter resident accounting omitted source state\n";
        return 2;
    }

    std::cout << "STREAMING_SCIENTIFIC_MASTER_TILE_SOURCE_V0_1_PASS\n";
    std::cout << "canonical_digest_match=1\n";
    std::cout << "canonical_tile_schedule_enforced=1\n";
    std::cout << "full_frame_master_materialized=0\n";
    std::cout << "physical_frame_count=" << canonical.physicalFrameCount << '\n';
    std::cout << "independent_evidence_count=" << canonical.independentEvidenceCount << '\n';
    return 0;
}
