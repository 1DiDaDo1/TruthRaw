#pragma once

#include "full_frame_streaming_v0_1.h"
#include "truthnegative_n2_candidate_pipeline_v0_1.h"
#include "truthraw_sha256_v0_69.h"

#include <array>
#include <cstdint>
#include <vector>

namespace truthraw::truthnegative_n2_cfa_audit::v0_1 {

namespace stream = truthraw::streaming_v0_1;
namespace n2 = truthraw::truthnegative_n2_candidate_pipeline::v0_1;
using Digest = truthraw::sha256_v0_69::Digest;

struct Binding final {
    Digest sourceEvidenceSha256{};
    Digest truthNegativeStateSha256{};
};

struct Options final {
    std::uint32_t tileEdge = 64u;
    // Even period. period=2 audits every CFA sample. period=8 samples one
    // location of each 2x2 CFA phase per 8x8 source block (1/16 of pixels).
    std::uint32_t samplingPeriod = 8u;
    // Optional low-resolution appearance-only correction grid. Zero/zero
    // disables it. This never changes source, Scientific Master or TN state.
    std::uint32_t appearanceGridWidth = 0u;
    std::uint32_t appearanceGridHeight = 0u;
    // Optional bounded source-domain audit region. regionWidth/Height=0
    // means the full admitted source. Coordinates remain source-native.
    std::uint32_t regionX = 0u;
    std::uint32_t regionY = 0u;
    std::uint32_t regionWidth = 0u;
    std::uint32_t regionHeight = 0u;
};

struct CorrectedSampleCoordinate final {
    std::uint32_t x = 0u;
    std::uint32_t y = 0u;
};

struct TileAudit final {
    std::uint32_t x = 0u;
    std::uint32_t y = 0u;
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    n2::Audit audit{};
    std::array<std::uint64_t,4u> cfaPhaseSamples{};
    std::uint64_t borderProtected = 0u;
    std::uint64_t sampled = 0u;
    // In-memory sparse index only. These fields are deliberately excluded
    // from v0.1 candidate/audit/spatial hashes so the sealed v0.1 output
    // identity is unchanged. They let later read-only audits reuse the exact
    // already-computed correction locations without rerunning v0.1 per tile.
    std::uint64_t correctedSampleOffset = 0u;
    std::uint64_t correctedSampleCount = 0u;
};

struct AppearanceCorrectionBin final {
    std::array<double,3u> correctionSum{};
    std::array<std::uint32_t,3u> sampled{};
    std::array<std::uint32_t,3u> corrected{};
    std::array<std::uint32_t,3u> protectedCount{};
    // Bit N records that at least one source sample in this bin was preserved
    // for PreserveReason N. Bit 0 therefore represents PreserveReason::None.
    std::uint32_t preserveReasonMask = 0u;
};

struct Result final {
    n2::Audit audit{};
    Digest candidateSha256{};
    Digest auditSha256{};
    Digest spatialSha256{};
    Digest appearanceGridSha256{};
    std::array<std::uint64_t,4u> cfaPhaseSamples{};
    std::uint64_t borderProtected = 0u;
    std::uint64_t sampled = 0u;
    std::uint32_t samplingPeriod = 0u;
    std::uint32_t tileEdge = 0u;
    std::vector<TileAudit> tiles{};
    // Sparse, in-memory-only coordinates for samples where v0.1 set
    // correctionApplied=true. This is derived from the same single v0.1 pass,
    // is never promoted to evidence, and is not included in v0.1 hashes.
    std::vector<CorrectedSampleCoordinate> correctedSampleCoordinates{};
    // Runtime optimization index is bounded. If this becomes false the
    // scientific v0.1 result remains valid and downstream code must use the
    // legacy v0.2.1 rerun path instead of sparse reuse.
    bool correctedSampleCoordinatesComplete = true;
    std::uint32_t appearanceGridWidth = 0u;
    std::uint32_t appearanceGridHeight = 0u;
    std::uint32_t regionX = 0u;
    std::uint32_t regionY = 0u;
    std::uint32_t regionWidth = 0u;
    std::uint32_t regionHeight = 0u;
    std::vector<AppearanceCorrectionBin> appearanceGrid{};
    bool appearanceGridDerived = false;
    bool noiseProfileAvailable = false;
    bool sourceValuesModified = false;
    bool truthNegativeModified = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

bool run(
    stream::IRawTileSource& source,
    const Binding& binding,
    const Options& options,
    Result& out) noexcept;

} // namespace truthraw::truthnegative_n2_cfa_audit::v0_1
