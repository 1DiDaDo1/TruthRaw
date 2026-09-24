#include <jni.h>

#include "dng_color_binding_producer_v0_2.h"
#include "raw_source_adapter_bridge_common.h"
#include "open_scene_canonical_v0_70.h"
#include "open_scene_field_v0_85.h"
#include "truthnegative_local_authority_projection_v0_4.h"
#include "scientific_master_digest_v0_1.h"
#include "scientific_master_linear_dng_projection_v0_1.h"
#include "scientific_master_streaming_binding_v0_2.h"
#include "scientific_preview_source_binding_v0_1.h"
#include "scientific_preview_source_binding_v0_2.h"
#include "streaming_scientific_master_tile_source_v0_1.h"
#include "technical_backplane_phase2_v0_1.h"
#include "technical_backplane_v0_1.h"
#include "tile_native_dng_source_v0_1.h"
#include "truthraw/core.h"
#include "scientific_master_f64_reconstruction_v0_1.h"
#include "truthraw_sha256_v0_69.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <unistd.h>
#include <vector>

namespace {

using truthraw::CfaPattern;
using truthraw::scientific_master_f64_reconstruction_v0_1::ResearchEdgeAwareMeasuredPreservingReconstructionF64;
using truthraw::TileRect;
using truthraw::dng_color_binding_producer_v0_2::ProducerResult;
using truthraw::scientific_preview_binding_v0_1::ColorClaimScope;
using truthraw::scientific_preview_binding_v0_1::SourceSeal;
using truthraw::scientific_preview_binding_v0_2::PreparedScientificPreviewSource;
using truthraw::tile_dng_v0_1::PosixFdByteSource;

namespace master_projection = truthraw::scientific_master_linear_dng_projection::v0_1;
namespace adapter = truthraw::multivendor_raw_source_adapter::v0_1;
namespace digest = truthraw::scientific_master_digest::v0_1;
namespace canonical_scene = truthraw::open_scene_canonical::v0_70;
namespace field85 = truthraw::open_scene_field::v0_85;
namespace dense_field_v04 = truthraw::truthnegative_local_authority_projection::v0_4;

constexpr jlong kMagic = 0x54524e47; // TRNG
constexpr std::size_t kPacketLongs = 28u;
constexpr std::size_t kHeaderBytes = 8192u;
constexpr std::uint32_t kCellEdge = 64u;

enum class AuthorityByte : std::uint8_t {
    CalibratedEstimate = 1,
    Reconstructed = 2,
    Censored = 3,
    Unknown = 4,
};

bool write_all(int fd, const std::uint8_t* data, std::size_t size) noexcept {
    std::size_t offset = 0u;
    while (offset < size) {
        const ssize_t n = ::write(fd, data + offset, size - offset);
        if (n <= 0) return false;
        offset += static_cast<std::size_t>(n);
    }
    return true;
}

void append_u32_le(std::vector<std::uint8_t>& out, std::uint32_t v) {
    out.push_back(static_cast<std::uint8_t>(v & 0xffu));
    out.push_back(static_cast<std::uint8_t>((v >> 8u) & 0xffu));
    out.push_back(static_cast<std::uint8_t>((v >> 16u) & 0xffu));
    out.push_back(static_cast<std::uint8_t>((v >> 24u) & 0xffu));
}

std::string hex_bytes(std::span<const std::uint8_t> bytes) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out(bytes.size() * 2u, '0');
    for (std::size_t i = 0u; i < bytes.size(); ++i) {
        out[2u * i] = kHex[(bytes[i] >> 4u) & 0x0fu];
        out[2u * i + 1u] = kHex[bytes[i] & 0x0fu];
    }
    return out;
}

std::uint64_t f64_bits(double v) noexcept {
    return std::bit_cast<std::uint64_t>(v);
}

std::string hex_u64(std::uint64_t v) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out(16u, '0');
    for (std::size_t i = 0u; i < 16u; ++i) {
        const unsigned shift = static_cast<unsigned>((15u - i) * 4u);
        out[i] = kHex[(v >> shift) & 0x0fu];
    }
    return out;
}

int measured_channel(CfaPattern cfa, int x, int y) noexcept {
    const bool xe = (x & 1) == 0;
    const bool ye = (y & 1) == 0;
    switch (cfa) {
        case CfaPattern::BGGR:
            if (ye && xe) return 2;
            if (!ye && !xe) return 0;
            return 1;
        case CfaPattern::RGGB:
            if (ye && xe) return 0;
            if (!ye && !xe) return 2;
            return 1;
        case CfaPattern::GRBG:
            if (ye && !xe) return 0;
            if (!ye && xe) return 2;
            return 1;
        case CfaPattern::GBRG:
            if (!ye && xe) return 0;
            if (ye && !xe) return 2;
            return 1;
    }
    return -1;
}

