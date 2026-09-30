#pragma once

#include "full_frame_streaming_v0_1.h"
#include "truthnegative_n2_candidate_pipeline_v0_1.h"
#include "truthraw_sha256_v0_69.h"

#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace truthraw::truthnegative_n2_support_distance::v0_1 {

namespace stream = truthraw::streaming_v0_1;
namespace n2 = truthraw::truthnegative_n2_candidate_pipeline::v0_1;
using Digest = truthraw::sha256_v0_69::Digest;

inline constexpr const char* kSchemaName =
    "D.RAW/TruthNegative/N2SampleSupportDistance/0.1";
inline constexpr std::uint32_t kEvaluationTileEdge = 64u;
inline constexpr std::uint32_t kSamplingPeriod = 8u;
inline constexpr std::array<std::uint32_t,4u> kCenterRadiiPx{{8u,16u,32u,64u}};
inline constexpr std::array<std::uint32_t,4u> kRectMarginRadiiPx{{0u,8u,16u,32u}};

struct Binding final {
    Digest sourceEvidenceSha256{};
    Digest scientificMasterSha256{};
    Digest authorityFieldSha256{};
    Digest truthNegativeStateSha256{};
};

struct QueryRegion final {
    std::uint32_t id = 0u;
    std::uint32_t frontsideX = 0u;
    std::uint32_t frontsideY = 0u;
    std::uint32_t frontsideWidth = 0u;
    std::uint32_t frontsideHeight = 0u;
    std::uint32_t left = 0u;
    std::uint32_t top = 0u;
    std::uint32_t right = 0u;   // exclusive
    std::uint32_t bottom = 0u;  // exclusive
};

struct Nearest final {
    double squaredDistance = std::numeric_limits<double>::infinity();
    std::uint32_t x = 0u;
    std::uint32_t y = 0u;
    bool available = false;
};

struct RadiusCounts final {
    std::array<std::uint64_t,4u> sampled{};
    std::array<std::uint64_t,4u> structure{};
    std::array<std::uint64_t,4u> censored{};
    std::array<std::uint64_t,4u> censorBoundary{};
};

struct QueryMetrics final {
    QueryRegion query{};
    RadiusCounts center{};
    RadiusCounts rectMargin{};
    Nearest nearestStructureFromCenter{};
    Nearest nearestCensoredFromCenter{};
    Nearest nearestCensorBoundaryFromCenter{};
    Nearest nearestStructureToRect{};
    Nearest nearestCensoredToRect{};
    Nearest nearestCensorBoundaryToRect{};
};

struct ProtectedPoint final {
    std::uint32_t x = 0u;
    std::uint32_t y = 0u;
};

struct Report final {
    std::string json{};
    Digest jsonSha256{};
    Digest candidateSha256{};
    Digest auditSha256{};
    Digest supportPointStreamSha256{};
    n2::Audit audit{};
    std::array<std::uint64_t,4u> cfaPhaseSamples{};
    std::uint64_t borderProtected = 0u;
    std::uint64_t sampled = 0u;
    std::vector<ProtectedPoint> structurePoints{};
    std::vector<ProtectedPoint> censoredPoints{};
    std::vector<ProtectedPoint> censorBoundaryPoints{};
    std::vector<QueryMetrics> queries{};
    bool exactSampleCoordinatesRecorded = true;
    bool unsampledPixelsInferred = false;
    bool scalarProbabilityCreated = false;
    bool canReduceProtection = false;
    bool canEnableCorrection = false;
    bool candidateApplied = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

bool run(
    stream::IRawTileSource& source,
    const Binding& binding,
    const std::vector<QueryRegion>& queries,
    Report& out) noexcept;

} // namespace truthraw::truthnegative_n2_support_distance::v0_1
