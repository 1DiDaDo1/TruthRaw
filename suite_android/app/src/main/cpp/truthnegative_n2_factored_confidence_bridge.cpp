#include <jni.h>

#include "truthnegative_center_excluded_spatial_audit_v0_2_1.h"
#include "truthnegative_n2_cfa_audit_v0_1.h"
#include "truthnegative_n2_confidence_field_v0_3.h"
#include "truthnegative_n2_factored_confidence_state_v0_3_1.h"
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
namespace factored =
    truthraw::truthnegative_n2_factored_confidence_state::v0_3_1;
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

} // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_truthraw_adaptiveui_TruthNegativeN2FactoredConfidenceBridge_exportAndVerify(
    JNIEnv* env,
    jobject,
    jint sourceFd,
    jint destinationFd,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    if(destinationFd<0){
        return status(env,-90,"invalid destination fd");
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
        return status(env,-91,"v0.1 factored reference audit failed");
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
        return status(env,-92,"v0.2.1 factored reference audit failed");
    }

    confidence::Binding confidenceBinding{};
    confidenceBinding.sourceEvidenceSha256=ctx.sourceSeal.sha256;
    confidenceBinding.scientificMasterSha256=
        ctx.scientific.scientificMasterHash;
    confidenceBinding.authorityFieldSha256=
        ctx.authorityField.contentSha256;
    confidenceBinding.truthNegativeStateSha256=
        ctx.truthNegativeState.stateSha256;
    confidenceBinding.v01CandidateSha256=v01.candidateSha256;
    confidenceBinding.v01AuditSha256=v01.auditSha256;
    confidenceBinding.v01SpatialSha256=v01.spatialSha256;
    confidenceBinding.centerExcludedAuditSha256=ceAudit.auditSha256;

    confidence::Result confidenceField{};
    if(!confidence::derive(
            confidenceBinding,
            v01,
            ceAudit,
            confidenceField)||
       !confidenceField.exactV01ParityVerified||
       !confidenceField.exactCenterExcludedParityVerified||
       confidenceField.promotionEligible||
       confidenceField.candidateApplied||
       confidenceField.createsNewEvidence||
       confidenceField.scientificWritebackAllowed){
        return status(env,-93,"v0.3 confidence reference failed");
    }

    factored::Binding binding{};
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
    binding.confidenceFieldSha256=confidenceField.fieldSha256;

    factored::Result state{};
    if(!factored::derive(binding,confidenceField,state)||
       !state.exactConfidenceFieldBindingVerified||
       !state.vectorValuedNoScalarProbability||
       !state.legacyClassNonAuthoritative||
       !state.cfaPhaseDiagnosticOnly||
       state.supportDistanceAdmitted||
       state.promotionEligible||
       state.candidateApplied||
       state.createsNewEvidence||
       state.scientificWritebackAllowed){
        return status(env,-94,"N2 factored confidence derivation failed");
    }

    factored::Report report{};
    if(!factored::encode(
            binding,
            ctx.width,
            ctx.height,
            state,
            report)||
       report.promotionEligible||
       report.candidateApplied||
       report.createsNewEvidence||
       report.scientificWritebackAllowed){
        return status(env,-95,"N2 factored confidence encode failed");
    }

    if(!write_all(destinationFd,report.json)){
        return status(env,-96,"N2 factored confidence write failed");
    }

    sha::Digest writtenSha{};
    if(!readback_sha(
            destinationFd,
            report.json.size(),
            writtenSha)||
       writtenSha!=report.jsonSha256){
        return status(env,-97,"N2 factored confidence post-write SHA mismatch");
    }

    if(!pipeline::reverify(ctx)){
        return status(env,-98,"source changed during N2 factored confidence export");
    }

    std::ostringstream o;
    o<<"{\"status\":0";
    o<<",\"width\":"<<ctx.width;
    o<<",\"height\":"<<ctx.height;
    o<<",\"fileBytes\":"<<report.json.size();
    o<<",\"tileCount\":"<<report.tileCount;
    o<<",\"hasCandidateTiles\":"<<state.hasCandidateTiles;
    o<<",\"allCandidatesPredictableTiles\":"
      <<state.allCandidatesPredictableTiles;
    o<<",\"centerOutlierFreeTiles\":"
      <<state.centerOutlierFreeTiles;
    o<<",\"predictableAndCenterOutlierFreeTiles\":"
      <<state.predictableAndCenterOutlierFreeTiles;
    o<<",\"pairRejectionFreeTiles\":"
      <<state.pairRejectionFreeTiles;
    o<<",\"scaleRejectionFreeTiles\":"
      <<state.scaleRejectionFreeTiles;
    o<<",\"structureProtectionPresentTiles\":"
      <<state.structureProtectionPresentTiles;
    o<<",\"censorProtectionPresentTiles\":"
      <<state.censorProtectionPresentTiles;
    o<<",\"censorBoundaryProtectionPresentTiles\":"
      <<state.censorBoundaryProtectionPresentTiles;
    o<<",\"maxPredictorVarianceLeCenterVarianceTiles\":"
      <<state.maxPredictorVarianceLeCenterVarianceTiles;
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
      <<sha::hex(confidenceField.fieldSha256)<<"\"";
    o<<",\"factoredStateSha256\":\""
      <<sha::hex(state.stateSha256)<<"\"";
    o<<",\"jsonSha256\":\""
      <<sha::hex(report.jsonSha256)<<"\"";
    o<<",\"exactConfidenceFieldBindingVerified\":true";
    o<<",\"vectorValuedNoScalarProbability\":true";
    o<<",\"legacyClassNonAuthoritative\":true";
    o<<",\"cfaPhaseDiagnosticOnly\":true";
    o<<",\"supportDistanceAdmitted\":false";
    o<<",\"promotionEligible\":false";
    o<<",\"postWriteVerified\":true";
    o<<",\"candidateApplied\":false";
    o<<",\"createsNewEvidence\":false";
    o<<",\"scientificWritebackAllowed\":false}";
    return env->NewStringUTF(o.str().c_str());
}
