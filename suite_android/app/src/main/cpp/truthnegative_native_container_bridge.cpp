#include <jni.h>

#include "truthnegative_native_container_v0_1.h"
#include "truthnegative_pipeline_bridge_common.h"
#include "truthraw_sha256_v0_69.h"

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace {

namespace pipeline =
    truthraw::android_truthnegative_pipeline::v0_1;
namespace container =
    truthraw::truthnegative_native_container::v0_1;
namespace tn = truthraw::truthnegative_continuous::v0_5;
namespace sha = truthraw::sha256_v0_69;

class FdAccess final :
    public container::IRandomAccessSink,
    public container::IRandomAccessSource {
public:
    explicit FdAccess(int fd) : fd_(fd) {}

    bool writeAt(
        std::uint64_t offset,
        const void* data,
        std::size_t size) noexcept override {
        const auto* p = static_cast<const std::uint8_t*>(data);
        std::size_t done = 0u;
        while (done < size) {
            const ssize_t n = ::pwrite(
                fd_,
                p + done,
                size - done,
                static_cast<off_t>(offset + done));
            if (n <= 0) return false;
            done += static_cast<std::size_t>(n);
        }
        return true;
    }

    bool resize(std::uint64_t size) noexcept override {
        return ::ftruncate(fd_, static_cast<off_t>(size)) == 0;
    }

    std::uint64_t sizeBytes() const noexcept override {
        struct stat st {};
        if (::fstat(fd_, &st) != 0 || st.st_size < 0) return 0u;
        return static_cast<std::uint64_t>(st.st_size);
    }

    bool readAt(
        std::uint64_t offset,
        void* data,
        std::size_t size) const noexcept override {
        auto* p = static_cast<std::uint8_t*>(data);
        std::size_t done = 0u;
        while (done < size) {
            const ssize_t n = ::pread(
                fd_,
                p + done,
                size - done,
                static_cast<off_t>(offset + done));
            if (n <= 0) return false;
            done += static_cast<std::size_t>(n);
        }
        return true;
    }

private:
    int fd_;
};

jstring status(
    JNIEnv* env,
    int code,
    const std::string& message) {
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

}  // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_truthraw_adaptiveui_TruthNegativeNativeContainerBridge_exportAndVerify(
    JNIEnv* env,
    jobject,
    jint sourceFd,
    jint destinationFd,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    if (destinationFd < 0) {
        return status(env, -40, "invalid destination fd");
    }

    pipeline::Context ctx{};
    const auto prepared = pipeline::prepare(
        sourceFd,
        static_cast<std::size_t>(
            std::max(0, maxSourceResidentBytes)),
        static_cast<std::size_t>(
            std::max(0, maxLogicalResidentBytes)),
        ctx);
    if (!prepared) {
        return status(env, prepared.code, prepared.message);
    }

    FdAccess file(destinationFd);
    container::WriteInput input{};
    input.state = ctx.truthNegativeState;
    input.colorBindingId = ctx.produced.color.bindingId;

    container::Summary written{};
    if (!container::write(
            input, *ctx.fieldSource, file, written)) {
        return status(env, -41, "native TN container write failed");
    }

    container::Reader reader{};
    if (!reader.open(file)) {
        return status(
            env,
            -42,
            std::string("native TN import verify failed: ") +
                reader.error());
    }

    tn::AuthorityFieldSummary importedField{};
    if (!tn::summarizeAuthorityField(
            reader, importedField)) {
        return status(
            env, -43, "imported authority field summary failed");
    }
    if (importedField.contentSha256 !=
        ctx.authorityField.contentSha256) {
        return status(
            env, -44, "imported authority field identity mismatch");
    }

    auto reboundInput = ctx.truthNegativeState.input;
    reboundInput.authorityFieldSha256 =
        importedField.contentSha256;
    tn::State rebound{};
    if (!tn::finalizeState(reboundInput, rebound) ||
        rebound.stateSha256 !=
            ctx.truthNegativeState.stateSha256) {
        return status(
            env, -45, "imported TruthNegative state identity mismatch");
    }

    if (!pipeline::reverify(ctx)) {
        return status(
            env, -46, "source changed during container export");
    }

    std::ostringstream o;
    o << "{\"status\":0";
    o << ",\"width\":" << written.width;
    o << ",\"height\":" << written.height;
    o << ",\"tileCount\":" << written.tileCount;
    o << ",\"recordCount\":" << written.recordCount;
    o << ",\"bodyBytes\":" << written.bodyBytes;
    o << ",\"fileBytes\":" << written.fileBytes;
    o << ",\"roleSourceMeasuredCfa\":" << written.roleSourceMeasuredCfa;
    o << ",\"roleScientificReconstruction\":" << written.roleScientificReconstruction;
    o << ",\"roleDenseProjection\":" << written.roleDenseProjection;
    o << ",\"authorityCalibratedEstimate\":" << written.authorityCalibratedEstimate;
    o << ",\"authorityReconstructed\":" << written.authorityReconstructed;
    o << ",\"authorityCensored\":" << written.authorityCensored;
    o << ",\"authorityUnknown\":" << written.authorityUnknown;
    o << ",\"uncertaintyKnownCount\":" << written.uncertaintyKnownCount;
    o << ",\"supportKnownCount\":" << written.supportKnownCount;
    o << ",\"boundKnownCount\":" << written.boundKnownCount;
    o << ",\"valueNegativeCount\":" << written.valueNegativeCount;
    o << ",\"valueAboveOneCount\":" << written.valueAboveOneCount;
    o << ",\"valueNonFiniteCount\":" << written.valueNonFiniteCount;
    o << ",\"sourceSha256\":\""
      << sha::hex(written.sourceEvidenceSha256) << "\"";
    o << ",\"scientificMasterSha256\":\""
      << sha::hex(written.scientificMasterSha256) << "\"";
    o << ",\"authorityFieldSha256\":\""
      << sha::hex(written.authorityFieldSha256) << "\"";
    o << ",\"truthNegativeStateSha256\":\""
      << sha::hex(written.truthNegativeStateSha256) << "\"";
    o << ",\"bodySha256\":\""
      << sha::hex(written.bodySha256) << "\"";
    o << ",\"containerSha256\":\""
      << sha::hex(written.containerSha256) << "\"";
    o << ",\"nativeImportVerified\":true";
    o << ",\"authorityRoundtripVerified\":true";
    o << ",\"stateRoundtripVerified\":true";
    o << ",\"physicalFrameCount\":1";
    o << ",\"independentEvidenceCount\":1";
    o << ",\"createsNewEvidence\":false";
    o << ",\"scientificWritebackAllowed\":false}";
    return env->NewStringUTF(o.str().c_str());
}
