#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace truthraw::certificate::v0_1 {

constexpr std::size_t kHashBytes = 32u;
constexpr std::size_t kSignatureBytes = 64u;
constexpr std::size_t kSerializedBytes = 296u;
constexpr std::uint16_t kVersion = 1u;

using Hash256 = std::array<std::uint8_t, kHashBytes>;
using Signature64 = std::array<std::uint8_t, kSignatureBytes>;
using SerializedCertificate = std::array<std::uint8_t, kSerializedBytes>;

enum class ProjectionClass : std::uint8_t {
    DirectCfaEvidence = 1,
    ReconstructedCfaProjection = 2,
    LinearDngCompatibilityProjection = 3,
    TruthRawPureFloat32Dng = 4,
    JpegPresentation = 5,
    JpegXlPresentation = 6,
};

enum class ClaimClass : std::uint8_t {
    MeasuredEvidence = 1,
    SourceMetadataBound = 2,
    Reconstructed = 3,
    Appearance = 4,
};

enum class SignatureState : std::uint8_t {
    UnsignedDevelopment = 0,
    SignedUnverified = 1,
    Verified = 2,
};

enum class SignatureAlgorithm : std::uint8_t {
    None = 0,
    EcdsaP256Sha256Raw64 = 1,
};

enum class Status : std::uint8_t {
    Ok = 0,
    InvalidInput,
    EvidenceInvariantViolation,
    InvalidProjectionClaim,
    InvalidSignatureState,
    CorruptRecord,
    UnsupportedVersion,
};

struct State final {
    ProjectionClass projectionClass = ProjectionClass::TruthRawPureFloat32Dng;
    ClaimClass claimClass = ClaimClass::Reconstructed;
    SignatureState signatureState = SignatureState::UnsignedDevelopment;
    SignatureAlgorithm signatureAlgorithm = SignatureAlgorithm::None;
    std::uint8_t colorClaimScope = 0u;

    Hash256 sourceEvidenceSha256{};
    Hash256 scientificMasterSha256{};
    Hash256 zeroLineSha256{};
    Hash256 sceneScaleSha256{};
    std::uint32_t technicalBackplaneCrc32 = 0u;
    std::uint32_t physicalFrameCount = 1u;
    std::uint32_t independentEvidenceCount = 1u;
    Hash256 buildIdentitySha256{};
    Hash256 issuerKeyIdSha256{};
    Signature64 signature{};
};

// Canonical certificate record for embedding in DNGPrivateData/XMP or another
// file-internal metadata carrier. It is provenance/authority metadata, never a
// visible watermark and never image evidence.
Status validate(const State& state) noexcept;
Status serialize(const State& state, SerializedCertificate& out) noexcept;
Status deserialize(std::span<const std::uint8_t> bytes, State& out) noexcept;
std::uint32_t crc32(std::span<const std::uint8_t> bytes) noexcept;
const char* status_name(Status status) noexcept;

// UI/consumer gate: metadata is never allowed to self-assert verification.
// A VERIFIED badge requires both a structurally valid record marked Verified
// and an independently computed cryptographic verification result.
bool verified_badge_allowed(
    const State& state,
    bool signatureCryptographicallyVerified) noexcept;

}  // namespace truthraw::certificate::v0_1
