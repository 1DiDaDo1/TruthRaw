#include "truthnegative_n2_factored_confidence_state_v0_3_1.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>

namespace truthraw::truthnegative_n2_factored_confidence_state::v0_3_1 {
namespace {

bool nonzero(const Digest& d) noexcept {
    return std::any_of(
        d.begin(),d.end(),[](std::uint8_t v){return v!=0u;});
}

std::string hex(const Digest& d) {
    static constexpr char kHex[]="0123456789abcdef";
    std::string out(d.size()*2u,'0');
    for(std::size_t i=0u;i<d.size();++i){
        out[2u*i]=kHex[d[i]>>4u];
        out[2u*i+1u]=kHex[d[i]&0x0fu];
    }
    return out;
}

const char* legacy_class_name(cf::SupportClass c) noexcept {
    switch(c){
        case cf::SupportClass::NoCandidate:return "NO_CANDIDATE";
        case cf::SupportClass::Unresolved:return "UNRESOLVED";
        case cf::SupportClass::Mixed:return "MIXED";
        case cf::SupportClass::FullyCoherent:return "FULLY_COHERENT";
    }
    return "UNRESOLVED";
}

void hash_u32(
    truthraw::sha256_v0_69::Hasher& h,
    std::uint32_t v) noexcept {
    const std::array<std::uint8_t,4u> b{
        static_cast<std::uint8_t>(v),
        static_cast<std::uint8_t>(v>>8u),
        static_cast<std::uint8_t>(v>>16u),
        static_cast<std::uint8_t>(v>>24u)};
    h.update(b);
}

void hash_u64(
    truthraw::sha256_v0_69::Hasher& h,
    std::uint64_t v) noexcept {
    std::array<std::uint8_t,8u> b{};
    for(std::size_t i=0u;i<8u;++i){
        b[i]=static_cast<std::uint8_t>(v>>(8u*i));
    }
    h.update(b);
}

void hash_f64(
    truthraw::sha256_v0_69::Hasher& h,
    double v) noexcept {
    hash_u64(h,std::bit_cast<std::uint64_t>(v));
}

void hash_bool(
    truthraw::sha256_v0_69::Hasher& h,
    bool v) noexcept {
    hash_u32(h,v?1u:0u);
}

bool finite_fraction(double v) noexcept {
    return std::isfinite(v)&&v>=0.0&&v<=1.0;
}

bool fact_consistent(const TileFacts& t) noexcept {
    std::uint64_t candidatePhases=0u;
    std::uint64_t validPhases=0u;
    for(auto v:t.v01CandidateCfaPhase)candidatePhases+=v;
    for(auto v:t.predictorValidCfaPhase)validPhases+=v;

    const bool hasCandidates=t.v01CandidateCenters>0u;
    const bool predictorAvailable=t.predictorValid>0u;
    const bool allCandidatesPredictable=
        hasCandidates&&
        t.predictorValid==t.v01CandidateCenters&&
        t.predictorInvalid==0u;
    const bool centerOutlierFree=
        predictorAvailable&&t.centerZGt2==0u;
    const bool predictableAndCenterOutlierFree=
        allCandidatesPredictable&&centerOutlierFree;
    const bool pairRejectionFree=
        t.pairsConsidered>0u&&t.pairsRejected==0u;
    const bool scaleRejectionFree=
        t.scalesConsidered>0u&&t.scalesRejected==0u;
    const bool structureProtectionPresent=t.structureProtected>0u;
    const bool censorProtectionPresent=t.censoredProtected>0u;
    const bool censorBoundaryProtectionPresent=
        t.censorBoundaryProtected>0u;
    const bool maxPredictorVarianceLeCenterVariance=
        predictorAvailable&&
        t.maxEstimateToCenterVarianceRatio<=1.0;

    return
        t.width>0u&&t.height>0u&&
        t.v01CandidateCenters<=t.sampled&&
        t.predictorValid+t.predictorInvalid==t.v01CandidateCenters&&
        t.pairsAccepted+t.pairsRejected<=t.pairsConsidered&&
        t.scalesAccepted+t.scalesRejected==t.scalesConsidered&&
        t.centerZGt2<=t.predictorValid&&
        t.structureProtected<=t.sampled&&
        t.censoredProtected<=t.sampled&&
        t.censorBoundaryProtected<=t.sampled&&
        candidatePhases==t.v01CandidateCenters&&
        validPhases==t.predictorValid&&
        finite_fraction(t.candidateFraction)&&
        finite_fraction(t.predictorCoverage)&&
        finite_fraction(t.pairAcceptance)&&
        finite_fraction(t.scaleAcceptance)&&
        finite_fraction(t.centerZGt2Fraction)&&
        finite_fraction(t.structureProtectionFraction)&&
        finite_fraction(t.censorProtectionFraction)&&
        finite_fraction(t.censorBoundaryProtectionFraction)&&
        std::isfinite(t.meanEstimateToCenterVarianceRatio)&&
        std::isfinite(t.maxEstimateToCenterVarianceRatio)&&
        t.meanEstimateToCenterVarianceRatio>=0.0&&
        t.maxEstimateToCenterVarianceRatio>=
            t.meanEstimateToCenterVarianceRatio&&
        t.hasCandidates==hasCandidates&&
        t.predictorAvailable==predictorAvailable&&
        t.allCandidatesPredictable==allCandidatesPredictable&&
        t.centerOutlierFree==centerOutlierFree&&
        t.predictableAndCenterOutlierFree==
            predictableAndCenterOutlierFree&&
        t.pairRejectionFree==pairRejectionFree&&
        t.scaleRejectionFree==scaleRejectionFree&&
        t.structureProtectionPresent==structureProtectionPresent&&
        t.censorProtectionPresent==censorProtectionPresent&&
        t.censorBoundaryProtectionPresent==
            censorBoundaryProtectionPresent&&
        t.maxPredictorVarianceLeCenterVariance==
            maxPredictorVarianceLeCenterVariance&&
        !t.promotionEligible;
}

void hash_tile(
    truthraw::sha256_v0_69::Hasher& h,
    const TileFacts& t) noexcept {
    hash_u32(h,t.x);
    hash_u32(h,t.y);
    hash_u32(h,t.width);
    hash_u32(h,t.height);
    hash_u64(h,t.sampled);
    hash_u64(h,t.v01CandidateCenters);
    hash_u64(h,t.predictorValid);
    hash_u64(h,t.predictorInvalid);
    hash_u64(h,t.pairsConsidered);
    hash_u64(h,t.pairsAccepted);
    hash_u64(h,t.pairsRejected);
    hash_u64(h,t.scalesConsidered);
    hash_u64(h,t.scalesAccepted);
    hash_u64(h,t.scalesRejected);
    hash_u64(h,t.centerZGt2);
    hash_u64(h,t.structureProtected);
    hash_u64(h,t.censoredProtected);
    hash_u64(h,t.censorBoundaryProtected);
    hash_f64(h,t.candidateFraction);
    hash_f64(h,t.predictorCoverage);
    hash_f64(h,t.pairAcceptance);
    hash_f64(h,t.scaleAcceptance);
    hash_f64(h,t.centerZGt2Fraction);
    hash_f64(h,t.structureProtectionFraction);
    hash_f64(h,t.censorProtectionFraction);
    hash_f64(h,t.censorBoundaryProtectionFraction);
    hash_f64(h,t.meanEstimateToCenterVarianceRatio);
    hash_f64(h,t.maxEstimateToCenterVarianceRatio);
    for(auto v:t.v01CandidateCfaPhase)hash_u64(h,v);
    for(auto v:t.predictorValidCfaPhase)hash_u64(h,v);
    hash_bool(h,t.hasCandidates);
    hash_bool(h,t.predictorAvailable);
    hash_bool(h,t.allCandidatesPredictable);
    hash_bool(h,t.centerOutlierFree);
    hash_bool(h,t.predictableAndCenterOutlierFree);
    hash_bool(h,t.pairRejectionFree);
    hash_bool(h,t.scaleRejectionFree);
    hash_bool(h,t.structureProtectionPresent);
    hash_bool(h,t.censorProtectionPresent);
    hash_bool(h,t.censorBoundaryProtectionPresent);
    hash_bool(h,t.maxPredictorVarianceLeCenterVariance);
    hash_u32(h,static_cast<std::uint32_t>(t.legacySupportClass));
    hash_bool(h,t.promotionEligible);
}

void write_phase(
    std::ostringstream& o,
    const std::array<std::uint64_t,4u>& values) {
    o<<"["<<values[0]<<","<<values[1]<<","
     <<values[2]<<","<<values[3]<<"]";
}

void write_tile(std::ostringstream& o,const TileFacts& t) {
    o<<"{";
    o<<"\"x\":"<<t.x;
    o<<",\"y\":"<<t.y;
    o<<",\"width\":"<<t.width;
    o<<",\"height\":"<<t.height;
    o<<",\"sampled\":"<<t.sampled;
    o<<",\"v01_candidate_centers\":"<<t.v01CandidateCenters;
    o<<",\"predictor_valid\":"<<t.predictorValid;
    o<<",\"predictor_invalid\":"<<t.predictorInvalid;
    o<<",\"pairs_considered\":"<<t.pairsConsidered;
    o<<",\"pairs_accepted\":"<<t.pairsAccepted;
    o<<",\"pairs_rejected\":"<<t.pairsRejected;
    o<<",\"scales_considered\":"<<t.scalesConsidered;
    o<<",\"scales_accepted\":"<<t.scalesAccepted;
    o<<",\"scales_rejected\":"<<t.scalesRejected;
    o<<",\"center_z_gt_2\":"<<t.centerZGt2;
    o<<",\"structure_protected\":"<<t.structureProtected;
    o<<",\"censored_protected\":"<<t.censoredProtected;
    o<<",\"censor_boundary_protected\":"
     <<t.censorBoundaryProtected;
    o<<",\"candidate_fraction\":"<<t.candidateFraction;
    o<<",\"predictor_coverage\":"<<t.predictorCoverage;
    o<<",\"pair_acceptance\":"<<t.pairAcceptance;
    o<<",\"scale_acceptance\":"<<t.scaleAcceptance;
    o<<",\"center_z_gt_2_fraction\":"<<t.centerZGt2Fraction;
    o<<",\"structure_protection_fraction\":"
     <<t.structureProtectionFraction;
    o<<",\"censor_protection_fraction\":"
     <<t.censorProtectionFraction;
    o<<",\"censor_boundary_protection_fraction\":"
     <<t.censorBoundaryProtectionFraction;
    o<<",\"mean_estimate_to_center_variance_ratio\":"
     <<t.meanEstimateToCenterVarianceRatio;
    o<<",\"max_estimate_to_center_variance_ratio\":"
     <<t.maxEstimateToCenterVarianceRatio;
    o<<",\"v01_candidate_cfa_phase\":";
    write_phase(o,t.v01CandidateCfaPhase);
    o<<",\"predictor_valid_cfa_phase\":";
    write_phase(o,t.predictorValidCfaPhase);
    o<<",\"has_candidates\":"<<(t.hasCandidates?"true":"false");
    o<<",\"predictor_available\":"
     <<(t.predictorAvailable?"true":"false");
    o<<",\"all_candidates_predictable\":"
     <<(t.allCandidatesPredictable?"true":"false");
    o<<",\"center_outlier_free\":"
     <<(t.centerOutlierFree?"true":"false");
    o<<",\"predictable_and_center_outlier_free\":"
     <<(t.predictableAndCenterOutlierFree?"true":"false");
    o<<",\"pair_rejection_free\":"
     <<(t.pairRejectionFree?"true":"false");
    o<<",\"scale_rejection_free\":"
     <<(t.scaleRejectionFree?"true":"false");
    o<<",\"structure_protection_present\":"
     <<(t.structureProtectionPresent?"true":"false");
    o<<",\"censor_protection_present\":"
     <<(t.censorProtectionPresent?"true":"false");
    o<<",\"censor_boundary_protection_present\":"
     <<(t.censorBoundaryProtectionPresent?"true":"false");
    o<<",\"max_predictor_variance_le_center_variance\":"
     <<(t.maxPredictorVarianceLeCenterVariance?"true":"false");
    o<<",\"legacy_support_class\":\""
     <<legacy_class_name(t.legacySupportClass)<<"\"";
    o<<",\"promotion_eligible\":false";
    o<<"}";
}

} // namespace

bool derive(
    const Binding& binding,
    const cf::Result& confidenceField,
    Result& out) noexcept {
    out={};
    try{
        if(!nonzero(binding.sourceEvidenceSha256)||
           !nonzero(binding.scientificMasterSha256)||
           !nonzero(binding.authorityFieldSha256)||
           !nonzero(binding.truthNegativeStateSha256)||
           !nonzero(binding.v01CandidateSha256)||
           !nonzero(binding.v01AuditSha256)||
           !nonzero(binding.v01SpatialSha256)||
           !nonzero(binding.centerExcludedAuditSha256)||
           !nonzero(binding.confidenceFieldSha256)||
           binding.confidenceFieldSha256!=confidenceField.fieldSha256||
           confidenceField.tiles.empty()||
           !confidenceField.exactV01ParityVerified||
           !confidenceField.exactCenterExcludedParityVerified||
           !confidenceField.vectorValuedNoScalarProbability||
           !confidenceField.cfaPhaseDiagnosticOnly||
           confidenceField.supportDistanceAdmitted||
           confidenceField.promotionEligible||
           confidenceField.candidateApplied||
           confidenceField.createsNewEvidence||
           confidenceField.scientificWritebackAllowed){
            return false;
        }

        out.tileEdge=confidenceField.tileEdge;
        out.samplingPeriod=confidenceField.samplingPeriod;
        out.tiles.reserve(confidenceField.tiles.size());

        for(const auto& source:confidenceField.tiles){
            TileFacts t{};
            t.x=source.x;
            t.y=source.y;
            t.width=source.width;
            t.height=source.height;
            t.sampled=source.sampled;
            t.v01CandidateCenters=source.v01CandidateCenters;
            t.predictorValid=source.predictorValid;
            t.predictorInvalid=source.predictorInvalid;
            t.pairsConsidered=source.pairsConsidered;
            t.pairsAccepted=source.pairsAccepted;
            t.pairsRejected=source.pairsRejected;
            t.scalesConsidered=source.scalesConsidered;
            t.scalesAccepted=source.scalesAccepted;
            t.scalesRejected=source.scalesRejected;
            t.centerZGt2=source.centerZGt2;
            t.structureProtected=source.structureProtected;
            t.censoredProtected=source.censoredProtected;
            t.censorBoundaryProtected=source.censorBoundaryProtected;
            t.candidateFraction=source.candidateFraction;
            t.predictorCoverage=source.predictorCoverage;
            t.pairAcceptance=source.pairAcceptance;
            t.scaleAcceptance=source.scaleAcceptance;
            t.centerZGt2Fraction=source.centerZGt2Fraction;
            t.structureProtectionFraction=
                source.structureProtectionFraction;
            t.censorProtectionFraction=
                source.censorProtectionFraction;
            t.censorBoundaryProtectionFraction=
                source.censorBoundaryProtectionFraction;
            t.meanEstimateToCenterVarianceRatio=
                source.meanEstimateToCenterVarianceRatio;
            t.maxEstimateToCenterVarianceRatio=
                source.maxEstimateToCenterVarianceRatio;
            t.v01CandidateCfaPhase=source.v01CandidateCfaPhase;
            t.predictorValidCfaPhase=source.predictorValidCfaPhase;

            t.hasCandidates=t.v01CandidateCenters>0u;
            t.predictorAvailable=t.predictorValid>0u;
            t.allCandidatesPredictable=
                t.hasCandidates&&
                t.predictorValid==t.v01CandidateCenters&&
                t.predictorInvalid==0u;
            t.centerOutlierFree=
                t.predictorAvailable&&t.centerZGt2==0u;
            t.predictableAndCenterOutlierFree=
                t.allCandidatesPredictable&&t.centerOutlierFree;
            t.pairRejectionFree=
                t.pairsConsidered>0u&&t.pairsRejected==0u;
            t.scaleRejectionFree=
                t.scalesConsidered>0u&&t.scalesRejected==0u;
            t.structureProtectionPresent=t.structureProtected>0u;
            t.censorProtectionPresent=t.censoredProtected>0u;
            t.censorBoundaryProtectionPresent=
                t.censorBoundaryProtected>0u;
            t.maxPredictorVarianceLeCenterVariance=
                t.predictorAvailable&&
                t.maxEstimateToCenterVarianceRatio<=1.0;
            t.legacySupportClass=source.supportClass;
            t.promotionEligible=false;

            if(!fact_consistent(t))return false;

            if(t.hasCandidates)++out.hasCandidateTiles;
            if(t.allCandidatesPredictable){
                ++out.allCandidatesPredictableTiles;
            }
            if(t.centerOutlierFree)++out.centerOutlierFreeTiles;
            if(t.predictableAndCenterOutlierFree){
                ++out.predictableAndCenterOutlierFreeTiles;
            }
            if(t.pairRejectionFree)++out.pairRejectionFreeTiles;
            if(t.scaleRejectionFree)++out.scaleRejectionFreeTiles;
            if(t.structureProtectionPresent){
                ++out.structureProtectionPresentTiles;
            }
            if(t.censorProtectionPresent){
                ++out.censorProtectionPresentTiles;
            }
            if(t.censorBoundaryProtectionPresent){
                ++out.censorBoundaryProtectionPresentTiles;
            }
            if(t.maxPredictorVarianceLeCenterVariance){
                ++out.maxPredictorVarianceLeCenterVarianceTiles;
            }
            out.tiles.push_back(t);
        }

        truthraw::sha256_v0_69::Hasher h;
        constexpr char domain[]=
            "D_RAW_TN_N2_FACTORED_CONFIDENCE_STATE_V0_3_1";
        h.update(
            reinterpret_cast<const std::uint8_t*>(domain),
            sizeof(domain)-1u);
        h.update(binding.sourceEvidenceSha256);
        h.update(binding.scientificMasterSha256);
        h.update(binding.authorityFieldSha256);
        h.update(binding.truthNegativeStateSha256);
        h.update(binding.v01CandidateSha256);
        h.update(binding.v01AuditSha256);
        h.update(binding.v01SpatialSha256);
        h.update(binding.centerExcludedAuditSha256);
        h.update(binding.confidenceFieldSha256);
        hash_u32(h,out.tileEdge);
        hash_u32(h,out.samplingPeriod);
        hash_u64(h,out.hasCandidateTiles);
        hash_u64(h,out.allCandidatesPredictableTiles);
        hash_u64(h,out.centerOutlierFreeTiles);
        hash_u64(h,out.predictableAndCenterOutlierFreeTiles);
        hash_u64(h,out.pairRejectionFreeTiles);
        hash_u64(h,out.scaleRejectionFreeTiles);
        hash_u64(h,out.structureProtectionPresentTiles);
        hash_u64(h,out.censorProtectionPresentTiles);
        hash_u64(h,out.censorBoundaryProtectionPresentTiles);
        hash_u64(h,out.maxPredictorVarianceLeCenterVarianceTiles);
        for(const auto& t:out.tiles)hash_tile(h,t);
        out.stateSha256=h.finalize();

        out.exactConfidenceFieldBindingVerified=true;
        out.vectorValuedNoScalarProbability=true;
        out.legacyClassNonAuthoritative=true;
        out.cfaPhaseDiagnosticOnly=true;
        out.supportDistanceAdmitted=false;
        out.promotionEligible=false;
        out.candidateApplied=false;
        out.createsNewEvidence=false;
        out.scientificWritebackAllowed=false;

        return nonzero(out.stateSha256);
    }catch(...){
        out={};
        return false;
    }
}

bool encode(
    const Binding& binding,
    std::uint32_t sourceWidth,
    std::uint32_t sourceHeight,
    const Result& state,
    Report& out) noexcept {
    out={};
    try{
        if(sourceWidth==0u||sourceHeight==0u||
           !nonzero(binding.sourceEvidenceSha256)||
           !nonzero(binding.scientificMasterSha256)||
           !nonzero(binding.authorityFieldSha256)||
           !nonzero(binding.truthNegativeStateSha256)||
           !nonzero(binding.v01CandidateSha256)||
           !nonzero(binding.v01AuditSha256)||
           !nonzero(binding.v01SpatialSha256)||
           !nonzero(binding.centerExcludedAuditSha256)||
           !nonzero(binding.confidenceFieldSha256)||
           !nonzero(state.stateSha256)||
           state.tiles.empty()||
           !state.exactConfidenceFieldBindingVerified||
           !state.vectorValuedNoScalarProbability||
           !state.legacyClassNonAuthoritative||
           !state.cfaPhaseDiagnosticOnly||
           state.supportDistanceAdmitted||
           state.promotionEligible||
           state.candidateApplied||
           state.createsNewEvidence||
           state.scientificWritebackAllowed){
            return false;
        }

        for(const auto& t:state.tiles){
            if(!fact_consistent(t))return false;
        }

        std::ostringstream o;
        o.setf(std::ios::fixed);
        o<<std::setprecision(12);
        o<<"{\n";
        o<<"  \"schema\":\""<<kSchemaName<<"\",\n";
        o<<"  \"source_width\":"<<sourceWidth<<",\n";
        o<<"  \"source_height\":"<<sourceHeight<<",\n";
        o<<"  \"tile_edge\":"<<state.tileEdge<<",\n";
        o<<"  \"sampling_period\":"<<state.samplingPeriod<<",\n";
        o<<"  \"source_sha256\":\""
         <<hex(binding.sourceEvidenceSha256)<<"\",\n";
        o<<"  \"scientific_master_sha256\":\""
         <<hex(binding.scientificMasterSha256)<<"\",\n";
        o<<"  \"authority_field_sha256\":\""
         <<hex(binding.authorityFieldSha256)<<"\",\n";
        o<<"  \"truthnegative_state_sha256\":\""
         <<hex(binding.truthNegativeStateSha256)<<"\",\n";
        o<<"  \"v01_candidate_sha256\":\""
         <<hex(binding.v01CandidateSha256)<<"\",\n";
        o<<"  \"v01_audit_sha256\":\""
         <<hex(binding.v01AuditSha256)<<"\",\n";
        o<<"  \"v01_spatial_sha256\":\""
         <<hex(binding.v01SpatialSha256)<<"\",\n";
        o<<"  \"center_excluded_audit_sha256\":\""
         <<hex(binding.centerExcludedAuditSha256)<<"\",\n";
        o<<"  \"confidence_field_sha256\":\""
         <<hex(binding.confidenceFieldSha256)<<"\",\n";
        o<<"  \"factored_state_sha256\":\""
         <<hex(state.stateSha256)<<"\",\n";
        o<<"  \"exact_confidence_field_binding_verified\":true,\n";
        o<<"  \"vector_valued_no_scalar_probability\":true,\n";
        o<<"  \"legacy_class_non_authoritative\":true,\n";
        o<<"  \"cfa_phase_diagnostic_only\":true,\n";
        o<<"  \"support_distance_admitted\":false,\n";
        o<<"  \"promotion_eligible\":false,\n";
        o<<"  \"candidate_applied\":false,\n";
        o<<"  \"creates_new_evidence\":false,\n";
        o<<"  \"scientific_writeback_allowed\":false,\n";
        o<<"  \"global\":{";
        o<<"\"tile_count\":"<<state.tiles.size();
        o<<",\"has_candidate_tiles\":"<<state.hasCandidateTiles;
        o<<",\"all_candidates_predictable_tiles\":"
         <<state.allCandidatesPredictableTiles;
        o<<",\"center_outlier_free_tiles\":"
         <<state.centerOutlierFreeTiles;
        o<<",\"predictable_and_center_outlier_free_tiles\":"
         <<state.predictableAndCenterOutlierFreeTiles;
        o<<",\"pair_rejection_free_tiles\":"
         <<state.pairRejectionFreeTiles;
        o<<",\"scale_rejection_free_tiles\":"
         <<state.scaleRejectionFreeTiles;
        o<<",\"structure_protection_present_tiles\":"
         <<state.structureProtectionPresentTiles;
        o<<",\"censor_protection_present_tiles\":"
         <<state.censorProtectionPresentTiles;
        o<<",\"censor_boundary_protection_present_tiles\":"
         <<state.censorBoundaryProtectionPresentTiles;
        o<<",\"max_predictor_variance_le_center_variance_tiles\":"
         <<state.maxPredictorVarianceLeCenterVarianceTiles;
        o<<"},\n";
        o<<"  \"tiles\":[\n";
        for(std::size_t i=0u;i<state.tiles.size();++i){
            o<<"    ";
            write_tile(o,state.tiles[i]);
            if(i+1u<state.tiles.size())o<<",";
            o<<"\n";
        }
        o<<"  ]\n";
        o<<"}\n";

        out.json=o.str();
        truthraw::sha256_v0_69::Hasher h;
        h.update(
            reinterpret_cast<const std::uint8_t*>(out.json.data()),
            out.json.size());
        out.jsonSha256=h.finalize();
        out.tileCount=state.tiles.size();
        out.promotionEligible=false;
        out.candidateApplied=false;
        out.createsNewEvidence=false;
        out.scientificWritebackAllowed=false;
        return !out.json.empty()&&nonzero(out.jsonSha256);
    }catch(...){
        out={};
        return false;
    }
}

} // namespace truthraw::truthnegative_n2_factored_confidence_state::v0_3_1
