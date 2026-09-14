#include "truthraw_certificate_v0_1.h"

#include <cstdlib>
#include <iostream>

namespace cert = truthraw::certificate::v0_1;

#define REQUIRE(expr) do { \
    if (!(expr)) { \
        std::cerr << "REQUIRE failed: " #expr " at " << __FILE__ << ':' << __LINE__ << '\n'; \
        std::exit(2); \
    } \
} while (false)

namespace {

cert::Hash256 hash_seed(std::uint8_t seed) {
    cert::Hash256 out{};
    for (std::size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<std::uint8_t>(seed + static_cast<std::uint8_t>(i));
    }
    return out;
}

cert::State unsigned_pure() {
    cert::State state{};
    state.projectionClass = cert::ProjectionClass::TruthRawPureFloat32Dng;
    state.claimClass = cert::ClaimClass::Reconstructed;
    state.signatureState = cert::SignatureState::UnsignedDevelopment;
    state.signatureAlgorithm = cert::SignatureAlgorithm::None;
    state.colorClaimScope = 1u;
    state.sourceEvidenceSha256 = hash_seed(0x10u);
    state.scientificMasterSha256 = hash_seed(0x30u);
    state.zeroLineSha256 = hash_seed(0x50u);
    state.sceneScaleSha256 = hash_seed(0x70u);
    state.technicalBackplaneCrc32 = 0x6a51beefu;
    state.physicalFrameCount = 1u;
    state.independentEvidenceCount = 1u;
    state.buildIdentitySha256 = hash_seed(0x90u);
    return state;
}

void test_unsigned_roundtrip() {
    const auto state = unsigned_pure();
    REQUIRE(cert::validate(state) == cert::Status::Ok);
    REQUIRE(!cert::verified_badge_allowed(state, false));
    REQUIRE(!cert::verified_badge_allowed(state, true));

    cert::SerializedCertificate bytes{};
    REQUIRE(cert::serialize(state, bytes) == cert::Status::Ok);
    REQUIRE(bytes[0] == 'T' && bytes[1] == 'R' && bytes[2] == 'C');

    cert::State decoded{};
    REQUIRE(cert::deserialize(bytes, decoded) == cert::Status::Ok);
    REQUIRE(decoded.projectionClass == state.projectionClass);
    REQUIRE(decoded.claimClass == state.claimClass);
    REQUIRE(decoded.sourceEvidenceSha256 == state.sourceEvidenceSha256);
    REQUIRE(decoded.scientificMasterSha256 == state.scientificMasterSha256);
    REQUIRE(decoded.zeroLineSha256 == state.zeroLineSha256);
    REQUIRE(decoded.sceneScaleSha256 == state.sceneScaleSha256);
    REQUIRE(decoded.technicalBackplaneCrc32 == state.technicalBackplaneCrc32);
    REQUIRE(decoded.physicalFrameCount == 1u);
    REQUIRE(decoded.independentEvidenceCount == 1u);
}

void test_corruption_and_evidence_gate() {
    auto state = unsigned_pure();
    cert::SerializedCertificate bytes{};
    REQUIRE(cert::serialize(state, bytes) == cert::Status::Ok);
    bytes[60] ^= 0x01u;
    cert::State decoded{};
    REQUIRE(cert::deserialize(bytes, decoded) == cert::Status::CorruptRecord);

    state.physicalFrameCount = 2u;
    REQUIRE(cert::validate(state) == cert::Status::EvidenceInvariantViolation);
    state = unsigned_pure();
    state.independentEvidenceCount = 2u;
    REQUIRE(cert::validate(state) == cert::Status::EvidenceInvariantViolation);
}

void test_projection_claim_gate() {
    auto state = unsigned_pure();
    state.claimClass = cert::ClaimClass::MeasuredEvidence;
    REQUIRE(cert::validate(state) == cert::Status::InvalidProjectionClaim);

    state = unsigned_pure();
    state.projectionClass = cert::ProjectionClass::JpegPresentation;
    state.claimClass = cert::ClaimClass::Appearance;
    REQUIRE(cert::validate(state) == cert::Status::Ok);
}

void test_signature_truth_gate() {
    auto state = unsigned_pure();
    state.signatureState = cert::SignatureState::Verified;
    REQUIRE(cert::validate(state) == cert::Status::InvalidSignatureState);

    state.signatureAlgorithm = cert::SignatureAlgorithm::EcdsaP256Sha256Raw64;
    state.issuerKeyIdSha256 = hash_seed(0xb0u);
    for (std::size_t i = 0; i < state.signature.size(); ++i) {
        state.signature[i] = static_cast<std::uint8_t>(1u + (i % 251u));
    }
    REQUIRE(cert::validate(state) == cert::Status::Ok);
    REQUIRE(!cert::verified_badge_allowed(state, false));
    REQUIRE(cert::verified_badge_allowed(state, true));

    state.signatureState = cert::SignatureState::SignedUnverified;
    REQUIRE(cert::validate(state) == cert::Status::Ok);
    REQUIRE(!cert::verified_badge_allowed(state, true));
}

}  // namespace

int main() {
    test_unsigned_roundtrip();
    test_corruption_and_evidence_gate();
    test_projection_claim_gate();
    test_signature_truth_gate();
    std::cout << "TRUTHRAW_CERTIFICATE_V0_1_PASS\n";
    std::cout << "serialized_bytes=" << cert::kSerializedBytes << '\n';
    std::cout << "unsigned_never_verified=1\n";
    std::cout << "crypto_result_required_for_verified_badge=1\n";
    std::cout << "physical_frame_count=1\n";
    std::cout << "independent_evidence_count=1\n";
    return 0;
}
