#include "illumination_state_v0_82.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace illum = truthraw::illumination_state::v0_82;

namespace {
#define REQUIRE(x) do { if(!(x)){ std::cerr<<"FAIL line "<<__LINE__<<": "<<#x<<"\n"; std::exit(2);} } while(0)

illum::Digest digest(std::uint8_t seed) {
    illum::Digest d{};
    for (std::size_t i=0;i<d.size();++i) {
        d[i]=static_cast<std::uint8_t>(seed+i);
    }
    return d;
}

illum::Input base_input() {
    illum::Input in{};
    in.sourceEvidenceSha256=digest(1);
    in.scientificMasterSha256=digest(51);
    in.canonicalOpenSceneSha256=digest(101);
    in.sourceEvidenceId="sha256:0123456789abcdef";
    in.colourBindingId="dng-ifd0-source-metadata-v0.2:dual-ict:0123456789abcdef";
    in.physicalFrameCount=1u;
    in.independentEvidenceCount=1u;
    return in;
}

bool nonzero(const illum::Digest& d) {
    return std::any_of(d.begin(),d.end(),[](std::uint8_t v){return v!=0u;});
}

void uv_to_xy(double u,double v,double& x,double& y) {
    const double denom=2.0*u-8.0*v+4.0;
    REQUIRE(std::abs(denom)>1e-12);
    x=3.0*u/denom;
    y=2.0*v/denom;
}

void unknown_without_public_white_audit() {
    const auto in=base_input();
    illum::State s{};
    REQUIRE(illum::build(in,s));
    REQUIRE(s.whitePointAuthority==illum::EstimateAuthority::Unknown);
    REQUIRE(!s.whitePointKnown);
    REQUIRE(s.sceneLightKind==illum::SceneLightKind::Unknown);
    REQUIRE(s.spectrumAuthority==illum::SpectrumAuthority::Unknown);
    REQUIRE(s.directionAuthority==illum::SpatialAuthority::Unknown);
    REQUIRE(s.spatialExtentAuthority==illum::SpatialAuthority::Unknown);
    REQUIRE(s.temporalModulationAuthority==illum::TemporalAuthority::Unknown);
    REQUIRE(!s.cctIsSpdProof);
    REQUIRE(!s.calibrationIlluminantsAreSceneLightProof);
    REQUIRE(!s.createsNewEvidence);
    REQUIRE(!s.scientificMasterModified);
    REQUIRE(!s.channelAuthorityModified);
    REQUIRE(!s.counterfactual);
    REQUIRE(s.physicalFrameCount==1u);
    REQUIRE(s.independentEvidenceCount==1u);
    REQUIRE(nonzero(s.stateSha256));
}

void planckian_point_has_near_zero_duv_but_no_light_kind_claim() {
    auto in=base_input();
    in.dualCalibrationUsed=true;
    in.profileCalibrationIlluminant1=17; // profile reference only
    in.profileCalibrationIlluminant2=21; // profile reference only
    in.resolvedWhiteAvailable=true;

    // Exact Robertson table locus point r=200 (about 5000 K reciprocal-MK coordinate).
    uv_to_xy(0.21142,0.32312,in.resolvedWhiteX,in.resolvedWhiteY);
    in.resolvedWhiteCctK=5000.0;

    illum::State s{};
    REQUIRE(illum::build(in,s));
    REQUIRE(s.whitePointKnown);
    REQUIRE(s.whitePointAuthority==illum::EstimateAuthority::SourceMetadataBoundEstimate);
    REQUIRE(std::abs(s.duv1960PolylineEstimate)<=1e-12);
    REQUIRE(s.correlatedColorTemperatureK==5000.0);
    REQUIRE(s.sceneLightKind==illum::SceneLightKind::Unknown);
    REQUIRE(s.spectrumAuthority==illum::SpectrumAuthority::Unknown);
    REQUIRE(!s.cctIsSpdProof);
    REQUIRE(!s.calibrationIlluminantsAreSceneLightProof);
}

void off_locus_white_has_finite_duv_without_semantic_promotion() {
    auto in=base_input();
    in.dualCalibrationUsed=true;
    in.resolvedWhiteAvailable=true;
    uv_to_xy(0.21142,0.32912,in.resolvedWhiteX,in.resolvedWhiteY);
    in.resolvedWhiteCctK=5000.0;

    illum::State s{};
    REQUIRE(illum::build(in,s));
    REQUIRE(std::isfinite(s.duv1960PolylineEstimate));
    REQUIRE(std::abs(s.duv1960PolylineEstimate)>1e-5);
    REQUIRE(s.sceneLightKind==illum::SceneLightKind::Unknown);
    REQUIRE(s.spectrumAuthority==illum::SpectrumAuthority::Unknown);
    REQUIRE(s.directionAuthority==illum::SpatialAuthority::Unknown);
    REQUIRE(s.temporalModulationAuthority==illum::TemporalAuthority::Unknown);
}

void calibration_reference_codes_change_identity_not_scene_kind() {
    auto a=base_input();
    a.dualCalibrationUsed=true;
    a.profileCalibrationIlluminant1=17;
    a.profileCalibrationIlluminant2=21;

    auto b=a;
    b.profileCalibrationIlluminant1=3;
    b.profileCalibrationIlluminant2=11;

    illum::State sa{},sb{};
    REQUIRE(illum::build(a,sa));
    REQUIRE(illum::build(b,sb));
    REQUIRE(sa.sceneLightKind==illum::SceneLightKind::Unknown);
    REQUIRE(sb.sceneLightKind==illum::SceneLightKind::Unknown);
    REQUIRE(sa.stateSha256!=sb.stateSha256);
}

void ancestry_changes_identity() {
    auto a=base_input();
    auto b=a;
    b.sourceEvidenceSha256[0]^=1u;

    illum::State sa{},sb{};
    REQUIRE(illum::build(a,sa));
    REQUIRE(illum::build(b,sb));
    REQUIRE(sa.stateSha256!=sb.stateSha256);

    b=a;
    b.scientificMasterSha256[0]^=1u;
    REQUIRE(illum::build(b,sb));
    REQUIRE(sa.stateSha256!=sb.stateSha256);

    b=a;
    b.canonicalOpenSceneSha256[0]^=1u;
    REQUIRE(illum::build(b,sb));
    REQUIRE(sa.stateSha256!=sb.stateSha256);
}

void invalid_white_fails_closed() {
    auto in=base_input();
    in.resolvedWhiteAvailable=true;
    in.resolvedWhiteX=0.8;
    in.resolvedWhiteY=0.4;
    in.resolvedWhiteCctK=5000.0;
    illum::State s{};
    REQUIRE(!illum::build(in,s));

    in=base_input();
    in.resolvedWhiteAvailable=true;
    in.resolvedWhiteX=0.33;
    in.resolvedWhiteY=0.33;
    in.resolvedWhiteCctK=0.0;
    REQUIRE(!illum::build(in,s));
}

void evidence_count_must_remain_one_one() {
    auto in=base_input();
    illum::State s{};
    in.physicalFrameCount=2u;
    REQUIRE(!illum::build(in,s));
    in=base_input();
    in.independentEvidenceCount=2u;
    REQUIRE(!illum::build(in,s));
}

void deterministic() {
    const auto in=base_input();
    illum::State a{},b{};
    REQUIRE(illum::build(in,a));
    REQUIRE(illum::build(in,b));
    REQUIRE(a.stateSha256==b.stateSha256);
}

} // namespace

int main(){
    unknown_without_public_white_audit();
    planckian_point_has_near_zero_duv_but_no_light_kind_claim();
    off_locus_white_has_finite_duv_without_semantic_promotion();
    calibration_reference_codes_change_identity_not_scene_kind();
    ancestry_changes_identity();
    invalid_white_fails_closed();
    evidence_count_must_remain_one_one();
    deterministic();
    std::cout<<"ILLUMINATION_STATE_V0_82_PASS\n";
    return 0;
}
