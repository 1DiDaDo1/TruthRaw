#include <jni.h>

#include "truthnegative_center_excluded_spatial_audit_v0_2_1.h"
#include "truthnegative_center_excluded_spatial_audit_v0_2_2.h"
#include "truthnegative_n2_cfa_audit_v0_1.h"
#include "truthnegative_n2_confidence_field_v0_3.h"
#include "truthnegative_n2_factored_confidence_state_v0_3_1.h"
#include "truthnegative_n2_row_band_reuse_v0_1.h"
#include "truthnegative_pipeline_bridge_common.h"
#include "truthraw_sha256_v0_69.h"

#include <algorithm>
#include <chrono>
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
namespace ce_sparse =
    truthraw::truthnegative_center_excluded_spatial_audit::v0_2_2;
namespace confidence =
    truthraw::truthnegative_n2_confidence_field::v0_3;
namespace factored =
    truthraw::truthnegative_n2_factored_confidence_state::v0_3_1;
namespace row_band =
    truthraw::truthnegative_n2_row_band_reuse::v0_1;
namespace sha = truthraw::sha256_v0_69;
using SteadyClock = std::chrono::steady_clock;

double elapsed_ms(
    SteadyClock::time_point started,
    SteadyClock::time_point finished) noexcept {
    return std::chrono::duration<double,std::milli>(
        finished-started).count();
}

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

    const auto bridgeStarted=SteadyClock::now();

    std::shared_ptr<pipeline::Context> ctx;
    bool sharedPipelineCacheHit=false;
    pipeline::SharedAcquireTiming sharedAcquireTiming{};
    const auto prepared=pipeline::acquireShared(
        sourceFd,
        static_cast<std::size_t>(std::max(0,maxSourceResidentBytes)),
        static_cast<std::size_t>(std::max(0,maxLogicalResidentBytes)),
        ctx,
        sharedPipelineCacheHit,
        &sharedAcquireTiming);
    if(!prepared){
        return status(env,prepared.code,prepared.message);
    }
    const auto sharedAcquireFinished=SteadyClock::now();

    const auto sharedLockWaitStarted=SteadyClock::now();
    std::lock_guard<std::mutex> sharedContextUse(*ctx->useMutex);
    const auto sharedLockAcquired=SteadyClock::now();

    n2_cfa::Binding v01Binding{};
    v01Binding.sourceEvidenceSha256=ctx->sourceSeal.sha256;
    v01Binding.truthNegativeStateSha256=
        ctx->truthNegativeState.stateSha256;

    n2_cfa::Options options{};
    options.tileEdge=64u;
    options.samplingPeriod=8u;

    row_band::RowBandReuseTileSource n2ReadSource(
        *ctx->openedSource.source);

    const auto v01Started=SteadyClock::now();
    n2_cfa::Result v01{};
    if(!n2_cfa::run(
            n2ReadSource,
            v01Binding,
            options,
            v01)||
       v01.sourceValuesModified||
       v01.truthNegativeModified||
       v01.createsNewEvidence||
       v01.scientificWritebackAllowed||
       n2ReadSource.scientificValuesModified()||
       n2ReadSource.createsNewEvidence()||
       n2ReadSource.scientificWritebackAllowed()){
        return status(env,-91,"v0.1 factored reference audit failed");
    }
    const auto v01Finished=SteadyClock::now();

    const auto v01RowBandFillCount =
        n2ReadSource.bandFillCount();
    const auto v01RowBandServedRequestCount =
        n2ReadSource.bandServedRequestCount();
    const auto v01RowBandCacheHitRequestCount =
        n2ReadSource.bandCacheHitRequestCount();
    const auto v01RowBandFallbackRequestCount =
        n2ReadSource.fallbackRequestCount();

    ce_spatial::Binding ceBinding{};
    ceBinding.sourceEvidenceSha256=ctx->sourceSeal.sha256;
    ceBinding.scientificMasterSha256=
        ctx->scientific.scientificMasterHash;
    ceBinding.authorityFieldSha256=
        ctx->authorityField.contentSha256;
    ceBinding.truthNegativeStateSha256=
        ctx->truthNegativeState.stateSha256;
    ceBinding.v01CandidateSha256=v01.candidateSha256;
    ceBinding.v01AuditSha256=v01.auditSha256;
    ceBinding.v01SpatialSha256=v01.spatialSha256;

    const auto centerExcludedStarted=SteadyClock::now();
    ce_spatial::Result ceAudit{};
    ce_sparse::Diagnostics centerExcludedDiagnostics{};
    bool centerExcludedSparseDiagnosticsAvailable=false;
    bool v01SparseReferenceReuseVerified=false;
    bool v01RerunPerformed=false;
    if(v01.correctedSampleCoordinatesComplete){
        if(!ce_sparse::runSparseReference(
                n2ReadSource,
                ceBinding,
                v01,
                ceAudit,
                &centerExcludedDiagnostics)){
            return status(
                env,
                -92,
                "v0.2.1 sparse factored reference audit failed");
        }
        v01SparseReferenceReuseVerified=true;
        centerExcludedSparseDiagnosticsAvailable=true;
    }else{
        // Bounded optimization metadata is unavailable. Preserve the exact
        // established v0.2.1 scientific route rather than expanding memory.
        if(!ce_spatial::run(
                *ctx->openedSource.source,
                ceBinding,
                v01,
                ceAudit)){
            return status(
                env,
                -92,
                "v0.2.1 legacy factored reference audit failed");
        }
        v01RerunPerformed=true;
    }
    const auto centerExcludedFinished=SteadyClock::now();
    if(!ceAudit.v01TileParityVerified||
       ceAudit.candidateApplied||
       ceAudit.createsNewEvidence||
       ceAudit.scientificWritebackAllowed){
        return status(env,-92,"v0.2.1 factored reference audit failed");
    }

    confidence::Binding confidenceBinding{};
    confidenceBinding.sourceEvidenceSha256=ctx->sourceSeal.sha256;
    confidenceBinding.scientificMasterSha256=
        ctx->scientific.scientificMasterHash;
    confidenceBinding.authorityFieldSha256=
        ctx->authorityField.contentSha256;
    confidenceBinding.truthNegativeStateSha256=
        ctx->truthNegativeState.stateSha256;
    confidenceBinding.v01CandidateSha256=v01.candidateSha256;
    confidenceBinding.v01AuditSha256=v01.auditSha256;
    confidenceBinding.v01SpatialSha256=v01.spatialSha256;
    confidenceBinding.centerExcludedAuditSha256=ceAudit.auditSha256;

    const auto confidenceStarted=SteadyClock::now();
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
    const auto confidenceFinished=SteadyClock::now();

    factored::Binding binding{};
    binding.sourceEvidenceSha256=ctx->sourceSeal.sha256;
    binding.scientificMasterSha256=
        ctx->scientific.scientificMasterHash;
    binding.authorityFieldSha256=
        ctx->authorityField.contentSha256;
    binding.truthNegativeStateSha256=
        ctx->truthNegativeState.stateSha256;
    binding.v01CandidateSha256=v01.candidateSha256;
    binding.v01AuditSha256=v01.auditSha256;
    binding.v01SpatialSha256=v01.spatialSha256;
    binding.centerExcludedAuditSha256=ceAudit.auditSha256;
    binding.confidenceFieldSha256=confidenceField.fieldSha256;

    const auto factoredDeriveStarted=SteadyClock::now();
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
    const auto factoredDeriveFinished=SteadyClock::now();

    const auto factoredEncodeStarted=SteadyClock::now();
    factored::Report report{};
    if(!factored::encode(
            binding,
            ctx->width,
            ctx->height,
            state,
            report)||
       report.promotionEligible||
       report.candidateApplied||
       report.createsNewEvidence||
       report.scientificWritebackAllowed){
        return status(env,-95,"N2 factored confidence encode failed");
    }
    const auto factoredEncodeFinished=SteadyClock::now();

    const auto writeVerifyStarted=SteadyClock::now();
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

    if(!pipeline::reverify(*ctx)){
        return status(env,-98,"source changed during N2 factored confidence export");
    }
    const auto writeVerifyFinished=SteadyClock::now();
    const auto bridgeFinished=SteadyClock::now();

    std::ostringstream o;
    o<<"{\"status\":0";
    o<<",\"width\":"<<ctx->width;
    o<<",\"height\":"<<ctx->height;
    o<<",\"sharedPipelineCacheHit\":"<<(sharedPipelineCacheHit?"true":"false");
    o<<",\"v01SparseReferenceReuseVerified\":"
      <<(v01SparseReferenceReuseVerified?"true":"false");
    o<<",\"v01RerunPerformed\":"
      <<(v01RerunPerformed?"true":"false");
    o<<",\"v01SparseReferenceIndexComplete\":"
      <<(v01.correctedSampleCoordinatesComplete?"true":"false");
    o<<",\"rowBandReuseActive\":true";
    o<<",\"v01RowBandFillCount\":"<<v01RowBandFillCount;
    o<<",\"v01RowBandServedRequestCount\":"
      <<v01RowBandServedRequestCount;
    o<<",\"v01RowBandCacheHitRequestCount\":"
      <<v01RowBandCacheHitRequestCount;
    o<<",\"v01RowBandFallbackRequestCount\":"
      <<v01RowBandFallbackRequestCount;
    o<<",\"rowBandFillCountTotal\":"
      <<n2ReadSource.bandFillCount();
    o<<",\"rowBandServedRequestCountTotal\":"
      <<n2ReadSource.bandServedRequestCount();
    o<<",\"rowBandCacheHitRequestCountTotal\":"
      <<n2ReadSource.bandCacheHitRequestCount();
    o<<",\"rowBandFallbackRequestCountTotal\":"
      <<n2ReadSource.fallbackRequestCount();
    o<<",\"rowBandPeakCacheBytes\":"
      <<n2ReadSource.peakCacheBytes();
    o<<",\"rowBandScientificValuesModified\":false";
    o<<",\"nativePhaseTimingAvailable\":true";
    o<<",\"phaseSharedAcquireMs\":"
      <<elapsed_ms(bridgeStarted,sharedAcquireFinished);
    o<<",\"phaseSharedContextLockWaitMs\":"
      <<elapsed_ms(sharedLockWaitStarted,sharedLockAcquired);
    o<<",\"sharedAcquireSubphaseTimingAvailable\":true";
    o<<",\"sharedAcquireProbeSealMs\":"
      <<sharedAcquireTiming.probeSealMs;
    o<<",\"sharedAcquireCacheLookupMs\":"
      <<sharedAcquireTiming.cacheLookupMs;
    o<<",\"sharedAcquirePrepareTotalMs\":"
      <<sharedAcquireTiming.prepareTotalMs;
    o<<",\"prepareDuplicateAndByteSourceMs\":"
      <<sharedAcquireTiming.preparation.duplicateAndByteSourceMs;
    o<<",\"prepareSealSourceMs\":"
      <<sharedAcquireTiming.preparation.sealSourceMs;
    o<<",\"prepareColorBindingMs\":"
      <<sharedAcquireTiming.preparation.colorBindingMs;
    o<<",\"prepareColorSourceMs\":"
      <<sharedAcquireTiming.preparation.prepareColorSourceMs;
    o<<",\"preparePreOpenReverifyMs\":"
      <<sharedAcquireTiming.preparation.preOpenReverifyMs;
    o<<",\"prepareOpenDngAdapterMs\":"
      <<sharedAcquireTiming.preparation.openDngAdapterMs;
    o<<",\"prepareBindScientificMasterMs\":"
      <<sharedAcquireTiming.preparation.bindScientificMasterMs;
    o<<",\"prepareFinalizePhase2Ms\":"
      <<sharedAcquireTiming.preparation.finalizePhase2Ms;
    o<<",\"prepareSummarizeAuthorityFieldMs\":"
      <<sharedAcquireTiming.preparation.summarizeAuthorityFieldMs;
    o<<",\"authorityFieldFusedIntoScientificMasterPass\":"
      <<(sharedAcquireTiming.preparation.authorityFusedIntoScientificMasterPass
            ?"true":"false");
    o<<",\"authorityFieldReplayPassPerformed\":"
      <<(sharedAcquireTiming.preparation.authorityReplayPassPerformed
            ?"true":"false");
    o<<",\"authorityFieldFusedFinalizeMs\":"
      <<sharedAcquireTiming.preparation.authorityFusedFinalizeMs;
    o<<",\"prepareFinalizeTruthNegativeMs\":"
      <<sharedAcquireTiming.preparation.finalizeTruthNegativeMs;
    o<<",\"prepareFinalizeDrawNegativeMs\":"
      <<sharedAcquireTiming.preparation.finalizeDrawNegativeMs;
    o<<",\"prepareTotalInstrumentedMs\":"
      <<sharedAcquireTiming.preparation.totalMs;
    o<<",\"phaseV01CfaAuditMs\":"
      <<elapsed_ms(v01Started,v01Finished);
    o<<",\"phaseCenterExcludedMs\":"
      <<elapsed_ms(centerExcludedStarted,centerExcludedFinished);
    o<<",\"centerExcludedSubphaseTimingAvailable\":"
      <<(centerExcludedSparseDiagnosticsAvailable?"true":"false");
    o<<",\"centerExcludedFillStage2Ms\":"
      <<centerExcludedDiagnostics.fillStage2Ms;
    o<<",\"centerExcludedCandidateLoopMs\":"
      <<centerExcludedDiagnostics.candidateLoopMs;
    o<<",\"centerExcludedPredictorEstimateMs\":"
      <<centerExcludedDiagnostics.predictorEstimateMs;
    o<<",\"centerExcludedFinalHashMs\":"
      <<centerExcludedDiagnostics.finalHashMs;
    o<<",\"centerExcludedTotalInstrumentedMs\":"
      <<centerExcludedDiagnostics.totalMs;
    o<<",\"centerExcludedTileCount\":"
      <<centerExcludedDiagnostics.tileCount;
    o<<",\"centerExcludedCandidateTileCount\":"
      <<centerExcludedDiagnostics.candidateTileCount;
    o<<",\"centerExcludedCandidateCenterCount\":"
      <<centerExcludedDiagnostics.candidateCenterCount;
    o<<",\"phaseConfidenceDeriveMs\":"
      <<elapsed_ms(confidenceStarted,confidenceFinished);
    o<<",\"phaseFactoredDeriveMs\":"
      <<elapsed_ms(factoredDeriveStarted,factoredDeriveFinished);
    o<<",\"phaseFactoredEncodeMs\":"
      <<elapsed_ms(factoredEncodeStarted,factoredEncodeFinished);
    o<<",\"phaseWriteReadbackReverifyMs\":"
      <<elapsed_ms(writeVerifyStarted,writeVerifyFinished);
    o<<",\"phaseTotalBridgeMs\":"
      <<elapsed_ms(bridgeStarted,bridgeFinished);
    o<<",\"phaseTimingIsScientificEvidence\":false";
    o<<",\"phaseTimingMayChangeScientificAuthority\":false";
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
      <<sha::hex(ctx->sourceSeal.sha256)<<"\"";
    o<<",\"scientificMasterSha256\":\""
      <<sha::hex(ctx->scientific.scientificMasterHash)<<"\"";
    o<<",\"authorityFieldSha256\":\""
      <<sha::hex(ctx->authorityField.contentSha256)<<"\"";
    o<<",\"truthNegativeStateSha256\":\""
      <<sha::hex(ctx->truthNegativeState.stateSha256)<<"\"";
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

extern "C" JNIEXPORT jboolean JNICALL
Java_com_truthraw_adaptiveui_TruthNegativeN2FactoredConfidenceBridge_clearSharedPipelineCache(
    JNIEnv*,
    jobject) {
    pipeline::clearSharedCache();
    return JNI_TRUE;
}

