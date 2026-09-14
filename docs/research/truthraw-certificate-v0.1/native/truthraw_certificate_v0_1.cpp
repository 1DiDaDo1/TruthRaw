#include "truthraw_certificate_v0_1.h"

#include <algorithm>

namespace truthraw::certificate::v0_1 {
namespace {

constexpr std::array<std::uint8_t, 8> kMagic{'T','R','C','E','R','T','0','1'};
constexpr std::size_t kSourceOffset = 20u;
constexpr std::size_t kMasterOffset = 52u;
constexpr std::size_t kZeroLineOffset = 84u;
constexpr std::size_t kSceneScaleOffset = 116u;
constexpr std::size_t kBackplaneCrcOffset = 148u;
constexpr std::size_t kPhysicalFrameOffset = 152u;
constexpr std::size_t kEvidenceCountOffset = 156u;
constexpr std::size_t kBuildIdentityOffset = 160u;
constexpr std::size_t kIssuerKeyOffset = 192u;
constexpr std::size_t kSignatureOffset = 224u;
constexpr std::size_t kReservedTailOffset = 288u;
constexpr std::size_t kCrcOffset = 292u;

bool nonzero(const Hash256& value) noexcept {
    return std::any_of(value.begin(), value.end(), [](std::uint8_t b) { return b != 0u; });
}

bool nonzero(const Signature64& value) noexcept {
    return std::any_of(value.begin(), value.end(), [](std::uint8_t b) { return b != 0u; });
}

bool valid_projection(ProjectionClass value) noexcept {
    const auto v = static_cast<std::uint8_t>(value);
    return v >= static_cast<std::uint8_t>(ProjectionClass::DirectCfaEvidence) &&
           v <= static_cast<std::uint8_t>(ProjectionClass::JpegXlPresentation);
}

bool valid_claim(ClaimClass value) noexcept {
    const auto v = static_cast<std::uint8_t>(value);
    return v >= static_cast<std::uint8_t>(ClaimClass::MeasuredEvidence) &&
           v <= static_cast<std::uint8_t>(ClaimClass::Appearance);
}

bool valid_signature_state(SignatureState value) noexcept {
    return static_cast<std::uint8_t>(value) <=
           static_cast<std::uint8_t>(SignatureState::Verified);
}

bool valid_signature_algorithm(SignatureAlgorithm value) noexcept {
    return static_cast<std::uint8_t>(value) <=
           static_cast<std::uint8_t>(SignatureAlgorithm::EcdsaP256Sha256Raw64);
}

bool projection_claim_compatible(ProjectionClass projection, ClaimClass claim) noexcept {
    switch (projection) {
        case ProjectionClass::DirectCfaEvidence:
            return claim == ClaimClass::MeasuredEvidence;
        case ProjectionClass::ReconstructedCfaProjection:
        case ProjectionClass::LinearDngCompatibilityProjection:
        case ProjectionClass::TruthRawPureFloat32Dng:
            return claim == ClaimClass::Reconstructed;
        case ProjectionClass::JpegPresentation:
        case ProjectionClass::JpegXlPresentation:
            return claim == ClaimClass::Appearance;
    }
    return false;
}

void put_u16(SerializedCertificate& out, std::size_t offset, std::uint16_t value) noexcept {
    out[offset + 0u] = static_cast<std::uint8_t>(value & 0xffu);
    out[offset + 1u] = static_cast<std::uint8_t>((value >> 8u) & 0xffu);
}

void put_u32(SerializedCertificate& out, std::size_t offset, std::uint32_t value) noexcept {
    out[offset + 0u] = static_cast<std::uint8_t>(value & 0xffu);
    out[offset + 1u] = static_cast<std::uint8_t>((value >> 8u) & 0xffu);
    out[offset + 2u] = static_cast<std::uint8_t>((value >> 16u) & 0xffu);
    out[offset + 3u] = static_cast<std::uint8_t>((value >> 24u) & 0xffu);
}

std::uint16_t get_u16(std::span<const std::uint8_t> bytes, std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(bytes[offset + 0u]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[offset + 1u]) << 8u);
}

std::uint32_t get_u32(std::span<const std::uint8_t> bytes, std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(bytes[offset + 0u]) |
           (static_cast<std::uint32_t>(bytes[offset + 1u]) << 8u) |
           (static_cast<std::uint32_t>(bytes[offset + 2u]) << 16u) |
           (static_cast<std::uint32_t>(bytes[offset + 3u]) << 24u);
}

template <std::size_t N>
void copy_out(SerializedCertificate& out,
              std::size_t offset,
              const std::array<std::uint8_t, N>& value) noexcept {
    std::copy(value.begin(), value.end(), out.begin() + static_cast<std::ptrdiff_t>(offset));
}

template <std::size_t N>
void copy_in(std::span<const std::uint8_t> bytes,
             std::size_t offset,
             std::array<std::uint8_t, N>& value) noexcept {
    std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(offset), N, value.begin());
}

}  // namespace

