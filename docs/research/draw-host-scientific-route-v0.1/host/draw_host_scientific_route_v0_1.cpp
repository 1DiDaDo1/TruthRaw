#include "truthnegative_pipeline_bridge_common.h"

#include "scientific_preview_source_binding_v0_1.h"
#include "truthraw_sha256_v0_69.h"

#include <fcntl.h>
#include <unistd.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

namespace pipeline = truthraw::android_truthnegative_pipeline::v0_1;
namespace source_binding = truthraw::scientific_preview_binding_v0_1;
namespace sha = truthraw::sha256_v0_69;

namespace {

std::string escape_json(const std::string& in) {
    std::ostringstream out;
    for (const unsigned char c : in) {
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (c < 0x20u) {
                    out << "\\u"
                        << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<unsigned>(c)
                        << std::dec << std::setfill(' ');
                } else {
                    out << static_cast<char>(c);
                }
        }
    }
    return out.str();
}

const char* json_bool(bool v) noexcept {
    return v ? "true" : "false";
}

std::size_t parse_mebibytes(const char* text, std::size_t fallback) {
    if (text == nullptr) return fallback;
    char* end = nullptr;
    const unsigned long long value = std::strtoull(text, &end, 10);
    if (end == text || *end != '\0' || value == 0u) return fallback;
    constexpr unsigned long long mib = 1024ull * 1024ull;
    if (value > std::numeric_limits<std::size_t>::max() / mib) return fallback;
    return static_cast<std::size_t>(value * mib);
}