jlong clamp_jlong(std::uint64_t value) noexcept {
    const auto cap = static_cast<std::uint64_t>(std::numeric_limits<jlong>::max());
    return static_cast<jlong>(std::min(value, cap));
}

jlongArray packet(JNIEnv* env, jlong status) {
    std::array<jlong, kPacketLongs> values{};
    values[0] = kMagic;
    values[1] = status;
    auto out = env->NewLongArray(static_cast<jsize>(values.size()));
    if (out != nullptr) {
        env->SetLongArrayRegion(out, 0, static_cast<jsize>(values.size()), values.data());
    }
    return out;
}

jlong binding_status(const truthraw::scientific_preview_binding_v0_1::BindingStatus& s) {
    return 2000 + static_cast<jlong>(s.code);
}

jlong producer_status(const truthraw::dng_color_binding_producer_v0_2::ProducerStatus& s) {
    return 2100 + static_cast<jlong>(s.code);
}

jlong adapter_status(const adapter::AdapterStatus& s) {
    return 7000 + static_cast<jlong>(s.code);
}

jlong science_status(const truthraw::scientific_master_streaming_binding::v0_2::Status& s) {
    return 8000 + static_cast<jlong>(s.code);
}

jlong phase2_status(const truthraw::technical_backplane_phase2::v0_1::Status& s) {
    return 9000 + static_cast<jlong>(s.code);
}

