#include "truthnegative_n2_factored_confidence_state_v0_3_1.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace fs =
    truthraw::truthnegative_n2_factored_confidence_state::v0_3_1;
namespace cf = truthraw::truthnegative_n2_confidence_field::v0_3;

#define R(x) do{if(!(x))throw std::runtime_error(#x);}while(0)

static cf::TileVector tile(
    std::uint32_t x,
    std::uint64_t candidates,
    std::uint64_t valid,
    std::uint64_t invalid,
    std::uint64_t pairAccepted,
    std::uint64_t pairRejected,
    std::uint64_t scaleAccepted,
    std::uint64_t scaleRejected,
    std::uint64_t zGt2,
    std::uint64_t structure,
    std::uint64_t censored,
    std::uint64_t boundary,
    double maxVarianceRatio,
    cf::SupportClass legacy) {
    cf::TileVector t{};
    t.x=x;
    t.y=0u;
    t.width=64u;
    t.height=64u;
    t.sampled=256u;
    t.v01CandidateCenters=candidates;
    t.predictorValid=valid;
    t.predictorInvalid=invalid;
    t.pairsAccepted=pairAccepted;
    t.pairsRejected=pairRejected;
    t.pairsConsidered=pairAccepted+pairRejected;
    t.scalesAccepted=scaleAccepted;
    t.scalesRejected=scaleRejected;
    t.scalesConsidered=scaleAccepted+scaleRejected;
    t.centerZLe1=valid-zGt2;
    t.centerZ1To2=0u;
    t.centerZGt2=zGt2;
    t.v01Preserved=256u-candidates;
    t.structureProtected=structure;
    t.censoredProtected=censored;
    t.censorBoundaryProtected=boundary;
    t.candidateFraction=
        static_cast<double>(candidates)/256.0;
    t.predictorCoverage=
        candidates>0u
            ? static_cast<double>(valid)/static_cast<double>(candidates)
            : 0.0;
    t.pairAcceptance=
        t.pairsConsidered>0u
            ? static_cast<double>(pairAccepted)/
                static_cast<double>(t.pairsConsidered)
            : 0.0;
    t.scaleAcceptance=
        t.scalesConsidered>0u
            ? static_cast<double>(scaleAccepted)/
                static_cast<double>(t.scalesConsidered)
            : 0.0;
    t.centerZLe1Fraction=
        valid>0u
            ? static_cast<double>(t.centerZLe1)/
                static_cast<double>(valid)
            : 0.0;
    t.centerZ1To2Fraction=0.0;
    t.centerZGt2Fraction=
        valid>0u
            ? static_cast<double>(zGt2)/static_cast<double>(valid)
            : 0.0;
    t.preserveFraction=
        static_cast<double>(t.v01Preserved)/256.0;
    t.structureProtectionFraction=
        static_cast<double>(structure)/256.0;
    t.censorProtectionFraction=
        static_cast<double>(censored)/256.0;
    t.censorBoundaryProtectionFraction=
        static_cast<double>(boundary)/256.0;
    t.meanEstimateToCenterVarianceRatio=
        valid>0u?0.25:0.0;
    t.maxEstimateToCenterVarianceRatio=
        valid>0u?maxVarianceRatio:0.0;
    t.v01CandidateCfaPhase={
        candidates/4u,
        candidates/4u,
        candidates/4u,
        candidates-(candidates/4u)*3u};
    t.predictorValidCfaPhase={
        valid/4u,
        valid/4u,
        valid/4u,
        valid-(valid/4u)*3u};
    t.supportClass=legacy;
    t.promotionEligible=false;
    return t;
}

