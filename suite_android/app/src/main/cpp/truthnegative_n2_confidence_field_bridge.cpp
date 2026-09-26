#include <jni.h>

#include "truthnegative_center_excluded_spatial_audit_v0_2_1.h"
#include "truthnegative_n2_cfa_audit_v0_1.h"
#include "truthnegative_n2_confidence_field_v0_3.h"
#include "truthnegative_pipeline_bridge_common.h"
#include "truthraw_sha256_v0_69.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {

namespace pipeline =
    truthraw::android_truthnegative_pipeline::v0_1;
namespace n2_cfa =
    truthraw::truthnegative_n2_cfa_audit::v0_1;
namespace ce_spatial =
    truthraw::truthnegative_center_excluded_spatial_audit::v0_2_1;
namespace confidence =
    truthraw::truthnegative_n2_confidence_field::v0_3;
namespace sha = truthraw::sha256_v0_69;

jstring status(JNIEnv* env,int code,const std::string& message) {
    std::ostringstream o;
    o<<"{\"status\":"<<code<<",\"message\":\"";
    for(char c:message){
        if(c=='"'||c=='\\')o<<'\\';
        if(c=='\n'||c=='\r')o<<' ';
        else o<<c;
    }
    o<<"\"}";
    return env->NewStringUTF(o.str().c_str());
}

bool write_all(int fd,const std::string& data) noexcept {
    if(fd<0)return false;
    if(::ftruncate(fd,0)!=0)return false;
    std::size_t done=0u;
    while(done<data.size()){
        const ssize_t n=::pwrite(
            fd,
            data.data()+done,
            data.size()-done,
            static_cast<off_t>(done));
        if(n<=0)return false;
        done+=static_cast<std::size_t>(n);
    }
    return ::fsync(fd)==0;
}

bool readback_sha(
    int fd,
    std::size_t expectedBytes,
    sha::Digest& digest) noexcept {
    if(fd<0)return false;
    struct stat st{};
    if(::fstat(fd,&st)!=0||st.st_size<0||
       static_cast<std::uint64_t>(st.st_size)!=expectedBytes){
        return false;
    }
    sha::Hasher h;
    std::vector<std::uint8_t> buffer(64u*1024u);
    std::size_t offset=0u;
    while(offset<expectedBytes){
        const std::size_t want=
            std::min(buffer.size(),expectedBytes-offset);
        const ssize_t n=::pread(
            fd,buffer.data(),want,static_cast<off_t>(offset));
        if(n<=0)return false;
        h.update(buffer.data(),static_cast<std::size_t>(n));
        offset+=static_cast<std::size_t>(n);
    }
    digest=h.finalize();
    return true;
}

