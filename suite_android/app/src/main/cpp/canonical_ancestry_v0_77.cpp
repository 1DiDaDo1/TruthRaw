#include "canonical_ancestry_v0_77.h"

#include <algorithm>
#include <string_view>

namespace truthraw::canonical_ancestry::v0_77 {
namespace {

bool nonzero(const Digest& d) noexcept {
    return std::any_of(d.begin(), d.end(), [](std::uint8_t v) { return v != 0u; });
}

bool safe_text(std::string_view s) noexcept {
    if (s.empty() || s.size() > 2048u) return false;
    for (const unsigned char c : s) {
        if (c < 0x20u || c > 0x7eu || c == '\n' || c == '\r') return false;
    }
    return true;
}

Digest hash_bytes(const std::uint8_t* data, std::size_t size) noexcept {
    truthraw::sha256_v0_69::Hasher h;
    h.update(data, size);
    return h.finalize();
}

Digest hash_text(const std::string& s) noexcept {
    return hash_bytes(
        reinterpret_cast<const std::uint8_t*>(s.data()),
        s.size());
}

std::string hex_backplane(
    const truthraw::technical_backplane::v0_1::SerializedBackplane& b) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out(b.size() * 2u, '0');
    for (std::size_t i = 0; i < b.size(); ++i) {
        out[2u * i] = kHex[(b[i] >> 4u) & 0x0fu];
        out[2u * i + 1u] = kHex[b[i] & 0x0fu];
    }
    return out;
}

bool binding_valid(const Binding& b) noexcept {
    if (!nonzero(b.sourceEvidenceSha256) ||
        !nonzero(b.scientificMasterSha256) ||
        !nonzero(b.zeroLineSha256) ||
        !nonzero(b.sceneScaleSha256) ||
        !nonzero(b.openSceneArtifactSha256) ||
        !nonzero(b.derivativeRasterSha256) ||
        !nonzero(b.restorationRoleMaskSha256) ||
        b.width == 0u || b.height == 0u ||
        b.physicalFrameCount != 1u ||
        b.independentEvidenceCount != 1u ||
        !safe_text(b.sourceEvidenceId) ||
        !safe_text(b.colourBindingId) ||
        !safe_text(b.precisionPolicyId) ||
        !safe_text(b.reconstructionBackendId) ||
        !safe_text(b.scientificCoordinateSpace) ||
        !safe_text(b.derivativeRole)) {
        return false;
    }

    truthraw::technical_backplane::v0_1::State decoded{};
    const auto status = truthraw::technical_backplane::v0_1::deserialize(
        std::span<const std::uint8_t>(
            b.serializedBackplane.data(), b.serializedBackplane.size()),
        decoded);
    if (status != truthraw::technical_backplane::v0_1::Status::Ok) return false;

    return decoded.sourceEvidenceHash == b.sourceEvidenceSha256 &&
           decoded.scientificMasterHash == b.scientificMasterSha256 &&
           decoded.zeroLineHash == b.zeroLineSha256 &&
           decoded.sceneScaleHash == b.sceneScaleSha256 &&
           decoded.physicalFrameCount == 1u &&
           decoded.independentEvidenceCount == 1u &&
           decoded.forbiddenFlags == 0u;
}

std::string canonical_text(
    const Binding& b,
    const Digest& backplaneHash) {
    std::string t;
    t.reserve(2600u);
    t += "schema=TruthRawCanonicalAncestry/0.77\n";
    t += "ancestry_semantics=FORMAT_NEUTRAL_IMMUTABLE_PARENT_GRAPH\n";
    t += "source_evidence_sha256=" + truthraw::sha256_v0_69::hex(b.sourceEvidenceSha256) + "\n";
    t += "scientific_master_sha256=" + truthraw::sha256_v0_69::hex(b.scientificMasterSha256) + "\n";
    t += "zero_line_sha256=" + truthraw::sha256_v0_69::hex(b.zeroLineSha256) + "\n";
    t += "scene_scale_sha256=" + truthraw::sha256_v0_69::hex(b.sceneScaleSha256) + "\n";
    t += "technical_backplane_sha256=" + truthraw::sha256_v0_69::hex(backplaneHash) + "\n";
    t += "technical_backplane_bytes=" + std::to_string(b.serializedBackplane.size()) + "\n";
    t += "technical_backplane_serialized_hex=" + hex_backplane(b.serializedBackplane) + "\n";
    t += "open_scene_artifact_sha256=" + truthraw::sha256_v0_69::hex(b.openSceneArtifactSha256) + "\n";
    t += "derivative_raster_sha256=" + truthraw::sha256_v0_69::hex(b.derivativeRasterSha256) + "\n";
    t += "restoration_role_mask_sha256=" + truthraw::sha256_v0_69::hex(b.restorationRoleMaskSha256) + "\n";
    t += "width=" + std::to_string(b.width) + "\n";
    t += "height=" + std::to_string(b.height) + "\n";
    t += "physical_frame_count=1\n";
    t += "independent_evidence_count=1\n";
    t += "source_evidence_id=" + b.sourceEvidenceId + "\n";
    t += "colour_binding_id=" + b.colourBindingId + "\n";
    t += "precision_policy_id=" + b.precisionPolicyId + "\n";
    t += "reconstruction_backend_id=" + b.reconstructionBackendId + "\n";
    t += "scientific_coordinate_space=" + b.scientificCoordinateSpace + "\n";
    t += "derivative_role=" + b.derivativeRole + "\n";
    t += "source_evidence_immutable=1\n";
    t += "scientific_master_modified=0\n";
    t += "scientific_writeback_allowed=0\n";
    t += "appearance_or_restoration_creates_new_evidence=0\n";
    t += "counterfactual_upgrades_authority=0\n";
    t += "representation_may_exceed_source=1\n";
    t += "knowledge_claims_may_exceed_evidence=0\n";
    return t;
}

} // namespace

bool build(const Binding& binding, Manifest& out) noexcept {
    try {
        if (!binding_valid(binding)) return false;
        out = Manifest{};
        out.technicalBackplaneSha256 = hash_bytes(
            binding.serializedBackplane.data(),
            binding.serializedBackplane.size());
        out.canonicalText = canonical_text(binding, out.technicalBackplaneSha256);
        out.sha256 = hash_text(out.canonicalText);
        return nonzero(out.sha256);
    } catch (...) {
        return false;
    }
}

bool validate_manifest(const Binding& binding, const Manifest& manifest) noexcept {
    Manifest rebuilt{};
    if (!build(binding, rebuilt)) return false;
    return manifest.schema == rebuilt.schema &&
           manifest.canonicalText == rebuilt.canonicalText &&
           manifest.sha256 == rebuilt.sha256 &&
           manifest.technicalBackplaneSha256 == rebuilt.technicalBackplaneSha256;
}

const char* schema_name() noexcept {
    return "TruthRawCanonicalAncestry/0.77";
}

}  // namespace truthraw::canonical_ancestry::v0_77
