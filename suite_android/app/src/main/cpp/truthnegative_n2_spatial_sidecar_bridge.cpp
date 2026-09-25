#include <jni.h>

#include "truthnegative_n2_cfa_audit_v0_1.h"
#include "truthnegative_n2_spatial_sidecar_v0_1.h"
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
namespace spatial =
    truthraw::truthnegative_n2_spatial_sidecar::v0_1;
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
       static_cast<std::uint64_t>(st.st_size)!=expectedBytes)return false;
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
Java_com_truthraw_adaptiveui_TruthNegativeN2SpatialSidecarBridge_exportAndVerify(
    JNIEnv* env,
    jobject,
    jint sourceFd,
    jint destinationFd,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    if(destinationFd<0){
        return status(env,-60,"invalid destination fd");
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

    n2_cfa::Binding auditBinding{};
    auditBinding.sourceEvidenceSha256=ctx.sourceSeal.sha256;
    auditBinding.truthNegativeStateSha256=
        ctx.truthNegativeState.stateSha256;

    n2_cfa::Options options{};
    options.tileEdge=64u;
    options.samplingPeriod=8u;

    n2_cfa::Result audit{};
    if(!n2_cfa::run(
            *ctx.openedSource.source,
            auditBinding,
            options,
            audit)||
       audit.sourceValuesModified||
       audit.truthNegativeModified||
       audit.createsNewEvidence||
       audit.scientificWritebackAllowed){
        return status(env,-61,"N2 spatial CFA audit failed");
    }

    spatial::Binding binding{};
    binding.sourceEvidenceSha256=ctx.sourceSeal.sha256;
    binding.scientificMasterSha256=
        ctx.scientific.scientificMasterHash;
    binding.authorityFieldSha256=
        ctx.authorityField.contentSha256;
    binding.truthNegativeStateSha256=
        ctx.truthNegativeState.stateSha256;

    spatial::Report report{};
    if(!spatial::encode(
            binding,
            ctx.width,
            ctx.height,
            audit,
            report)||
       report.createsNewEvidence||
       report.scientificWritebackAllowed||
       report.candidateApplied){
        return status(env,-62,"N2 spatial sidecar encode failed");
    }

    if(!write_all(destinationFd,report.json)){
        return status(env,-63,"N2 spatial sidecar write failed");
    }

    sha::Digest writtenSha{};
    if(!readback_sha(
            destinationFd,
            report.json.size(),
            writtenSha)||
       writtenSha!=report.jsonSha256){
        return status(env,-64,"N2 spatial sidecar post-write SHA mismatch");
    }

    if(!pipeline::reverify(ctx)){
        return status(env,-65,"source changed during N2 spatial export");
    }

    std::ostringstream o;
    o<<"{\"status\":0";
    o<<",\"width\":"<<ctx.width;
    o<<",\"height\":"<<ctx.height;
    o<<",\"fileBytes\":"<<report.json.size();
    o<<",\"tileCount\":"<<report.tileCount;
    o<<",\"sampled\":"<<audit.sampled;
    o<<",\"candidateCorrected\":"<<audit.audit.corrected;
    o<<",\"preserved\":"<<audit.audit.preserved;
    o<<",\"structureProtected\":"<<audit.audit.structureProtected;
    o<<",\"censoredProtected\":"<<audit.audit.censoredProtected;
    o<<",\"censorBoundaryProtected\":"<<audit.audit.censorBoundaryProtected;
    o<<",\"maxAbsCorrectionStage2\":"<<audit.audit.maxAbsCorrection;
    const double removedFraction=
        audit.audit.totalResidualEnergy>0.0
            ? audit.audit.removedResidualEnergy/
                audit.audit.totalResidualEnergy
            : 0.0;
    o<<",\"removedResidualEnergyFraction\":"<<removedFraction;
    o<<",\"sourceSha256\":\""
      <<sha::hex(ctx.sourceSeal.sha256)<<"\"";
    o<<",\"scientificMasterSha256\":\""
      <<sha::hex(ctx.scientific.scientificMasterHash)<<"\"";
    o<<",\"authorityFieldSha256\":\""
      <<sha::hex(ctx.authorityField.contentSha256)<<"\"";
    o<<",\"truthNegativeStateSha256\":\""
      <<sha::hex(ctx.truthNegativeState.stateSha256)<<"\"";
    o<<",\"candidateSha256\":\""
      <<sha::hex(audit.candidateSha256)<<"\"";
    o<<",\"auditSha256\":\""
      <<sha::hex(audit.auditSha256)<<"\"";
    o<<",\"spatialSha256\":\""
      <<sha::hex(audit.spatialSha256)<<"\"";
    o<<",\"jsonSha256\":\""
      <<sha::hex(report.jsonSha256)<<"\"";
    o<<",\"postWriteVerified\":true";
    o<<",\"createsNewEvidence\":false";
    o<<",\"scientificWritebackAllowed\":false";
    o<<",\"candidateApplied\":false}";
    return env->NewStringUTF(o.str().c_str());
}
