#include "truthnegative_n2_confidence_field_v0_3.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>

namespace truthraw::truthnegative_n2_confidence_field::v0_3 {
namespace {

bool nonzero(const Digest& d) noexcept {
    return std::any_of(
        d.begin(),d.end(),[](std::uint8_t v){return v!=0u;});
}

double fraction(std::uint64_t n,std::uint64_t d) noexcept {
    return d>0u
        ? static_cast<double>(n)/static_cast<double>(d)
        : 0.0;
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

const char* support_class_name(SupportClass c) noexcept {
    switch(c){
        case SupportClass::NoCandidate:return "NO_CANDIDATE";
        case SupportClass::Unresolved:return "UNRESOLVED";
        case SupportClass::Mixed:return "MIXED";
        case SupportClass::FullyCoherent:return "FULLY_COHERENT";
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

void hash_tile(
    truthraw::sha256_v0_69::Hasher& h,
    const TileVector& t) noexcept {
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
    hash_u64(h,t.centerZLe1);
    hash_u64(h,t.centerZ1To2);
    hash_u64(h,t.centerZGt2);
    hash_u64(h,t.v01Preserved);
    hash_u64(h,t.structureProtected);
    hash_u64(h,t.censoredProtected);
    hash_u64(h,t.censorBoundaryProtected);
    hash_f64(h,t.candidateFraction);
    hash_f64(h,t.predictorCoverage);
    hash_f64(h,t.pairAcceptance);
    hash_f64(h,t.scaleAcceptance);
    hash_f64(h,t.centerZLe1Fraction);
    hash_f64(h,t.centerZ1To2Fraction);
    hash_f64(h,t.centerZGt2Fraction);
    hash_f64(h,t.preserveFraction);
    hash_f64(h,t.structureProtectionFraction);
    hash_f64(h,t.censorProtectionFraction);
    hash_f64(h,t.censorBoundaryProtectionFraction);
    hash_f64(h,t.meanEstimateToCenterVarianceRatio);
    hash_f64(h,t.maxEstimateToCenterVarianceRatio);
    hash_f64(h,t.maxDirectionalDisagreementSigma);
    hash_f64(h,t.maxCrossScaleDisagreementSigma);
    for(auto v:t.v01CandidateCfaPhase)hash_u64(h,v);
    for(auto v:t.predictorValidCfaPhase)hash_u64(h,v);
    hash_u32(h,static_cast<std::uint32_t>(t.supportClass));
    hash_u32(h,t.promotionEligible?1u:0u);
}

bool finite_fraction(double v) noexcept {
    return std::isfinite(v)&&v>=0.0&&v<=1.0;
}

bool tile_consistent(const TileVector& t) noexcept {
    std::uint64_t candidatePhase=0u;
    std::uint64_t predictorPhase=0u;
    for(auto v:t.v01CandidateCfaPhase)candidatePhase+=v;
    for(auto v:t.predictorValidCfaPhase)predictorPhase+=v;

    return
        t.width>0u&&t.height>0u&&
        t.v01CandidateCenters<=t.sampled&&
        t.predictorValid+t.predictorInvalid==t.v01CandidateCenters&&
        t.pairsAccepted+t.pairsRejected<=t.pairsConsidered&&
        t.scalesAccepted+t.scalesRejected==t.scalesConsidered&&
        t.centerZLe1+t.centerZ1To2+t.centerZGt2==t.predictorValid&&
        t.v01Preserved<=t.sampled&&
        t.structureProtected<=t.v01Preserved&&
        t.censoredProtected<=t.v01Preserved&&
        t.censorBoundaryProtected<=t.v01Preserved&&
        candidatePhase==t.v01CandidateCenters&&
        predictorPhase==t.predictorValid&&
        finite_fraction(t.candidateFraction)&&
        finite_fraction(t.predictorCoverage)&&
        finite_fraction(t.pairAcceptance)&&
        finite_fraction(t.scaleAcceptance)&&
        finite_fraction(t.centerZLe1Fraction)&&
        finite_fraction(t.centerZ1To2Fraction)&&
        finite_fraction(t.centerZGt2Fraction)&&
        finite_fraction(t.preserveFraction)&&
        finite_fraction(t.structureProtectionFraction)&&
        finite_fraction(t.censorProtectionFraction)&&
        finite_fraction(t.censorBoundaryProtectionFraction)&&
        std::isfinite(t.meanEstimateToCenterVarianceRatio)&&
        std::isfinite(t.maxEstimateToCenterVarianceRatio)&&
        t.meanEstimateToCenterVarianceRatio>=0.0&&
        t.maxEstimateToCenterVarianceRatio>=
            t.meanEstimateToCenterVarianceRatio&&
        std::isfinite(t.maxDirectionalDisagreementSigma)&&
        t.maxDirectionalDisagreementSigma>=0.0&&
        std::isfinite(t.maxCrossScaleDisagreementSigma)&&
        t.maxCrossScaleDisagreementSigma>=0.0&&
        !t.promotionEligible;
}

SupportClass classify(const TileVector& t) noexcept {
    if(t.v01CandidateCenters==0u){
        return SupportClass::NoCandidate;
    }
    if(t.predictorValid==0u){
        return SupportClass::Unresolved;
    }
    const bool exactFullCoherence=
        t.predictorValid==t.v01CandidateCenters&&
        t.predictorInvalid==0u&&
        t.pairsConsidered>0u&&
        t.pairsRejected==0u&&
        t.scalesConsidered>0u&&
        t.scalesRejected==0u&&
        t.centerZGt2==0u;
    return exactFullCoherence
        ? SupportClass::FullyCoherent
        : SupportClass::Mixed;
}

void write_phase(
    std::ostringstream& o,
    const std::array<std::uint64_t,4u>& v) {
    o<<"["<<v[0]<<","<<v[1]<<","<<v[2]<<","<<v[3]<<"]";
}

void write_tile(std::ostringstream& o,const TileVector& t) {
    o<<"{";
    o<<"\"x\":"<<t.x;
    o<<",\"y\":"<<t.y;
    o<<",\"width\":"<<t.width;
    o<<",\"height\":"<<t.height;
    o<<",\"sampled\":"<<t.sampled;
    o<<",\"v01_candidate_centers\":"<<t.v01CandidateCenters;
    o<<",\"predictor_valid\":"<<t.predictorValid;
    o<<",\"predictor_invalid\":"<<t.predictorInvalid;
    o<<",\"candidate_fraction\":"<<t.candidateFraction;
    o<<",\"predictor_coverage\":"<<t.predictorCoverage;
    o<<",\"pair_acceptance\":"<<t.pairAcceptance;
    o<<",\"scale_acceptance\":"<<t.scaleAcceptance;
    o<<",\"center_z_le_1_fraction\":"<<t.centerZLe1Fraction;
    o<<",\"center_z_1_to_2_fraction\":"<<t.centerZ1To2Fraction;
    o<<",\"center_z_gt_2_fraction\":"<<t.centerZGt2Fraction;
    o<<",\"preserve_fraction\":"<<t.preserveFraction;
    o<<",\"structure_protection_fraction\":"
     <<t.structureProtectionFraction;
    o<<",\"censor_protection_fraction\":"
     <<t.censorProtectionFraction;
    o<<",\"censor_boundary_protection_fraction\":"
     <<t.censorBoundaryProtectionFraction;
    o<<",\"pairs_considered\":"<<t.pairsConsidered;
    o<<",\"pairs_accepted\":"<<t.pairsAccepted;
    o<<",\"pairs_rejected\":"<<t.pairsRejected;
    o<<",\"scales_considered\":"<<t.scalesConsidered;
    o<<",\"scales_accepted\":"<<t.scalesAccepted;
    o<<",\"scales_rejected\":"<<t.scalesRejected;
    o<<",\"center_z_le_1\":"<<t.centerZLe1;
    o<<",\"center_z_1_to_2\":"<<t.centerZ1To2;
    o<<",\"center_z_gt_2\":"<<t.centerZGt2;
    o<<",\"v01_preserved\":"<<t.v01Preserved;
    o<<",\"structure_protected\":"<<t.structureProtected;
    o<<",\"censored_protected\":"<<t.censoredProtected;
    o<<",\"censor_boundary_protected\":"
     <<t.censorBoundaryProtected;
    o<<",\"mean_estimate_to_center_variance_ratio\":"
     <<t.meanEstimateToCenterVarianceRatio;
    o<<",\"max_estimate_to_center_variance_ratio\":"
     <<t.maxEstimateToCenterVarianceRatio;
    o<<",\"max_directional_disagreement_sigma\":"
     <<t.maxDirectionalDisagreementSigma;
    o<<",\"max_cross_scale_disagreement_sigma\":"
     <<t.maxCrossScaleDisagreementSigma;
    o<<",\"v01_candidate_cfa_phase\":";
    write_phase(o,t.v01CandidateCfaPhase);
    o<<",\"predictor_valid_cfa_phase\":";
    write_phase(o,t.predictorValidCfaPhase);
    o<<",\"support_class\":\""
     <<support_class_name(t.supportClass)<<"\"";
    o<<",\"promotion_eligible\":false";
    o<<"}";
}

} // namespace

bool derive(
    const Binding& binding,
    const v01::Result& v01Reference,
    const ce::Result& centerExcludedReference,
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
           binding.v01CandidateSha256!=v01Reference.candidateSha256||
           binding.v01AuditSha256!=v01Reference.auditSha256||
           binding.v01SpatialSha256!=v01Reference.spatialSha256||
           binding.centerExcludedAuditSha256!=
               centerExcludedReference.auditSha256||
           v01Reference.tiles.empty()||
           centerExcludedReference.tiles.empty()||
           v01Reference.tiles.size()!=
               centerExcludedReference.tiles.size()||
           v01Reference.tileEdge!=centerExcludedReference.tileEdge||
           v01Reference.samplingPeriod!=
               centerExcludedReference.samplingPeriod||
           !centerExcludedReference.v01TileParityVerified||
           !centerExcludedReference.centerOnlySigmaPrimary||
           !centerExcludedReference.combinedSigmaDiagnosticOnly||
           centerExcludedReference.noiseIndependenceAdmitted||
           centerExcludedReference.candidateApplied||
           centerExcludedReference.createsNewEvidence||
           centerExcludedReference.scientificWritebackAllowed||
           v01Reference.sourceValuesModified||
           v01Reference.truthNegativeModified||
           v01Reference.createsNewEvidence||
           v01Reference.scientificWritebackAllowed){
            return false;
        }

        out.tileEdge=v01Reference.tileEdge;
        out.samplingPeriod=v01Reference.samplingPeriod;
        out.tiles.reserve(v01Reference.tiles.size());

        for(std::size_t i=0u;i<v01Reference.tiles.size();++i){
            const auto& a=v01Reference.tiles[i];
            const auto& c=centerExcludedReference.tiles[i];
            if(a.x!=c.x||a.y!=c.y||
               a.width!=c.width||a.height!=c.height||
               a.sampled!=c.metrics.sampled||
               a.audit.corrected!=c.metrics.v01CandidateCenters||
               c.metrics.predictorValid+
                   c.metrics.predictorInvalid!=
                   c.metrics.v01CandidateCenters||
               c.metrics.v01CandidateCfaPhase!=
                   c.metrics.v01CandidateCfaPhase){
                return false;
            }

            TileVector t{};
            t.x=a.x;
            t.y=a.y;
            t.width=a.width;
            t.height=a.height;
            t.sampled=a.sampled;
            t.v01CandidateCenters=c.metrics.v01CandidateCenters;
            t.predictorValid=c.metrics.predictorValid;
            t.predictorInvalid=c.metrics.predictorInvalid;
            t.pairsConsidered=c.metrics.symmetricPairsConsidered;
            t.pairsAccepted=c.metrics.symmetricPairsAccepted;
            t.pairsRejected=c.metrics.symmetricPairsRejected;
            t.scalesConsidered=c.metrics.scalesConsidered;
            t.scalesAccepted=c.metrics.scalesAccepted;
            t.scalesRejected=c.metrics.scalesRejected;
            t.centerZLe1=c.metrics.centerResidualWithin1Sigma;
            t.centerZ1To2=c.metrics.centerResidualBetween1And2Sigma;
            t.centerZGt2=c.metrics.centerResidualAbove2Sigma;

            t.v01Preserved=a.audit.preserved;
            t.structureProtected=a.audit.structureProtected;
            t.censoredProtected=a.audit.censoredProtected;
            t.censorBoundaryProtected=a.audit.censorBoundaryProtected;

            t.candidateFraction=fraction(
                t.v01CandidateCenters,t.sampled);
            t.predictorCoverage=fraction(
                t.predictorValid,t.v01CandidateCenters);
            t.pairAcceptance=fraction(
                t.pairsAccepted,t.pairsConsidered);
            t.scaleAcceptance=fraction(
                t.scalesAccepted,t.scalesConsidered);
            t.centerZLe1Fraction=fraction(
                t.centerZLe1,t.predictorValid);
            t.centerZ1To2Fraction=fraction(
                t.centerZ1To2,t.predictorValid);
            t.centerZGt2Fraction=fraction(
                t.centerZGt2,t.predictorValid);
            t.preserveFraction=fraction(
                t.v01Preserved,t.sampled);
            t.structureProtectionFraction=fraction(
                t.structureProtected,t.sampled);
            t.censorProtectionFraction=fraction(
                t.censoredProtected,t.sampled);
            t.censorBoundaryProtectionFraction=fraction(
                t.censorBoundaryProtected,t.sampled);

            t.meanEstimateToCenterVarianceRatio=
                t.predictorValid>0u
                    ? c.metrics.estimateToCenterVarianceRatioSum/
                        static_cast<double>(t.predictorValid)
                    : 0.0;
            t.maxEstimateToCenterVarianceRatio=
                c.metrics.maxEstimateToCenterVarianceRatio;
            t.maxDirectionalDisagreementSigma=
                c.metrics.maxDirectionalDisagreementSigma;
            t.maxCrossScaleDisagreementSigma=
                c.metrics.maxCrossScaleDisagreementSigma;
            t.v01CandidateCfaPhase=
                c.metrics.v01CandidateCfaPhase;
            t.predictorValidCfaPhase=
                c.metrics.predictorValidCfaPhase;

            t.supportClass=classify(t);
            t.promotionEligible=false;

            if(!tile_consistent(t))return false;

            switch(t.supportClass){
                case SupportClass::NoCandidate:
                    ++out.noCandidateTiles;
                    break;
                case SupportClass::Unresolved:
                    ++out.unresolvedTiles;
                    break;
                case SupportClass::Mixed:
                    ++out.mixedTiles;
                    break;
                case SupportClass::FullyCoherent:
                    ++out.fullyCoherentTiles;
                    break;
            }
            out.tiles.push_back(t);
        }

        const auto classTotal=
            out.noCandidateTiles+
            out.unresolvedTiles+
            out.mixedTiles+
            out.fullyCoherentTiles;
        if(classTotal!=out.tiles.size())return false;

        truthraw::sha256_v0_69::Hasher h;
        constexpr char domain[]=
            "D_RAW_TN_N2_CONFIDENCE_FIELD_V0_3";
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
        hash_u32(h,out.tileEdge);
        hash_u32(h,out.samplingPeriod);
        hash_u64(h,out.noCandidateTiles);
        hash_u64(h,out.unresolvedTiles);
        hash_u64(h,out.mixedTiles);
        hash_u64(h,out.fullyCoherentTiles);
        for(const auto& t:out.tiles)hash_tile(h,t);
        out.fieldSha256=h.finalize();

        out.exactV01ParityVerified=true;
        out.exactCenterExcludedParityVerified=true;
        out.vectorValuedNoScalarProbability=true;
        out.cfaPhaseDiagnosticOnly=true;
        out.supportDistanceAdmitted=false;
        out.promotionEligible=false;
        out.candidateApplied=false;
        out.createsNewEvidence=false;
        out.scientificWritebackAllowed=false;

        return nonzero(out.fieldSha256);
    }catch(...){
        out={};
        return false;
    }
}

bool encode(
    const Binding& binding,
    std::uint32_t sourceWidth,
    std::uint32_t sourceHeight,
    const Result& field,
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
           !nonzero(field.fieldSha256)||
           field.tiles.empty()||
           !field.exactV01ParityVerified||
           !field.exactCenterExcludedParityVerified||
           !field.vectorValuedNoScalarProbability||
           !field.cfaPhaseDiagnosticOnly||
           field.supportDistanceAdmitted||
           field.promotionEligible||
           field.candidateApplied||
           field.createsNewEvidence||
           field.scientificWritebackAllowed){
            return false;
        }

