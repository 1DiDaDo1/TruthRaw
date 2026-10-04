#include "scientific_master_pass_artifact_dispatch_v0_1.h"

#include <mutex>
#include <sstream>
#include <string>

namespace truthraw::android_scientific_master_pass_artifact::v0_1 {
namespace {

thread_local Diagnostics gLastDiagnostics{};
std::mutex gBoundDiagnosticsMutex;
smsb2::Hash256 gBoundScientificMasterHash{};
Diagnostics gBoundDiagnostics{};
bool gBoundDiagnosticsAvailable = false;

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

void publish_bound_diagnostics(
    const smsb2::Hash256& scientificMasterHash,
    const Diagnostics& diagnostics) noexcept {
    try {
        std::lock_guard<std::mutex> guard(gBoundDiagnosticsMutex);
        gBoundScientificMasterHash = scientificMasterHash;
        gBoundDiagnostics = diagnostics;
        gBoundDiagnosticsAvailable = true;
    } catch (...) {
        // Telemetry is diagnostic-only. Locking failure must never alter the
        // scientific result or trigger a second scientific route.
        gBoundDiagnosticsAvailable = false;
    }
}

void append_json_string(std::ostringstream& out, const char* value) {
    out << '\"';
    const char* p = value != nullptr ? value : "";
    while (*p != '\0') {
        const unsigned char c = static_cast<unsigned char>(*p++);
        switch (c) {
            case '\"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (c >= 0x20u) out << static_cast<char>(c);
                break;
        }
    }
    out << '\"';
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
    if (!status) {
        return status;
    }

    // Publish only after a successful, firewall-clean scientific bind. The
    // Scientific-Master hash is the binding key, so later JNI consumers can
    // never attach this snapshot to a different prepared source/context.
    publish_bound_diagnostics(out.scientificMasterHash, diagnostics);
    return status;
}

const Diagnostics& last_thread_diagnostics() noexcept {
    return gLastDiagnostics;
}

std::string bound_diagnostics_json(
    const smsb2::Hash256& scientificMasterHash) noexcept {
    static constexpr const char* kUnavailable =
        "{\"schema\":\"D.RAW/ScientificMasterPassArtifactDiagnostics/0.1\","
        "\"available\":false,\"binding_verified\":false,"
        "\"authority\":\"DIAGNOSTIC_RUNTIME_ONLY\","
        "\"creates_new_evidence\":false,"
        "\"scientific_writeback_allowed\":false}";
    try {
        std::lock_guard<std::mutex> guard(gBoundDiagnosticsMutex);
        if (!gBoundDiagnosticsAvailable ||
            scientificMasterHash != gBoundScientificMasterHash) {
            return kUnavailable;
        }

        const Diagnostics& d = gBoundDiagnostics;
        std::ostringstream out;
        out << "{\"schema\":\"D.RAW/ScientificMasterPassArtifactDiagnostics/0.1\"";
        out << ",\"available\":true";
        out << ",\"binding_verified\":true";
        out << ",\"binding\":\"SCIENTIFIC_MASTER_SHA256\"";
        out << ",\"selected_artifact_id\":"
            << static_cast<unsigned int>(d.selectedArtifact);
        out << ",\"artifact_type\":";
        append_json_string(out, d.artifactType);
        out << ",\"artifact_version\":";
        append_json_string(out, d.artifactVersion);
        out << ",\"route_used\":";
        append_json_string(out, d.routeUsed);
        out << ",\"fallback_reason\":";
        append_json_string(out, d.fallbackReason);
        out << ",\"geometric_eligible_upper_bound\":"
            << d.geometricEligibleUpperBound;
        out << ",\"eligible_retained_samples\":"
            << d.eligibleRetainedSamples;
        out << ",\"retained_bytes_requested\":"
            << d.retainedBytesRequested;
        out << ",\"retained_bytes_reserved\":"
            << d.retainedBytesReserved;
        out << ",\"retained_bytes_used\":"
            << d.retainedBytesUsed;
        out << ",\"caller_budget_bytes\":"
            << d.callerBudgetBytes;
        out << ",\"candidate_logical_resident_upper_bound\":"
            << d.candidateLogicalResidentUpperBound;
        out << ",\"stage2_gauge_scan_passes_actually_used\":"
            << d.stage2GaugeScanPassesActuallyUsed;
        out << ",\"pass2_stage2_tile_reads_avoided\":"
            << d.pass2Stage2TileReadsAvoided;
        out << ",\"optimization_applied\":"
            << (d.optimizationApplied ? "true" : "false");
        out << ",\"candidate_applied\":"
            << (d.candidateApplied ? "true" : "false");
        out << ",\"source_values_modified\":"
            << (d.sourceValuesModified ? "true" : "false");
        out << ",\"creates_new_evidence\":"
            << (d.createsNewEvidence ? "true" : "false");
        out << ",\"scientific_writeback_allowed\":"
            << (d.scientificWritebackAllowed ? "true" : "false");
        out << ",\"authority\":\"DIAGNOSTIC_RUNTIME_ONLY\"}";
        return out.str();
    } catch (...) {
        return kUnavailable;
    }
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
