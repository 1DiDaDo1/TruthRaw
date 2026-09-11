#include "technical_backplane_phase2_v0_1.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <vector>

namespace truthraw::technical_backplane_phase2::v0_1 {
namespace {

using scientific_preview_binding_v0_1::BindingStatusCode;
using scientific_preview_binding_v0_1::seal_source_sha256;
using scientific_preview_binding_v0_1::SourceSeal;
using scientific_preview_binding_v0_2::PreparedScientificPreviewSource;
using scientific_preview_binding_v0_2::prepare_scientific_color_source;
using technical_backplane::v0_1::ClaimStatus;
using technical_backplane::v0_1::RoomStatus;

bool hash_nonzero(const Hash256& hash) noexcept {
    return std::any_of(hash.begin(), hash.end(), [](std::uint8_t b) { return b != 0u; });
}

void append_u16_le(std::vector<std::uint8_t>& out, std::uint16_t v) {
    out.push_back(static_cast<std::uint8_t>(v & 0xffu));
    out.push_back(static_cast<std::uint8_t>((v >> 8u) & 0xffu));
}

void append_u32_le(std::vector<std::uint8_t>& out, std::uint32_t v) {
    out.push_back(static_cast<std::uint8_t>(v & 0xffu));
    out.push_back(static_cast<std::uint8_t>((v >> 8u) & 0xffu));
    out.push_back(static_cast<std::uint8_t>((v >> 16u) & 0xffu));
    out.push_back(static_cast<std::uint8_t>((v >> 24u) & 0xffu));
}

void append_u64_le(std::vector<std::uint8_t>& out, std::uint64_t v) {
    for (unsigned shift = 0; shift < 64u; shift += 8u) {
        out.push_back(static_cast<std::uint8_t>((v >> shift) & 0xffu));
    }
}

bool append_string(std::vector<std::uint8_t>& out, const std::string& value) {
    if (value.size() > std::numeric_limits<std::uint32_t>::max()) return false;
    append_u32_le(out, static_cast<std::uint32_t>(value.size()));
    out.insert(out.end(), value.begin(), value.end());
    return true;
}

class MemoryByteSource final : public tile_dng_v0_1::IRandomAccessByteSource {
public:
    explicit MemoryByteSource(const std::vector<std::uint8_t>& bytes) : bytes_(bytes) {}

    std::uint64_t sizeBytes() const override {
        return static_cast<std::uint64_t>(bytes_.size());
    }

    std::size_t residentBytesUpperBound() const override {
        return sizeof(*this) + bytes_.size();
    }