bool write_header(
    int fd,
    const SourceSeal& sourceSeal,
    const truthraw::scientific_master_streaming_binding::v0_2::Result& scientific,
    const truthraw::technical_backplane_phase2::v0_1::Phase2Result& phase2,
    int width,
    int height,
    truthraw::Orientation orientation,
    std::uint64_t cellCount,
    std::uint64_t calibrated,
    std::uint64_t reconstructed,
    std::uint64_t censored,
    std::uint64_t unknown,
    std::uint64_t payloadBytes,
    std::uint64_t authorityBytes,
    std::uint64_t openSceneBytes,
    std::uint64_t openSceneFinitePixels,
    std::uint64_t openSceneCensoredPixels,
    std::uint64_t openSceneFieldStorageBytes,
    const canonical_scene::Summary& openSceneSummary,
    const field85::Summary& openSceneFieldSummary) noexcept {
    const auto sourceHex = hex_bytes(sourceSeal.sha256);
    const auto masterHex = digest::to_hex(scientific.scientificMasterHash);
    const auto zeroHex = hex_bytes(phase2.zeroLineHash);
    const auto sceneHex = hex_bytes(phase2.sceneScaleHash);
    const auto backplaneHex = hex_bytes(phase2.serializedBackplane);

    std::string text;
    text.reserve(6800u);
    text += "magic=TRUTHNEGATIVE_V0_4_TN4\n";
    text += "container_version=4\n";
    text += "role=TRUTHNEGATIVE_TN4_OPEN_SCENE_LOCAL_AUTHORITY_SCIENTIFIC_NEGATIVE\n";
    text += "pixel_role=CAMERA_NATIVE_SCIENTIFIC_MASTER_RGB\n";
    text += "sample_encoding=IEEE754_BINARY32_LE\n";
    text += "layout=CANONICAL_64X64_CELL_SEQUENCE_RGB_AUTHORITY_OPEN_SCENE_OPEN_SCENE_FIELD_V085\n";
    text += "width=" + std::to_string(width) + "\n";
    text += "height=" + std::to_string(height) + "\n";
    text += "channels=3\n";
    text += "orientation=" + std::to_string(static_cast<int>(orientation)) + "\n";
    text += "header_bytes=" + std::to_string(kHeaderBytes) + "\n";
    text += "cell_count=" + std::to_string(cellCount) + "\n";
    text += "source_sha256=" + sourceHex + "\n";
    text += "scientific_master_sha256=" + masterHex + "\n";
    text += "zero_line_sha256=" + zeroHex + "\n";
    text += "zero_line_l0_f64_bits=0x" + hex_u64(f64_bits(scientific.zeroLineGauge.L0)) + "\n";
    text += "scene_scale_sha256=" + sceneHex + "\n";
    text += "technical_backplane_serialized_hex=" + backplaneHex + "\n";
    text += "dynamic_authority_schema=TRUTHRAW_DYNAMIC_AUTHORITY_GENERIC_FAIL_CLOSED_V0_69\n";
    text += "open_scene_state_schema=TRUTHRAW_OPEN_SCENE_FULLFRAME_DENSE_V0_69\n";
    text += "open_scene_canonical_schema=" + std::string(canonical_scene::schema_name()) + "\n";
    text += "open_scene_semantic_parent_region=" + std::string(canonical_scene::semantic_parent_region()) + "\n";
    text += "open_scene_semantic_parent_stream=" + std::string(canonical_scene::semantic_parent_stream()) + "\n";
    text += "dynamic_authority_artifact_sha256=" +
        truthraw::sha256_v0_69::hex(openSceneSummary.dynamicAuthoritySha256) + "\n";
    text += "open_scene_state_sha256=" +
        truthraw::sha256_v0_69::hex(openSceneSummary.contentSha256) + "\n";
    text += "open_scene_policy_sha256=" +
        truthraw::sha256_v0_69::hex(openSceneSummary.policySha256) + "\n";
    text += "open_scene_artifact_sha256=" +
        truthraw::sha256_v0_69::hex(openSceneSummary.artifactSha256) + "\n";
    text += "open_scene_field_schema=" + std::string(field85::schema_name()) + "\n";
    text += "open_scene_field_content_sha256=" +
        truthraw::sha256_v0_69::hex(openSceneFieldSummary.contentSha256) + "\n";
    text += "open_scene_field_encoding_sha256=" +
        truthraw::sha256_v0_69::hex(openSceneFieldSummary.encodingSha256) + "\n";
    text += "open_scene_field_policy_sha256=" +
        truthraw::sha256_v0_69::hex(openSceneFieldSummary.policySha256) + "\n";
    text += "open_scene_field_artifact_sha256=" +
        truthraw::sha256_v0_69::hex(openSceneFieldSummary.artifactSha256) + "\n";
    text += "open_scene_field_storage_bytes=" +
        std::to_string(openSceneFieldStorageBytes) + "\n";
    text += "open_scene_field_encoded_bytes=" +
        std::to_string(openSceneFieldSummary.encodedBytes) + "\n";
    text += "open_scene_field_record_count=" +
        std::to_string(openSceneFieldSummary.recordCount) + "\n";
    text += "open_scene_field_tile_count=" +
        std::to_string(openSceneFieldSummary.tileCount) + "\n";
    text += "open_scene_field_role_source_measured_cfa=" +
        std::to_string(openSceneFieldSummary.creationRoleCounts[1]) + "\n";
    text += "open_scene_field_role_scientific_reconstruction=" +
        std::to_string(openSceneFieldSummary.creationRoleCounts[2]) + "\n";
    text += "open_scene_field_role_dense_projection=" +
        std::to_string(openSceneFieldSummary.creationRoleCounts[3]) + "\n";
    text += "open_scene_field_authority_calibrated_estimate=" +
        std::to_string(openSceneFieldSummary.authorityCounts[0]) + "\n";
    text += "open_scene_field_authority_reconstructed=" +
        std::to_string(openSceneFieldSummary.authorityCounts[1]) + "\n";
    text += "open_scene_field_authority_censored=" +
        std::to_string(openSceneFieldSummary.authorityCounts[2]) + "\n";
    text += "open_scene_field_authority_unknown=" +
        std::to_string(openSceneFieldSummary.authorityCounts[3]) + "\n";
    text += "open_scene_field_p95_known_count=" +
        std::to_string(openSceneFieldSummary.p95KnownCount) + "\n";
    text += "open_scene_field_support_known_count=" +
        std::to_string(openSceneFieldSummary.supportKnownCount) + "\n";
    text += "open_scene_field_bound_known_count=" +
        std::to_string(openSceneFieldSummary.boundKnownCount) + "\n";
    text += "open_scene_field_per_pixel_per_channel_authority=1\n";
    text += "open_scene_field_per_pixel_per_channel_uncertainty=1\n";
    text += "open_scene_field_per_pixel_per_channel_bounds=1\n";
    text += "open_scene_field_encoding_changes_scientific_identity=0\n";
    text += "open_scene_field_creates_new_evidence=0\n";
    text += "open_scene_field_scientific_writeback_allowed=0\n";
    text += "truthnegative_local_authority_projection_schema=" +
        std::string(dense_field_v04::schema_name()) + "\n";
    text += "truthnegative_local_authority_projection_policy=" +
        std::string(dense_field_v04::policy_name()) + "\n";
    text += "truthnegative_local_authority_projection_applied_to_native_grid=0\n";
    text += "open_scene_state_bytes=" + std::to_string(openSceneBytes) + "\n";
    text += "open_scene_finite_pixels=" + std::to_string(openSceneFinitePixels) + "\n";
    text += "open_scene_censored_pixels=" + std::to_string(openSceneCensoredPixels) + "\n";
    text += "open_scene_state_1=R_CALIBRATED_ESTIMATE__G_UNKNOWN__B_UNKNOWN\n";
    text += "open_scene_state_2=R_UNKNOWN__G_CALIBRATED_ESTIMATE__B_UNKNOWN\n";
    text += "open_scene_state_3=R_UNKNOWN__G_UNKNOWN__B_CALIBRATED_ESTIMATE\n";
    text += "open_scene_state_4=R_CENSORED__G_UNKNOWN__B_UNKNOWN\n";
    text += "open_scene_state_5=R_UNKNOWN__G_CENSORED__B_UNKNOWN\n";
    text += "open_scene_state_6=R_UNKNOWN__G_UNKNOWN__B_CENSORED\n";
    text += "open_scene_colour_authority=SOURCE_METADATA_BOUND\n";
    text += "open_scene_illumination_authority=SOURCE_BOUND_ESTIMATE\n";
    text += "open_scene_detail_status=NEUTRAL_OR_BLOCKED\n";
    text += "open_scene_chunking_changes_scientific_identity=0\n";
    text += "open_scene_counterfactual_pixels=" +
        std::to_string(openSceneSummary.counterfactualPixelCount) + "\n";
    text += "open_scene_scientific_master_writeback_pixels=" +
        std::to_string(openSceneSummary.scientificWritebackPixelCount) + "\n";
    text += "open_scene_scientific_master_writeback_allowed=0\n";
    text += "open_scene_creates_new_evidence=0\n";
    text += "authority_calibrated_estimate=" + std::to_string(calibrated) + "\n";
    text += "authority_reconstructed=" + std::to_string(reconstructed) + "\n";
    text += "authority_censored=" + std::to_string(censored) + "\n";
    text += "authority_unknown=" + std::to_string(unknown) + "\n";
    text += "authority_counterfactual=0\n";
    text += "authority_appearance_only=0\n";
    text += "generic_missing_channel_policy=UNKNOWN_UNTIL_SOURCE_BOUND_UNCERTAINTY_IS_ADMITTED\n";
    text += "source_direct_sample_policy=CALIBRATED_ESTIMATE_UNLESS_CENSORED\n";
    text += "creates_new_evidence=0\n";
    text += "creates_second_scientific_world=0\n";
    text += "implies_physical_sensor_geometry=0\n";
    text += "scientific_master_modified=0\n";
    text += "appearance_applied=0\n";
    text += "counterfactual_observation_created=0\n";
    text += "physical_frame_count=1\n";
    text += "independent_evidence_count=1\n";
    text += "payload_bytes=" + std::to_string(payloadBytes) + "\n";
    text += "authority_bytes=" + std::to_string(authorityBytes) + "\n";
    text += "tn3_legacy_open_scene_state_retained=1\n";
    text += "tn4_open_scene_field_v085_bound=1\n";
    text += "END_HEADER\n";

    if (text.size() > kHeaderBytes) return false;
    std::array<std::uint8_t, kHeaderBytes> header{};
    std::copy(text.begin(), text.end(), header.begin());
    if (::lseek(fd, 0, SEEK_SET) < 0) return false;
    return write_all(fd, header.data(), header.size());
}

} // namespace

