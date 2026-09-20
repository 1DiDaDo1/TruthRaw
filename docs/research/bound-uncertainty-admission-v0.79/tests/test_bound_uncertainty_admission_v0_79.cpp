#include "bound_uncertainty_admission_v0_79.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace admission = truthraw::bound_uncertainty_admission::v0_79;

namespace {
#define REQUIRE(x) do { if(!(x)){ std::cerr<<"FAIL line "<<__LINE__<<": "<<#x<<"\n"; std::exit(2);} } while(0)

admission::Digest from_hex(std::string_view s) {
    admission::Digest out{};
    REQUIRE(s.size()==64u);
    auto nib=[](char c)->int{
        if(c>='0'&&c<='9')return c-'0';
        if(c>='a'&&c<='f')return 10+c-'a';
        if(c>='A'&&c<='F')return 10+c-'A';
        return -1;
    };
    for(std::size_t i=0;i<32u;++i){
        const int a=nib(s[2u*i]),b=nib(s[2u*i+1u]);
        REQUIRE(a>=0&&b>=0);
        out[i]=static_cast<std::uint8_t>((a<<4)|b);
    }
    return out;
}

admission::Candidate exact_historical_candidate() {
    admission::Candidate c{};
    c.sourceDomain=admission::SourceDomain::HonorBkqN49TeleVendorDngV5gP1;
    c.make="HONOR";
    c.model="BKQ-N49";
    c.sourceClass="HONOR vendor DNG";
    c.reconstructionBackendId="research_edge_aware_support_limited_measured_preserving_v47i";
    c.width=4080u;c.height=3072u;c.cfaCode=0u;
    c.whiteLevel=1023.0f;c.focalLengthMm=22.48f;c.noiseProfileValid=true;
    c.sourceEvidenceSha256=from_hex("7f64a628ff431272fc76f17b63179c93af8e496c40cdbd3b45d7dd67fd58b344");
    c.decodedCfaSha256=from_hex("883cbe13fd2a5afe7e9f3f4147df3a8ed3be681340a3c0422933f42d0f4d719c");
    c.productionCoreCppSha256=from_hex("68f52c4896d47c4605adc1cf670e55f95d42a687e18c06a167028a64ce37b94c");
    c.productionCoreHSha256=from_hex("b7f6e2189d6ecccfc5fda80084041990bd12757145c5ff2c40f2ae0d0046b167");
    c.productionBackendCombinedSha256=from_hex("8d3a2ad2a97729c2a6a72028099dca7eda6190fcc4df3a3a4020f1c92e15c7ca");
    c.featureExtractorSha256=from_hex("b1cfbf061a32aa4abccb9d8bb86a9b5b9257f88498081eb9aa659f9402d0919d");
    c.featureSchemaSha256=from_hex("8c8e56b762b83a5a846c5201ab3ba43c9a2549d896873cc0ceb66574e74a83e3");
    c.uncertaintyModelSha256=from_hex("8831bee921999e823466cfc462812e40620b2834080c0f7d64c6f11d7ead626f");
    c.uncertaintyBindingSha256=from_hex("61c99b0e29316730ca1323fd3e91fd069ebbc50cba73127cd81b9803b10178a0");
    c.backendBindingFileSha256=from_hex("d0134dc5322cf721778d1b5e4607c2e82e0dde45b909acfa2e6ea2c8e588d42d");
    c.prospectiveResultSha256=from_hex("7895961af0b4fd942edeeb32ec79476a3b841c74862af9fa5088681682116781");
    c.ptcBridgeFileSha256=from_hex("0bfce3cd071e6667454f1e9a390057848168a69602f58350b365952e521186bb");
    return c;
}

bool nonzero(const admission::Digest& d) {
    return std::any_of(d.begin(),d.end(),[](std::uint8_t v){return v!=0u;});
}

void exact_old_tele_still_hits_trace_gate() {
    const auto c=exact_historical_candidate();
    const auto d=admission::evaluate(c);
    REQUIRE(d.code==admission::DecisionCode::EligibleTraceGateOpen);
    REQUIRE(d.sourceClassExact);
    REQUIRE(d.backendExact);
    REQUIRE(d.assetsExact);
    REQUIRE(d.prospectiveEvidenceExact);
    REQUIRE(!d.f64TraceCertified);
    REQUIRE(!d.reconstructedAuthorityAllowed);
    REQUIRE(nonzero(d.uncertaintyBindingSha256));
    REQUIRE(nonzero(d.decisionSha256));

    auto fake=c;
    fake.f64TraceCertificateSha256=from_hex(
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    const auto f=admission::evaluate(fake);
    REQUIRE(f.code==admission::DecisionCode::EligibleTraceGateOpen);
    REQUIRE(!f.reconstructedAuthorityAllowed);
    REQUIRE(f.decisionSha256!=d.decisionSha256);
}

void camera5_never_inherits_old_tele_model() {
    auto src=from_hex("a2e3742c0616a9cd28cb8d2de347363dfb7ab852d9a7d734b8421911d36aa359");
    auto c=admission::make_current_camera5_derived_blocked_candidate(
        src,4080u,3072u,0u,1023.0f,
        "research_edge_aware_support_limited_measured_preserving_v47i");
    const auto d=admission::evaluate(c);
    REQUIRE(d.code==admission::DecisionCode::BlockedSourceDomainMismatch);
    REQUIRE(!d.reconstructedAuthorityAllowed);
    REQUIRE(nonzero(d.decisionSha256));
}

void unattested_import_blocks() {
    admission::Candidate c{};
    c.sourceEvidenceSha256=from_hex("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
    const auto d=admission::evaluate(c);
    REQUIRE(d.code==admission::DecisionCode::BlockedNoSourceAttestation);
    REQUIRE(!d.reconstructedAuthorityAllowed);
}

void each_exact_scope_axis_fails_closed() {
    const auto base=exact_historical_candidate();

    auto source=base;source.focalLengthMm=22.47f;
    REQUIRE(admission::evaluate(source).code==admission::DecisionCode::BlockedSourceClassMismatch);

    auto backend=base;backend.productionCoreCppSha256[0]^=1u;
    REQUIRE(admission::evaluate(backend).code==admission::DecisionCode::BlockedBackendMismatch);

    auto model=base;model.uncertaintyModelSha256[0]^=1u;
    REQUIRE(admission::evaluate(model).code==admission::DecisionCode::BlockedModelAssetMismatch);

    auto prospective=base;prospective.prospectiveResultSha256[0]^=1u;
    REQUIRE(admission::evaluate(prospective).code==admission::DecisionCode::BlockedModelAssetMismatch);

    auto dimensions=base;dimensions.width=4079u;
    REQUIRE(admission::evaluate(dimensions).code==admission::DecisionCode::BlockedSourceClassMismatch);
}

void decision_is_deterministic() {
    const auto c=exact_historical_candidate();
    const auto a=admission::evaluate(c);
    const auto b=admission::evaluate(c);
    REQUIRE(a.decisionSha256==b.decisionSha256);
    REQUIRE(a.scopeId==b.scopeId);
    REQUIRE(a.reason==b.reason);
}

} // namespace

int main(){
    exact_old_tele_still_hits_trace_gate();
    camera5_never_inherits_old_tele_model();
    unattested_import_blocks();
    each_exact_scope_axis_fails_closed();
    decision_is_deterministic();
    std::cout<<"BOUND_UNCERTAINTY_ADMISSION_V0_79_PASS\n";
    return 0;
}
