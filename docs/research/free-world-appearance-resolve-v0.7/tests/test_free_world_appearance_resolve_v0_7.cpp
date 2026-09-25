#include "free_world_appearance_resolve_v0_7.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace app = truthraw::free_world_appearance_resolve::v0_7;
namespace deep = truthraw::free_world_deep_scene_contribution::v0_4;
namespace fw = truthraw::free_world_pixel_resolve_2d::v0_2;

#define REQUIRE(x) do { if (!(x)) throw std::runtime_error(#x); } while (0)

namespace {

app::Digest digest(std::uint8_t seed) {
    app::Digest d{};
    for (std::size_t i = 0u; i < d.size(); ++i) {
        d[i] = static_cast<std::uint8_t>(seed + i);
    }
    return d;
}

app::Matrix3 identityMatrix() {
    app::Matrix3 m{};
    m.m = {
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0};
    return m;
}

deep::DeepResolvedPixel scenePixel() {
    deep::DeepResolvedPixel p{};
    p.sceneLinearRgb = {0.18, 0.18, 0.18};
    p.channelAuthority = {
        fw::ResolvedAuthority::Reconstructed,
        fw::ResolvedAuthority::Censored,
        fw::ResolvedAuthority::Unknown};
    p.uncertaintyKnown = {true, false, false};
    p.p95Uncertainty = {0.02, 0.0, 0.0};
    p.sourcePacketSha256 = digest(1u);
    p.createsNewEvidence = false;
    p.scientificWritebackAllowed = false;
    p.physicalFrameCount = 1u;
    p.independentEvidenceCount = 1u;
    return p;
}

app::AppearanceInput baseInput() {
    app::AppearanceInput in{};
    in.scene = scenePixel();

    in.sceneColorimetry.rgbToXyz = identityMatrix();
    in.sceneColorimetry.sceneReferenceWhiteNits = 100.0;
    in.sceneColorimetry.identitySha256 = digest(20u);

    in.viewing.adaptingLuminanceNits = 20.0;
    in.viewing.backgroundLuminanceNits = 20.0;
    in.viewing.surround = app::Surround::Average;
    in.viewing.viewingDistanceMeters = 0.5;
    in.viewing.identitySha256 = digest(40u);

    in.display.xyzToRgb = identityMatrix();
    in.display.referenceWhiteNits = 100.0;
    in.display.peakLuminanceNits = 100.0;
    in.display.blackLuminanceNits = 0.0;
    in.display.transfer = app::TransferFunction::Srgb;
    in.display.identitySha256 = digest(60u);

    in.policy.exposureEv = 0.0;
    in.policy.colorfulnessScale = 1.0;
    in.policy.highlightCompression = 1.0;
    in.policy.identitySha256 = digest(80u);
    return in;
}

void test_transfer_functions() {
    REQUIRE(std::abs(app::encodeSrgb(0.0)) < 1e-15);
    REQUIRE(std::abs(app::encodeSrgb(1.0) - 1.0) < 1e-12);

    const double pq0 = app::encodePqSt2084(0.0);
    const double pq100 = app::encodePqSt2084(100.0);
    const double pq1000 = app::encodePqSt2084(1000.0);
    REQUIRE(pq0 >= 0.0);
    REQUIRE(pq100 > pq0);
    REQUIRE(pq1000 > pq100);
    REQUIRE(pq1000 < 1.0);
}

void test_neutral_remains_neutral() {
    auto in = baseInput();
    app::AppearanceResolvedPixel out{};
    REQUIRE(app::resolveAppearance(in, out));

    REQUIRE(std::abs(out.encodedRgb[0] - out.encodedRgb[1]) < 1e-12);
    REQUIRE(std::abs(out.encodedRgb[1] - out.encodedRgb[2]) < 1e-12);
    REQUIRE(out.appearanceApplied);
    REQUIRE(out.displayEncoded);
    REQUIRE(!out.sourceSceneMutated);
    REQUIRE(!out.createsNewEvidence);
    REQUIRE(!out.scientificWritebackAllowed);
}

void test_authority_and_uncertainty_are_not_upgraded() {
    auto in = baseInput();
    app::AppearanceResolvedPixel out{};
    REQUIRE(app::resolveAppearance(in, out));

    REQUIRE(out.channelAuthority == in.scene.channelAuthority);
    REQUIRE(out.uncertaintyKnown == in.scene.uncertaintyKnown);
    REQUIRE(out.p95Uncertainty == in.scene.p95Uncertainty);
    REQUIRE(out.sourceSceneSha256 == in.scene.sourcePacketSha256);
}

void test_display_target_changes_output_not_scene_identity() {
    auto sdr = baseInput();
    auto hdr = baseInput();

    hdr.display.peakLuminanceNits = 1000.0;
    hdr.display.transfer = app::TransferFunction::PqSt2084;
    hdr.display.identitySha256 = digest(61u);

    sdr.scene.sceneLinearRgb = {4.0, 4.0, 4.0};
    hdr.scene.sceneLinearRgb = sdr.scene.sceneLinearRgb;

    app::AppearanceResolvedPixel a{};
    app::AppearanceResolvedPixel b{};
    REQUIRE(app::resolveAppearance(sdr, a));
    REQUIRE(app::resolveAppearance(hdr, b));

    REQUIRE(a.sourceSceneSha256 == b.sourceSceneSha256);
    REQUIRE(a.channelAuthority == b.channelAuthority);
    REQUIRE(a.outputSha256 != b.outputSha256);
    REQUIRE(a.appearanceStateSha256 != b.appearanceStateSha256);
}

void test_viewing_conditions_change_appearance_only() {
    auto average = baseInput();
    auto dark = baseInput();

    dark.viewing.surround = app::Surround::Dark;
    dark.viewing.identitySha256 = digest(41u);
    average.scene.sceneLinearRgb = {0.04, 0.04, 0.04};
    dark.scene.sceneLinearRgb = average.scene.sceneLinearRgb;

    app::AppearanceResolvedPixel a{};
    app::AppearanceResolvedPixel b{};
    REQUIRE(app::resolveAppearance(average, a));
    REQUIRE(app::resolveAppearance(dark, b));

    REQUIRE(a.sourceSceneSha256 == b.sourceSceneSha256);
    REQUIRE(a.channelAuthority == b.channelAuthority);
    REQUIRE(a.outputSha256 != b.outputSha256);
    REQUIRE(std::abs(a.encodedRgb[0] - b.encodedRgb[0]) > 1e-8);
}

void test_highlights_compress_to_peak_without_scene_mutation() {
    auto in = baseInput();
    in.scene.sceneLinearRgb = {20.0, 20.0, 20.0};
    in.display.peakLuminanceNits = 400.0;
    in.display.referenceWhiteNits = 100.0;
    in.display.transfer = app::TransferFunction::LinearNormalized;

    app::AppearanceResolvedPixel out{};
    REQUIRE(app::resolveAppearance(in, out));
    REQUIRE(out.mappedLuminanceNits <= 400.0);
    REQUIRE(out.displayLinearNits[0] <= 400.0);
    REQUIRE(!out.sourceSceneMutated);
    REQUIRE(out.sourceSceneSha256 == in.scene.sourcePacketSha256);
}

void test_display_clamp_is_explicit() {
    auto in = baseInput();
    in.scene.sceneLinearRgb = {-1.0, 0.5, 2.0};
    in.display.transfer = app::TransferFunction::LinearNormalized;

    app::AppearanceResolvedPixel out{};
    REQUIRE(app::resolveAppearance(in, out));
    REQUIRE(out.gamutOrDisplayClampApplied);
    for (double v : out.encodedRgb) {
        REQUIRE(v >= 0.0);
        REQUIRE(v <= 1.0);
    }
    REQUIRE(in.scene.sceneLinearRgb[0] == -1.0);
    REQUIRE(in.scene.sceneLinearRgb[2] == 2.0);
}

void test_invalid_identity_fails_closed() {
    auto in = baseInput();
    in.display.identitySha256 = {};
    app::AppearanceResolvedPixel out{};
    REQUIRE(!app::resolveAppearance(in, out));
}

}  // namespace

int main() {
    test_transfer_functions();
    test_neutral_remains_neutral();
    test_authority_and_uncertainty_are_not_upgraded();
    test_display_target_changes_output_not_scene_identity();
    test_viewing_conditions_change_appearance_only();
    test_highlights_compress_to_peak_without_scene_mutation();
    test_display_clamp_is_explicit();
    test_invalid_identity_fails_closed();

    std::cout << "FreeWorldAppearanceResolve/0.7 PASS\n";
    std::cout << "appearance_method=" << app::kAppearanceMethodId << "\n";
    std::cout << "source_scene_mutated=0\n";
    std::cout << "scientific_writeback_allowed=0\n";
    return 0;
}