extern "C" JNIEXPORT jlongArray JNICALL
Java_com_truthraw_adaptiveui_TruthNegativeNativeBridge_exportTruthNegative(
    JNIEnv* env,
    jobject,
    jint sourceFd,
    jint outputFd,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    if (sourceFd < 0 || outputFd < 0 ||
        maxSourceResidentBytes <= 0 || maxLogicalResidentBytes <= 0) {
        return packet(env, -1);
    }
    if (::ftruncate(outputFd, 0) != 0 || ::lseek(outputFd, 0, SEEK_SET) < 0) {
        return packet(env, -2);
    }

    auto bytes = std::make_shared<PosixFdByteSource>(static_cast<int>(sourceFd));

    SourceSeal sourceSeal;
    const auto sealed =
        truthraw::scientific_preview_binding_v0_1::seal_source_sha256(*bytes, sourceSeal);
    if (!sealed) return packet(env, binding_status(sealed));

    ProducerResult produced;
    const auto colorStatus =
        truthraw::dng_color_binding_producer_v0_2::produce_source_metadata_color_binding(
            *bytes, sourceSeal, produced);
    if (!colorStatus) return packet(env, producer_status(colorStatus));

    PreparedScientificPreviewSource prepared;
    const auto preparedStatus =
        truthraw::scientific_preview_binding_v0_2::prepare_scientific_color_source(
            sourceSeal, produced.color, prepared);
    if (!preparedStatus) return packet(env, binding_status(preparedStatus));

    if (!prepared.mainHouseComputeAllowed ||
        prepared.physicalFrameCount != 1u ||
        prepared.independentEvidenceCount != 1u) {
        return packet(env, -3);
    }

    const auto preVerified =
        truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes, sourceSeal);
    if (!preVerified) return packet(env, binding_status(preVerified));

    auto openOptions = prepared.tileNativeOptions;
    openOptions.maxResidentBytes = static_cast<std::size_t>(maxSourceResidentBytes);

    truthraw::android_raw_adapter_bridge::v0_1::OpenedDngSource openedSource;
    const auto opened = truthraw::android_raw_adapter_bridge::v0_1::openDngViaAdapter(
        bytes, sourceSeal, openOptions, openedSource);
    if (!opened) return packet(env, adapter_status(opened));
    auto& source = openedSource.source;

    auto reconstruction =
        std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstructionF64>();

    truthraw::scientific_master_streaming_binding::v0_2::Options scientificOptions;
    scientificOptions.memoryBudgetBytes = static_cast<std::size_t>(maxLogicalResidentBytes);
    truthraw::scientific_master_streaming_binding::v0_2::Result scientific;
    const auto scientificStatus =
        truthraw::scientific_master_streaming_binding::v0_2::bind_scientific_master_streaming(
            *source, *reconstruction, scientificOptions, scientific);
    if (!scientificStatus) return packet(env, science_status(scientificStatus));

    truthraw::technical_backplane_phase2::v0_1::Phase2Input phaseInput;
    phaseInput.prepared = prepared;
    phaseInput.scientificMasterHash = scientific.scientificMasterHash;
    phaseInput.zeroLineGauge = scientific.zeroLineGauge;
    phaseInput.sceneBinding = scientific.sceneBinding;
    phaseInput.roomStatus.fill(
        truthraw::technical_backplane::v0_1::RoomStatus::ResearchOnly);
    phaseInput.claimStatus = truthraw::technical_backplane::v0_1::ClaimStatus::Candidate;

    truthraw::technical_backplane_phase2::v0_1::Phase2Result phase2;
    const auto phaseStatus =
        truthraw::technical_backplane_phase2::v0_1::finalize_phase2(phaseInput, phase2);
    if (!phaseStatus) return packet(env, phase2_status(phaseStatus));

    if (phase2.admission.claimScope == ColorClaimScope::None ||
        phase2.backplane.sourceEvidenceHash != sourceSeal.sha256 ||
        phase2.backplane.scientificMasterHash != scientific.scientificMasterHash ||
        phase2.backplane.forbiddenFlags != 0u ||
        phase2.backplane.physicalFrameCount != 1u ||
        phase2.backplane.independentEvidenceCount != 1u) {
        return packet(env, -4);
    }

    master_projection::StreamingScientificMasterTileSource masterSource(
        *source, *reconstruction);

    const int width = source->metadata().width;
    const int height = source->metadata().height;
    if (width <= 0 || height <= 0) return packet(env, -5);

    std::array<std::uint8_t, kHeaderBytes> blankHeader{};
    if (!write_all(outputFd, blankHeader.data(), blankHeader.size())) {
        (void)::ftruncate(outputFd, 0);
        return packet(env, -6);
    }

    digest::ScientificMasterDigestAccumulator replayDigest(
        static_cast<std::uint32_t>(width),
        static_cast<std::uint32_t>(height));
    if (!replayDigest.valid()) {
        (void)::ftruncate(outputFd, 0);
        return packet(env, -7);
    }

    std::uint64_t calibrated = 0u;
    std::uint64_t reconstructed = 0u;
    std::uint64_t censored = 0u;
    std::uint64_t unknown = 0u;
    std::uint64_t payloadBytes = 0u;
    std::uint64_t authorityBytes = 0u;
    std::uint64_t openSceneBytes = 0u;
    std::uint64_t openSceneFinitePixels = 0u;
    std::uint64_t openSceneCensoredPixels = 0u;
    std::uint64_t cellCount = 0u;
    canonical_scene::Binding openSceneBinding{};
    openSceneBinding.sourceEvidenceSha256 = sourceSeal.sha256;
    openSceneBinding.scientificMasterSha256 = scientific.scientificMasterHash;
    openSceneBinding.width = static_cast<std::uint32_t>(width);
    openSceneBinding.height = static_cast<std::uint32_t>(height);
    openSceneBinding.physicalFrameCount = scientific.physicalFrameCount;
    openSceneBinding.independentEvidenceCount = scientific.independentEvidenceCount;
    openSceneBinding.colourBindingId = produced.color.bindingId;
    canonical_scene::Builder openSceneBuilder(openSceneBinding);
    if (!openSceneBuilder.valid()) {
        (void)::ftruncate(outputFd, 0);
        return packet(env, -16);
    }

    field85::Binding fieldBinding{};
    fieldBinding.sourceEvidenceSha256 = sourceSeal.sha256;
    fieldBinding.scientificMasterSha256 = scientific.scientificMasterHash;
    fieldBinding.zeroLineSha256 = phase2.zeroLineHash;
    fieldBinding.sceneScaleSha256 = phase2.sceneScaleHash;
    fieldBinding.width = static_cast<std::uint32_t>(width);
    fieldBinding.height = static_cast<std::uint32_t>(height);
    fieldBinding.physicalFrameCount = scientific.physicalFrameCount;
    fieldBinding.independentEvidenceCount = scientific.independentEvidenceCount;
    fieldBinding.reconstructionBackendId = reconstruction->name();
    fieldBinding.colourBindingId = produced.color.bindingId;
    field85::Builder openSceneFieldBuilder(fieldBinding);
    if (!openSceneFieldBuilder.valid()) {
        (void)::ftruncate(outputFd, 0);
        return packet(env, -19);
    }

    std::uint64_t openSceneFieldStorageBytes = 0u;
    std::vector<float> rgb;
    std::vector<std::uint16_t> raw;
    std::vector<float> gain;
    std::vector<std::uint8_t> record;
    std::vector<std::uint8_t> auth;
    std::vector<std::uint8_t> openScene;
    std::vector<field85::ChannelRecord> openSceneFieldRecords;
    field85::EncodedTile openSceneFieldTile{};

    for (int y = 0; y < height; y += static_cast<int>(kCellEdge)) {
        const int h = std::min(static_cast<int>(kCellEdge), height - y);
        for (int x = 0; x < width; x += static_cast<int>(kCellEdge)) {
            const int w = std::min(static_cast<int>(kCellEdge), width - x);
            const std::size_t pixels =
                static_cast<std::size_t>(w) * static_cast<std::size_t>(h);
            if (pixels > std::numeric_limits<std::size_t>::max() / 3u) {
                (void)::ftruncate(outputFd, 0);
                return packet(env, -8);
            }
            const std::size_t samples = 3u * pixels;
            rgb.resize(samples);

            const auto readMaster = masterSource.readCameraNativeTile(
                static_cast<std::uint32_t>(x),
                static_cast<std::uint32_t>(y),
                static_cast<std::uint32_t>(w),
                static_cast<std::uint32_t>(h),
                rgb.data(),
                rgb.size());
            if (!readMaster) {
                (void)::ftruncate(outputFd, 0);
                return packet(env, 10000 + static_cast<jlong>(readMaster.code));
            }

            digest::TileView view{};
            view.x = static_cast<std::uint32_t>(x);
            view.y = static_cast<std::uint32_t>(y);
            view.width = static_cast<std::uint32_t>(w);
            view.height = static_cast<std::uint32_t>(h);
            view.rgb = rgb.data();
            view.rowStrideSamples = static_cast<std::size_t>(w) * 3u;
            if (!replayDigest.add_tile(view)) {
                (void)::ftruncate(outputFd, 0);
                return packet(env, -9);
            }

            raw.resize(pixels);
            if (source->metadata().hasGainField) gain.resize(pixels); else gain.clear();
            TileRect rect{x, y, x + w, y + h, x, y, x + w, y + h};
            const auto rawStatus = source->readRawTile(
                rect,
                raw.data(),
                raw.size(),
                source->metadata().hasGainField ? gain.data() : nullptr,
                source->metadata().hasGainField ? gain.size() : 0u);
            if (!rawStatus) {
                (void)::ftruncate(outputFd, 0);
                return packet(env, 4000 + static_cast<jlong>(rawStatus.code));
            }

            if (!field85::build_source_tile_records(
                    source->metadata().cfa,
                    static_cast<std::uint32_t>(x),
                    static_cast<std::uint32_t>(y),
                    static_cast<std::uint32_t>(w),
                    static_cast<std::uint32_t>(h),
                    raw,
                    source->metadata().whiteLevel,
                    rgb,
                    openSceneFieldRecords) ||
                !field85::encode_tile(
                    static_cast<std::uint32_t>(x),
                    static_cast<std::uint32_t>(y),
                    static_cast<std::uint32_t>(w),
                    static_cast<std::uint32_t>(h),
                    openSceneFieldRecords,
                    openSceneFieldTile) ||
                !openSceneFieldBuilder.appendTile(
                    static_cast<std::uint32_t>(x),
                    static_cast<std::uint32_t>(y),
                    static_cast<std::uint32_t>(w),
                    static_cast<std::uint32_t>(h),
                    openSceneFieldRecords,
                    openSceneFieldTile.bytes)) {
                (void)::ftruncate(outputFd, 0);
                return packet(env, -20);
            }

            auth.assign(samples, static_cast<std::uint8_t>(AuthorityByte::Unknown));
            openScene.assign(pixels, 0u);
            for (int yy = 0; yy < h; ++yy) {
                for (int xx = 0; xx < w; ++xx) {
                    const std::size_t pi =
                        static_cast<std::size_t>(yy) * static_cast<std::size_t>(w) +
                        static_cast<std::size_t>(xx);
                    const int channel = measured_channel(
                        source->metadata().cfa, x + xx, y + yy);
                    if (channel < 0 || channel > 2) {
                        (void)::ftruncate(outputFd, 0);
                        return packet(env, -10);
                    }
                    const bool clipped =
                        static_cast<float>(raw[pi]) >= source->metadata().whiteLevel;
                    auth[3u * pi + static_cast<std::size_t>(channel)] =
                        static_cast<std::uint8_t>(
                            clipped ? AuthorityByte::Censored
                                    : AuthorityByte::CalibratedEstimate);
                    if (clipped) {
                        ++censored;
                        ++openSceneCensoredPixels;
                        openScene[pi] = static_cast<std::uint8_t>(
                            channel == 0 ? canonical_scene::PixelState::RCensored :
                            channel == 1 ? canonical_scene::PixelState::GCensored :
                                           canonical_scene::PixelState::BCensored);
                    } else {
                        ++calibrated;
                        ++openSceneFinitePixels;
                        openScene[pi] = static_cast<std::uint8_t>(
                            channel == 0 ? canonical_scene::PixelState::RCalibratedEstimate :
                            channel == 1 ? canonical_scene::PixelState::GCalibratedEstimate :
                                           canonical_scene::PixelState::BCalibratedEstimate);
                    }
                    // Generic v0.66 has no admitted source-bound uncertainty model
                    // for arbitrary DNGs. The two demosaiced channels therefore keep
                    // their values but fail closed to UNKNOWN scientific authority.
                    unknown += 2u;
                }
            }

            if (!openSceneBuilder.append(auth, openScene)) {
                (void)::ftruncate(outputFd, 0);
                return packet(env, -17);
            }

            record.clear();
            record.reserve(
                16u + samples * 4u + auth.size() + openScene.size() +
                4u + openSceneFieldTile.bytes.size());
            append_u32_le(record, static_cast<std::uint32_t>(x));
            append_u32_le(record, static_cast<std::uint32_t>(y));
            append_u32_le(record, static_cast<std::uint32_t>(w));
            append_u32_le(record, static_cast<std::uint32_t>(h));
            for (const float value : rgb) {
                const std::uint32_t bits = std::bit_cast<std::uint32_t>(value);
                append_u32_le(record, bits);
            }
            record.insert(record.end(), auth.begin(), auth.end());
            record.insert(record.end(), openScene.begin(), openScene.end());
            append_u32_le(
                record,
                static_cast<std::uint32_t>(openSceneFieldTile.bytes.size()));
            record.insert(
                record.end(),
                openSceneFieldTile.bytes.begin(),
                openSceneFieldTile.bytes.end());

            if (!write_all(outputFd, record.data(), record.size())) {
                (void)::ftruncate(outputFd, 0);
                return packet(env, -11);
            }
            payloadBytes += static_cast<std::uint64_t>(samples) * 4u;
            authorityBytes += static_cast<std::uint64_t>(auth.size());
            openSceneBytes += static_cast<std::uint64_t>(openScene.size());
            openSceneFieldStorageBytes +=
                4u + static_cast<std::uint64_t>(openSceneFieldTile.bytes.size());
            ++cellCount;
        }
    }

    digest::Sha256 replayHash{};
    if (!replayDigest.finalize(replayHash) ||
        replayHash != scientific.scientificMasterHash) {
        (void)::ftruncate(outputFd, 0);
        return packet(env, -12);
    }

    if (reconstructed != 0u ||
        calibrated + censored != static_cast<std::uint64_t>(width) *
                                 static_cast<std::uint64_t>(height) ||
        unknown != 2u * static_cast<std::uint64_t>(width) *
                         static_cast<std::uint64_t>(height) ||
        openSceneFinitePixels + openSceneCensoredPixels !=
            static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height) ||
        openSceneBytes != static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height)) {
        (void)::ftruncate(outputFd, 0);
        return packet(env, -13);
    }

    canonical_scene::Summary openSceneSummary{};
    if (!openSceneBuilder.finalize(openSceneSummary) ||
        openSceneSummary.pixelCount !=
            static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height) ||
        openSceneSummary.counterfactualPixelCount != 0u ||
        openSceneSummary.scientificWritebackPixelCount != 0u ||
        openSceneSummary.createsNewEvidence ||
        openSceneSummary.chunkingChangesScientificIdentity) {
        (void)::ftruncate(outputFd, 0);
        return packet(env, -18);
    }

    field85::Summary openSceneFieldSummary{};
    const std::uint64_t sourcePixels =
        static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height);
    if (!openSceneFieldBuilder.finalize(
            openSceneSummary.artifactSha256,
            openSceneFieldSummary) ||
        openSceneFieldSummary.recordCount != sourcePixels * 3u ||
        openSceneFieldSummary.creationRoleCounts[1] != sourcePixels ||
        openSceneFieldSummary.creationRoleCounts[2] != sourcePixels * 2u ||
        openSceneFieldSummary.creationRoleCounts[3] != 0u ||
        openSceneFieldSummary.authorityCounts[0] != calibrated ||
        openSceneFieldSummary.authorityCounts[1] != 0u ||
        openSceneFieldSummary.authorityCounts[2] != censored ||
        openSceneFieldSummary.authorityCounts[3] != unknown ||
        openSceneFieldSummary.p95KnownCount != 0u ||
        openSceneFieldSummary.supportKnownCount != sourcePixels ||
        openSceneFieldSummary.boundKnownCount != censored ||
        openSceneFieldSummary.createsNewEvidence ||
        openSceneFieldSummary.scientificWritebackAllowed ||
        !openSceneFieldSummary.perPixelPerChannelAuthority ||
        !openSceneFieldSummary.perPixelPerChannelUncertainty ||
        !openSceneFieldSummary.perPixelPerChannelBounds) {
        (void)::ftruncate(outputFd, 0);
        return packet(env, -21);
    }

    if (!write_header(
            outputFd,
            sourceSeal,
            scientific,
            phase2,
            width,
            height,
            source->metadata().orientation,
            cellCount,
            calibrated,
            reconstructed,
            censored,
            unknown,
            payloadBytes,
            authorityBytes,
            openSceneBytes,
            openSceneFinitePixels,
            openSceneCensoredPixels,
            openSceneFieldStorageBytes,
            openSceneSummary,
            openSceneFieldSummary)) {
        (void)::ftruncate(outputFd, 0);
        return packet(env, -14);
    }

    const std::uint64_t expectedBytes =
        static_cast<std::uint64_t>(kHeaderBytes) +
        cellCount * 16u +
        payloadBytes +
        authorityBytes +
        openSceneBytes +
        openSceneFieldStorageBytes;
    if (::ftruncate(outputFd, static_cast<off_t>(expectedBytes)) != 0 ||
        ::fsync(outputFd) != 0) {
        (void)::ftruncate(outputFd, 0);
        return packet(env, -15);
    }

    const auto postVerified =
        truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(
            *bytes, sourceSeal);
    if (!postVerified) {
        (void)::ftruncate(outputFd, 0);
        return packet(env, binding_status(postVerified));
    }

    std::array<jlong, kPacketLongs> values{};
    values[0] = kMagic;
    values[1] = 0;
    values[2] = width;
    values[3] = height;
    values[4] = 3;
    values[5] = 32;
    values[6] = clamp_jlong(expectedBytes);
    values[7] = clamp_jlong(cellCount);
    values[8] = clamp_jlong(calibrated);
    values[9] = clamp_jlong(reconstructed);
    values[10] = clamp_jlong(censored);
    values[11] = clamp_jlong(unknown);
    values[12] = 0; // counterfactual
    values[13] = 0; // appearance-only
    values[14] = 1; // master digest replay verified
    values[15] = 1; // exact master sample bits written
    values[16] = 0; // creates new evidence
    values[17] = 0; // creates second scientific world
    values[18] = scientific.physicalFrameCount;
    values[19] = scientific.independentEvidenceCount;
    values[20] = clamp_jlong(payloadBytes);
    values[21] = clamp_jlong(authorityBytes);
    values[22] = clamp_jlong(openSceneBytes);
    values[23] = clamp_jlong(openSceneFinitePixels);
    values[24] = clamp_jlong(openSceneCensoredPixels);
    values[25] = 1; // full Open Scene State dense sidecar hashed
    values[26] = 0; // open-scene counterfactual pixels
    values[27] = 4; // TN-4 Open Scene Field v0.85

    auto out = env->NewLongArray(static_cast<jsize>(values.size()));
    if (out != nullptr) {
        env->SetLongArrayRegion(out, 0, static_cast<jsize>(values.size()), values.data());
    }
    return out;
}
