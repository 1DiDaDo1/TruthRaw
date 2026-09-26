#include "truthnegative_n2_confidence_field_v0_3.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

namespace cf=truthraw::truthnegative_n2_confidence_field::v0_3;
namespace v01=truthraw::truthnegative_n2_cfa_audit::v0_1;
namespace ce=
    truthraw::truthnegative_center_excluded_spatial_audit::v0_2_1;

#define R(x) do{if(!(x))throw std::runtime_error(#x);}while(0)

static v01::TileAudit make_v01_tile(
    std::uint32_t x,
    std::uint64_t sampled,
    std::uint64_t corrected,
    std::uint64_t preserved,
    std::uint64_t structure,
    std::uint64_t censored,
    std::uint64_t boundary) {
    v01::TileAudit t{};
    t.x=x;
    t.y=0u;
    t.width=64u;
    t.height=64u;
    t.sampled=sampled;
    t.audit.total=sampled;
    t.audit.corrected=corrected;
    t.audit.eligible=corrected;
    t.audit.preserved=preserved;
    t.audit.structureProtected=structure;
    t.audit.censoredProtected=censored;
    t.audit.censorBoundaryProtected=boundary;
    t.cfaPhaseSamples={sampled/4u,sampled/4u,sampled/4u,sampled/4u};
    return t;
}

static ce::TileMetrics make_ce_tile(
    std::uint32_t x,
    std::uint64_t sampled,
    std::uint64_t candidates,
    std::uint64_t valid,
    std::uint64_t invalid,
    std::uint64_t pairs,
    std::uint64_t pairAccepted,
    std::uint64_t pairRejected,
    std::uint64_t scales,
    std::uint64_t scaleAccepted,
    std::uint64_t scaleRejected,
    std::uint64_t z1,
    std::uint64_t z12,
    std::uint64_t z2) {
    ce::TileMetrics t{};
    t.x=x;
    t.y=0u;
    t.width=64u;
    t.height=64u;
    auto& m=t.metrics;
    m.sampled=sampled;
    m.v01CandidateCenters=candidates;
    m.predictorValid=valid;
    m.predictorInvalid=invalid;
    m.symmetricPairsConsidered=pairs;
    m.symmetricPairsAccepted=pairAccepted;
    m.symmetricPairsRejected=pairRejected;
    m.scalesConsidered=scales;
    m.scalesAccepted=scaleAccepted;
    m.scalesRejected=scaleRejected;
    m.centerResidualWithin1Sigma=z1;
    m.centerResidualBetween1And2Sigma=z12;
    m.centerResidualAbove2Sigma=z2;
    m.combinedResidualWithin1Sigma=z1;
    m.combinedResidualBetween1And2Sigma=z12;
    m.combinedResidualAbove2Sigma=z2;
    m.estimateToCenterVarianceRatioSum=
        valid>0u?0.1*static_cast<double>(valid):0.0;
    m.maxEstimateToCenterVarianceRatio=valid>0u?0.2:0.0;
    m.maxDirectionalDisagreementSigma=valid>0u?3.0:0.0;
    m.maxCrossScaleDisagreementSigma=valid>0u?2.0:0.0;
    m.v01CandidateCfaPhase={
        candidates/4u,
        candidates/4u,
        candidates/4u,
        candidates-(candidates/4u)*3u};
    m.predictorValidCfaPhase={
        valid/4u,
        valid/4u,
        valid/4u,
        valid-(valid/4u)*3u};
    return t;
}

