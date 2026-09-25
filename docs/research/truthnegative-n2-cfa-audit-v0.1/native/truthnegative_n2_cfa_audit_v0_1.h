#pragma once

#include "full_frame_streaming_v0_1.h"
#include "truthnegative_n2_candidate_pipeline_v0_1.h"
#include "truthraw_sha256_v0_69.h"

#include <array>
#include <cstdint>

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
};

struct Result final {
    n2::Audit audit{};
    Digest candidateSha256{};
    Digest auditSha256{};
    std::array<std::uint64_t,4u> cfaPhaseSamples{};
    std::uint64_t borderProtected = 0u;
    std::uint64_t sampled = 0u;
    std::uint32_t samplingPeriod = 0u;
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