Status validate(const State& state) noexcept {
    if (!valid_projection(state.projectionClass) ||
        !valid_claim(state.claimClass) ||
        !valid_signature_state(state.signatureState) ||
        !valid_signature_algorithm(state.signatureAlgorithm) ||
        state.colorClaimScope == 0u || state.colorClaimScope > 2u ||
        !nonzero(state.sourceEvidenceSha256) ||
        !nonzero(state.scientificMasterSha256) ||
        !nonzero(state.zeroLineSha256) ||
        !nonzero(state.sceneScaleSha256) ||
        !nonzero(state.buildIdentitySha256)) {
        return Status::InvalidInput;
    }

    if (state.physicalFrameCount != 1u || state.independentEvidenceCount != 1u) {
        return Status::EvidenceInvariantViolation;
    }

    if (!projection_claim_compatible(state.projectionClass, state.claimClass)) {
        return Status::InvalidProjectionClaim;
    }

    const bool issuerPresent = nonzero(state.issuerKeyIdSha256);
    const bool signaturePresent = nonzero(state.signature);
    if (state.signatureState == SignatureState::UnsignedDevelopment) {
        if (state.signatureAlgorithm != SignatureAlgorithm::None ||
            issuerPresent || signaturePresent) {
            return Status::InvalidSignatureState;
        }
    } else {
        if (state.signatureAlgorithm == SignatureAlgorithm::None ||
            !issuerPresent || !signaturePresent) {
            return Status::InvalidSignatureState;
        }
    }

    return Status::Ok;
}

std::uint32_t crc32(std::span<const std::uint8_t> bytes) noexcept {
    std::uint32_t crc = 0xffffffffu;
    for (const auto byte : bytes) {
        crc ^= static_cast<std::uint32_t>(byte);
        for (int bit = 0; bit < 8; ++bit) {
            const std::uint32_t mask = 0u - (crc & 1u);
            crc = (crc >> 1u) ^ (0xedb88320u & mask);
        }
    }
    return ~crc;
}

Status serialize(const State& state, SerializedCertificate& out) noexcept {
    const auto valid = validate(state);
    if (valid != Status::Ok) return valid;

    out.fill(0u);
    std::copy(kMagic.begin(), kMagic.end(), out.begin());
    put_u16(out, 8u, kVersion);
    put_u16(out, 10u, static_cast<std::uint16_t>(kSerializedBytes));
    out[12u] = static_cast<std::uint8_t>(state.projectionClass);
    out[13u] = static_cast<std::uint8_t>(state.claimClass);
    out[14u] = static_cast<std::uint8_t>(state.signatureState);
    out[15u] = static_cast<std::uint8_t>(state.signatureAlgorithm);
    out[16u] = state.colorClaimScope;

    copy_out(out, kSourceOffset, state.sourceEvidenceSha256);
    copy_out(out, kMasterOffset, state.scientificMasterSha256);
    copy_out(out, kZeroLineOffset, state.zeroLineSha256);
    copy_out(out, kSceneScaleOffset, state.sceneScaleSha256);
    put_u32(out, kBackplaneCrcOffset, state.technicalBackplaneCrc32);
    put_u32(out, kPhysicalFrameOffset, state.physicalFrameCount);
    put_u32(out, kEvidenceCountOffset, state.independentEvidenceCount);
    copy_out(out, kBuildIdentityOffset, state.buildIdentitySha256);
    copy_out(out, kIssuerKeyOffset, state.issuerKeyIdSha256);
    copy_out(out, kSignatureOffset, state.signature);

    const std::uint32_t checksum = crc32(
        std::span<const std::uint8_t>(out.data(), kCrcOffset));
    put_u32(out, kCrcOffset, checksum);
    return Status::Ok;
}