template <std::size_t N>
void print_u64_array(
    const char* key,
    const std::array<std::uint64_t, N>& values,
    bool comma = true) {
    std::cout << "  \"" << key << "\":[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0u) std::cout << ",";
        std::cout << values[i];
    }
    std::cout << "]" << (comma ? "," : "") << "\n";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2 || argc > 4) {
        std::cerr
            << "usage: draw_host_scientific_route_v0_1 SOURCE.dng "
            << "[max_source_mib=64] [max_logical_mib=512]\n";
        return 2;
    }

    const std::size_t maxSource =
        parse_mebibytes(argc >= 3 ? argv[2] : nullptr, 64u * 1024u * 1024u);
    const std::size_t maxLogical =
        parse_mebibytes(argc >= 4 ? argv[3] : nullptr, 512u * 1024u * 1024u);

    const int fd = ::open(argv[1], O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        std::cerr << "{\"schema\":\"D.RAW/HostScientificRouteResult/0.1\","
                  << "\"status\":\"OPEN_FAILED\"}\n";
        return 3;
    }

    pipeline::Context context{};
    const auto status =
        pipeline::prepare(fd, maxSource, maxLogical, context);
    if (!status) {
        std::cerr
            << "{\"schema\":\"D.RAW/HostScientificRouteResult/0.1\","
            << "\"status\":\"PIPELINE_REJECTED\","
            << "\"code\":" << status.code << ","
            << "\"message\":\"" << escape_json(status.message) << "\"}\n";
        ::close(fd);
        return 4;
    }

    const bool sourceReverified = pipeline::reverify(context);
    const auto* dng = context.openedSource.dngAuditSource;
    const auto& descriptor = context.openedSource.descriptor;
    const auto& audit = dng->audit();

    std::cout << std::setprecision(17);
    std::cout << "{\n";
    std::cout << "  \"schema\":\"D.RAW/HostScientificRouteResult/0.1\",\n";
    std::cout << "  \"status\":\"PASS\",\n";
    std::cout << "  \"source_evidence_sha256\":\""
              << sha::hex(context.sourceSeal.sha256) << "\",\n";
    std::cout << "  \"source_byte_length\":" << context.sourceSeal.byteLength << ",\n";
    std::cout << "  \"source_reverified\":" << json_bool(sourceReverified) << ",\n";
    std::cout << "  \"width\":" << context.width << ",\n";
    std::cout << "  \"height\":" << context.height << ",\n";
    std::cout << "  \"colour_binding_id\":\""
              << escape_json(context.produced.color.bindingId) << "\",\n";
    std::cout << "  \"colour_authority\":\""
              << source_binding::authority_name(context.produced.color.authority)
              << "\",\n";
    std::cout << "  \"scientific_master_sha256\":\""
              << sha::hex(context.scientific.scientificMasterHash) << "\",\n";
    std::cout << "  \"authority_field_sha256\":\""
              << sha::hex(context.authorityField.contentSha256) << "\",\n";
    std::cout << "  \"truthnegative_state_sha256\":\""
              << sha::hex(context.truthNegativeState.stateSha256) << "\",\n";
    std::cout << "  \"drawnegative_state_sha256\":\""
              << sha::hex(context.drawNegativeState.stateSha256) << "\",\n";
    std::cout << "  \"observation_id\":\""
              << escape_json(context.drawNegativeState.observationId) << "\",\n";
    std::cout << "  \"scale_gauge_id\":\""
              << escape_json(context.drawNegativeState.scaleGaugeId) << "\",\n";
    std::cout << "  \"common_gauge_admitted\":"
              << json_bool(context.drawNegativeState.commonGaugeAdmitted) << ",\n";
    std::cout << "  \"cross_observation_radiometric_equality_allowed\":"
              << json_bool(context.drawNegativeState.crossObservationRadiometricEqualityAllowed)
              << ",\n";
    std::cout << "  \"cross_observation_radiometric_fusion_allowed\":"
              << json_bool(context.drawNegativeState.crossObservationRadiometricFusionAllowed)
              << ",\n";
    std::cout << "  \"branch_sensitive_compute_float64\":"
              << json_bool(context.drawNegativeState.branchSensitiveComputeFloat64)
              << ",\n";
    std::cout << "  \"zero_line_gauge_id\":\""
              << escape_json(context.scientific.zeroLineGauge.gaugeId) << "\",\n";
    std::cout << "  \"zero_line_l0\":" << context.scientific.zeroLineGauge.L0 << ",\n";
    std::cout << "  \"zero_line_cross_scene_comparable\":"
              << json_bool(context.scientific.zeroLineGauge.crossSceneComparable) << ",\n";
    std::cout << "  \"zero_line_absolute_physical_units\":"
              << json_bool(context.scientific.zeroLineGauge.absolutePhysicalUnits) << ",\n";
    std::cout << "  \"scene_scale_id\":\""
              << escape_json(context.scientific.sceneBinding.sceneScaleId) << "\",\n";
    std::cout << "  \"gain_map_applied_exactly_once\":"
              << json_bool(context.scientific.sceneBinding.gainMapAppliedExactlyOnce) << ",\n";
    std::cout << "  \"exposure_normalized_to_common_scene\":"
              << json_bool(context.scientific.sceneBinding.exposureNormalizedToCommonScene)
              << ",\n";
    std::cout << "  \"gain_normalized_to_common_scene\":"
              << json_bool(context.scientific.sceneBinding.gainNormalizedToCommonScene)
              << ",\n";
    std::cout << "  \"self_gauge_eligible_samples\":"
              << context.scientific.selfGaugeEligibleSamples << ",\n";
    std::cout << "  \"master_tiles_processed\":"
              << context.scientific.masterTilesProcessed << ",\n";
    std::cout << "  \"stage2_gauge_scan_passes\":"
              << context.scientific.stage2GaugeScanPasses << ",\n";
    std::cout << "  \"logical_workspace_peak_bytes\":"
              << context.scientific.logicalWorkspacePeakBytes << ",\n";
    std::cout << "  \"logical_resident_upper_bound\":"
              << context.scientific.logicalResidentUpperBound << ",\n";
    std::cout << "  \"physical_frame_count\":"
              << context.scientific.physicalFrameCount << ",\n";
    std::cout << "  \"independent_evidence_count\":"
              << context.scientific.independentEvidenceCount << ",\n";
    std::cout << "  \"adapter_decoder_id\":\""
              << escape_json(descriptor.decoderId) << "\",\n";
    std::cout << "  \"adapter_exact_cfa_samples_available\":"
              << json_bool(descriptor.exactCfaSamplesAvailable) << ",\n";
    std::cout << "  \"adapter_scientific_admission_ready\":"
              << json_bool(descriptor.scientificAdmissionReady) << ",\n";
    std::cout << "  \"adapter_stored_sample_sensel_semantics_certified\":"
              << json_bool(descriptor.storedSampleSenselSemanticsCertified) << ",\n";
    std::cout << "  \"adapter_direct_sensor_adc_claim_allowed\":"
              << json_bool(descriptor.directSensorAdcClaimAllowed) << ",\n";
    std::cout << "  \"dng_raw_ifd_offset\":" << audit.rawIfdOffset << ",\n";
    std::cout << "  \"dng_strile_count\":" << audit.strileCount << ",\n";
    std::cout << "  \"dng_gain_map_present\":"
              << json_bool(audit.gainMapPresent) << ",\n";
    std::cout << "  \"dng_raw_payload_bytes_read\":"
              << audit.rawPayloadBytesRead << ",\n";
    std::cout << "  \"dng_metadata_bytes_read\":"
              << audit.metadataBytesRead << ",\n";
    std::cout << "  \"authority_record_count\":"
              << context.authorityField.recordCount << ",\n";
    std::cout << "  \"authority_p95_known_count\":"
              << context.authorityField.p95KnownCount << ",\n";
    std::cout << "  \"authority_support_known_count\":"
              << context.authorityField.supportKnownCount << ",\n";
    std::cout << "  \"authority_bound_known_count\":"
              << context.authorityField.boundKnownCount << ",\n";
    print_u64_array("authority_creation_role_counts",
                    context.authorityField.creationRoleCounts);
    print_u64_array("authority_counts",
                    context.authorityField.authorityCounts);
    std::cout << "  \"creates_new_evidence\":"
              << json_bool(context.drawNegativeState.createsNewEvidence) << ",\n";
    std::cout << "  \"scientific_writeback_allowed\":"
              << json_bool(context.drawNegativeState.scientificWritebackAllowed) << "\n";
    std::cout << "}\n";

    ::close(fd);
    return sourceReverified ? 0 : 5;
}
