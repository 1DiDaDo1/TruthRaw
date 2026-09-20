#pragma once

#include "truthraw_sha256_v0_69.h"

#include <array>
#include <cstdint>
#include <string>

namespace truthraw::bound_uncertainty_admission::v0_79 {

using Digest = truthraw::sha256_v0_69::Digest;

enum class SourceDomain : std::uint8_t {
    Unattested = 0,
    HonorBkqN49TeleVendorDngV5gP1 = 1,
    Camera5DerivedProcessingDng = 2,
    Other = 3,
};

enum class DecisionCode : std::uint8_t {
    BlockedNoSourceAttestation = 0,
    BlockedSourceDomainMismatch = 1,
    BlockedSourceClassMismatch = 2,
    BlockedBackendMismatch = 3,
    BlockedModelAssetMismatch = 4,
    EligibleTraceGateOpen = 5,
    Admitted = 6,
};

struct Candidate final {
    SourceDomain sourceDomain = SourceDomain::Unattested;

    std::string make;
    std::string model;
    std::string sourceClass;
    std::string reconstructionBackendId;

    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    std::uint32_t cfaCode = 0u;
    float whiteLevel = 0.0f;
    float focalLengthMm = 0.0f;
    bool noiseProfileValid = false;

    Digest sourceEvidenceSha256{};
    Digest decodedCfaSha256{};
    Digest productionCoreCppSha256{};
    Digest productionCoreHSha256{};
    Digest productionBackendCombinedSha256{};
    Digest featureExtractorSha256{};
    Digest featureSchemaSha256{};
    Digest uncertaintyModelSha256{};
    Digest uncertaintyBindingSha256{};
    Digest backendBindingFileSha256{};
    Digest prospectiveResultSha256{};
    Digest ptcBridgeFileSha256{};

    // Exact historical v5.0g -> F64 Scientific Master reconstructed-quantity
    // trace certificate. No registry certificate exists yet, so this must not
    // unlock admission merely by being nonzero.
    Digest f64TraceCertificateSha256{};
};

struct Decision final {
    DecisionCode code = DecisionCode::BlockedNoSourceAttestation;
    bool sourceClassExact = false;
    bool backendExact = false;
    bool assetsExact = false;
    bool prospectiveEvidenceExact = false;
    bool f64TraceCertified = false;
    bool reconstructedAuthorityAllowed = false;

    Digest uncertaintyBindingSha256{};
    Digest decisionSha256{};
    std::string scopeId;
    std::string reason;
};

Decision evaluate(const Candidate& candidate) noexcept;

Candidate make_current_camera5_derived_blocked_candidate(
    const Digest& sourceEvidenceSha256,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t cfaCode,
    float whiteLevel,
    const std::string& reconstructionBackendId) noexcept;

const char* schema_name() noexcept;
const char* decision_name(DecisionCode code) noexcept;

}  // namespace truthraw::bound_uncertainty_admission::v0_79
