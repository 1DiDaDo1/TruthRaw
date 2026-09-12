#include <jni.h>

#include "bounded_srgb_preview_sink_v0_1.h"
#include "dng_color_binding_producer_v0_2.h"
#include "finalized_scientific_preview_release_v0_2.h"
#include "scientific_master_linear_dng_projection_v0_1.h"
#include "streaming_scientific_master_tile_source_v0_1.h"
#include "scientific_preview_source_binding_v0_1.h"
#include "scientific_preview_source_binding_v0_2.h"
#include "technical_backplane_v0_1.h"
#include "tile_native_dng_source_v0_1.h"
#include "truthraw/core.h"

#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <memory>

#include <unistd.h>

namespace {

using truthraw::NeutralReferenceAppearance;
using truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction;
using truthraw::dng_color_binding_producer_v0_2::ProducerResult;
using truthraw::finalized_scientific_preview_release::v0_2::PreviewAuthority;
using truthraw::finalized_scientific_preview_release::v0_2::ReleaseResult;
using truthraw::preview_surface_v0_1::BoundedSrgbPreviewSink;
using truthraw::scientific_master_linear_dng_projection::v0_1::ProjectionDescriptor;
using truthraw::scientific_master_linear_dng_projection::v0_1::Result;
using truthraw::scientific_master_linear_dng_projection::v0_1::StreamingScientificMasterTileSource;
using truthraw::scientific_preview_binding_v0_1::SourceSeal;
using truthraw::scientific_preview_binding_v0_2::PreparedScientificPreviewSource;
using truthraw::tile_dng_v0_1::PosixFdByteSource;
using truthraw::tile_dng_v0_1::TileNativeDngSource;

constexpr jlong kPacketMagic = 0x54524644LL; // TRFD
constexpr int kGatePreviewEdge = 64;

enum class ExportCode : std::int32_t {
    Ok = 0,
    InvalidArgument,
    SourceSealFailed,
    ColorBindingFailed,
    PrepareFailed,
    SourceReverificationFailed,
    SourceOpenFailed,
    FinalizedGateFailed,
    DngProjectionFailed,
    ProjectionInvariantFailed,
};

class FdTransactionalSink final
    : public truthraw::scientific_master_linear_dng_projection::v0_1::ITransactionalByteSink {
public:
    explicit FdTransactionalSink(int fd) noexcept : fd_(fd) {}

    std::size_t residentBytesUpperBound() const noexcept override { return 0u; }

    bool begin(std::uint64_t expectedBytes) noexcept override {
        if (fd_ < 0 || expectedBytes == 0u) return false;
        if (::lseek(fd_, 0, SEEK_SET) < 0) return false;
        if (::ftruncate(fd_, 0) != 0) return false;
        active_ = true;
        committed_ = false;
        bytes_ = 0u;
        return true;
    }

    bool write(const std::uint8_t* data, std::size_t size) noexcept override {
        if (!active_ || (data == nullptr && size != 0u)) return false;
        std::size_t done = 0u;
        while (done < size) {
            const ssize_t n = ::write(fd_, data + done, size - done);
            if (n < 0) {
                if (errno == EINTR) continue;
                return false;
            }
            if (n == 0) return false;
            done += static_cast<std::size_t>(n);
            bytes_ += static_cast<std::uint64_t>(n);
        }
        return true;
    }

    bool commit() noexcept override {
        if (!active_) return false;
        if (::fsync(fd_) != 0) return false;
        active_ = false;
        committed_ = true;
        return true;
    }

    void abort() noexcept override {
        if (fd_ >= 0) {
            (void)::ftruncate(fd_, 0);
            (void)::lseek(fd_, 0, SEEK_SET);
        }
        active_ = false;
        committed_ = false;
        bytes_ = 0u;
    }

    bool committed() const noexcept { return committed_; }
    std::uint64_t bytesWritten() const noexcept { return bytes_; }

private:
    int fd_ = -1;
    bool active_ = false;
    bool committed_ = false;
    std::uint64_t bytes_ = 0u;
};

jlongArray packet(JNIEnv* env,
                  ExportCode code,
                  const Result* result = nullptr,
                  int width = 0,
                  int height = 0) {
    std::array<jlong, 12> values{};
    values[0] = kPacketMagic;
    values[1] = static_cast<jlong>(code);
    values[2] = width;
    values[3] = height;
    if (result != nullptr) {
        values[4] = static_cast<jlong>(result->bytesWritten);
        values[5] = static_cast<jlong>(result->projectedPixels);
        values[6] = static_cast<jlong>(result->negativeComponentCount);
        values[7] = static_cast<jlong>(result->overOneComponentCount);
        values[8] = static_cast<jlong>(result->tilesWritten);
        values[9] = result->scientificMasterIdentityVerified ? 1 : 0;
        values[10] = result->artifactCommitted ? 1 : 0;
        values[11] = result->representationOnly ? 1 : 0;
    }
    jlongArray out = env->NewLongArray(static_cast<jsize>(values.size()));
    if (out != nullptr) {
        env->SetLongArrayRegion(out, 0, static_cast<jsize>(values.size()), values.data());
    }
    return out;
}

} // namespace