Status deserialize(std::span<const std::uint8_t> bytes, State& out) noexcept {
    out = {};
    if (bytes.size() != kSerializedBytes) return Status::InvalidInput;
    if (!std::equal(kMagic.begin(), kMagic.end(), bytes.begin())) return Status::CorruptRecord;
    if (get_u16(bytes, 8u) != kVersion ||
        get_u16(bytes, 10u) != static_cast<std::uint16_t>(kSerializedBytes)) {
        return Status::UnsupportedVersion;
    }
    if (get_u32(bytes, kCrcOffset) != crc32(bytes.first(kCrcOffset))) {
        return Status::CorruptRecord;
    }
    for (std::size_t i = 17u; i < kSourceOffset; ++i) {
        if (bytes[i] != 0u) return Status::CorruptRecord;
    }
    for (std::size_t i = kReservedTailOffset; i < kCrcOffset; ++i) {
        if (bytes[i] != 0u) return Status::CorruptRecord;
    }

    out.projectionClass = static_cast<ProjectionClass>(bytes[12u]);
    out.claimClass = static_cast<ClaimClass>(bytes[13u]);
    out.signatureState = static_cast<SignatureState>(bytes[14u]);
    out.signatureAlgorithm = static_cast<SignatureAlgorithm>(bytes[15u]);
    out.colorClaimScope = bytes[16u];
    copy_in(bytes, kSourceOffset, out.sourceEvidenceSha256);
    copy_in(bytes, kMasterOffset, out.scientificMasterSha256);
    copy_in(bytes, kZeroLineOffset, out.zeroLineSha256);
    copy_in(bytes, kSceneScaleOffset, out.sceneScaleSha256);
    out.technicalBackplaneCrc32 = get_u32(bytes, kBackplaneCrcOffset);
    out.physicalFrameCount = get_u32(bytes, kPhysicalFrameOffset);
    out.independentEvidenceCount = get_u32(bytes, kEvidenceCountOffset);
    copy_in(bytes, kBuildIdentityOffset, out.buildIdentitySha256);
    copy_in(bytes, kIssuerKeyOffset, out.issuerKeyIdSha256);
    copy_in(bytes, kSignatureOffset, out.signature);
    return validate(out);
}

bool verified_badge_allowed(
    const State& state,
    bool signatureCryptographicallyVerified) noexcept {
    return validate(state) == Status::Ok &&
           state.signatureState == SignatureState::Verified &&
           state.signatureAlgorithm != SignatureAlgorithm::None &&
           signatureCryptographicallyVerified;
}

const char* status_name(Status status) noexcept {
    switch (status) {
        case Status::Ok: return "OK";
        case Status::InvalidInput: return "INVALID_INPUT";
        case Status::EvidenceInvariantViolation: return "EVIDENCE_INVARIANT_VIOLATION";
        case Status::InvalidProjectionClaim: return "INVALID_PROJECTION_CLAIM";
        case Status::InvalidSignatureState: return "INVALID_SIGNATURE_STATE";
        case Status::CorruptRecord: return "CORRUPT_RECORD";
        case Status::UnsupportedVersion: return "UNSUPPORTED_VERSION";
    }
    return "UNKNOWN";
}

}  // namespace truthraw::certificate::v0_1
