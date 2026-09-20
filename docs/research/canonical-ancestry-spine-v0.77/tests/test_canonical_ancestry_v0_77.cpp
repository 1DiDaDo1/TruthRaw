#include "canonical_ancestry_v0_77.h"

#include <cstdlib>
#include <iostream>

namespace ancestry = truthraw::canonical_ancestry::v0_77;
namespace backplane = truthraw::technical_backplane::v0_1;

namespace {
#define REQUIRE(x) do { if(!(x)){ std::cerr<<"FAIL line "<<__LINE__<<": "<<#x<<"\n"; std::exit(2);} } while(0)

ancestry::Digest digest(std::uint8_t seed) {
    ancestry::Digest d{};
    for (std::size_t i=0;i<d.size();++i) d[i]=static_cast<std::uint8_t>(seed+i);
    return d;
}

ancestry::Binding binding() {
    ancestry::Binding b{};
    b.sourceEvidenceSha256=digest(1);
    b.scientificMasterSha256=digest(41);
    b.zeroLineSha256=digest(81);
    b.sceneScaleSha256=digest(121);
    b.openSceneArtifactSha256=digest(151);
    b.derivativeRasterSha256=digest(181);
    b.restorationRoleMaskSha256=digest(211);
    b.width=4080;
    b.height=3072;
    b.sourceEvidenceId="CAMERA_CAPTURE_SOURCE_A";
    b.colourBindingId="SOURCE_METADATA_BOUND_DUAL_ILLUMINANT";
    b.precisionPolicyId="TRUTHRAW_STAGE_SPECIFIC_PRECISION_V1";
    b.reconstructionBackendId="research_edge_aware_support_limited_measured_preserving_v47i";

    backplane::State s{};
    s.sourceEvidenceHash=b.sourceEvidenceSha256;
    s.scientificMasterHash=b.scientificMasterSha256;
    s.zeroLineHash=b.zeroLineSha256;
    s.sceneScaleHash=b.sceneScaleSha256;
    s.physicalFrameCount=1;
    s.independentEvidenceCount=1;
    s.roomStatus.fill(backplane::RoomStatus::ResearchOnly);
    s.claimStatus=backplane::ClaimStatus::Candidate;
    s.forbiddenFlags=0;
    REQUIRE(backplane::serialize(s,b.serializedBackplane)==backplane::Status::Ok);
    return b;
}

void deterministic() {
    const auto b=binding();
    ancestry::Manifest a{},c{};
    REQUIRE(ancestry::build(b,a));
    REQUIRE(ancestry::build(b,c));
    REQUIRE(ancestry::validate_manifest(b,a));
    REQUIRE(a.canonicalText==c.canonicalText);
    REQUIRE(a.sha256==c.sha256);
    REQUIRE(a.technicalBackplaneSha256==c.technicalBackplaneSha256);
    REQUIRE(a.canonicalText.find("schema=TruthRawCanonicalAncestry/0.77\n") == 0u);
    REQUIRE(a.canonicalText.find("scientific_writeback_allowed=0\n") != std::string::npos);
    REQUIRE(a.canonicalText.find("knowledge_claims_may_exceed_evidence=0\n") != std::string::npos);
}

void each_parent_changes_identity() {
    const auto base=binding();
    ancestry::Manifest m0{}; REQUIRE(ancestry::build(base,m0));
    for(int which=0;which<7;++which){
        auto b=base;
        switch(which){
            case 0: b.sourceEvidenceSha256[0]^=1u; break;
            case 1: b.scientificMasterSha256[0]^=1u; break;
            case 2: b.zeroLineSha256[0]^=1u; break;
            case 3: b.sceneScaleSha256[0]^=1u; break;
            case 4: b.openSceneArtifactSha256[0]^=1u; break;
            case 5: b.derivativeRasterSha256[0]^=1u; break;
            case 6: b.restorationRoleMaskSha256[0]^=1u; break;
        }
        if(which<=3){
            // Core lineage mutation must fail because Backplane no longer agrees.
            ancestry::Manifest bad{};
            REQUIRE(!ancestry::build(b,bad));
        } else {
            ancestry::Manifest m{}; REQUIRE(ancestry::build(b,m));
            REQUIRE(m.sha256!=m0.sha256);
        }
    }
}

void text_injection_fails() {
    auto b=binding();
    b.colourBindingId="SAFE\nsource_evidence_sha256=fake";
    ancestry::Manifest m{};
    REQUIRE(!ancestry::build(b,m));
}

void evidence_counts_fail_closed() {
    auto b=binding();
    b.independentEvidenceCount=2;
    ancestry::Manifest m{};
    REQUIRE(!ancestry::build(b,m));
}

} // namespace

int main(){
    deterministic();
    each_parent_changes_identity();
    text_injection_fails();
    evidence_counts_fail_closed();
    std::cout<<"CANONICAL_ANCESTRY_V0_77_PASS\n";
    return 0;
}