double fraction(std::uint64_t n,std::uint64_t d) noexcept {
    return d>0u
        ? static_cast<double>(n)/static_cast<double>(d)
        : 0.0;
}

} // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_truthraw_adaptiveui_TruthNegativeN2ConfidenceFieldBridge_exportAndVerify(
    JNIEnv* env,
    jobject,
    jint sourceFd,
    jint destinationFd,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    if(destinationFd<0){
        return status(env,-80,"invalid destination fd");
    }

    pipeline::Context ctx{};
    const auto prepared=pipeline::prepare(
        sourceFd,
        static_cast<std::size_t>(std::max(0,maxSourceResidentBytes)),
        static_cast<std::size_t>(std::max(0,maxLogicalResidentBytes)),
        ctx);
    if(!prepared){
        return status(env,prepared.code,prepared.message);
    }

    n2_cfa::Binding v01Binding{};
    v01Binding.sourceEvidenceSha256=ctx.sourceSeal.sha256;
    v01Binding.truthNegativeStateSha256=
        ctx.truthNegativeState.stateSha256;

    n2_cfa::Options options{};
    options.tileEdge=64u;
    options.samplingPeriod=8u;

    n2_cfa::Result v01{};
    if(!n2_cfa::run(
            *ctx.openedSource.source,
            v01Binding,
            options,
            v01)||
       v01.sourceValuesModified||
       v01.truthNegativeModified||
       v01.createsNewEvidence||
       v01.scientificWritebackAllowed){
        return status(env,-81,"v0.1 confidence reference audit failed");
    }

    ce_spatial::Binding ceBinding{};
    ceBinding.sourceEvidenceSha256=ctx.sourceSeal.sha256;
    ceBinding.scientificMasterSha256=
        ctx.scientific.scientificMasterHash;
    ceBinding.authorityFieldSha256=
        ctx.authorityField.contentSha256;
    ceBinding.truthNegativeStateSha256=
        ctx.truthNegativeState.stateSha256;
    ceBinding.v01CandidateSha256=v01.candidateSha256;
    ceBinding.v01AuditSha256=v01.auditSha256;
    ceBinding.v01SpatialSha256=v01.spatialSha256;

    ce_spatial::Result ceAudit{};
    if(!ce_spatial::run(
            *ctx.openedSource.source,
            ceBinding,
            v01,
            ceAudit)||
       !ceAudit.v01TileParityVerified||
       ceAudit.candidateApplied||
       ceAudit.createsNewEvidence||
       ceAudit.scientificWritebackAllowed){
        return status(env,-82,"v0.2.1 confidence reference audit failed");
    }

    confidence::Binding binding{};
    binding.sourceEvidenceSha256=ctx.sourceSeal.sha256;
    binding.scientificMasterSha256=
        ctx.scientific.scientificMasterHash;
    binding.authorityFieldSha256=
        ctx.authorityField.contentSha256;
    binding.truthNegativeStateSha256=
        ctx.truthNegativeState.stateSha256;
    binding.v01CandidateSha256=v01.candidateSha256;
    binding.v01AuditSha256=v01.auditSha256;
    binding.v01SpatialSha256=v01.spatialSha256;
    binding.centerExcludedAuditSha256=ceAudit.auditSha256;

    confidence::Result field{};
    if(!confidence::derive(binding,v01,ceAudit,field)||
       !field.exactV01ParityVerified||
       !field.exactCenterExcludedParityVerified||
       !field.vectorValuedNoScalarProbability||
       !field.cfaPhaseDiagnosticOnly||
       field.supportDistanceAdmitted||
       field.promotionEligible||
       field.candidateApplied||
       field.createsNewEvidence||
       field.scientificWritebackAllowed){
        return status(env,-83,"N2 confidence field derivation failed");
    }

    confidence::Report report{};
    if(!confidence::encode(
            binding,
            ctx.width,
            ctx.height,
            field,
            report)||
       report.promotionEligible||
       report.candidateApplied||
       report.createsNewEvidence||
       report.scientificWritebackAllowed){
        return status(env,-84,"N2 confidence field encode failed");
    }

    if(!write_all(destinationFd,report.json)){
        return status(env,-85,"N2 confidence field write failed");
    }

    sha::Digest writtenSha{};
    if(!readback_sha(
            destinationFd,
            report.json.size(),
            writtenSha)||
       writtenSha!=report.jsonSha256){
        return status(env,-86,"N2 confidence field post-write SHA mismatch");
    }

    if(!pipeline::reverify(ctx)){
        return status(env,-87,"source changed during N2 confidence export");
    }

    const auto& m=ceAudit.metrics;
    std::ostringstream o;
    o<<"{\"status\":0";
    o<<",\"width\":"<<ctx.width;
    o<<",\"height\":"<<ctx.height;
    o<<",\"fileBytes\":"<<report.json.size();
    o<<",\"tileCount\":"<<report.tileCount;
    o<<",\"sampled\":"<<m.sampled;
    o<<",\"v01CandidateCenters\":"<<m.v01CandidateCenters;
    o<<",\"predictorValid\":"<<m.predictorValid;
    o<<",\"candidateFraction\":"
      <<fraction(m.v01CandidateCenters,m.sampled);
    o<<",\"predictorCoverage\":"
      <<fraction(m.predictorValid,m.v01CandidateCenters);
    o<<",\"pairAcceptance\":"
      <<fraction(m.symmetricPairsAccepted,m.symmetricPairsConsidered);
    o<<",\"scaleAcceptance\":"
      <<fraction(m.scalesAccepted,m.scalesConsidered);
    o<<",\"centerZGt2Fraction\":"
      <<fraction(m.centerResidualAbove2Sigma,m.predictorValid);
    o<<",\"noCandidateTiles\":"<<field.noCandidateTiles;
    o<<",\"unresolvedTiles\":"<<field.unresolvedTiles;
    o<<",\"mixedTiles\":"<<field.mixedTiles;
    o<<",\"fullyCoherentTiles\":"<<field.fullyCoherentTiles;
    o<<",\"sourceSha256\":\""
      <<sha::hex(ctx.sourceSeal.sha256)<<"\"";
    o<<",\"scientificMasterSha256\":\""
      <<sha::hex(ctx.scientific.scientificMasterHash)<<"\"";
    o<<",\"authorityFieldSha256\":\""
      <<sha::hex(ctx.authorityField.contentSha256)<<"\"";
    o<<",\"truthNegativeStateSha256\":\""
      <<sha::hex(ctx.truthNegativeState.stateSha256)<<"\"";
    o<<",\"v01CandidateSha256\":\""
      <<sha::hex(v01.candidateSha256)<<"\"";
    o<<",\"v01AuditSha256\":\""
      <<sha::hex(v01.auditSha256)<<"\"";
    o<<",\"v01SpatialSha256\":\""
      <<sha::hex(v01.spatialSha256)<<"\"";
    o<<",\"centerExcludedAuditSha256\":\""
      <<sha::hex(ceAudit.auditSha256)<<"\"";
    o<<",\"confidenceFieldSha256\":\""
      <<sha::hex(field.fieldSha256)<<"\"";
    o<<",\"jsonSha256\":\""
      <<sha::hex(report.jsonSha256)<<"\"";
    o<<",\"exactV01ParityVerified\":true";
    o<<",\"exactCenterExcludedParityVerified\":true";
    o<<",\"vectorValuedNoScalarProbability\":true";
    o<<",\"cfaPhaseDiagnosticOnly\":true";
    o<<",\"supportDistanceAdmitted\":false";
    o<<",\"promotionEligible\":false";
    o<<",\"postWriteVerified\":true";
    o<<",\"candidateApplied\":false";
    o<<",\"createsNewEvidence\":false";
    o<<",\"scientificWritebackAllowed\":false}";
    return env->NewStringUTF(o.str().c_str());
}
