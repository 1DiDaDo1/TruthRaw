#include "bound_uncertainty_admission_v0_79.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <string_view>
#include <utility>

namespace truthraw::bound_uncertainty_admission::v0_79 {
namespace {

constexpr std::string_view kScopeId =
    "HONOR_BKQ_N49_TELE_VENDOR_DNG_4080X3072_BGGR_W1023_F22_48_V5G_P1";
constexpr std::string_view kBackendId =
    "research_edge_aware_support_limited_measured_preserving_v47i";

// There is intentionally no accepted F64 trace certificate yet.
// A future version must replace this zero digest with one exact, independently
// validated certificate digest before DecisionCode::Admitted can become reachable.
constexpr Digest kExpectedF64TraceCertificate{};

Digest from_hex(std::string_view s) noexcept {
    Digest out{};
    if (s.size() != 64u) return out;
    auto nibble=[](char c)->int{
        if(c>='0'&&c<='9')return c-'0';
        if(c>='a'&&c<='f')return 10+c-'a';
        if(c>='A'&&c<='F')return 10+c-'A';
        return -1;
    };
    for(std::size_t i=0;i<32u;++i){
        const int a=nibble(s[2u*i]),b=nibble(s[2u*i+1u]);
        if(a<0||b<0)return Digest{};
        out[i]=static_cast<std::uint8_t>((a<<4)|b);
    }
    return out;
}

bool nonzero(const Digest& d) noexcept {
    return std::any_of(d.begin(),d.end(),[](std::uint8_t v){return v!=0u;});
}

bool exact_float(float a,float b) noexcept {
    return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);
}

const Digest& expected_core_cpp() noexcept {
    static const Digest d=from_hex("68f52c4896d47c4605adc1cf670e55f95d42a687e18c06a167028a64ce37b94c");
    return d;
}
const Digest& expected_core_h() noexcept {
    static const Digest d=from_hex("b7f6e2189d6ecccfc5fda80084041990bd12757145c5ff2c40f2ae0d0046b167");
    return d;
}
const Digest& expected_backend_combined() noexcept {
    static const Digest d=from_hex("8d3a2ad2a97729c2a6a72028099dca7eda6190fcc4df3a3a4020f1c92e15c7ca");
    return d;
}
const Digest& expected_extractor() noexcept {
    static const Digest d=from_hex("b1cfbf061a32aa4abccb9d8bb86a9b5b9257f88498081eb9aa659f9402d0919d");
    return d;
}
const Digest& expected_feature_schema() noexcept {
    static const Digest d=from_hex("8c8e56b762b83a5a846c5201ab3ba43c9a2549d896873cc0ceb66574e74a83e3");
    return d;
}
const Digest& expected_model() noexcept {
    static const Digest d=from_hex("8831bee921999e823466cfc462812e40620b2834080c0f7d64c6f11d7ead626f");
    return d;
}
const Digest& expected_uncertainty_binding() noexcept {
    static const Digest d=from_hex("61c99b0e29316730ca1323fd3e91fd069ebbc50cba73127cd81b9803b10178a0");
    return d;
}
const Digest& expected_backend_binding_file() noexcept {
    static const Digest d=from_hex("d0134dc5322cf721778d1b5e4607c2e82e0dde45b909acfa2e6ea2c8e588d42d");
    return d;
}
const Digest& expected_prospective_result() noexcept {
    static const Digest d=from_hex("7895961af0b4fd942edeeb32ec79476a3b841c74862af9fa5088681682116781");
    return d;
}
const Digest& expected_ptc_bridge_file() noexcept {
    static const Digest d=from_hex("0bfce3cd071e6667454f1e9a390057848168a69602f58350b365952e521186bb");
    return d;
}

Digest decision_hash(const Candidate& c,const Decision& d) noexcept {
    truthraw::sha256_v0_69::Hasher h;
    constexpr char schema[]="TruthRawBoundUncertaintyAdmission/0.79";
    h.update(reinterpret_cast<const std::uint8_t*>(schema),sizeof(schema)-1u);
    const std::array<std::uint8_t,7> flags{
        static_cast<std::uint8_t>(c.sourceDomain),
        static_cast<std::uint8_t>(d.code),
        static_cast<std::uint8_t>(d.sourceClassExact),
        static_cast<std::uint8_t>(d.backendExact),
        static_cast<std::uint8_t>(d.assetsExact),
        static_cast<std::uint8_t>(d.prospectiveEvidenceExact),
        static_cast<std::uint8_t>(d.reconstructedAuthorityAllowed)};
    h.update(flags);
    h.update(c.sourceEvidenceSha256);
    h.update(c.decodedCfaSha256);
    h.update(c.uncertaintyBindingSha256);
    h.update(c.f64TraceCertificateSha256);
    h.update(reinterpret_cast<const std::uint8_t*>(d.scopeId.data()),d.scopeId.size());
    return h.finalize();
}

Decision finish(const Candidate& c,Decision d) noexcept {
    d.decisionSha256=decision_hash(c,d);
    return d;
}

bool exact_source_class(const Candidate& c) noexcept {
    // BGGR is enum/code 0 in the canonical reconstruction API.
    return c.make=="HONOR" &&
           c.model=="BKQ-N49" &&
           c.sourceClass=="HONOR vendor DNG" &&
           c.width==4080u && c.height==3072u &&
           c.cfaCode==0u &&
           exact_float(c.whiteLevel,1023.0f) &&
           exact_float(c.focalLengthMm,22.48f) &&
           c.noiseProfileValid &&
           nonzero(c.sourceEvidenceSha256) &&
           nonzero(c.decodedCfaSha256);
}

bool exact_backend(const Candidate& c) noexcept {
    return c.reconstructionBackendId==kBackendId &&
           c.productionCoreCppSha256==expected_core_cpp() &&
           c.productionCoreHSha256==expected_core_h() &&
           c.productionBackendCombinedSha256==expected_backend_combined();
}

bool exact_assets(const Candidate& c) noexcept {
    return c.featureExtractorSha256==expected_extractor() &&
           c.featureSchemaSha256==expected_feature_schema() &&
           c.uncertaintyModelSha256==expected_model() &&
           c.uncertaintyBindingSha256==expected_uncertainty_binding() &&
           c.backendBindingFileSha256==expected_backend_binding_file();
}

bool exact_prospective(const Candidate& c) noexcept {
    return c.prospectiveResultSha256==expected_prospective_result() &&
           c.ptcBridgeFileSha256==expected_ptc_bridge_file();
}

} // namespace

