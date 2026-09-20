#pragma once

#include "technical_backplane_v0_1.h"
#include "truthraw_sha256_v0_69.h"

#include <cstdint>
#include <string>

namespace truthraw::canonical_ancestry::v0_77 {

using Digest = truthraw::sha256_v0_69::Digest;

struct Binding final {
    Digest sourceEvidenceSha256{};
    Digest scientificMasterSha256{};
    Digest zeroLineSha256{};
    Digest sceneScaleSha256{};
    truthraw::technical_backplane::v0_1::SerializedBackplane serializedBackplane{};
    Digest openSceneArtifactSha256{};
    Digest derivativeRasterSha256{};
    Digest restorationRoleMaskSha256{};

    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    std::uint32_t physicalFrameCount = 1u;
    std::uint32_t independentEvidenceCount = 1u;

    std::string sourceEvidenceId;
    std::string colourBindingId;
    std::string precisionPolicyId;
    std::string reconstructionBackendId;
    std::string scientificCoordinateSpace = "CAMERA_NATIVE_SCENE_LINEAR_RGB";
    std::string derivativeRole = "RETREATABLE_RESTORATION_DERIVATIVE";
};

struct Manifest final {
    std::string schema = "TruthRawCanonicalAncestry/0.77";
    std::string canonicalText;
    Digest sha256{};
    Digest technicalBackplaneSha256{};
};

bool build(const Binding& binding, Manifest& out) noexcept;
bool validate_manifest(const Binding& binding, const Manifest& manifest) noexcept;
const char* schema_name() noexcept;

}  // namespace truthraw::canonical_ancestry::v0_77
