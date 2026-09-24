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

const char* schema_name() noexcept {
    return "TruthRawOpenSceneLocalPolicy/0.86";
}

} // namespace truthraw::open_scene_local_policy::v0_86