extern "C" JNIEXPORT jlongArray JNICALL
Java_com_truthraw_adaptiveui_NativeScientificDngBridge_exportFinalizedScientificMasterLinearDng(
    JNIEnv* env,
    jobject,
    jint sourceFd,
    jint tempOutputFd,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    if (sourceFd < 0 || tempOutputFd < 0 || maxSourceResidentBytes <= 0 ||
        maxLogicalResidentBytes <= 0) {
        return packet(env, ExportCode::InvalidArgument);
    }

    auto bytes = std::make_shared<PosixFdByteSource>(static_cast<int>(sourceFd));
    SourceSeal sourceSeal;
    const auto sealed = truthraw::scientific_preview_binding_v0_1::seal_source_sha256(*bytes, sourceSeal);
    if (!sealed) return packet(env, ExportCode::SourceSealFailed);

    ProducerResult produced;
    const auto colorStatus =
        truthraw::dng_color_binding_producer_v0_2::produce_source_metadata_color_binding(
            *bytes, sourceSeal, produced);
    if (!colorStatus) return packet(env, ExportCode::ColorBindingFailed);

    PreparedScientificPreviewSource prepared;
    const auto preparedStatus =
        truthraw::scientific_preview_binding_v0_2::prepare_scientific_color_source(
            sourceSeal, produced.color, prepared);
    if (!preparedStatus || !prepared.mainHouseComputeAllowed ||
        !prepared.sourceBoundAppearanceReleaseAllowed || prepared.scientificPreviewReleaseAllowed ||
        prepared.scientificClaimAllowed || prepared.physicalFrameCount != 1u ||
        prepared.independentEvidenceCount != 1u) {
        return packet(env, ExportCode::PrepareFailed);
    }

    const auto preVerified =
        truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes, sourceSeal);
    if (!preVerified) return packet(env, ExportCode::SourceReverificationFailed);

    auto openOptions = prepared.tileNativeOptions;
    openOptions.maxResidentBytes = static_cast<std::size_t>(maxSourceResidentBytes);
    std::unique_ptr<TileNativeDngSource> gateSource;
    const auto opened = TileNativeDngSource::open(bytes, openOptions, gateSource);
    if (!opened) return packet(env, ExportCode::SourceOpenFailed);

    auto reconstruction = std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto appearance = std::make_shared<NeutralReferenceAppearance>();
    BoundedSrgbPreviewSink gateSink(kGatePreviewEdge);

    truthraw::scientific_master_streaming_binding::v0_2::Options scientificOptions;
    scientificOptions.memoryBudgetBytes = static_cast<std::size_t>(maxLogicalResidentBytes);
    truthraw::streaming_v0_1::StreamingOptions previewOptions;
    previewOptions.tile = {128, 16};
    previewOptions.workers = 1;
    previewOptions.hdrEnabled = true;
    previewOptions.streamScientificDiagnostics = false;
    previewOptions.sdrLutSize = 4096;
    previewOptions.memoryBudgetBytes = static_cast<std::size_t>(maxLogicalResidentBytes);

    std::array<truthraw::technical_backplane::v0_1::RoomStatus,
               truthraw::technical_backplane::v0_1::kRoomCount> roomStatus{};
    roomStatus.fill(truthraw::technical_backplane::v0_1::RoomStatus::ResearchOnly);

    ReleaseResult release;
    const auto released =
        truthraw::finalized_scientific_preview_release::v0_2::create_and_release_finalized_scientific_preview(
            prepared,
            *gateSource,
            reconstruction,
            appearance,
            scientificOptions,
            previewOptions,
            roomStatus,
            truthraw::technical_backplane::v0_1::ClaimStatus::Candidate,
            gateSink,
            release);
    if (!released || release.authority == PreviewAuthority::None || !gateSink.finished() ||
        release.streaming.provenance.physicalFrameCount != 1u ||
        release.streaming.provenance.independentEvidenceCount != 1u ||
        release.streaming.provenance.scientificMasterModifiedByAppearance ||
        release.streaming.provenance.counterfactualObservationCreated) {
        return packet(env, ExportCode::FinalizedGateFailed);
    }

    const auto postGateVerified =
        truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes, sourceSeal);
    if (!postGateVerified) return packet(env, ExportCode::SourceReverificationFailed);

    std::unique_ptr<TileNativeDngSource> exportSource;
    const auto exportOpened = TileNativeDngSource::open(bytes, openOptions, exportSource);
    if (!exportOpened) return packet(env, ExportCode::SourceOpenFailed);

    ProjectionDescriptor descriptor;
    descriptor.width = static_cast<std::uint32_t>(exportSource->metadata().width);
    descriptor.height = static_cast<std::uint32_t>(exportSource->metadata().height);
    descriptor.orientation = static_cast<std::uint16_t>(exportSource->metadata().orientation);
    descriptor.sealedSourceSha256 = sourceSeal.sha256;
    descriptor.scientificMasterSha256 = release.scientificIdentity.scientificMasterHash;
    descriptor.sourceEvidenceId = sourceSeal.sourceEvidenceId;
    descriptor.colorBindingId = produced.color.bindingId;

    StreamingScientificMasterTileSource masterSource(*exportSource, *reconstruction);
    FdTransactionalSink sink(static_cast<int>(tempOutputFd));
    Result projection;
    const auto projected =
        truthraw::scientific_master_linear_dng_projection::v0_1::write_xyz_d50_linear_dng_projection(
            masterSource,
            descriptor,
            produced.color.cameraToXyzD50,
            sink,
            projection);
    if (!projected) {
        sink.abort();
        return packet(env, ExportCode::DngProjectionFailed, &projection,
                      exportSource->metadata().width, exportSource->metadata().height);
    }

    if (!sink.committed() || !projection.scientificMasterIdentityVerified ||
        !projection.artifactCommitted || !projection.representationOnly ||
        projection.scientificMasterModified || projection.appearanceApplied ||
        projection.counterfactualObservationCreated || projection.physicalFrameCount != 1u ||
        projection.independentEvidenceCount != 1u || projection.bytesWritten != sink.bytesWritten()) {
        sink.abort();
        return packet(env, ExportCode::ProjectionInvariantFailed, &projection,
                      exportSource->metadata().width, exportSource->metadata().height);
    }

    const auto postVerified =
        truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes, sourceSeal);
    if (!postVerified) {
        sink.abort();
        return packet(env, ExportCode::SourceReverificationFailed);
    }

    return packet(env, ExportCode::Ok, &projection,
                  exportSource->metadata().width, exportSource->metadata().height);
}