int main(){
    cf::Result confidence{};
    confidence.tileEdge=64u;
    confidence.samplingPeriod=8u;
    confidence.fieldSha256[0]=9u;
    confidence.exactV01ParityVerified=true;
    confidence.exactCenterExcludedParityVerified=true;
    confidence.vectorValuedNoScalarProbability=true;
    confidence.cfaPhaseDiagnosticOnly=true;
    confidence.supportDistanceAdmitted=false;
    confidence.promotionEligible=false;
    confidence.candidateApplied=false;
    confidence.createsNewEvidence=false;
    confidence.scientificWritebackAllowed=false;

    confidence.tiles.push_back(tile(
        0u,0u,0u,0u,0u,0u,0u,0u,0u,
        30u,20u,5u,0.0,cf::SupportClass::NoCandidate));
    confidence.tiles.push_back(tile(
        64u,64u,64u,0u,700u,68u,150u,42u,0u,
        10u,0u,0u,0.8,cf::SupportClass::Mixed));
    confidence.tiles.push_back(tile(
        128u,128u,100u,28u,800u,400u,180u,120u,7u,
        80u,10u,8u,1.4,cf::SupportClass::Mixed));
    confidence.tiles.push_back(tile(
        192u,32u,32u,0u,384u,0u,96u,0u,0u,
        0u,0u,0u,0.5,cf::SupportClass::FullyCoherent));

    fs::Binding binding{};
    binding.sourceEvidenceSha256[0]=1u;
    binding.scientificMasterSha256[0]=2u;
    binding.authorityFieldSha256[0]=3u;
    binding.truthNegativeStateSha256[0]=4u;
    binding.v01CandidateSha256[0]=5u;
    binding.v01AuditSha256[0]=6u;
    binding.v01SpatialSha256[0]=7u;
    binding.centerExcludedAuditSha256[0]=8u;
    binding.confidenceFieldSha256=confidence.fieldSha256;

    fs::Result state{};
    R(fs::derive(binding,confidence,state));
    R(state.tiles.size()==4u);
    R(state.hasCandidateTiles==3u);
    R(state.allCandidatesPredictableTiles==2u);
    R(state.centerOutlierFreeTiles==2u);
    R(state.predictableAndCenterOutlierFreeTiles==2u);
    R(state.pairRejectionFreeTiles==1u);
    R(state.scaleRejectionFreeTiles==1u);
    R(state.structureProtectionPresentTiles==3u);
    R(state.censorProtectionPresentTiles==2u);
    R(state.censorBoundaryProtectionPresentTiles==2u);
    R(state.maxPredictorVarianceLeCenterVarianceTiles==2u);

    const auto& noCandidate=state.tiles[0];
    R(!noCandidate.hasCandidates);
    R(!noCandidate.predictorAvailable);
    R(!noCandidate.allCandidatesPredictable);
    R(!noCandidate.centerOutlierFree);
    R(noCandidate.structureProtectionPresent);
    R(noCandidate.censorProtectionPresent);
    R(noCandidate.censorBoundaryProtectionPresent);

    const auto& predictable=state.tiles[1];
    R(predictable.hasCandidates);
    R(predictable.predictorAvailable);
    R(predictable.allCandidatesPredictable);
    R(predictable.centerOutlierFree);
    R(predictable.predictableAndCenterOutlierFree);
    R(!predictable.pairRejectionFree);
    R(!predictable.scaleRejectionFree);
    R(predictable.maxPredictorVarianceLeCenterVariance);

    const auto& mixed=state.tiles[2];
    R(!mixed.allCandidatesPredictable);
    R(!mixed.centerOutlierFree);
    R(!mixed.predictableAndCenterOutlierFree);
    R(!mixed.maxPredictorVarianceLeCenterVariance);

    const auto& full=state.tiles[3];
    R(full.allCandidatesPredictable);
    R(full.centerOutlierFree);
    R(full.pairRejectionFree);
    R(full.scaleRejectionFree);
    R(full.maxPredictorVarianceLeCenterVariance);
    R(!full.promotionEligible);

    R(state.exactConfidenceFieldBindingVerified);
    R(state.vectorValuedNoScalarProbability);
    R(state.legacyClassNonAuthoritative);
    R(state.cfaPhaseDiagnosticOnly);
    R(!state.supportDistanceAdmitted);
    R(!state.promotionEligible);
    R(!state.candidateApplied);
    R(!state.createsNewEvidence);
    R(!state.scientificWritebackAllowed);

    fs::Report report{};
    R(fs::encode(binding,4080u,3072u,state,report));
    R(report.tileCount==4u);
    R(report.json.find(
        "\"schema\":\"D.RAW/TruthNegative/"
        "N2FactoredConfidenceState/0.3.1\"")!=
      std::string::npos);
    R(report.json.find(
        "\"legacy_class_non_authoritative\":true")!=
      std::string::npos);
    R(report.json.find(
        "\"all_candidates_predictable\":true")!=
      std::string::npos);
    R(report.json.find(
        "\"predictable_and_center_outlier_free\":true")!=
      std::string::npos);
    R(report.json.find(
        "\"promotion_eligible\":false")!=
      std::string::npos);

    auto bad=binding;
    bad.confidenceFieldSha256[0]^=0x55u;
    fs::Result fail{};
    R(!fs::derive(bad,confidence,fail));

    std::cout
        <<"TruthNegativeN2FactoredConfidenceState/0.3.1 PASS "
        <<"tiles="<<state.tiles.size()
        <<" predictable="<<state.allCandidatesPredictableTiles
        <<" outlierFree="<<state.centerOutlierFreeTiles
        <<" both="<<state.predictableAndCenterOutlierFreeTiles
        <<"\n";
}