Decision evaluate(const Candidate& c) noexcept {
    Decision d{};
    d.scopeId=std::string(kScopeId);
    d.uncertaintyBindingSha256=expected_uncertainty_binding();

    if(c.sourceDomain==SourceDomain::Unattested){
        d.code=DecisionCode::BlockedNoSourceAttestation;
        d.reason="no source-class attestation";
        return finish(c,std::move(d));
    }
    if(c.sourceDomain==SourceDomain::Camera5DerivedProcessingDng){
        d.code=DecisionCode::BlockedSourceDomainMismatch;
        d.reason="camera5-derived processing DNG is not the validated HONOR vendor-DNG tele source class";
        return finish(c,std::move(d));
    }
    if(c.sourceDomain!=SourceDomain::HonorBkqN49TeleVendorDngV5gP1){
        d.code=DecisionCode::BlockedSourceDomainMismatch;
        d.reason="source domain is outside exact v5.0g-p1 scope";
        return finish(c,std::move(d));
    }

    d.sourceClassExact=exact_source_class(c);
    if(!d.sourceClassExact){
        d.code=DecisionCode::BlockedSourceClassMismatch;
        d.reason="source-class metadata/sample-domain mismatch";
        return finish(c,std::move(d));
    }

    d.backendExact=exact_backend(c);
    if(!d.backendExact){
        d.code=DecisionCode::BlockedBackendMismatch;
        d.reason="reconstruction backend/hash mismatch";
        return finish(c,std::move(d));
    }

    d.assetsExact=exact_assets(c);
    if(!d.assetsExact){
        d.code=DecisionCode::BlockedModelAssetMismatch;
        d.reason="uncertainty model/feature/binding asset mismatch";
        return finish(c,std::move(d));
    }

    d.prospectiveEvidenceExact=exact_prospective(c);
    if(!d.prospectiveEvidenceExact){
        d.code=DecisionCode::BlockedModelAssetMismatch;
        d.reason="prospective holdout/PTC bridge mismatch";
        return finish(c,std::move(d));
    }

    // Current registry intentionally has no accepted F64 trace certificate.
    if(!nonzero(kExpectedF64TraceCertificate) ||
       c.f64TraceCertificateSha256!=kExpectedF64TraceCertificate){
        d.code=DecisionCode::EligibleTraceGateOpen;
        d.reason="exact source/backend/model eligible; F64 reconstructed-quantity trace certificate still open";
        d.f64TraceCertified=false;
        d.reconstructedAuthorityAllowed=false;
        return finish(c,std::move(d));
    }

    d.code=DecisionCode::Admitted;
    d.reason="exact source/backend/model/trace binding admitted";
    d.f64TraceCertified=true;
    d.reconstructedAuthorityAllowed=true;
    return finish(c,std::move(d));
}

Candidate make_current_camera5_derived_blocked_candidate(
    const Digest& sourceEvidenceSha256,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t cfaCode,
    float whiteLevel,
    const std::string& reconstructionBackendId) noexcept {
    Candidate c{};
    c.sourceDomain=SourceDomain::Camera5DerivedProcessingDng;
    c.sourceEvidenceSha256=sourceEvidenceSha256;
    c.width=width;c.height=height;c.cfaCode=cfaCode;c.whiteLevel=whiteLevel;
    c.reconstructionBackendId=reconstructionBackendId;
    c.make="HONOR";c.model="BKQ-N49";
    c.sourceClass="TruthRaw Camera-5 derived processing DNG";
    return c;
}

const char* schema_name() noexcept{return "TruthRawBoundUncertaintyAdmission/0.79";}
const char* decision_name(DecisionCode code) noexcept{
    switch(code){
        case DecisionCode::BlockedNoSourceAttestation:return "BLOCKED_NO_SOURCE_ATTESTATION";
        case DecisionCode::BlockedSourceDomainMismatch:return "BLOCKED_SOURCE_DOMAIN_MISMATCH";
        case DecisionCode::BlockedSourceClassMismatch:return "BLOCKED_SOURCE_CLASS_MISMATCH";
        case DecisionCode::BlockedBackendMismatch:return "BLOCKED_BACKEND_MISMATCH";
        case DecisionCode::BlockedModelAssetMismatch:return "BLOCKED_MODEL_ASSET_MISMATCH";
        case DecisionCode::EligibleTraceGateOpen:return "ELIGIBLE_TRACE_GATE_OPEN";
        case DecisionCode::Admitted:return "ADMITTED";
    }
    return "INVALID";
}

} // namespace truthraw::bound_uncertainty_admission::v0_79
