#include "truthnegative_deep_scene_bridge_v0_8.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace bridge = truthraw::truthnegative_deep_scene_bridge::v0_8;
namespace tn = truthraw::truthnegative_continuous::v0_5;
namespace fw = truthraw::free_world_pixel_resolve_2d::v0_2;
namespace binding = truthraw::free_world_deep_scene_binding::v0_5;
namespace light = truthraw::free_world_light_transport_state::v0_6;

#define REQUIRE(x) do { if (!(x)) throw std::runtime_error(#x); } while (0)

namespace {

bridge::Digest digest(std::uint8_t seed) {
    bridge::Digest d{};
    for (std::size_t i = 0u; i < d.size(); ++i) {
        d[i] = static_cast<std::uint8_t>(seed + i);
    }
    return d;
}

class Scene final : public fw::IScenePlaneSource {
public:
    std::uint32_t width() const noexcept override { return 4u; }
    std::uint32_t height() const noexcept override { return 3u; }

    bool readPixel(
        std::uint32_t x,
        std::uint32_t y,
        fw::SourcePixel& out) const noexcept override {
        if (x >= width() || y >= height()) return false;
        for (std::size_t c = 0u; c < 3u; ++c) {
            auto& s = out.channel[c];
            s.value =
                -0.1 +
                0.4 * static_cast<double>(c) +
                0.03 * static_cast<double>(x) +
                0.02 * static_cast<double>(y);
            s.role =
                c == 1u
                    ? fw::SourceCreationRole::SourceMeasuredCfa
                    : fw::SourceCreationRole::ScientificReconstruction;
            s.authority =
                c == 1u
                    ? fw::SourceAuthority::CalibratedEstimate
                    : fw::SourceAuthority::Unknown;
            s.contributionMask =
                c == 1u ? 0x01u : 0x0au;
        }
        return true;
    }
};

tn::State state() {
    tn::StateInput in{};
    in.sourceEvidenceSha256 = digest(1u);
    in.scientificMasterSha256 = digest(33u);
    in.authorityFieldSha256 = digest(65u);
    in.width = 4u;
    in.height = 3u;
    in.reconstructionBackendId = "TEST_F64";
    in.colourBindingId = "TEST_COLOR";
    tn::State out{};
    REQUIRE(tn::finalizeState(in, out));
    return out;
}

tn::QueryResult query(const tn::State& s) {
    Scene scene;
    tn::QueryResult q{};
    REQUIRE(tn::resolvePixel(scene, s, 8u, 6u, 3u, 2u, q));
    return q;
}

bridge::CameraPlaneObjectInput object(
    binding::GeometryAuthority geometry) {
    bridge::CameraPlaneObjectInput in{};
    in.provenanceId = 1001u;
    in.regionId = 2002u;
    in.objectId = 3003u;
    in.depth = 4.0;
    in.geometryAuthority = geometry;
    return in;
}

void test_truthnegative_query_binds_to_deep_scene() {
    const auto s = state();
    const auto q = query(s);

    bridge::ScenePacket packet{};
    REQUIRE(bridge::buildCameraPlaneObject(
        s,
        q,
        object(binding::GeometryAuthority::ImagePlaneBound),
        packet));

    REQUIRE(packet.truthNegativeStateSha256 == s.stateSha256);
    REQUIRE(packet.truthNegativeQuerySha256 == q.querySha256);
    REQUIRE(packet.scenePacketSha256 != bridge::Digest{});
    REQUIRE(packet.deepPacket.finalized);
    REQUIRE(packet.boundPacket.finalized);
    REQUIRE(packet.radiometryBoundToTruthNegative);
    REQUIRE(packet.geometryAuthoritySeparate);
    REQUIRE(!packet.createsNewEvidence);
    REQUIRE(!packet.scientificWritebackAllowed);

    const auto& contribution =
        packet.boundPacket.contributions.front();
    REQUIRE(contribution.metadata.regionId == 2002u);
    REQUIRE(contribution.metadata.objectId == 3003u);
    REQUIRE(contribution.metadata.geometryAuthority ==
        binding::GeometryAuthority::ImagePlaneBound);

    for (std::size_t c = 0u; c < 3u; ++c) {
        REQUIRE(contribution.sample.channelAuthority[c] ==
            q.pixel.support[c].authority);
    }
}

void test_inferred_geometry_does_not_change_radiometric_authority() {
    const auto s = state();
    const auto q = query(s);
    bridge::ScenePacket packet{};
    REQUIRE(bridge::buildCameraPlaneObject(
        s,
        q,
        object(binding::GeometryAuthority::Inferred),
        packet));

    const auto& contribution =
        packet.boundPacket.contributions.front();
    REQUIRE(contribution.metadata.geometryAuthority ==
        binding::GeometryAuthority::Inferred);
    for (std::size_t c = 0u; c < 3u; ++c) {
        REQUIRE(contribution.sample.channelAuthority[c] ==
            q.pixel.support[c].authority);
    }
}

void test_state_query_mismatch_fails_closed() {
    auto a = state();
    auto b = state();
    b.input.scientificMasterSha256[0] ^= 1u;
    REQUIRE(tn::finalizeState(b.input, b));
    const auto q = query(a);

    bridge::ScenePacket packet{};
    REQUIRE(!bridge::buildCameraPlaneObject(
        b,
        q,
        object(binding::GeometryAuthority::ImagePlaneBound),
        packet));
}

void test_inferred_light_transport_seed_stays_hypothesis() {
    const auto s = state();
    const auto q = query(s);
    bridge::ScenePacket packet{};
    REQUIRE(bridge::buildCameraPlaneObject(
        s,
        q,
        object(binding::GeometryAuthority::ImagePlaneBound),
        packet));

    bridge::InferredLambertianSeedInput input{};
    input.scene = packet;
    input.provenanceId = 4004u;
    input.regionId = 2002u;
    input.objectId = 3003u;
    input.incomingDirection = {0.0, 0.0, 1.0};
    input.outgoingDirection = {0.0, 0.0, 1.0};
    input.surfaceNormal = {0.0, 0.0, 1.0};
    input.materialIdentitySha256 = digest(80u);
    input.illuminationIdentitySha256 = digest(100u);
    input.materialSpectralHypothesisSha256 = digest(120u);
    input.illuminationSpectralHypothesisSha256 = digest(140u);
    input.diffuseReflectanceRgb = {0.7, 0.5, 0.3};
    input.visibility = 0.8;

    bridge::InferredLambertianSeedResult result{};
    REQUIRE(bridge::buildInferredLambertianSeed(input, result));
    REQUIRE(result.state.finalized);
    REQUIRE(result.state.contributionClass ==
        truthraw::free_world_deep_scene_contribution::v0_4::
            ContributionClass::InferredScene);
    REQUIRE(result.state.material.authority ==
        light::ParameterAuthority::Inferred);
    REQUIRE(result.state.illumination.authority ==
        light::ParameterAuthority::Inferred);
    REQUIRE(result.state.surface.normalAuthority ==
        binding::GeometryAuthority::Inferred);
    REQUIRE(!result.inheritedScientificRadiometryAsMeasurement);
    REQUIRE(!result.createsNewEvidence);
    REQUIRE(!result.scientificWritebackAllowed);
}

void test_object_identity_changes_packet_identity() {
    const auto s = state();
    const auto q = query(s);
    auto aObject = object(binding::GeometryAuthority::Inferred);
    auto bObject = aObject;
    bObject.objectId += 1u;

    bridge::ScenePacket a{};
    bridge::ScenePacket b{};
    REQUIRE(bridge::buildCameraPlaneObject(s, q, aObject, a));
    REQUIRE(bridge::buildCameraPlaneObject(s, q, bObject, b));
    REQUIRE(a.scenePacketSha256 != b.scenePacketSha256);
}

}  // namespace

int main() {
    test_truthnegative_query_binds_to_deep_scene();
    test_inferred_geometry_does_not_change_radiometric_authority();
    test_state_query_mismatch_fails_closed();
    test_inferred_light_transport_seed_stays_hypothesis();
    test_object_identity_changes_packet_identity();

    std::cout << "TruthNegativeDeepSceneBridge/0.8 PASS\n";
    std::cout << "geometry_radiometry_separate=1\n";
    std::cout << "inferred_light_transport_is_measurement=0\n";
    std::cout << "scientific_writeback_allowed=0\n";
    return 0;
}
