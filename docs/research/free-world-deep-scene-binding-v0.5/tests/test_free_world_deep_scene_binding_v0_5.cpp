#include "free_world_deep_scene_binding_v0_5.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace bind = truthraw::free_world_deep_scene_binding::v0_5;
namespace deep = truthraw::free_world_deep_scene_contribution::v0_4;
namespace fw = truthraw::free_world_pixel_resolve_2d::v0_2;

#define REQUIRE(x) do { if (!(x)) throw std::runtime_error(#x); } while (0)

namespace {

bind::Digest digest(std::uint8_t seed) {
    bind::Digest d{};
    for (std::size_t i = 0u; i < d.size(); ++i) {
        d[i] = static_cast<std::uint8_t>(seed + i);
    }
    return d;
}

fw::ResolvedPixel evidencePixel() {
    fw::ResolvedPixel p{};
    p.sceneLinear = {-0.2, 0.4, 1.2};
    for (std::size_t c = 0u; c < 3u; ++c) {
        p.support[c].authority = fw::ResolvedAuthority::Reconstructed;
        p.support[c].uncertaintyKnown = true;
        p.support[c].p95Uncertainty =
            0.01 + 0.01 * static_cast<double>(c);
    }
    p.measuredTargetClaimCount = 0u;
    p.createsNewEvidence = false;
    p.physicalFrameCount = 1u;
    p.independentEvidenceCount = 1u;
    return p;
}

deep::DeepSample evidenceSample(
    double depth = 10.0,
    std::uint64_t provenance = 1001u) {
    deep::DeepSample s{};
    REQUIRE(deep::makeCameraPlaneContribution(
        evidencePixel(), depth, provenance, s));
    return s;
}

deep::DeepSample inferredSample() {
    deep::DeepSample s{};
    s.zFront = 2.0;
    s.zBack = 2.0;
    s.sceneLinearRgb = {0.6, 0.2, 0.1};
    s.opacity = 0.25;
    s.contributionClass = deep::ContributionClass::InferredScene;
    s.channelAuthority = {
        fw::ResolvedAuthority::Unknown,
        fw::ResolvedAuthority::Unknown,
        fw::ResolvedAuthority::Unknown};
    s.provenanceId = 2002u;
    REQUIRE(deep::validateSample(s));
    return s;
}

deep::DeepPixelPacket parentPacket() {
    deep::DeepPixelPacket p{};
    p.samples = {evidenceSample(), inferredSample()};
    REQUIRE(deep::finalizePacket(p));
    return p;
}

std::vector<bind::ContributionMetadata> metadataFor(
    const deep::DeepPixelPacket& p) {
    REQUIRE(p.samples.size() == 2u);
    std::vector<bind::ContributionMetadata> m(2u);

    for (std::size_t i = 0u; i < p.samples.size(); ++i) {
        m[i].provenanceId = p.samples[i].provenanceId;
        m[i].regionId = 10u + static_cast<std::uint64_t>(i);
        m[i].objectId = 20u + static_cast<std::uint64_t>(i);
        m[i].parentAncestrySha256 = digest(
            static_cast<std::uint8_t>(30u + i));
        if (p.samples[i].contributionClass ==
            deep::ContributionClass::EvidenceConstrained) {
            m[i].geometryAuthority =
                bind::GeometryAuthority::ImagePlaneBound;
        } else {
            m[i].geometryAuthority =
                bind::GeometryAuthority::Inferred;
        }
    }
    return m;
}

void test_packet_binds_scene_region_object_and_ancestry() {
    const auto parent = parentPacket();
    auto metadata = metadataFor(parent);

    bind::BoundDeepPacket out{};
    REQUIRE(bind::bindPacket(
        parent, digest(1u), metadata, out));
    REQUIRE(out.finalized);
    REQUIRE(out.cameraPlaneSceneSha256 == digest(1u));
    REQUIRE(out.parentDeepPacketSha256 == parent.packetSha256);
    REQUIRE(out.contributions.size() == parent.samples.size());
    REQUIRE(out.geometryAndRadiometrySeparated);
    REQUIRE(!out.createsNewEvidence);
    REQUIRE(!out.scientificWritebackAllowed);

    for (const auto& c : out.contributions) {
        REQUIRE(c.metadata.regionId != 0u);
        REQUIRE(c.metadata.objectId != 0u);
        REQUIRE(c.metadata.provenanceId == c.sample.provenanceId);
        REQUIRE(c.metadata.contributionAncestrySha256 != bind::Digest{});
    }
}

void test_scene_identity_and_object_identity_change_bound_hash() {
    const auto parent = parentPacket();
    auto aMeta = metadataFor(parent);
    auto bMeta = aMeta;
    bMeta[0].objectId ^= 0x55u;

    bind::BoundDeepPacket a{};
    bind::BoundDeepPacket b{};
    bind::BoundDeepPacket c{};
    REQUIRE(bind::bindPacket(parent, digest(1u), aMeta, a));
    REQUIRE(bind::bindPacket(parent, digest(1u), bMeta, b));
    REQUIRE(bind::bindPacket(parent, digest(2u), aMeta, c));

    REQUIRE(a.boundPacketSha256 != b.boundPacketSha256);
    REQUIRE(a.boundPacketSha256 != c.boundPacketSha256);
}

void test_geometry_authority_is_separate_from_radiometry() {
    const auto parent = parentPacket();
    auto meta = metadataFor(parent);

    // The evidence-constrained camera radiometry can stay reconstructed while
    // its later assigned depth/geometry is explicitly only INFERRED.
    for (std::size_t i = 0u; i < parent.samples.size(); ++i) {
        if (parent.samples[i].contributionClass ==
            deep::ContributionClass::EvidenceConstrained) {
            meta[i].geometryAuthority = bind::GeometryAuthority::Inferred;
        }
    }

    bind::BoundDeepPacket out{};
    REQUIRE(bind::bindPacket(parent, digest(4u), meta, out));

    bool sawEvidence = false;
    for (const auto& c : out.contributions) {
        if (c.sample.contributionClass ==
            deep::ContributionClass::EvidenceConstrained) {
            sawEvidence = true;
            REQUIRE(c.metadata.geometryAuthority ==
                bind::GeometryAuthority::Inferred);
            REQUIRE(c.sample.channelAuthority[0] ==
                fw::ResolvedAuthority::Reconstructed);
        }
    }
    REQUIRE(sawEvidence);
}

void test_counterfactual_geometry_requires_counterfactual_class() {
    auto parent = parentPacket();
    auto meta = metadataFor(parent);
    meta[0].geometryAuthority = bind::GeometryAuthority::Counterfactual;

    bind::BoundDeepPacket out{};
    REQUIRE(!bind::bindPacket(parent, digest(8u), meta, out));
}

void test_factorised_path_radiance_reference() {
    bind::PathRadianceInput input{};
    input.pathKind = bind::PathKind::SurfaceReflection;
    input.geometryAuthority = bind::GeometryAuthority::Inferred;
    input.contributionClass = deep::ContributionClass::InferredScene;
    input.provenanceId = 5005u;
    input.regionId = 55u;
    input.objectId = 77u;
    input.zFront = 3.0;
    input.zBack = 3.0;
    input.opacity = 0.4;
    input.incidentSceneLinearRgb = {2.0, 1.0, 0.5};
    input.throughputRgb = {0.25, 0.50, 0.80};
    input.emittedSceneLinearRgb = {0.1, 0.2, 0.3};
    input.visibility = 0.5;
    input.cosineTerm = 0.8;
    input.radiometricAuthority = {
        fw::ResolvedAuthority::Unknown,
        fw::ResolvedAuthority::Unknown,
        fw::ResolvedAuthority::Unknown};

    bind::PathRadianceResult out{};
    REQUIRE(bind::evaluatePathRadiance(input, out));

    REQUIRE(std::abs(out.sample.sceneLinearRgb[0] - 0.3) < 1e-12);
    REQUIRE(std::abs(out.sample.sceneLinearRgb[1] - 0.4) < 1e-12);
    REQUIRE(std::abs(out.sample.sceneLinearRgb[2] - 0.46) < 1e-12);
    REQUIRE(out.geometryAuthority == bind::GeometryAuthority::Inferred);
    REQUIRE(out.pathKind == bind::PathKind::SurfaceReflection);
    REQUIRE(!out.spectralMeasurementClaimed);
    REQUIRE(!out.fullPathTracingClaimed);
    REQUIRE(!out.createsNewEvidence);
    REQUIRE(!out.scientificWritebackAllowed);
}

void test_non_evidence_path_cannot_claim_radiometric_authority() {
    bind::PathRadianceInput input{};
    input.pathKind = bind::PathKind::Transmission;
    input.geometryAuthority = bind::GeometryAuthority::Inferred;
    input.contributionClass = deep::ContributionClass::InferredScene;
    input.provenanceId = 6006u;
    input.regionId = 66u;
    input.objectId = 88u;
    input.zFront = 1.0;
    input.zBack = 2.0;
    input.opacity = 0.5;
    input.incidentSceneLinearRgb = {1.0, 1.0, 1.0};
    input.throughputRgb = {0.5, 0.5, 0.5};
    input.emittedSceneLinearRgb = {0.0, 0.0, 0.0};
    input.radiometricAuthority = {
        fw::ResolvedAuthority::Reconstructed,
        fw::ResolvedAuthority::Unknown,
        fw::ResolvedAuthority::Unknown};

    bind::PathRadianceResult out{};
    REQUIRE(!bind::evaluatePathRadiance(input, out));
}

void test_zero_scene_digest_and_identity_mismatch_fail_closed() {
    const auto parent = parentPacket();
    auto meta = metadataFor(parent);

    bind::BoundDeepPacket out{};
    REQUIRE(!bind::bindPacket(parent, bind::Digest{}, meta, out));

    meta[0].provenanceId ^= 1u;
    REQUIRE(!bind::bindPacket(parent, digest(9u), meta, out));
}

}  // namespace

int main() {
    test_packet_binds_scene_region_object_and_ancestry();
    test_scene_identity_and_object_identity_change_bound_hash();
    test_geometry_authority_is_separate_from_radiometry();
    test_counterfactual_geometry_requires_counterfactual_class();
    test_factorised_path_radiance_reference();
    test_non_evidence_path_cannot_claim_radiometric_authority();
    test_zero_scene_digest_and_identity_mismatch_fail_closed();

    std::cout << "FreeWorldDeepSceneBinding/0.5 PASS\n";
    std::cout << "path_method=" << bind::kPathMethodId << "\n";
    std::cout << "geometry_radiometry_separate=1\n";
    std::cout << "spectral_measurement_claimed=0\n";
    std::cout << "full_path_tracing_claimed=0\n";
    return 0;
}
