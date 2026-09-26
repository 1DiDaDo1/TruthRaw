#pragma once

#include "truthnegative_appearance_highlight_detail_audit_v0_1.h"
#include "truthraw_sha256_v0_69.h"

#include <cstdint>
#include <string>
#include <vector>

namespace truthraw::truthnegative_appearance_highlight_headroom_sweep::v0_2 {

namespace detail =
    truthraw::truthnegative_appearance_highlight_detail_audit::v0_1;
using Digest = truthraw::sha256_v0_69::Digest;

inline constexpr const char* kSchemaName =
    "D.RAW/TruthNegative/AppearanceHighlightHeadroomSweep/0.2";

struct Binding final {
    Digest sourceEvidenceSha256{};
    Digest scientificMasterSha256{};
    Digest authorityFieldSha256{};
    Digest truthNegativeStateSha256{};
};

struct VariantInput final {
    std::string id{};
    double referenceWhiteNits = 100.0;
    double peakNits = 100.0;
    detail::Report detailAudit{};
    Digest appearanceStateSha256{};
    std::uint64_t belowKneeSampleCount = 0u;
    std::uint64_t belowKneeMappedLuminanceChanged = 0u;
};

struct VariantResult final {
    std::string id{};
    double referenceWhiteNits = 0.0;
    double peakNits = 0.0;
    std::uint64_t sampleCount = 0u;
    std::uint64_t mappedAtPeak = 0u;
    std::uint64_t sourceCensored = 0u;
    std::uint64_t gamutOrDisplayClamp = 0u;
    std::uint64_t sourceDistinctAdjacentPairs = 0u;
    std::uint64_t peakCollapsedDistinctAdjacentPairs = 0u;
    std::uint64_t brightPeakCollapsedDistinctAdjacentPairs = 0u;
    std::uint64_t belowKneeSampleCount = 0u;
    std::uint64_t belowKneeMappedLuminanceChanged = 0u;
    double sourceGradientSum = 0.0;
    double mappedGradientSum = 0.0;
    double mappedGradientRetention = 0.0;
    double collapseFractionOfDistinctPairs = 0.0;
    double collapseReductionVsBaseline = 0.0;
    bool noHighlightHeadroom = false;
    bool mappedPeakCollapseObserved = false;
    Digest appearanceStateSha256{};
    Digest detailAuditSha256{};
};

struct Input final {
    Binding binding{};
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    std::vector<VariantInput> variants{};
};

struct Report final {
    Digest sweepSha256{};
    Digest jsonSha256{};
    std::string json{};
    std::vector<VariantResult> variants{};
    bool sourceIdentityConsistent = false;
    bool sourceGradientConsistent = false;
    bool sourceCensorCountConsistent = false;
    bool baselineIsFirst = false;
    bool lowerRangePreservationAudited = false;
    bool automaticWinnerSelected = false;
    bool sourceSceneMutated = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

bool run(const Input& input, Report& out) noexcept;

} // namespace truthraw::truthnegative_appearance_highlight_headroom_sweep::v0_2
