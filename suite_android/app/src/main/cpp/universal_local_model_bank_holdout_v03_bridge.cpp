#include <jni.h>

#include "truthnegative_pipeline_bridge_common.h"
#include "truthraw_sha256_v0_69.h"
#include "universal_local_model_bank_holdout_v0_3.h"

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
namespace bank =
    truthraw::universal_local_model_bank_holdout::v0_3;
namespace sha = truthraw::sha256_v0_69;

jstring status(JNIEnv* env, int code, const std::string& message) {
    std::ostringstream o;
    o << "{\"status\":" << code << ",\"message\":\"";
    for (char c : message) {
        if (c == '"' || c == '\\') o << '\\';
        if (c == '\n' || c == '\r') o << ' ';
        else o << c;
    }
    o << "\"}";
    return env->NewStringUTF(o.str().c_str());
}

bool write_all(int fd, const std::string& data) noexcept {
    if (fd < 0) return false;
    if (::ftruncate(fd, 0) != 0) return false;
    std::size_t done = 0u;
    while (done < data.size()) {
        const ssize_t n = ::pwrite(
            fd,
            data.data() + done,
            data.size() - done,
            static_cast<off_t>(done));
        if (n <= 0) return false;
        done += static_cast<std::size_t>(n);
    }
    return ::fsync(fd) == 0;
}

bool readback_sha(
    int fd,
    std::size_t expectedBytes,
    sha::Digest& digest) noexcept {
    if (fd < 0) return false;
    struct stat st{};
    if (::fstat(fd, &st) != 0 ||
        st.st_size < 0 ||
        static_cast<std::uint64_t>(st.st_size) != expectedBytes) {
        return false;
    }

    sha::Hasher h;
    std::vector<std::uint8_t> buffer(64u * 1024u);
    std::size_t offset = 0u;
    while (offset < expectedBytes) {
        const std::size_t want =
            std::min(buffer.size(), expectedBytes - offset);
        const ssize_t n = ::pread(
            fd,
            buffer.data(),
            want,
            static_cast<off_t>(offset));
        if (n <= 0) return false;
        h.update(buffer.data(), static_cast<std::size_t>(n));
        offset += static_cast<std::size_t>(n);
    }
    digest = h.finalize();
    return true;
}

} // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_truthraw_adaptiveui_UniversalLocalModelBankHoldoutV03Bridge_exportAndVerify(
    JNIEnv* env,
    jobject,
    jint sourceFd,
    jint destinationFd,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    if (destinationFd < 0) {
        return status(env, -120, "invalid destination fd");
    }

    pipeline::Context ctx{};
    const auto prepared = pipeline::prepare(
        sourceFd,
        static_cast<std::size_t>(std::max(0, maxSourceResidentBytes)),
        static_cast<std::size_t>(std::max(0, maxLogicalResidentBytes)),
        ctx);
    if (!prepared) {
        return status(env, prepared.code, prepared.message);
    }

    bank::Binding binding{};
    binding.sourceEvidenceSha256 = ctx.sourceSeal.sha256;
    binding.scientificMasterSha256 =
        ctx.scientific.scientificMasterHash;
    binding.authorityFieldSha256 =
        ctx.authorityField.contentSha256;
    binding.truthNegativeStateSha256 =
        ctx.truthNegativeState.stateSha256;

    bank::Report report{};
    if (!bank::run(
            *ctx.openedSource.source,
            binding,
            report) ||
        report.targetValueUsedByModels ||
        !report.selectorUsesSupportCrossfit ||
        !report.selectionUsesCommonValidationRms ||
        report.crossFamilyComplexityPenaltyApplied ||
        !report.complexityUsedOnlyAsTieBreak ||
        !report.directionalSupportConditioned ||
        report.targetValueUsedBySelector ||
        report.holdoutErrorUsedBySelector ||
        report.postRevealOracleUsedBySelector ||
        report.lensCalibrationUsed ||
        report.cameraModelUsed ||
        report.vendorMappingUsed ||
        report.measuredAnchorsModified ||
        report.unanchoredValuesPromotedToMeasured ||
        report.modelBankAppliedToScientificMaster ||
        report.candidateApplied ||
        report.createsNewEvidence ||
        report.scientificWritebackAllowed) {
        return status(env, -121, "universal model-bank holdout v0.3 audit failed");
    }

    if (!write_all(destinationFd, report.json)) {
        return status(env, -122, "universal model-bank holdout v0.3 write failed");
    }

    sha::Digest writtenSha{};
    if (!readback_sha(
            destinationFd,
            report.json.size(),
            writtenSha) ||
        writtenSha != report.jsonSha256) {
        return status(
            env,
            -123,
            "universal model-bank holdout v0.3 post-write SHA mismatch");
    }

    if (!pipeline::reverify(ctx)) {
        return status(
            env,
            -124,
            "source changed during universal model-bank holdout v0.3 audit");
    }

    std::ostringstream o;
    o << "{\"status\":0";
    o << ",\"width\":" << ctx.width;
    o << ",\"height\":" << ctx.height;
    o << ",\"fileBytes\":" << report.json.size();
    o << ",\"holdouts\":" << report.global.holdouts;
    o << ",\"selectedValid\":" << report.global.selectedValid;
    o << ",\"baselineValid\":" << report.global.baselineValid;
    o << ",\"noModelSelected\":" << report.global.noModelSelected;
    o << ",\"selectedLowerAbsErrorThanBaseline\":"
      << report.global.selectedLowerAbsErrorThanBaseline;
    o << ",\"baselineLowerAbsErrorThanSelected\":"
      << report.global.baselineLowerAbsErrorThanSelected;
    o << ",\"sourceSha256\":\""
      << sha::hex(ctx.sourceSeal.sha256) << "\"";
    o << ",\"scientificMasterSha256\":\""
      << sha::hex(ctx.scientific.scientificMasterHash) << "\"";
    o << ",\"authorityFieldSha256\":\""
      << sha::hex(ctx.authorityField.contentSha256) << "\"";
    o << ",\"truthNegativeStateSha256\":\""
      << sha::hex(ctx.truthNegativeState.stateSha256) << "\"";
    o << ",\"holdoutStreamSha256\":\""
      << sha::hex(report.holdoutStreamSha256) << "\"";
    o << ",\"jsonSha256\":\""
      << sha::hex(report.jsonSha256) << "\"";
    o << ",\"postWriteVerified\":true";
    o << ",\"targetValueUsedByModels\":false";
    o << ",\"selectorUsesSupportCrossfit\":true";
    o << ",\"selectionUsesCommonValidationRms\":true";
    o << ",\"crossFamilyComplexityPenaltyApplied\":false";
    o << ",\"complexityUsedOnlyAsTieBreak\":true";
    o << ",\"directionalSupportConditioned\":true";
    o << ",\"targetValueUsedBySelector\":false";
    o << ",\"holdoutErrorUsedBySelector\":false";
    o << ",\"postRevealOracleUsedBySelector\":false";
    o << ",\"lensCalibrationUsed\":false";
    o << ",\"cameraModelUsed\":false";
    o << ",\"vendorMappingUsed\":false";
    o << ",\"measuredAnchorsModified\":false";
    o << ",\"unanchoredValuesPromotedToMeasured\":false";
    o << ",\"modelBankAppliedToScientificMaster\":false";
    o << ",\"candidateApplied\":false";
    o << ",\"createsNewEvidence\":false";
    o << ",\"scientificWritebackAllowed\":false}";
    return env->NewStringUTF(o.str().c_str());
}
