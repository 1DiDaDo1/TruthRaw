#include "hdr_authority_v0_83.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>

namespace hdr = truthraw::hdr_authority::v0_83;

namespace {
#define REQUIRE(x) do { if(!(x)){ std::cerr<<"FAIL line "<<__LINE__<<": "<<#x<<"\n"; std::exit(2);} } while(0)

hdr::Digest digest(std::uint8_t seed) {
    hdr::Digest d{};
    for(std::size_t i=0;i<d.size();++i){
        d[i]=static_cast<std::uint8_t>(seed+i);
    }
    return d;
}

hdr::Input base() {
    hdr::Input in{};
    in.sourceEvidenceSha256=digest(1);
    in.scientificMasterSha256=digest(41);
    in.canonicalOpenSceneSha256=digest(81);
    in.channelAuthoritySha256=digest(121);
    in.uncertaintyAdmissionSha256=digest(151);
    in.illuminationStateSha256=digest(181);
    in.calibratedEstimateChannelRecords=100u;
    in.reconstructedChannelRecords=0u;
    in.censoredChannelRecords=0u;
    in.unknownChannelRecords=200u;
    in.outputPixelCount=100u;
    in.presentationHdrEnabled=true;
    in.presentationHdrGainPixels=30u;
    in.perOutputChannelAuthorityAvailable=false;
    in.reconstructedUncertaintyAdmitted=false;
    in.censoredGainSuppressed=true;
    in.physicalFrameCount=1u;
    in.independentEvidenceCount=1u;
    return in;
}

bool nonzero(const hdr::Digest& d){
    return std::any_of(d.begin(),d.end(),[](std::uint8_t v){return v!=0u;});
}

void current_runtime_is_presentation_only() {
    auto in=base();
    hdr::State s{};
    REQUIRE(hdr::build(in,s));
    REQUIRE(s.scientificAuthority==hdr::ScientificHdrAuthority::Blocked);
    REQUIRE(s.blockedReason==hdr::BlockedReason::NoPerOutputChannelAuthority);
    REQUIRE(!s.scientificGainAllowed);
    REQUIRE(s.presentationAuthority==hdr::PresentationHdrAuthority::AppearanceOnly);
    REQUIRE(s.presentationGainAllowed);
    REQUIRE(!s.censoredExactRecoveryAllowed);
    REQUIRE(!s.unknownHeadroomAllowed);
    REQUIRE(!s.illuminationCreatesHeadroom);
    REQUIRE(s.requiresPerOutputChannelAuthority);
    REQUIRE(s.requiresAdmittedUncertaintyForReconstructed);
    REQUIRE(nonzero(s.stateSha256));
}

void illumination_context_never_promotes() {
    auto a=base();
    auto b=a;
    b.illuminationWhitePointKnown=true;
    b.illuminationSpectrumKnown=true;
    b.illuminationStateSha256[0]^=1u;

    hdr::State sa{},sb{};
    REQUIRE(hdr::build(a,sa));
    REQUIRE(hdr::build(b,sb));
    REQUIRE(sa.scientificGainAllowed==sb.scientificGainAllowed);
    REQUIRE(sa.scientificAuthority==sb.scientificAuthority);
    REQUIRE(sa.blockedReason==sb.blockedReason);
    REQUIRE(!sb.illuminationCreatesHeadroom);
    REQUIRE(sa.stateSha256!=sb.stateSha256);
}

void unknown_blocks_when_output_authority_exists() {
    auto in=base();
    in.perOutputChannelAuthorityAvailable=true;
    hdr::State s{};
    REQUIRE(hdr::build(in,s));
    REQUIRE(s.blockedReason==hdr::BlockedReason::UnknownChannelAuthorityPresent);
    REQUIRE(!s.scientificGainAllowed);
}

void reconstructed_requires_admitted_uncertainty() {
    auto in=base();
    in.perOutputChannelAuthorityAvailable=true;
    in.unknownChannelRecords=0u;
    in.reconstructedChannelRecords=200u;
    in.calibratedEstimateChannelRecords=100u;

    hdr::State s{};
    REQUIRE(hdr::build(in,s));
    REQUIRE(s.blockedReason==
            hdr::BlockedReason::ReconstructedUncertaintyNotAdmitted);
    REQUIRE(!s.scientificGainAllowed);

    in.reconstructedUncertaintyAdmitted=true;
    REQUIRE(hdr::build(in,s));
    REQUIRE(s.blockedReason==hdr::BlockedReason::None);
    REQUIRE(s.scientificGainAllowed);
    REQUIRE(s.scientificAuthority==
            hdr::ScientificHdrAuthority::ReconstructionUncertaintyBound);
}

void censored_requires_suppression() {
    auto in=base();
    in.perOutputChannelAuthorityAvailable=true;
    in.unknownChannelRecords=0u;
    in.censoredChannelRecords=3u;
    in.censoredGainSuppressed=false;

    hdr::State s{};
    REQUIRE(hdr::build(in,s));
    REQUIRE(s.blockedReason==hdr::BlockedReason::CensoredGainNotSuppressed);
    REQUIRE(!s.scientificGainAllowed);

    in.censoredGainSuppressed=true;
    REQUIRE(hdr::build(in,s));
    REQUIRE(s.blockedReason==hdr::BlockedReason::None);
    REQUIRE(s.scientificAuthority==
            hdr::ScientificHdrAuthority::DirectEvidenceBound);
    REQUIRE(s.scientificGainAllowed);
    REQUIRE(!s.censoredExactRecoveryAllowed);
}

void direct_evidence_only_can_be_admitted() {
    auto in=base();
    in.perOutputChannelAuthorityAvailable=true;
    in.unknownChannelRecords=0u;
    in.reconstructedChannelRecords=0u;
    in.censoredChannelRecords=0u;
    in.calibratedEstimateChannelRecords=300u;
    hdr::State s{};
    REQUIRE(hdr::build(in,s));
    REQUIRE(s.blockedReason==hdr::BlockedReason::None);
    REQUIRE(s.scientificAuthority==
            hdr::ScientificHdrAuthority::DirectEvidenceBound);
    REQUIRE(s.scientificGainAllowed);
}

void disabled_presentation_requires_zero_gain_pixels() {
    auto in=base();
    in.presentationHdrEnabled=false;
    in.presentationHdrGainPixels=0u;
    hdr::State s{};
    REQUIRE(hdr::build(in,s));
    REQUIRE(s.presentationAuthority==hdr::PresentationHdrAuthority::Disabled);
    REQUIRE(!s.presentationGainAllowed);

    in.presentationHdrGainPixels=1u;
    REQUIRE(!hdr::build(in,s));
}

void invalid_lineage_or_evidence_fails() {
    auto in=base();
    hdr::State s{};
    in.sourceEvidenceSha256={};
    REQUIRE(!hdr::build(in,s));

    in=base();
    in.physicalFrameCount=2u;
    REQUIRE(!hdr::build(in,s));

    in=base();
    in.independentEvidenceCount=2u;
    REQUIRE(!hdr::build(in,s));

    in=base();
    in.presentationHdrGainPixels=in.outputPixelCount+1u;
    REQUIRE(!hdr::build(in,s));
}

void deterministic() {
    const auto in=base();
    hdr::State a{},b{};
    REQUIRE(hdr::build(in,a));
    REQUIRE(hdr::build(in,b));
    REQUIRE(a.stateSha256==b.stateSha256);
}

} // namespace

int main(){
    current_runtime_is_presentation_only();
    illumination_context_never_promotes();
    unknown_blocks_when_output_authority_exists();
    reconstructed_requires_admitted_uncertainty();
    censored_requires_suppression();
    direct_evidence_only_can_be_admitted();
    disabled_presentation_requires_zero_gain_pixels();
    invalid_lineage_or_evidence_fails();
    deterministic();
    std::cout<<"HDR_AUTHORITY_V0_83_PASS\n";
    return 0;
}
