#include <jni.h>

#include "truthnegative_n2_support_distance_v0_1.h"
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
namespace distance =
    truthraw::truthnegative_n2_support_distance::v0_1;
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

std::uint32_t floor_map(
    std::uint32_t v,
    std::uint32_t sourceExtent,
    std::uint32_t analysisExtent) noexcept {
    return static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(v) * sourceExtent) /
        analysisExtent);
}

std::uint32_t ceil_map(
    std::uint32_t v,
    std::uint32_t sourceExtent,
    std::uint32_t analysisExtent) noexcept {
    const std::uint64_t num =
        static_cast<std::uint64_t>(v) * sourceExtent;
    return static_cast<std::uint32_t>(
        (num + analysisExtent - 1u) / analysisExtent);
}

bool build_queries(
    JNIEnv* env,
    jintArray frontsideTiles,
    jint analysisWidth,
    jint analysisHeight,
    std::uint32_t sourceWidth,
    std::uint32_t sourceHeight,
    std::vector<distance::QueryRegion>& out) {
    out.clear();
    if (frontsideTiles == nullptr ||
        analysisWidth <= 0 ||
        analysisHeight <= 0 ||
        sourceWidth == 0u ||
        sourceHeight == 0u) {
        return false;
    }

    const jsize n = env->GetArrayLength(frontsideTiles);
    if (n <= 0 || (n % 4) != 0 || n / 4 > 512) return false;

    std::vector<jint> values(static_cast<std::size_t>(n));
    env->GetIntArrayRegion(frontsideTiles, 0, n, values.data());
    if (env->ExceptionCheck()) return false;

    const auto aw = static_cast<std::uint32_t>(analysisWidth);
    const auto ah = static_cast<std::uint32_t>(analysisHeight);
    const std::size_t count = values.size() / 4u;
    out.reserve(count);

    for (std::size_t i = 0u; i < count; ++i) {
        const jint fx0 = values[4u * i + 0u];
        const jint fy0 = values[4u * i + 1u];
        const jint fw0 = values[4u * i + 2u];
        const jint fh0 = values[4u * i + 3u];
        if (fx0 < 0 || fy0 < 0 || fw0 <= 0 || fh0 <= 0) return false;

        const auto fx = static_cast<std::uint32_t>(fx0);
        const auto fy = static_cast<std::uint32_t>(fy0);
        const auto fw = static_cast<std::uint32_t>(fw0);
        const auto fh = static_cast<std::uint32_t>(fh0);
        if (fx >= aw || fy >= ah ||
            static_cast<std::uint64_t>(fx) + fw > aw ||
            static_cast<std::uint64_t>(fy) + fh > ah) {
            return false;
        }

        distance::QueryRegion q{};
        q.id = static_cast<std::uint32_t>(i);
        q.frontsideX = fx;
        q.frontsideY = fy;
        q.frontsideWidth = fw;
        q.frontsideHeight = fh;
        q.left = floor_map(fx, sourceWidth, aw);
        q.top = floor_map(fy, sourceHeight, ah);
        q.right = std::min(
            sourceWidth,
            ceil_map(fx + fw, sourceWidth, aw));
        q.bottom = std::min(
            sourceHeight,
            ceil_map(fy + fh, sourceHeight, ah));
        if (q.right <= q.left || q.bottom <= q.top) return false;
        out.push_back(q);
    }
    return !out.empty();
}

} // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_truthraw_adaptiveui_TruthNegativeN2SupportDistanceBridge_exportAndVerify(
    JNIEnv* env,
    jobject,
    jint sourceFd,
    jint destinationFd,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes,
    jint analysisWidth,
    jint analysisHeight,
    jintArray frontsideTiles) {
    if (destinationFd < 0) {
        return status(env, -100, "invalid destination fd");
    }

    std::shared_ptr<pipeline::Context> ctx;
    bool sharedPipelineCacheHit = false;
    const auto prepared = pipeline::acquireShared(
        sourceFd,
        static_cast<std::size_t>(std::max(0, maxSourceResidentBytes)),
        static_cast<std::size_t>(std::max(0, maxLogicalResidentBytes)),
        ctx,
        sharedPipelineCacheHit);
    if (!prepared) {
        return status(env, prepared.code, prepared.message);
    }

    std::lock_guard<std::mutex> sharedContextUse(*ctx->useMutex);

    std::vector<distance::QueryRegion> queries;
    if (!build_queries(
            env,
            frontsideTiles,
            analysisWidth,
            analysisHeight,
            ctx->width,
            ctx->height,
            queries)) {
        return status(env, -101, "invalid frontside candidate query set");
    }

    distance::Binding binding{};
    binding.sourceEvidenceSha256 = ctx->sourceSeal.sha256;
    binding.scientificMasterSha256 =
        ctx->scientific.scientificMasterHash;
    binding.authorityFieldSha256 =
        ctx->authorityField.contentSha256;
    binding.truthNegativeStateSha256 =
        ctx->truthNegativeState.stateSha256;

    distance::Report report{};
    if (!distance::run(
            *ctx->openedSource.source,
            binding,
            queries,
            report) ||
        !report.exactSampleCoordinatesRecorded ||
        report.unsampledPixelsInferred ||
        report.scalarProbabilityCreated ||
        report.canReduceProtection ||
        report.canEnableCorrection ||
        report.candidateApplied ||
        report.createsNewEvidence ||
        report.scientificWritebackAllowed) {
        return status(env, -102, "N2 support-distance audit failed");
    }

    if (!write_all(destinationFd, report.json)) {
        return status(env, -103, "N2 support-distance write failed");
    }

    sha::Digest writtenSha{};
    if (!readback_sha(
            destinationFd,
            report.json.size(),
            writtenSha) ||
        writtenSha != report.jsonSha256) {
        return status(
            env,
            -104,
            "N2 support-distance post-write SHA mismatch");
    }

    if (!pipeline::reverify(*ctx)) {
        return status(
            env,
            -105,
            "source changed during N2 support-distance export");
    }

    std::ostringstream o;
    o << "{\"status\":0";
    o << ",\"width\":" << ctx->width;
    o << ",\"height\":" << ctx->height;
    o << ",\"sharedPipelineCacheHit\":"
      << (sharedPipelineCacheHit ? "true" : "false");
    o << ",\"fileBytes\":" << report.json.size();
    o << ",\"queryCount\":" << report.queries.size();
    o << ",\"sampled\":" << report.sampled;
    o << ",\"structureProtected\":" << report.audit.structureProtected;
    o << ",\"censoredProtected\":" << report.audit.censoredProtected;
    o << ",\"censorBoundaryProtected\":"
      << report.audit.censorBoundaryProtected;
    o << ",\"sourceSha256\":\""
      << sha::hex(ctx->sourceSeal.sha256) << "\"";
    o << ",\"scientificMasterSha256\":\""
      << sha::hex(ctx->scientific.scientificMasterHash) << "\"";
    o << ",\"authorityFieldSha256\":\""
      << sha::hex(ctx->authorityField.contentSha256) << "\"";
    o << ",\"truthNegativeStateSha256\":\""
      << sha::hex(ctx->truthNegativeState.stateSha256) << "\"";
    o << ",\"candidateSha256\":\""
      << sha::hex(report.candidateSha256) << "\"";
    o << ",\"auditSha256\":\""
      << sha::hex(report.auditSha256) << "\"";
    o << ",\"supportPointStreamSha256\":\""
      << sha::hex(report.supportPointStreamSha256) << "\"";
    o << ",\"jsonSha256\":\""
      << sha::hex(report.jsonSha256) << "\"";
    o << ",\"postWriteVerified\":true";
    o << ",\"exactSampleCoordinatesRecorded\":true";
    o << ",\"unsampledPixelsInferred\":false";
    o << ",\"scalarProbabilityCreated\":false";
    o << ",\"canReduceProtection\":false";
    o << ",\"canEnableCorrection\":false";
    o << ",\"candidateApplied\":false";
    o << ",\"createsNewEvidence\":false";
    o << ",\"scientificWritebackAllowed\":false}";
    return env->NewStringUTF(o.str().c_str());
}