int main(){
    v01::Result a{};
    a.tileEdge=64u;
    a.samplingPeriod=8u;
    a.sampled=1024u;
    a.candidateSha256[0]=1u;
    a.auditSha256[0]=2u;
    a.spatialSha256[0]=3u;
    a.tiles.push_back(make_v01_tile(0u,256u,0u,256u,120u,20u,10u));
    a.tiles.push_back(make_v01_tile(64u,256u,32u,224u,100u,10u,8u));
    a.tiles.push_back(make_v01_tile(128u,256u,128u,128u,80u,5u,3u));
    a.tiles.push_back(make_v01_tile(192u,256u,64u,192u,60u,4u,2u));

    ce::Result c{};
    c.tileEdge=64u;
    c.samplingPeriod=8u;
    c.auditSha256[0]=4u;
    c.v01TileParityVerified=true;
    c.centerOnlySigmaPrimary=true;
    c.combinedSigmaDiagnosticOnly=true;
    c.noiseIndependenceAdmitted=false;
    c.candidateApplied=false;
    c.createsNewEvidence=false;
    c.scientificWritebackAllowed=false;
    c.tiles.push_back(make_ce_tile(
        0u,256u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0u));
    c.tiles.push_back(make_ce_tile(
        64u,256u,32u,0u,32u,0u,0u,0u,0u,0u,0u,0u,0u,0u));
    c.tiles.push_back(make_ce_tile(
        128u,256u,128u,120u,8u,1440u,1000u,440u,
        360u,200u,160u,70u,40u,10u));
    c.tiles.push_back(make_ce_tile(
        192u,256u,64u,64u,0u,768u,768u,0u,
        192u,192u,0u,50u,14u,0u));

    cf::Binding binding{};
    binding.sourceEvidenceSha256[0]=10u;
    binding.scientificMasterSha256[0]=11u;
    binding.authorityFieldSha256[0]=12u;
    binding.truthNegativeStateSha256[0]=13u;
    binding.v01CandidateSha256=a.candidateSha256;
    binding.v01AuditSha256=a.auditSha256;
    binding.v01SpatialSha256=a.spatialSha256;
    binding.centerExcludedAuditSha256=c.auditSha256;

    cf::Result field{};
    R(cf::derive(binding,a,c,field));
    R(field.tiles.size()==4u);
    R(field.noCandidateTiles==1u);
    R(field.unresolvedTiles==1u);
    R(field.mixedTiles==1u);
    R(field.fullyCoherentTiles==1u);
    R(field.tiles[0].supportClass==cf::SupportClass::NoCandidate);
    R(field.tiles[1].supportClass==cf::SupportClass::Unresolved);
    R(field.tiles[2].supportClass==cf::SupportClass::Mixed);
    R(field.tiles[3].supportClass==cf::SupportClass::FullyCoherent);
    R(field.tiles[2].candidateFraction==0.5);
    R(field.tiles[3].predictorCoverage==1.0);
    R(field.tiles[3].pairAcceptance==1.0);
    R(field.tiles[3].scaleAcceptance==1.0);
    R(field.tiles[3].centerZGt2Fraction==0.0);
    R(!field.supportDistanceAdmitted);
    R(!field.promotionEligible);
    R(!field.candidateApplied);
    R(!field.createsNewEvidence);
    R(!field.scientificWritebackAllowed);

    cf::Report report{};
    R(cf::encode(binding,4080u,3072u,field,report));
    R(report.tileCount==4u);
    R(report.json.find(
        "\"schema\":\"D.RAW/TruthNegative/N2ConfidenceField/0.3\"")!=
      std::string::npos);
    R(report.json.find(
        "\"vector_valued_no_scalar_probability\":true")!=
      std::string::npos);
    R(report.json.find(
        "\"support_distance_admitted\":false")!=
      std::string::npos);
    R(report.json.find(
        "\"promotion_eligible\":false")!=std::string::npos);
    R(report.json.find(
        "\"support_class\":\"FULLY_COHERENT\"")!=
      std::string::npos);

    auto bad=binding;
    bad.centerExcludedAuditSha256[0]^=0x55u;
    cf::Result fail{};
    R(!cf::derive(bad,a,c,fail));

    std::cout
        <<"TruthNegativeN2ConfidenceField/0.3 PASS tiles="
        <<field.tiles.size()
        <<" mixed="<<field.mixedTiles
        <<" fully="<<field.fullyCoherentTiles
        <<"\n";
}
