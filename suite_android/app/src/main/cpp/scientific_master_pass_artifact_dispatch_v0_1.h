#pragma once

#include "scientific_master_exact_gauge_retained_artifact_v0_3.h"
#include "scientific_master_streaming_binding_v0_2.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace truthraw::android_scientific_master_pass_artifact::v0_1 {

namespace smsb2 = scientific_master_streaming_binding::v0_2;
namespace exact_gauge = scientific_master_exact_gauge_retained_artifact::v0_3;

enum class ArtifactId : std::uint8_t {
    ExactGaugeRetainedV03 = 0,
};

struct Descriptor final {
    ArtifactId id = ArtifactId::ExactGaugeRetainedV03;
    const char* artifactType = exact_gauge::kArtifactType;
    const char* artifactVersion = exact_gauge::kArtifactVersion;
    const char* producerSemanticVersion = exact_gauge::kProducerSemanticVersion;
    const char* consumerSemanticVersion = exact_gauge::kConsumerSemanticVersion;
    const char* eligibilityContractId = exact_gauge::kEligibilityContractId;
};

inline constexpr std::array<Descriptor, 1> kRegistry{{
    Descriptor{},
}};

struct Diagnostics final {
    ArtifactId selectedArtifact = ArtifactId::ExactGaugeRetainedV03;
    const char* artifactType = exact_gauge::kArtifactType;
    const char* artifactVersion = exact_gauge::kArtifactVersion;
    const char* routeUsed = "UNAVAILABLE";
    const char* fallbackReason = "UNAVAILABLE";

    std::uint64_t geometricEligibleUpperBound = 0u;
    std::uint64_t eligibleRetainedSamples = 0u;
    std::size_t retainedBytesRequested = 0u;
    std::size_t retainedBytesReserved = 0u;
    std::size_t retainedBytesUsed = 0u;
    std::size_t callerBudgetBytes = 0u;
    std::size_t candidateLogicalResidentUpperBound = 0u;
    std::size_t stage2GaugeScanPassesActuallyUsed = 0u;
    std::size_t pass2Stage2TileReadsAvoided = 0u;

    bool optimizationApplied = false;
    bool candidateApplied = false;
    bool sourceValuesModified = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

// Selection is deliberately non-executing. Exactly one registered artifact is
// selected before semantic processing begins. The dispatcher must never try a
// second artifact after invoking the selected implementation: only the selected
// artifact itself may take its explicitly proven pre-admission canonical
// fallback. This preserves the v0.3 no-replay boundary after semantic start.
const Descriptor& select_for_observed_scientific_master() noexcept;

smsb2::Status bind_observed(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const smsb2::Options& options,
    smsb2::ICanonicalTileObserver& observer,
    smsb2::Result& out,
    Diagnostics& diagnostics) noexcept;

// Diagnostic-only thread-local snapshot of the most recent dispatch on the
// calling native worker thread. It is never evidence, authority or writeback.
const Diagnostics& last_thread_diagnostics() noexcept;

}  // namespace truthraw::android_scientific_master_pass_artifact::v0_1

namespace truthraw::scientific_master_streaming_binding::v0_2 {

// ABI-local Android integration shim. It deliberately has the canonical
// observed-binder signature so only the Android Foundation preparation callsite
// needs to be redirected; the sealed v0.2 binder itself stays unchanged.
Status bind_scientific_master_streaming_observed_pass_artifact(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const Options& options,
    ICanonicalTileObserver& observer,
    Result& out) noexcept;

}  // namespace truthraw::scientific_master_streaming_binding::v0_2
