#include <jni.h>

#include "truthnegative_center_excluded_spatial_audit_v0_2_1.h"
#include "truthnegative_n2_cfa_audit_v0_1.h"
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

double safe_mean(double sum,std::uint64_t count) noexcept {
    return count>0u?sum/static_cast<double>(count):0.0;
}

} // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_truthraw_adaptiveui_TruthNegativeN2CenterExcludedSpatialBridge_exportAndVerify(
    JNIEnv* env,
    jobject,
    jint sourceFd,
    jint destinationFd,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    if(destinationFd<0){
        return status(env,-70,"invalid destination fd");
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

    n2_cfa::Result reference{};
    if(!n2_cfa::run(
            *ctx.openedSource.source,
            v01Binding,
            options,
            reference)||
       reference.sourceValuesModified||
       reference.truthNegativeModified||
       reference.createsNewEvidence||
       reference.scientificWritebackAllowed){
        return status(env,-71,"v0.1 spatial reference audit failed");
    }

    ce_spatial::Binding binding{};
    binding.sourceEvidenceSha256=ctx.sourceSeal.sha256;
    binding.scientificMasterSha256=
        ctx.scientific.scientificMasterHash;
    binding.authorityFieldSha256=
        ctx.authorityField.contentSha256;
    binding.truthNegativeStateSha256=
        ctx.truthNegativeState.stateSha256;
    binding.v01CandidateSha256=reference.candidateSha256;
    binding.v01AuditSha256=reference.auditSha256;
    binding.v01SpatialSha256=reference.spatialSha256;

    ce_spatial::Result audit{};
    if(!ce_spatial::run(
            *ctx.openedSource.source,
            binding,
            reference,
            audit)||
       !audit.v01TileParityVerified||
       !audit.centerOnlySigmaPrimary||
       !audit.combinedSigmaDiagnosticOnly||
       audit.noiseIndependenceAdmitted||
       audit.candidateApplied||
       audit.createsNewEvidence||
       audit.scientificWritebackAllowed){
        return status(env,-72,"center-excluded spatial audit failed");
    }

    ce_spatial::Report report{};
    if(!ce_spatial::encode(
            binding,
            ctx.width,
            ctx.height,
            audit,
            report)||
       report.candidateApplied||
       report.createsNewEvidence||
       report.scientificWritebackAllowed){
        return status(env,-73,"center-excluded sidecar encode failed");
    }

    if(!write_all(destinationFd,report.json)){
        return status(env,-74,"center-excluded sidecar write failed");
    }

    sha::Digest writtenSha{};
    if(!readback_sha(
            destinationFd,
            report.json.size(),
            writtenSha)||
       writtenSha!=report.jsonSha256){
        return status(
            env,-75,
            "center-excluded sidecar post-write SHA mismatch");
    }

    if(!pipeline::reverify(ctx)){
        return status(
            env,-76,
            "source changed during center-excluded spatial export");
    }

    const auto& m=audit.metrics;
    std::ostringstream o;
    o<<"{\"status\":0";
    o<<",\"width\":"<<ctx.width;
    o<<",\"height\":"<<ctx.height;
    o<<",\"fileBytes\":"<<report.json.size();
    o<<",\"tileCount\":"<<report.tileCount;
    o<<",\"sampled\":"<<m.sampled;
    o<<",\"v01CandidateCenters\":"<<m.v01CandidateCenters;
    o<<",\"predictorValid\":"<<m.predictorValid;
    o<<",\"predictorInvalid\":"<<m.predictorInvalid;
    o<<",\"pairsConsidered\":"<<m.symmetricPairsConsidered;
    o<<",\"pairsAccepted\":"<<m.symmetricPairsAccepted;
    o<<",\"pairsRejected\":"<<m.symmetricPairsRejected;
    o<<",\"scalesConsidered\":"<<m.scalesConsidered;
    o<<",\"scalesAccepted\":"<<m.scalesAccepted;
    o<<",\"scalesRejected\":"<<m.scalesRejected;
    o<<",\"centerZLe1\":"<<m.centerResidualWithin1Sigma;
    o<<",\"centerZ1To2\":"<<m.centerResidualBetween1And2Sigma;
    o<<",\"centerZGt2\":"<<m.centerResidualAbove2Sigma;
    o<<",\"combinedZLe1\":"<<m.combinedResidualWithin1Sigma;
    o<<",\"combinedZ1To2\":"<<m.combinedResidualBetween1And2Sigma;
    o<<",\"combinedZGt2\":"<<m.combinedResidualAbove2Sigma;
    o<<",\"meanAbsResidual\":"
      <<safe_mean(m.absResidualSum,m.predictorValid);
    o<<",\"maxAbsResidual\":"<<m.maxAbsResidual;
    o<<",\"meanCenterVariance\":"
      <<safe_mean(m.centerVarianceSum,m.predictorValid);
    o<<",\"meanEstimateVariance\":"
      <<safe_mean(m.estimateVarianceSum,m.predictorValid);
    o<<",\"meanEstimateToCenterVarianceRatio\":"
      <<safe_mean(
            m.estimateToCenterVarianceRatioSum,
            m.predictorValid);
    o<<",\"maxEstimateToCenterVarianceRatio\":"
      <<m.maxEstimateToCenterVarianceRatio;
    o<<",\"maxDirectionalSigma\":"
      <<m.maxDirectionalDisagreementSigma;
    o<<",\"maxCrossScaleSigma\":"
      <<m.maxCrossScaleDisagreementSigma;
    o<<",\"sourceSha256\":\""
      <<sha::hex(ctx.sourceSeal.sha256)<<"\"";
    o<<",\"scientificMasterSha256\":\""
      <<sha::hex(ctx.scientific.scientificMasterHash)<<"\"";
    o<<",\"authorityFieldSha256\":\""
      <<sha::hex(ctx.authorityField.contentSha256)<<"\"";
    o<<",\"truthNegativeStateSha256\":\""
      <<sha::hex(ctx.truthNegativeState.stateSha256)<<"\"";
    o<<",\"v01CandidateSha256\":\""
      <<sha::hex(reference.candidateSha256)<<"\"";
    o<<",\"v01AuditSha256\":\""
      <<sha::hex(reference.auditSha256)<<"\"";
    o<<",\"v01SpatialSha256\":\""
      <<sha::hex(reference.spatialSha256)<<"\"";
    o<<",\"centerExcludedAuditSha256\":\""
      <<sha::hex(audit.auditSha256)<<"\"";
    o<<",\"jsonSha256\":\""
      <<sha::hex(report.jsonSha256)<<"\"";
    o<<",\"v01TileParityVerified\":true";
    o<<",\"centerOnlySigmaPrimary\":true";
    o<<",\"combinedSigmaDiagnosticOnly\":true";
    o<<",\"noiseIndependenceAdmitted\":false";
    o<<",\"postWriteVerified\":true";
    o<<",\"candidateApplied\":false";
    o<<",\"createsNewEvidence\":false";
    o<<",\"scientificWritebackAllowed\":false}";
    return env->NewStringUTF(o.str().c_str());
}