    bool readExact(std::uint64_t offset, void* dst, std::size_t count) override {
        if (dst == nullptr && count != 0u) return false;
        if (offset > bytes_.size()) return false;
        const auto start = static_cast<std::size_t>(offset);
        if (count > bytes_.size() - start) return false;
        if (count != 0u) std::memcpy(dst, bytes_.data() + start, count);
        return true;
    }

private:
    const std::vector<std::uint8_t>& bytes_;
};

Status hash_bytes(const std::vector<std::uint8_t>& bytes, Hash256& out) noexcept {
    if (bytes.empty()) return Status::error(StatusCode::InvalidSceneScale, "canonical identity byte stream is empty");
    MemoryByteSource source(bytes);
    SourceSeal seal{};
    const auto status = seal_source_sha256(source, seal, 1024u);
    if (!status) {
        return Status::error(StatusCode::BackplaneSerializationFailed,
                             "SHA-256 identity hashing failed: " + status.message);
    }
    out = seal.sha256;
    return Status::ok();
}

bool prepared_matches(const PreparedScientificPreviewSource& a,
                      const PreparedScientificPreviewSource& b) noexcept {
    return a.source.sha256 == b.source.sha256 &&
           a.source.byteLength == b.source.byteLength &&
           a.source.sourceEvidenceId == b.source.sourceEvidenceId &&
           a.color.authority == b.color.authority &&
           a.color.sourceEvidenceId == b.color.sourceEvidenceId &&
           a.color.bindingId == b.color.bindingId &&
           a.color.cameraToXyzD50 == b.color.cameraToXyzD50 &&
           a.color.normalized == b.color.normalized &&
           a.color.validated == b.color.validated &&
           a.eventualClaimScope == b.eventualClaimScope &&
           a.mainHouseComputeAllowed == b.mainHouseComputeAllowed &&
           a.sourceBoundAppearanceReleaseAllowed == b.sourceBoundAppearanceReleaseAllowed &&
           a.scientificPreviewReleaseAllowed == b.scientificPreviewReleaseAllowed &&
           a.scientificClaimAllowed == b.scientificClaimAllowed &&
           a.physicalFrameCount == b.physicalFrameCount &&
           a.independentEvidenceCount == b.independentEvidenceCount;
}

bool valid_room_status(RoomStatus status) noexcept {
    return static_cast<std::uint8_t>(status) <= static_cast<std::uint8_t>(RoomStatus::Rejected);
}

bool valid_claim_status(ClaimStatus status) noexcept {
    return static_cast<std::uint8_t>(status) <= static_cast<std::uint8_t>(ClaimStatus::Promoted);
}

Status validate_zero_line_mode(const TruthRangeGaugeV02& gauge,
                               const LatentSceneBindingV02* sceneBinding) noexcept {
    if (!(gauge.L0 > 0.0) || !std::isfinite(gauge.L0) || gauge.gaugeId.empty()) {
        return Status::error(StatusCode::InvalidZeroLine,
                             "zero-line requires finite positive L0 and a non-empty gaugeId");
    }

    switch (gauge.mode) {
        case TruthRangeGaugeModeV02::SelfGauge:
            if (gauge.crossSceneComparable || gauge.absolutePhysicalUnits) {
                return Status::error(StatusCode::InvalidZeroLine,
                                     "self gauge cannot claim cross-scene or absolute physical authority");
            }
            break;
        case TruthRangeGaugeModeV02::ExternalRelativeGauge:
            if (!gauge.crossSceneComparable) {
                return Status::error(StatusCode::InvalidZeroLine,
                                     "external relative gauge requires cross-scene binding");
            }
            break;
        case TruthRangeGaugeModeV02::PhysicalAbsoluteGauge:
            if (!gauge.crossSceneComparable || !gauge.absolutePhysicalUnits) {
                return Status::error(StatusCode::InvalidZeroLine,
                                     "physical absolute gauge requires cross-scene and absolute flags");
            }
            if (sceneBinding != nullptr &&
                (!sceneBinding->exposureNormalizedToCommonScene ||
                 !sceneBinding->gainNormalizedToCommonScene)) {
                return Status::error(StatusCode::InvalidZeroLine,
                                     "physical absolute gauge requires exposure/gain-normalized scene binding");
            }
            break;
        default:
            return Status::error(StatusCode::InvalidZeroLine, "unsupported zero-line gauge mode");
    }
    return Status::ok();
}

}  // namespace

Status hash_zero_line_identity(const TruthRangeGaugeV02& gauge, Hash256& out) noexcept {
    const auto valid = validate_zero_line_mode(gauge, nullptr);
    if (!valid) return valid;

    std::vector<std::uint8_t> bytes;
    bytes.reserve(32u + gauge.gaugeId.size());
    constexpr std::uint8_t magic[8] = {'T','R','Z','E','R','O','0','1'};
    bytes.insert(bytes.end(), std::begin(magic), std::end(magic));
    append_u16_le(bytes, kVersion);
    bytes.push_back(static_cast<std::uint8_t>(gauge.mode));
    std::uint8_t flags = 0u;
    if (gauge.crossSceneComparable) flags |= 1u;
    if (gauge.absolutePhysicalUnits) flags |= 2u;
    bytes.push_back(flags);

    std::uint64_t l0Bits = 0u;
    static_assert(sizeof(l0Bits) == sizeof(gauge.L0), "zero-line identity requires binary64 double");
    std::memcpy(&l0Bits, &gauge.L0, sizeof(l0Bits));
    append_u64_le(bytes, l0Bits);
    if (!append_string(bytes, gauge.gaugeId)) {
        return Status::error(StatusCode::InvalidZeroLine, "zero-line gaugeId is too large");
    }
    return hash_bytes(bytes, out);
}

Status hash_scene_scale_identity(const LatentSceneBindingV02& binding, Hash256& out) noexcept {
    if (binding.sceneScaleId.empty()) {
        return Status::error(StatusCode::InvalidSceneScale, "sceneScaleId must be non-empty");
    }
    if (!binding.gainMapAppliedExactlyOnce) {
        return Status::error(StatusCode::InvalidSceneScale,
                             "phase-2 scene scale must preserve GainMap exactly-once semantics");
    }

    std::vector<std::uint8_t> bytes;
    bytes.reserve(24u + binding.sceneScaleId.size());
    constexpr std::uint8_t magic[8] = {'T','R','S','C','A','L','0','1'};
    bytes.insert(bytes.end(), std::begin(magic), std::end(magic));
    append_u16_le(bytes, kVersion);
    bytes.push_back(binding.gainMapAppliedExactlyOnce ? 1u : 0u);
    bytes.push_back(binding.exposureNormalizedToCommonScene ? 1u : 0u);
    bytes.push_back(binding.gainNormalizedToCommonScene ? 1u : 0u);
    bytes.push_back(0u);  // reserved
    if (!append_string(bytes, binding.sceneScaleId)) {
        return Status::error(StatusCode::InvalidSceneScale, "sceneScaleId is too large");
    }
    return hash_bytes(bytes, out);
}

Status finalize_phase2(const Phase2Input& input, Phase2Result& out) noexcept {
    out = {};

    PreparedScientificPreviewSource canonicalPrepared{};
    const auto prepareStatus = prepare_scientific_color_source(
        input.prepared.source, input.prepared.color, canonicalPrepared);
    if (!prepareStatus || !prepared_matches(input.prepared, canonicalPrepared)) {
        return Status::error(StatusCode::InvalidPreparedSource,
                             prepareStatus ? "prepared source state differs from canonical pre-master state"
                                           : "prepared source could not be revalidated: " + prepareStatus.message);
    }

    if (!hash_nonzero(input.scientificMasterHash)) {
        return Status::error(StatusCode::InvalidScientificMasterDigest,
                             "Scientific Master digest must be a real non-zero SHA-256 value");
    }

    const auto gaugeStatus = validate_zero_line_mode(input.zeroLineGauge, &input.sceneBinding);
    if (!gaugeStatus) return gaugeStatus;

    if (input.sceneBinding.sceneScaleId.empty() || !input.sceneBinding.gainMapAppliedExactlyOnce) {
        return Status::error(StatusCode::InvalidSceneScale,
                             "scene scale is incomplete or violates GainMap exactly-once semantics");
    }

    if (!valid_claim_status(input.claimStatus) ||
        !std::all_of(input.roomStatus.begin(), input.roomStatus.end(), valid_room_status)) {
        return Status::error(StatusCode::InvalidRoomState, "room or claim status is outside Backplane v0.1 enum range");
    }

    Phase2Result result{};
    auto status = hash_zero_line_identity(input.zeroLineGauge, result.zeroLineHash);
    if (!status) return status;
    status = hash_scene_scale_identity(input.sceneBinding, result.sceneScaleHash);
    if (!status) return status;

    result.backplane.sourceEvidenceHash = input.prepared.source.sha256;
    result.backplane.scientificMasterHash = input.scientificMasterHash;
    result.backplane.zeroLineHash = result.zeroLineHash;
    result.backplane.sceneScaleHash = result.sceneScaleHash;
    result.backplane.physicalFrameCount = 1u;
    result.backplane.independentEvidenceCount = 1u;
    result.backplane.roomStatus = input.roomStatus;
    result.backplane.claimStatus = input.claimStatus;
    result.backplane.forbiddenFlags = 0u;

    const auto backplaneStatus = technical_backplane::v0_1::validate(result.backplane);
    if (backplaneStatus != technical_backplane::v0_1::Status::Ok) {
        return Status::error(StatusCode::BackplaneRejected,
                             std::string("Technical Backplane rejected phase-2 state: ") +
                                 technical_backplane::v0_1::status_name(backplaneStatus));
    }

    const auto serializeStatus = technical_backplane::v0_1::serialize(
        result.backplane, result.serializedBackplane);
    if (serializeStatus != technical_backplane::v0_1::Status::Ok) {
        return Status::error(StatusCode::BackplaneSerializationFailed,
                             std::string("Technical Backplane serialization failed: ") +
                                 technical_backplane::v0_1::status_name(serializeStatus));
    }

    const auto finalizeStatus = scientific_preview_binding_v0_2::finalize_scientific_color_lineage(
        input.prepared, result.backplane, result.admission);
    if (!finalizeStatus) {
        return Status::error(StatusCode::PreviewFinalizationRejected,
                             "Scientific Preview finalization rejected phase-2 state: " + finalizeStatus.message);
    }

    out = result;
    return Status::ok();
}

const char* status_name(StatusCode code) noexcept {
    switch (code) {
        case StatusCode::Ok: return "OK";
        case StatusCode::InvalidPreparedSource: return "INVALID_PREPARED_SOURCE";
        case StatusCode::InvalidScientificMasterDigest: return "INVALID_SCIENTIFIC_MASTER_DIGEST";
        case StatusCode::InvalidZeroLine: return "INVALID_ZERO_LINE";
        case StatusCode::InvalidSceneScale: return "INVALID_SCENE_SCALE";
        case StatusCode::InvalidRoomState: return "INVALID_ROOM_STATE";
        case StatusCode::BackplaneRejected: return "BACKPLANE_REJECTED";
        case StatusCode::BackplaneSerializationFailed: return "BACKPLANE_SERIALIZATION_FAILED";
        case StatusCode::PreviewFinalizationRejected: return "PREVIEW_FINALIZATION_REJECTED";
    }
    return "UNKNOWN";
}

}  // namespace truthraw::technical_backplane_phase2::v0_1
