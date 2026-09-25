#include "free_world_deep_scene_contribution_v0_4.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace deep = truthraw::free_world_deep_scene_contribution::v0_4;
namespace fw = truthraw::free_world_pixel_resolve_2d::v0_2;

#define REQUIRE(x) do { if (!(x)) throw std::runtime_error(#x); } while (0)

namespace {

bool near(double a, double b, double eps = 1e-12) {
    return std::abs(a - b) <= eps;
}

fw::ResolvedPixel makeEvidencePixel() {
    fw::ResolvedPixel p{};
    p.sceneLinear = {-0.25, 0.50, 1.25};
    for (std::size_t c = 0u; c < 3u; ++c) {
        p.support[c].authority = fw::ResolvedAuthority::Reconstructed;
        p.support[c].uncertaintyKnown = true;
        p.support[c].p95Uncertainty = 0.02 + 0.01 * static_cast<double>(c);
    }
    p.measuredTargetClaimCount = 0u;
    p.createsNewEvidence = false;
    p.physicalFrameCount = 1u;
    p.independentEvidenceCount = 1u;
    return p;
}

deep::DeepSample inferredForeground() {
    deep::DeepSample s{};
    s.zFront = 1.0;
    s.zBack = 1.0;
    s.sceneLinearRgb = {0.8, 0.2, 0.1};
    s.opacity = 0.25;
    s.contributionClass = deep::ContributionClass::InferredScene;
    s.channelAuthority = {
        fw::ResolvedAuthority::Unknown,
        fw::ResolvedAuthority::Unknown,
        fw::ResolvedAuthority::Unknown};
    s.provenanceId = 2002u;
    return s;
}

deep::DeepSample restorationForeground() {
    deep::DeepSample s{};
    s.zFront = 0.5;
    s.zBack = 0.5;
    s.sceneLinearRgb = {0.1, 0.9, 0.2};
    s.opacity = 0.20;
    s.contributionClass =
        deep::ContributionClass::RestorationHypothesis;
    s.channelAuthority = {
        fw::ResolvedAuthority::Unknown,
        fw::ResolvedAuthority::Unknown,
        fw::ResolvedAuthority::Unknown};
    s.provenanceId = 3003u;
    return s;
}

deep::DeepSample counterfactualForeground() {
    deep::DeepSample s{};
    s.zFront = 0.25;
    s.zBack = 0.25;
    s.sceneLinearRgb = {2.0, 1.0, 0.0};
    s.opacity = 0.10;
    s.contributionClass =
        deep::ContributionClass::CounterfactualScene;
    s.channelAuthority = {
        fw::ResolvedAuthority::Unknown,
        fw::ResolvedAuthority::Unknown,
        fw::ResolvedAuthority::Unknown};
    s.provenanceId = 4004u;
    return s;
}

void test_camera_plane_contribution_contract() {
    const auto source = makeEvidencePixel();
    deep::DeepSample sample{};
    REQUIRE(deep::makeCameraPlaneContribution(
        source, 10.0, 1001u, sample));
    REQUIRE(sample.opacity == 1.0);
    REQUIRE(sample.zFront == 10.0 && sample.zBack == 10.0);
    REQUIRE(sample.contributionClass ==
        deep::ContributionClass::EvidenceConstrained);
    REQUIRE(sample.sceneLinearRgb == source.sceneLinear);
    REQUIRE(sample.channelAuthority[0] ==
        fw::ResolvedAuthority::Reconstructed);
    REQUIRE(sample.uncertaintyKnown[0]);
    REQUIRE(near(sample.p95Uncertainty[0], 0.02));
    REQUIRE(deep::validateSample(sample));
}

void test_non_evidence_cannot_claim_scientific_authority() {
    auto s = inferredForeground();
    s.channelAuthority[0] = fw::ResolvedAuthority::Reconstructed;
    REQUIRE(!deep::validateSample(s));
}

void test_packet_order_and_digest_are_deterministic() {
    deep::DeepSample base{};
    REQUIRE(deep::makeCameraPlaneContribution(
        makeEvidencePixel(), 10.0, 1001u, base));

    deep::DeepPixelPacket a{};
    a.samples = {base, inferredForeground(), restorationForeground()};
    REQUIRE(deep::finalizePacket(a));
    REQUIRE(a.finalized);
    REQUIRE(a.samples[0].zFront == 0.5);
    REQUIRE(a.samples[1].zFront == 1.0);
    REQUIRE(a.samples[2].zFront == 10.0);

    deep::DeepPixelPacket b{};
    b.samples = {base, inferredForeground(), restorationForeground()};
    REQUIRE(deep::finalizePacket(b));
    REQUIRE(a.packetSha256 == b.packetSha256);
}

void test_scientific_view_excludes_non_evidence() {
    deep::DeepSample base{};
    REQUIRE(deep::makeCameraPlaneContribution(
        makeEvidencePixel(), 10.0, 1001u, base));

    deep::DeepPixelPacket packet{};
    packet.samples = {
        counterfactualForeground(),
        restorationForeground(),
        inferredForeground(),
        base};
    REQUIRE(deep::finalizePacket(packet));

    deep::DeepResolvedPixel out{};
    REQUIRE(deep::resolve(
        packet, deep::ResolveView::ScientificView, out));

    REQUIRE(out.sceneLinearRgb == base.sceneLinearRgb);
    REQUIRE(out.visibility.evidenceWeight == 1.0);
    REQUIRE(out.visibility.inferredWeight == 0.0);
    REQUIRE(out.visibility.restorationWeight == 0.0);
    REQUIRE(out.visibility.counterfactualWeight == 0.0);
    REQUIRE(out.visibility.scientificObservation);
    REQUIRE(!out.appearanceApplied);
    REQUIRE(!out.displayEncoded);
    REQUIRE(!out.createsNewEvidence);
    REQUIRE(!out.scientificWritebackAllowed);
    for (std::size_t c = 0u; c < 3u; ++c) {
        REQUIRE(out.channelAuthority[c] ==
            fw::ResolvedAuthority::Reconstructed);
        REQUIRE(out.uncertaintyKnown[c]);
    }
}

void test_open_scene_view_adds_inferred_without_promoting_it() {
    deep::DeepSample base{};
    REQUIRE(deep::makeCameraPlaneContribution(
        makeEvidencePixel(), 10.0, 1001u, base));
    const auto inferred = inferredForeground();

    deep::DeepPixelPacket packet{};
    packet.samples = {base, inferred};
    REQUIRE(deep::finalizePacket(packet));

    deep::DeepResolvedPixel out{};
    REQUIRE(deep::resolve(
        packet, deep::ResolveView::OpenSceneView, out));

    const std::array<double,3u> expected{
        0.25 * inferred.sceneLinearRgb[0] + 0.75 * base.sceneLinearRgb[0],
        0.25 * inferred.sceneLinearRgb[1] + 0.75 * base.sceneLinearRgb[1],
        0.25 * inferred.sceneLinearRgb[2] + 0.75 * base.sceneLinearRgb[2]};
    for (std::size_t c = 0u; c < 3u; ++c) {
        REQUIRE(near(out.sceneLinearRgb[c], expected[c]));
        REQUIRE(out.channelAuthority[c] ==
            fw::ResolvedAuthority::Unknown);
        REQUIRE(!out.uncertaintyKnown[c]);
    }
    REQUIRE(near(out.visibility.inferredWeight, 0.25));
    REQUIRE(near(out.visibility.evidenceWeight, 0.75));
    REQUIRE(out.visibility.containsInferred);
    REQUIRE(!out.visibility.scientificObservation);
}

void test_view_progression_is_strict() {
    deep::DeepSample base{};
    REQUIRE(deep::makeCameraPlaneContribution(
        makeEvidencePixel(), 10.0, 1001u, base));

    deep::DeepPixelPacket packet{};
    packet.samples = {
        counterfactualForeground(),
        restorationForeground(),
        inferredForeground(),
        base};
    REQUIRE(deep::finalizePacket(packet));

    deep::DeepResolvedPixel open{};
    deep::DeepResolvedPixel restoration{};
    deep::DeepResolvedPixel counterfactual{};
    REQUIRE(deep::resolve(
        packet, deep::ResolveView::OpenSceneView, open));
    REQUIRE(deep::resolve(
        packet, deep::ResolveView::RestorationView, restoration));
    REQUIRE(deep::resolve(
        packet, deep::ResolveView::CounterfactualRender, counterfactual));

    REQUIRE(open.visibility.containsInferred);
    REQUIRE(!open.visibility.containsRestorationHypothesis);
    REQUIRE(!open.visibility.containsCounterfactual);

    REQUIRE(restoration.visibility.containsInferred);
    REQUIRE(restoration.visibility.containsRestorationHypothesis);
    REQUIRE(!restoration.visibility.containsCounterfactual);

    REQUIRE(counterfactual.visibility.containsInferred);
    REQUIRE(counterfactual.visibility.containsRestorationHypothesis);
    REQUIRE(counterfactual.visibility.containsCounterfactual);

    REQUIRE(!open.visibility.scientificObservation);
    REQUIRE(!restoration.visibility.scientificObservation);
    REQUIRE(!counterfactual.visibility.scientificObservation);
}

void test_opaque_foreground_occludes_farther_samples() {
    auto front = inferredForeground();
    front.opacity = 1.0;
    front.provenanceId = 9001u;

    deep::DeepSample base{};
    REQUIRE(deep::makeCameraPlaneContribution(
        makeEvidencePixel(), 10.0, 1001u, base));

    deep::DeepPixelPacket packet{};
    packet.samples = {base, front};
    REQUIRE(deep::finalizePacket(packet));

    deep::DeepResolvedPixel out{};
    REQUIRE(deep::resolve(
        packet, deep::ResolveView::OpenSceneView, out));

    REQUIRE(out.sceneLinearRgb == front.sceneLinearRgb);
    REQUIRE(near(out.visibility.inferredWeight, 1.0));
    REQUIRE(near(out.visibility.evidenceWeight, 0.0));
    REQUIRE(near(out.visibility.residualTransmittance, 0.0));
}

}  // namespace

int main() {
    test_camera_plane_contribution_contract();
    test_non_evidence_cannot_claim_scientific_authority();
    test_packet_order_and_digest_are_deterministic();
    test_scientific_view_excludes_non_evidence();
    test_open_scene_view_adds_inferred_without_promoting_it();
    test_view_progression_is_strict();
    test_opaque_foreground_occludes_farther_samples();

    std::cout << "FreeWorldDeepSceneContribution/0.4 PASS\n";
    std::cout << "resolve_method=" << deep::kResolveMethodId << "\n";
    std::cout << "scientific_writeback_allowed=0\n";
    std::cout << "creates_new_evidence=0\n";
    return 0;
}
