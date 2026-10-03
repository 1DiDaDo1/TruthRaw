#pragma once

#include "scientific_master_streaming_binding_v0_2.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace truthraw::scientific_master_exact_gauge_retained_artifact::v0_3 {

namespace smsb2 = scientific_master_streaming_binding::v0_2;

using Hash256 = smsb2::Hash256;
using StatusCode = smsb2::StatusCode;
using Status = smsb2::Status;
using Options = smsb2::Options;
using Result = smsb2::Result;
using ICanonicalTileObserver = smsb2::ICanonicalTileObserver;

inline constexpr const char* kArtifactType = "EXACT_GAUGE_FLOAT32_BITS";
inline constexpr const char* kArtifactVersion = "0.3";
inline constexpr const char* kProducerSemanticVersion = "SMSB_V0_2_STAGE2_SELF_GAUGE";
inline constexpr const char* kConsumerSemanticVersion = "EXACT_GAUGE_LOW16_V0_3";
inline constexpr const char* kEligibilityContractId =
    "SMSB_V0_2_POSITIVE_FINITE_RAW_LT_WHITE_CENTER80";

struct ArtifactOptions final {
    // 0 means no separate artifact-only ceiling. The established Options::
    // memoryBudgetBytes remains the total scientific logical resident ceiling.
    std::size_t retainedArtifactBudgetBytes = 0u;
};

enum class RouteUsed : std::uint8_t {
    RetainedExactFloat32Bits = 0,
    CanonicalV02Fallback,
};

enum class FallbackReason : std::uint8_t {
    None = 0,
    GeometricBoundOverflow,
    RetainedByteBoundOverflow,
    MemoryBudgetInsufficient,
    // Admission-policy subtype. It deliberately aliases the existing memory
    // reason value so the initial scalar candidate does not silently widen the
    // persisted diagnostic enum ABI before Android integration. The dedicated
    // artifact budget remains separately visible in Diagnostics.
    RetainedArtifactBudgetInsufficient = MemoryBudgetInsufficient,
    ArtifactAllocationFailed,
    ArtifactCardinalityExceeded,
    CandidateResidentBudgetExceeded,
};

struct Diagnostics final {
    RouteUsed routeUsed = RouteUsed::CanonicalV02Fallback;
    FallbackReason fallbackReason = FallbackReason::None;

    std::string artifactType = kArtifactType;
    std::string artifactVersion = kArtifactVersion;
    std::string producerSemanticVersion = kProducerSemanticVersion;
    std::string consumerSemanticVersion = kConsumerSemanticVersion;
    std::string eligibilityContractId = kEligibilityContractId;

    std::uint64_t geometricEligibleUpperBound = 0;
    std::uint64_t eligibleRetainedSamples = 0;
    std::size_t retainedBytesRequested = 0;
    std::size_t retainedBytesReserved = 0;
    std::size_t retainedBytesUsed = 0;
    std::size_t retainedArtifactBudgetBytes = 0;
    std::size_t callerBudgetBytes = 0;
    std::size_t candidateLogicalResidentUpperBound = 0;

    std::uint32_t lowerMedianFloat32Bits = 0;
    std::uint32_t upperMedianFloat32Bits = 0;
    std::size_t stage2GaugeScanPassesActuallyUsed = 0;
    std::size_t pass2Stage2TileReadsAvoided = 0;

    bool optimizationApplied = false;
    bool sourceValuesModified = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

Status bind_scientific_master_retained_exact_gauge(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const Options& options,
    const ArtifactOptions& artifactOptions,
    Result& out,
    Diagnostics& diagnostics) noexcept;

Status bind_scientific_master_retained_exact_gauge(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const Options& options,
    Result& out,
    Diagnostics& diagnostics) noexcept;

Status bind_scientific_master_retained_exact_gauge_observed(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const Options& options,
    const ArtifactOptions& artifactOptions,
    ICanonicalTileObserver& observer,
    Result& out,
    Diagnostics& diagnostics) noexcept;

Status bind_scientific_master_retained_exact_gauge_observed(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const Options& options,
    ICanonicalTileObserver& observer,
    Result& out,
    Diagnostics& diagnostics) noexcept;

const char* route_name(RouteUsed route) noexcept;
const char* fallback_reason_name(FallbackReason reason) noexcept;

}  // namespace truthraw::scientific_master_exact_gauge_retained_artifact::v0_3
