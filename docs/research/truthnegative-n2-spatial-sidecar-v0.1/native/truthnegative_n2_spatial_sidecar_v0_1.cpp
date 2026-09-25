#include "truthnegative_n2_spatial_sidecar_v0_1.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace truthraw::truthnegative_n2_spatial_sidecar::v0_1 {
namespace {

bool nonzero(const Digest& d) noexcept {
    return std::any_of(d.begin(), d.end(),
        [](std::uint8_t v){ return v != 0u; });
}

std::string hex(const Digest& d) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out(d.size()*2u, '0');
    for(std::size_t i=0;i<d.size();++i){
        out[2u*i]=kHex[d[i]>>4u];
        out[2u*i+1u]=kHex[d[i]&0x0fu];
    }
    return out;
}

double removed_fraction(const cfa::n2::Audit& a) noexcept {
    if(!(a.totalResidualEnergy>0.0) ||
       !std::isfinite(a.totalResidualEnergy) ||
       !std::isfinite(a.removedResidualEnergy)) return 0.0;
    return std::clamp(
        a.removedResidualEnergy/a.totalResidualEnergy,0.0,1.0);
}

void write_audit(std::ostringstream& o,const cfa::n2::Audit& a) {
    o<<"\"sampled\":"<<a.total;
    o<<",\"eligible\":"<<a.eligible;
    o<<",\"candidate_corrected\":"<<a.corrected;
    o<<",\"preserved\":"<<a.preserved;
    o<<",\"censored_protected\":"<<a.censoredProtected;
    o<<",\"censor_boundary_protected\":"<<a.censorBoundaryProtected;
    o<<",\"unknown_noise_protected\":"<<a.unknownNoiseProtected;
    o<<",\"non_measured_protected\":"<<a.nonMeasuredProtected;
    o<<",\"weak_registration_protected\":"<<a.weakRegistrationProtected;
    o<<",\"structure_protected\":"<<a.structureProtected;
    o<<",\"no_neighborhood_protected\":"<<a.noNeighborhoodProtected;
    o<<",\"residual_outlier_protected\":"<<a.residualOutlierProtected;
    o<<",\"total_residual_energy\":"<<a.totalResidualEnergy;
    o<<",\"removed_residual_energy\":"<<a.removedResidualEnergy;
    o<<",\"removed_residual_energy_fraction\":"<<removed_fraction(a);
    o<<",\"max_abs_correction_stage2\":"<<a.maxAbsCorrection;
}

} // namespace

bool encode(
    const Binding& binding,
    std::uint32_t sourceWidth,
    std::uint32_t sourceHeight,
    const cfa::Result& audit,
    Report& out) noexcept {
    out={};
    try{
        if(sourceWidth==0u||sourceHeight==0u||
           !nonzero(binding.sourceEvidenceSha256)||
           !nonzero(binding.scientificMasterSha256)||
           !nonzero(binding.authorityFieldSha256)||
           !nonzero(binding.truthNegativeStateSha256)||
           !nonzero(audit.candidateSha256)||
           !nonzero(audit.auditSha256)||
           !nonzero(audit.spatialSha256)||
           audit.sampled==0u||audit.tiles.empty()||
           audit.audit.total!=audit.sampled||
           audit.sourceValuesModified||
           audit.truthNegativeModified||
           audit.createsNewEvidence||
           audit.scientificWritebackAllowed){
            return false;
        }

        std::uint64_t tileSamples=0u;
        for(const auto& t:audit.tiles){
            if(t.width==0u||t.height==0u||
               t.x+t.width>sourceWidth||
               t.y+t.height>sourceHeight||
               t.audit.total!=t.sampled||
               t.sampled==0u) return false;
            tileSamples+=t.sampled;
        }
        if(tileSamples!=audit.sampled)return false;

        std::ostringstream o;
        o.setf(std::ios::fixed);
        o<<std::setprecision(12);
        o<<"{\n";
        o<<"  \"schema\":\""<<kSchemaName<<"\",\n";
        o<<"  \"source_width\":"<<sourceWidth<<",\n";
        o<<"  \"source_height\":"<<sourceHeight<<",\n";
        o<<"  \"tile_edge\":"<<audit.tileEdge<<",\n";
        o<<"  \"sampling_period\":"<<audit.samplingPeriod<<",\n";
        o<<"  \"source_sha256\":\""<<hex(binding.sourceEvidenceSha256)<<"\",\n";
        o<<"  \"scientific_master_sha256\":\""<<hex(binding.scientificMasterSha256)<<"\",\n";
        o<<"  \"authority_field_sha256\":\""<<hex(binding.authorityFieldSha256)<<"\",\n";
        o<<"  \"truthnegative_state_sha256\":\""<<hex(binding.truthNegativeStateSha256)<<"\",\n";
        o<<"  \"candidate_sha256\":\""<<hex(audit.candidateSha256)<<"\",\n";
        o<<"  \"n2_audit_sha256\":\""<<hex(audit.auditSha256)<<"\",\n";
        o<<"  \"spatial_sha256\":\""<<hex(audit.spatialSha256)<<"\",\n";
        o<<"  \"noise_profile_available\":"<<(audit.noiseProfileAvailable?"true":"false")<<",\n";
        o<<"  \"creates_new_evidence\":false,\n";
        o<<"  \"scientific_writeback_allowed\":false,\n";
        o<<"  \"candidate_applied\":false,\n";
        o<<"  \"global\":{";
        write_audit(o,audit.audit);
        o<<",\"border_protected\":"<<audit.borderProtected;
        o<<",\"cfa_phase_samples\":["
         <<audit.cfaPhaseSamples[0]<<","<<audit.cfaPhaseSamples[1]<<","
         <<audit.cfaPhaseSamples[2]<<","<<audit.cfaPhaseSamples[3]<<"]},\n";
        o<<"  \"tiles\":[\n";
        for(std::size_t i=0u;i<audit.tiles.size();++i){
            const auto& t=audit.tiles[i];
            o<<"    {\"x\":"<<t.x<<",\"y\":"<<t.y
             <<",\"width\":"<<t.width<<",\"height\":"<<t.height<<",";
            write_audit(o,t.audit);
            o<<",\"border_protected\":"<<t.borderProtected;
            o<<",\"cfa_phase_samples\":["
             <<t.cfaPhaseSamples[0]<<","<<t.cfaPhaseSamples[1]<<","
             <<t.cfaPhaseSamples[2]<<","<<t.cfaPhaseSamples[3]<<"]}";
            if(i+1u<audit.tiles.size())o<<",";
            o<<"\n";
        }
        o<<"  ]\n";
        o<<"}\n";

        out.json=o.str();
        truthraw::sha256_v0_69::Hasher hasher;
        hasher.update(
            reinterpret_cast<const std::uint8_t*>(out.json.data()),
            out.json.size());
        out.jsonSha256=hasher.finalize();
        out.tileCount=audit.tiles.size();
        out.createsNewEvidence=false;
        out.scientificWritebackAllowed=false;
        out.candidateApplied=false;
        return !out.json.empty()&&nonzero(out.jsonSha256);
    }catch(...){
        out={};
        return false;
    }
}

} // namespace truthraw::truthnegative_n2_spatial_sidecar::v0_1
