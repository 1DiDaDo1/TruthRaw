#include "scientific_master_pass_artifact_dispatch_v0_1.h"

#include <string>

namespace truthraw::android_scientific_master_pass_artifact::v0_1 {
namespace {

thread_local Diagnostics gLastDiagnostics{};

void import_exact_gauge_diagnostics(
    const exact_gauge::Diagnostics& source,
    Diagnostics& destination) noexcept {
    destination.artifactType = exact_gauge::kArtifactType;
    destination.artifactVersion = exact_gauge::kArtifactVersion;
    destination.routeUsed = exact_gauge::route_name(source.routeUsed);
    destination.fallbackReason =
        exact_gauge::fallback_reason_name(source.fallbackReason);
    destination.geometricEligibleUpperBound =
        source.geometricEligibleUpperBound;
    destination.eligibleRetainedSamples = source.eligibleRetainedSamples;
    destination.retainedBytesRequested = source.retainedBytesRequested;
    destination.retainedBytesReserved = source.retainedBytesReserved;
    destination.retainedBytesUsed = source.retainedBytesUsed;
    destination.callerBudgetBytes = source.callerBudgetBytes;
    destination.candidateLogicalResidentUpperBound =
        source.candidateLogicalResidentUpperBound;
    destination.stage2GaugeScanPassesActuallyUsed =
        source.stage2GaugeScanPassesActuallyUsed;
    destination.pass2Stage2TileReadsAvoided =
        source.pass2Stage2TileReadsAvoided;
    destination.optimizationApplied = source.optimizationApplied;
    destination.candidateApplied = false;
    destination.sourceValuesModified = source.sourceValuesModified;
    destination.createsNewEvidence = source.createsNewEvidence;
    destination.scientificWritebackAllowed =
        source.scientificWritebackAllowed;
}

smsb2::Status firewall_failure(const char* message) noexcept {
    return smsb2::Status::error(
        smsb2::StatusCode::GaugeFailed,
        std::string("PassArtifact firewall violation: ") + message);
}

}  // namespace

const Descriptor& select_for_observed_scientific_master() noexcept {
    // v0.1 has exactly one registered artifact. Future versions may add
    // descriptor-only eligibility selection here, but execution must remain a
    // single-shot decision before any candidate semantics begin.
    return kRegistry.front();
}

smsb2::Status bind_observed(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const smsb2::Options& options,
    smsb2::ICanonicalTileObserver& observer,
    smsb2::Result& out,
    Diagnostics& diagnostics) noexcept {
    diagnostics = {};
    const Descriptor& selected = select_for_observed_scientific_master();
    diagnostics.selectedArtifact = selected.id;
    diagnostics.artifactType = selected.artifactType;
    diagnostics.artifactVersion = selected.artifactVersion;

    exact_gauge::ArtifactOptions artifactOptions{};
    exact_gauge::Diagnostics exactDiagnostics{};

    // Do not add fallback here. Exact Gauge v0.3 owns the only admitted
    // pre-semantic-start fallback to canonical v0.2. Any failure after v0.3
    // commits to candidate semantics must propagate unchanged and must never
    // replay source/reconstruction/observer work through another route.
    const auto status =
        exact_gauge::bind_scientific_master_retained_exact_gauge_observed(
            source,
            reconstruction,
            options,
            artifactOptions,
            observer,
            out,
            exactDiagnostics);

    import_exact_gauge_diagnostics(exactDiagnostics, diagnostics);
    gLastDiagnostics = diagnostics;

    if (diagnostics.sourceValuesModified) {
        return firewall_failure("source values modified");
    }
    if (diagnostics.createsNewEvidence) {
        return firewall_failure("artifact reported new evidence");
    }
    if (diagnostics.scientificWritebackAllowed) {
        return firewall_failure("scientific writeback reported allowed");
    }
    if (diagnostics.candidateApplied) {
        return firewall_failure("scientific candidate application reported");
    }

    return status;
}

const Diagnostics& last_thread_diagnostics() noexcept {
    return gLastDiagnostics;
}

}  // namespace truthraw::android_scientific_master_pass_artifact::v0_1

namespace truthraw::scientific_master_streaming_binding::v0_2 {

Status bind_scientific_master_streaming_observed_pass_artifact(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const Options& options,
    ICanonicalTileObserver& observer,
    Result& out) noexcept {
    android_scientific_master_pass_artifact::v0_1::Diagnostics diagnostics{};
    return android_scientific_master_pass_artifact::v0_1::bind_observed(
        source,
        reconstruction,
        options,
        observer,
        out,
        diagnostics);
}

}  // namespace truthraw::scientific_master_streaming_binding::v0_2
