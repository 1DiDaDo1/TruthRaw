#include "open_scene_local_policy_v0_86.h"

namespace truthraw::open_scene_local_policy::v0_86 {

bool evaluate(const field::ChannelRecord& record, Decision& out) noexcept {
    out=Decision{};
    if(!field::validate_record(record)) return false;

    out.scientificWritebackAllowed=false;
    out.createsNewEvidence=false;
    out.exactCensoredRecoveryAllowed=false;
    out.unknownMayBecomeMeasured=false;

    switch(record.authority){
        case field::Authority::CalibratedEstimate:
            out.restoration=RestorationDisposition::Preserve;
            out.hdr=HdrDisposition::DirectEvidenceEligible;
            out.detail=DetailSupportDisposition::MeasuredSupport;
            return true;

        case field::Authority::Reconstructed:
            // validate_record already requires admitted finite uncertainty and
            // positive support for RECONSTRUCTED authority.
            out.restoration=RestorationDisposition::PreserveReconstruction;
            out.hdr=HdrDisposition::ReconstructionBoundEligible;
            out.detail=DetailSupportDisposition::ReconstructedSupport;
            return true;

        case field::Authority::Censored:
            out.restoration=RestorationDisposition::CensorBoundOnly;
            out.hdr=HdrDisposition::CensoredExactGainForbidden;
            out.detail=DetailSupportDisposition::CensoredSupportBlocked;
            return true;

        case field::Authority::Unknown:
            out.restoration=RestorationDisposition::AppearanceOnlyNoWriteback;
            out.hdr=HdrDisposition::UnknownHeadroomForbidden;
            out.detail=DetailSupportDisposition::UnknownSupportBlocked;
            return true;
    }
    return false;
}

bool summarize(
    std::span<const field::ChannelRecord> records,
    Summary& out) noexcept {
    out={};
    if(records.empty()) return false;

    for(const auto& record:records){
        Decision d{};
        if(!evaluate(record,d)) {
            out={};
            return false;
        }

        const auto restorationIndex=
            static_cast<std::uint8_t>(d.restoration)-1u;
        const auto hdrIndex=static_cast<std::uint8_t>(d.hdr)-1u;
        const auto detailIndex=static_cast<std::uint8_t>(d.detail)-1u;
        if(restorationIndex>=out.restorationCounts.size() ||
           hdrIndex>=out.hdrCounts.size() ||
           detailIndex>=out.detailCounts.size()){
            out={};return false;
        }
        ++out.restorationCounts[restorationIndex];
        ++out.hdrCounts[hdrIndex];
        ++out.detailCounts[detailIndex];
        ++out.recordCount;

        if(record.authority==field::Authority::Censored) out.anyCensored=true;
        if(record.authority==field::Authority::Unknown) out.anyUnknown=true;
    }

    out.allScientificHdrEligible=
        out.hdrCounts[
            static_cast<std::size_t>(
                static_cast<std::uint8_t>(
                    HdrDisposition::CensoredExactGainForbidden)-1u)]==0u &&
        out.hdrCounts[
            static_cast<std::size_t>(
                static_cast<std::uint8_t>(
                    HdrDisposition::UnknownHeadroomForbidden)-1u)]==0u;
    out.createsNewEvidence=false;
    out.scientificWritebackAllowed=false;
    return true;
}

const char* schema_name() noexcept {
    return "TruthRawOpenSceneLocalPolicy/0.86";
}

} // namespace truthraw::open_scene_local_policy::v0_86