        std::uint64_t sampled=0u;
        std::uint64_t candidates=0u;
        std::uint64_t valid=0u;
        std::uint64_t pairsConsidered=0u;
        std::uint64_t pairsAccepted=0u;
        std::uint64_t scalesConsidered=0u;
        std::uint64_t scalesAccepted=0u;
        std::uint64_t centerGt2=0u;
        for(const auto& t:field.tiles){
            if(!tile_consistent(t))return false;
            sampled+=t.sampled;
            candidates+=t.v01CandidateCenters;
            valid+=t.predictorValid;
            pairsConsidered+=t.pairsConsidered;
            pairsAccepted+=t.pairsAccepted;
            scalesConsidered+=t.scalesConsidered;
            scalesAccepted+=t.scalesAccepted;
            centerGt2+=t.centerZGt2;
        }

        std::ostringstream o;
        o.setf(std::ios::fixed);
        o<<std::setprecision(12);
        o<<"{\n";
        o<<"  \"schema\":\""<<kSchemaName<<"\",\n";
        o<<"  \"source_width\":"<<sourceWidth<<",\n";
        o<<"  \"source_height\":"<<sourceHeight<<",\n";
        o<<"  \"tile_edge\":"<<field.tileEdge<<",\n";
        o<<"  \"sampling_period\":"<<field.samplingPeriod<<",\n";
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
         <<hex(field.fieldSha256)<<"\",\n";
        o<<"  \"exact_v01_parity_verified\":true,\n";
        o<<"  \"exact_center_excluded_parity_verified\":true,\n";
        o<<"  \"vector_valued_no_scalar_probability\":true,\n";
        o<<"  \"cfa_phase_diagnostic_only\":true,\n";
        o<<"  \"support_distance_admitted\":false,\n";
        o<<"  \"promotion_eligible\":false,\n";
        o<<"  \"candidate_applied\":false,\n";
        o<<"  \"creates_new_evidence\":false,\n";
        o<<"  \"scientific_writeback_allowed\":false,\n";
        o<<"  \"global\":{";
        o<<"\"sampled\":"<<sampled;
        o<<",\"v01_candidate_centers\":"<<candidates;
        o<<",\"predictor_valid\":"<<valid;
        o<<",\"candidate_fraction\":"<<fraction(candidates,sampled);
        o<<",\"predictor_coverage\":"<<fraction(valid,candidates);
        o<<",\"pair_acceptance\":"
         <<fraction(pairsAccepted,pairsConsidered);
        o<<",\"scale_acceptance\":"
         <<fraction(scalesAccepted,scalesConsidered);
        o<<",\"center_z_gt_2_fraction\":"
         <<fraction(centerGt2,valid);
        o<<",\"no_candidate_tiles\":"<<field.noCandidateTiles;
        o<<",\"unresolved_tiles\":"<<field.unresolvedTiles;
        o<<",\"mixed_tiles\":"<<field.mixedTiles;
        o<<",\"fully_coherent_tiles\":"<<field.fullyCoherentTiles;
        o<<"},\n";
        o<<"  \"tiles\":[\n";
        for(std::size_t i=0u;i<field.tiles.size();++i){
            o<<"    ";
            write_tile(o,field.tiles[i]);
            if(i+1u<field.tiles.size())o<<",";
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
        out.tileCount=field.tiles.size();
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

} // namespace truthraw::truthnegative_n2_confidence_field::v0_3
