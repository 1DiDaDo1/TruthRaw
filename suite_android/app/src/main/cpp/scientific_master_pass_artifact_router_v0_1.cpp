#include "scientific_master_pass_artifact_router_v0_1.h"

namespace truthraw::scientific_master_pass_artifact_router::v0_1 {

smsb2::Status bindObserved(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const smsb2::Options& options,
    smsb2::ICanonicalTileObserver& observer,
    smsb2::Result& out,
    Diagnostics& diagnostics) noexcept {
    diagnostics = {};

    ega3::Diagnostics cableDiagnostics{};
    const ega3::ArtifactOptions artifactOptions{};
    const auto status = ega3::bind_scientific_master_retained_exact_gauge_observed(
        source,
        reconstruction,
        options,
        artifactOptions,
        observer,
        out,
        cableDiagnostics);

    diagnostics.routeName = ega3::route_name(cableDiagnostics.routeUsed);
    diagnostics.fallbackReasonName =
        ega3::fallback_reason_name(cableDiagnostics.fallbackReason);
    diagnostics.eligibleRetainedSamples =
        cableDiagnostics.eligibleRetainedSamples;
    diagnostics.retainedBytesUsed = cableDiagnostics.retainedBytesUsed;
    diagnostics.stage2GaugeScanPassesActuallyUsed =
        cableDiagnostics.stage2GaugeScanPassesActuallyUsed;
    diagnostics.pass2Stage2TileReadsAvoided =
        cableDiagnostics.pass2Stage2TileReadsAvoided;
    diagnostics.optimizationApplied = cableDiagnostics.optimizationApplied;
    diagnostics.sourceValuesModified = cableDiagnostics.sourceValuesModified;
    diagnostics.createsNewEvidence = cableDiagnostics.createsNewEvidence;
    diagnostics.scientificWritebackAllowed =
        cableDiagnostics.scientificWritebackAllowed;

    if (!status) return status;

    if (diagnostics.sourceValuesModified ||
        diagnostics.createsNewEvidence ||
        diagnostics.scientificWritebackAllowed ||
        diagnostics.isScientificEvidence ||
        diagnostics.mayChangeScientificAuthority) {
        return smsb2::Status::error(
            smsb2::StatusCode::DigestFailed,
            "pass-artifact cable violated non-authoritative router contract");
    }

    return status;
}

}  // namespace truthraw::scientific_master_pass_artifact_router::v0_1
