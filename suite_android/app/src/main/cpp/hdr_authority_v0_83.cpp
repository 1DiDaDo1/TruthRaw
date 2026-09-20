#include "hdr_authority_v0_83.h"

#include <algorithm>
#include <array>
#include <cstddef>

namespace truthraw::hdr_authority::v0_83 {
namespace {

bool nonzero(const Digest& digest) noexcept {
    return std::any_of(
        digest.begin(), digest.end(),
        [](std::uint8_t v) { return v != 0u; });
}

void put_u32(truthraw::sha256_v0_69::Hasher& h, std::uint32_t v) noexcept {
    const std::array<std::uint8_t,4> b{
        static_cast<std::uint8_t>(v),
        static_cast<std::uint8_t>(v >> 8u),
        static_cast<std::uint8_t>(v >> 16u),
        static_cast<std::uint8_t>(v >> 24u),
    };
    h.update(b);
}

void put_u64(truthraw::sha256_v0_69::Hasher& h, std::uint64_t v) noexcept {
    std::array<std::uint8_t,8> b{};
    for(std::size_t i=0u;i<b.size();++i){
        b[i]=static_cast<std::uint8_t>(v >> (8u*i));
    }
    h.update(b);
}

Digest hash_state(const Input& input,const State& state) noexcept {
    truthraw::sha256_v0_69::Hasher h;
    constexpr char domain[]=
        "TruthRawHdrAuthorityContract/0.83\n"
        "presentation_hdr_is_not_scientific_authority=1\n"
        "censored_is_bound_not_exact_latent=1\n"
        "unknown_headroom_allowed=0\n"
        "illumination_context_cannot_create_hdr_authority=1\n"
        "reconstructed_hdr_requires_admitted_uncertainty=1\n"
        "scientific_gain_requires_per_output_channel_authority=1\n"
        "single_frame_evidence_count_remains_one=1\n";
    h.update(
        reinterpret_cast<const std::uint8_t*>(domain),
        sizeof(domain)-1u);
    h.update(input.sourceEvidenceSha256);
    h.update(input.scientificMasterSha256);
    h.update(input.canonicalOpenSceneSha256);
    h.update(input.channelAuthoritySha256);
    h.update(input.uncertaintyAdmissionSha256);
    h.update(input.illuminationStateSha256);

    const std::array<std::uint8_t,12> flags{
        static_cast<std::uint8_t>(input.presentationHdrEnabled),
        static_cast<std::uint8_t>(input.perOutputChannelAuthorityAvailable),
        static_cast<std::uint8_t>(input.reconstructedUncertaintyAdmitted),
        static_cast<std::uint8_t>(input.censoredGainSuppressed),
        static_cast<std::uint8_t>(input.illuminationWhitePointKnown),
        static_cast<std::uint8_t>(input.illuminationSpectrumKnown),
        static_cast<std::uint8_t>(state.scientificAuthority),
        static_cast<std::uint8_t>(state.presentationAuthority),
        static_cast<std::uint8_t>(state.blockedReason),
        static_cast<std::uint8_t>(state.scientificGainAllowed),
        static_cast<std::uint8_t>(state.presentationGainAllowed),
        static_cast<std::uint8_t>(state.illuminationCreatesHeadroom),
    };
    h.update(flags);

    put_u64(h,input.calibratedEstimateChannelRecords);
    put_u64(h,input.reconstructedChannelRecords);
    put_u64(h,input.censoredChannelRecords);
    put_u64(h,input.unknownChannelRecords);
    put_u64(h,input.outputPixelCount);
    put_u64(h,input.presentationHdrGainPixels);
    put_u32(h,input.physicalFrameCount);
    put_u32(h,input.independentEvidenceCount);
    return h.finalize();
}

bool valid_input(const Input& input) noexcept {
    if(!nonzero(input.sourceEvidenceSha256) ||
       !nonzero(input.scientificMasterSha256) ||
       !nonzero(input.canonicalOpenSceneSha256) ||
       !nonzero(input.channelAuthoritySha256) ||
       !nonzero(input.uncertaintyAdmissionSha256) ||
       !nonzero(input.illuminationStateSha256) ||
       input.outputPixelCount==0u ||
       input.presentationHdrGainPixels>input.outputPixelCount ||
       input.physicalFrameCount!=1u ||
       input.independentEvidenceCount!=1u){
        return false;
    }

    const std::uint64_t totalChannels =
        input.calibratedEstimateChannelRecords +
        input.reconstructedChannelRecords +
        input.censoredChannelRecords +
        input.unknownChannelRecords;
    return totalChannels>0u;
}

} // namespace

bool build(const Input& input,State& out) noexcept {
    out=State{};
    out.physicalFrameCount=input.physicalFrameCount;
    out.independentEvidenceCount=input.independentEvidenceCount;
    out.outputPixelCount=input.outputPixelCount;
    out.presentationHdrGainPixels=input.presentationHdrGainPixels;
    out.presentationAuthority=input.presentationHdrEnabled
        ? PresentationHdrAuthority::AppearanceOnly
        : PresentationHdrAuthority::Disabled;
    out.presentationGainAllowed=input.presentationHdrEnabled;

    out.scientificAuthority=ScientificHdrAuthority::Blocked;
    out.blockedReason=BlockedReason::InvalidInput;
    out.scientificGainAllowed=false;
    out.censoredExactRecoveryAllowed=false;
    out.unknownHeadroomAllowed=false;
    out.illuminationCreatesHeadroom=false;
    out.requiresPerOutputChannelAuthority=true;
    out.requiresAdmittedUncertaintyForReconstructed=true;

    if(!valid_input(input)){
        return false;
    }

    if(!input.perOutputChannelAuthorityAvailable){
        out.blockedReason=BlockedReason::NoPerOutputChannelAuthority;
    }else if(input.unknownChannelRecords>0u){
        out.blockedReason=BlockedReason::UnknownChannelAuthorityPresent;
    }else if(input.reconstructedChannelRecords>0u &&
             !input.reconstructedUncertaintyAdmitted){
        out.blockedReason=BlockedReason::ReconstructedUncertaintyNotAdmitted;
    }else if(input.censoredChannelRecords>0u &&
             !input.censoredGainSuppressed){
        out.blockedReason=BlockedReason::CensoredGainNotSuppressed;
    }else{
        out.blockedReason=BlockedReason::None;
        out.scientificGainAllowed=true;
        out.scientificAuthority=
            input.reconstructedChannelRecords>0u
                ? ScientificHdrAuthority::ReconstructionUncertaintyBound
                : ScientificHdrAuthority::DirectEvidenceBound;
    }

    // Illumination state is contextual. Even a future calibrated SPD does not
    // bypass RGB/channel authority, censoring, uncertainty or output mapping.
    out.illuminationCreatesHeadroom=false;

    out.stateSha256=hash_state(input,out);
    return nonzero(out.stateSha256);
}

const char* schema_name() noexcept {
    return "TruthRawHdrAuthorityContract/0.83";
}

const char* scientific_authority_name(ScientificHdrAuthority authority) noexcept {
    switch(authority){
        case ScientificHdrAuthority::Blocked:return "BLOCKED";
        case ScientificHdrAuthority::DirectEvidenceBound:return "DIRECT_EVIDENCE_BOUND";
        case ScientificHdrAuthority::ReconstructionUncertaintyBound:
            return "RECONSTRUCTION_UNCERTAINTY_BOUND";
    }
    return "INVALID";
}

const char* presentation_authority_name(PresentationHdrAuthority authority) noexcept {
    switch(authority){
        case PresentationHdrAuthority::Disabled:return "DISABLED";
        case PresentationHdrAuthority::AppearanceOnly:return "APPEARANCE_ONLY";
    }
    return "INVALID";
}

const char* blocked_reason_name(BlockedReason reason) noexcept {
    switch(reason){
        case BlockedReason::None:return "NONE";
        case BlockedReason::NoPerOutputChannelAuthority:
            return "NO_PER_OUTPUT_CHANNEL_AUTHORITY";
        case BlockedReason::UnknownChannelAuthorityPresent:
            return "UNKNOWN_CHANNEL_AUTHORITY_PRESENT";
        case BlockedReason::ReconstructedUncertaintyNotAdmitted:
            return "RECONSTRUCTED_UNCERTAINTY_NOT_ADMITTED";
        case BlockedReason::CensoredGainNotSuppressed:
            return "CENSORED_GAIN_NOT_SUPPRESSED";
        case BlockedReason::InvalidInput:return "INVALID_INPUT";
    }
    return "INVALID";
}

} // namespace truthraw::hdr_authority::v0_83
