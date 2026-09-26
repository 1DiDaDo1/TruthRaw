#pragma once

#include "free_world_pixel_resolve_2d_v0_2.h"
#include "truthraw_sha256_v0_69.h"

#include <cstdint>
#include <string>
#include <vector>

namespace truthraw::truthnegative_appearance_highlight_detail_audit::v0_1 {

namespace fw = truthraw::free_world_pixel_resolve_2d::v0_2;
using Digest = truthraw::sha256_v0_69::Digest;

inline constexpr const char* kSchemaName =
    "D.RAW/TruthNegative/AppearanceHighlightDetailAudit/0.1";

struct Binding final {
    Digest sourceEvidenceSha256{};
    Digest scientificMasterSha256{};
    Digest authorityFieldSha256{};
    Digest truthNegativeStateSha256{};
    Digest appearanceStateSha256{};
};

struct Sample final {
    double sourceLuminanceNits = 0.0;
    double mappedLuminanceNits = 0.0;
    bool gamutOrDisplayClampApplied = false;
    bool sourceCensored = false;
};

struct TileMetrics final {
    std::uint32_t x = 0u;
    std::uint32_t y = 0u;
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;

    std::uint64_t sampleCount = 0u;
    std::uint64_t sourceAboveReferenceWhite = 0u;
    std::uint64_t mappedAtPeak = 0u;
    std::uint64_t sourceCensored = 0u;
    std::uint64_t gamutOrDisplayClamp = 0u;

    std::uint64_t adjacentPairs = 0u;
    std::uint64_t sourceDistinctAdjacentPairs = 0u;
    std::uint64_t peakCollapsedDistinctAdjacentPairs = 0u;
    std::uint64_t brightPeakCollapsedDistinctAdjacentPairs = 0u;

    double sourceAbsGradientSum = 0.0;
    double mappedAbsGradientSum = 0.0;
    double collapsedSourceAbsGradientSum = 0.0;
    double maxCollapsedSourceAbsGradient = 0.0;
};

struct Input final {
    Binding binding{};
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    std::uint32_t tileEdge = 16u;
    double displayReferenceWhiteNits = 100.0;
    double displayPeakNits = 100.0;
    std::vector<Sample> samples{};
};

struct Report final {
    Digest auditSha256{};
    Digest jsonSha256{};
    std::string json{};

    std::uint64_t sampleCount = 0u;
    std::uint64_t sourceAboveReferenceWhite = 0u;
    std::uint64_t mappedAtPeak = 0u;
    std::uint64_t sourceCensored = 0u;
    std::uint64_t gamutOrDisplayClamp = 0u;

    std::uint64_t adjacentPairs = 0u;
    std::uint64_t sourceDistinctAdjacentPairs = 0u;
    std::uint64_t peakCollapsedDistinctAdjacentPairs = 0u;
    std::uint64_t brightPeakCollapsedDistinctAdjacentPairs = 0u;

    double sourceAbsGradientSum = 0.0;
    double mappedAbsGradientSum = 0.0;
    double collapsedSourceAbsGradientSum = 0.0;
    double maxCollapsedSourceAbsGradient = 0.0;

    std::vector<TileMetrics> tiles{};

    bool noHighlightHeadroom = false;
    bool mappedPeakCollapseObserved = false;
    bool sourceSceneMutated = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

bool run(const Input& input, Report& out) noexcept;

} // namespace truthraw::truthnegative_appearance_highlight_detail_audit::v0_1
