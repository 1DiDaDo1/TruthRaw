#include <jni.h>

#include "bounded_srgb_preview_sink_v0_1.h"
#include "dng_color_binding_producer_v0_2.h"
#include "finalized_scientific_preview_release_v0_2.h"
#include "scientific_master_linear_dng_projection_v0_1.h"
#include "scientific_preview_source_binding_v0_1.h"
#include "scientific_preview_source_binding_v0_2.h"
#include "streaming_scientific_master_tile_source_v0_1.h"
#include "technical_backplane_v0_1.h"
#include "tile_native_dng_source_v0_1.h"
#include "truthraw/core.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <unistd.h>

namespace {

using truthraw::NeutralReferenceAppearance;
using truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction;
using truthraw::dng_color_binding_producer_v0_2::ProducerResult;
using truthraw::finalized_scientific_preview_release::v0_2::PreviewAuthority;
using truthraw::finalized_scientific_preview_release::v0_2::ReleaseResult;
using truthraw::preview_surface_v0_1::BoundedSrgbPreviewSink;
using truthraw::scientific_preview_binding_v0_1::SourceSeal;
using truthraw::scientific_preview_binding_v0_2::PreparedScientificPreviewSource;
using truthraw::scientific_master_linear_dng_projection::v0_1::ITransactionalByteSink;
using truthraw::scientific_master_linear_dng_projection::v0_1::ProjectionDescriptor;
using truthraw::scientific_master_linear_dng_projection::v0_1::Result;
using truthraw::scientific_master_linear_dng_projection::v0_1::StreamingScientificMasterTileSource;
using truthraw::tile_dng_v0_1::PosixFdByteSource;
using truthraw::tile_dng_v0_1::TileNativeDngSource;

constexpr jlong kMagic = 0x54525046; // TRPF
constexpr std::size_t kPacketLongs = 19u;
constexpr int kGatePreviewEdge = 64;
constexpr int kTileCore = 128;
constexpr int kTileHalo = 16;

jlongArray packet(JNIEnv* env, jlong status) {
    std::array<jlong, kPacketLongs> values{};
    values[0] = kMagic;
    values[1] = status;
    auto out = env->NewLongArray(static_cast<jsize>(values.size()));
    if (out != nullptr) env->SetLongArrayRegion(out, 0, static_cast<jsize>(values.size()), values.data());
    return out;
}

jlong binding_status(const truthraw::scientific_preview_binding_v0_1::BindingStatus& status) {
    return 2000 + static_cast<jlong>(status.code);
}

jlong producer_status(const truthraw::dng_color_binding_producer_v0_2::ProducerStatus& status) {
    return 2100 + static_cast<jlong>(status.code);
}

jlong dng_status(const truthraw::tile_dng_v0_1::DngSourceStatus& status) {
    return 3000 + static_cast<jlong>(status.code);
}

jlong finalized_status(const truthraw::finalized_scientific_preview_release::v0_2::Status& status) {
    return 5000 + static_cast<jlong>(status.code);
}

jlong pure_status(const truthraw::scientific_master_linear_dng_projection::v0_1::Status& status) {
    return 7000 + static_cast<jlong>(status.code);
}

truthraw::streaming_v0_1::StreamingOptions gate_preview_options(std::size_t memoryBudgetBytes) {
    truthraw::streaming_v0_1::StreamingOptions options;
    options.tile = {kTileCore, kTileHalo};
    options.workers = 1;
    options.hdrEnabled = true;
    options.streamScientificDiagnostics = false;
    options.sdrLutSize = 4096;
    options.memoryBudgetBytes = memoryBudgetBytes;
    return options;
}

bool release_matches_source(
    const ReleaseResult& release,
    const SourceSeal& seal,
    const truthraw::streaming_v0_1::IRawTileSource& source) noexcept {
    const auto& phase2 = release.canonicalPhase2;
    return phase2.admission.sourceSeal.sha256 == seal.sha256 &&
           phase2.admission.sourceSeal.byteLength == seal.byteLength &&
           phase2.admission.sourceSeal.sourceEvidenceId == seal.sourceEvidenceId &&
           phase2.backplane.sourceEvidenceHash == seal.sha256 &&
           phase2.backplane.scientificMasterHash == release.scientificIdentity.scientificMasterHash &&
           source.metadata().sourceId == seal.sourceEvidenceId;
}

class PosixPrivateTransactionalSink final : public ITransactionalByteSink {
public:
    explicit PosixPrivateTransactionalSink(int fd) noexcept : fd_(fd) {}

    std::size_t residentBytesUpperBound() const noexcept override { return 0u; }

    bool begin(std::uint64_t expectedBytes) noexcept override {
        if (fd_ < 0 || begun_) return false;
        if (expectedBytes > static_cast<std::uint64_t>(std::numeric_limits<off_t>::max())) return false;
        if (::ftruncate(fd_, 0) != 0) return false;
        if (::lseek(fd_, 0, SEEK_SET) < 0) return false;
        expected_ = expectedBytes;
        written_ = 0u;
        begun_ = true;
        committed_ = false;
        return true;
    }

    bool write(const std::uint8_t* data, std::size_t size) noexcept override {
        if (!begun_ || committed_ || (size > 0u && data == nullptr)) return false;
        std::size_t offset = 0u;
        while (offset < size) {
            const ssize_t n = ::write(fd_, data + offset, size - offset);
            if (n <= 0) return false;
            offset += static_cast<std::size_t>(n);
            written_ += static_cast<std::uint64_t>(n);
            if (written_ > expected_) return false;
        }
        return true;
    }

    bool commit() noexcept override {
        if (!begun_ || committed_ || written_ != expected_) return false;
        if (::fsync(fd_) != 0) return false;
        committed_ = true;
        return true;
    }

    void abort() noexcept override {
        if (fd_ >= 0) {
            (void)::ftruncate(fd_, 0);
            (void)::lseek(fd_, 0, SEEK_SET);
        }
        begun_ = false;
        committed_ = false;
        expected_ = 0u;
        written_ = 0u;
    }

private:
    int fd_ = -1;
    std::uint64_t expected_ = 0u;
    std::uint64_t written_ = 0u;
    bool begun_ = false;
    bool committed_ = false;
};

} // namespace

extern "C" JNIEXPORT jlongArray JNICALL
Java_com_truthraw_adaptiveui_PureDngNativeBridge_exportPureFloat32Dng(
    JNIEnv* env,
    jobject,
    jint sourceFd,
    jint privateTempFd,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    if (sourceFd < 0 || privateTempFd < 0 ||
        maxSourceResidentBytes <= 0 || maxLogicalResidentBytes <= 0) {
        return packet(env, -1);
    }

    auto bytes = std::make_shared<PosixFdByteSource>(static_cast<int>(sourceFd));

    SourceSeal sourceSeal;
    const auto sealed = truthraw::scientific_preview_binding_v0_1::seal_source_sha256(*bytes, sourceSeal);
    if (!sealed) return packet(env, binding_status(sealed));

    ProducerResult produced;
    const auto colorStatus =
        truthraw::dng_color_binding_producer_v0_2::produce_source_metadata_color_binding(
            *bytes, sourceSeal, produced);
    if (!colorStatus) return packet(env, producer_status(colorStatus));

    PreparedScientificPreviewSource prepared;
    const auto preparedStatus =
        truthraw::scientific_preview_binding_v0_2::prepare_scientific_color_source(
            sourceSeal, produced.color, prepared);
    if (!preparedStatus) return packet(env, binding_status(preparedStatus));

    if (!prepared.mainHouseComputeAllowed || !prepared.sourceBoundAppearanceReleaseAllowed ||
        prepared.scientificPreviewReleaseAllowed || prepared.scientificClaimAllowed ||
        prepared.physicalFrameCount != 1u || prepared.independentEvidenceCount != 1u) {
        return packet(env, -2);
    }

    const auto preVerified =
        truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes, sourceSeal);
    if (!preVerified) return packet(env, binding_status(preVerified));

    auto openOptions = prepared.tileNativeOptions;
    openOptions.maxResidentBytes = static_cast<std::size_t>(maxSourceResidentBytes);

    std::unique_ptr<TileNativeDngSource> source;
    const auto opened = TileNativeDngSource::open(bytes, openOptions, source);
    if (!opened) return packet(env, dng_status(opened));

    auto reconstruction = std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto appearance = std::make_shared<NeutralReferenceAppearance>();
    BoundedSrgbPreviewSink gateSink(kGatePreviewEdge);

    truthraw::scientific_master_streaming_binding::v0_2::Options scientificOptions;
    scientificOptions.memoryBudgetBytes = static_cast<std::size_t>(maxLogicalResidentBytes);

    std::array<truthraw::technical_backplane::v0_1::RoomStatus,
               truthraw::technical_backplane::v0_1::kRoomCount> roomStatus{};
    roomStatus.fill(truthraw::technical_backplane::v0_1::RoomStatus::ResearchOnly);

    ReleaseResult release;
    const auto released =
        truthraw::finalized_scientific_preview_release::v0_2::create_and_release_finalized_scientific_preview(
            prepared,
            *source,
            reconstruction,
            appearance,
            scientificOptions,
            gate_preview_options(static_cast<std::size_t>(maxLogicalResidentBytes)),
            roomStatus,
            truthraw::technical_backplane::v0_1::ClaimStatus::Candidate,
            gateSink,
            release);
    if (!released) return packet(env, finalized_status(released));

    if (release.authority == PreviewAuthority::None ||
        release.scientificIdentity.physicalFrameCount != 1u ||
        release.scientificIdentity.independentEvidenceCount != 1u ||
        !release_matches_source(release, sourceSeal, *source)) {
        return packet(env, -3);
    }

    const auto verifiedBeforeProjection =
        truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes, sourceSeal);
    if (!verifiedBeforeProjection) return packet(env, binding_status(verifiedBeforeProjection));

    ProjectionDescriptor descriptor;
    descriptor.width = static_cast<std::uint32_t>(source->metadata().width);
    descriptor.height = static_cast<std::uint32_t>(source->metadata().height);
    descriptor.orientation = static_cast<std::uint16_t>(source->metadata().orientation);
    descriptor.sealedSourceSha256 = sourceSeal.sha256;
    descriptor.scientificMasterSha256 = release.scientificIdentity.scientificMasterHash;
    descriptor.sourceEvidenceId = sourceSeal.sourceEvidenceId;
    descriptor.colorBindingId = produced.color.bindingId;

    StreamingScientificMasterTileSource masterTiles(*source, *reconstruction);
    PosixPrivateTransactionalSink sink(static_cast<int>(privateTempFd));
    Result pure;
    const auto projected =
        truthraw::scientific_master_linear_dng_projection::v0_1::write_xyz_d50_linear_dng_projection(
            masterTiles,
            descriptor,
            produced.color.cameraToXyzD50,
            sink,
            pure);
    if (!projected) return packet(env, pure_status(projected));

    const auto postVerified =
        truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes, sourceSeal);
    if (!postVerified) return packet(env, binding_status(postVerified));

    if (!release_matches_source(release, sourceSeal, *source) ||
        !pure.scientificMasterIdentityVerified || !pure.artifactCommitted ||
        !pure.representationOnly || pure.scientificMasterModified ||
        pure.appearanceApplied || pure.counterfactualObservationCreated ||
        pure.physicalFrameCount != 1u || pure.independentEvidenceCount != 1u) {
        sink.abort();
        return packet(env, -4);
    }

    std::array<jlong, kPacketLongs> values{};
    values[0] = kMagic;
    values[1] = 0;
    values[2] = descriptor.width;
    values[3] = descriptor.height;
    values[4] = static_cast<jlong>(std::min<std::uint64_t>(
        pure.bytesWritten, static_cast<std::uint64_t>(std::numeric_limits<jlong>::max())));
    values[5] = static_cast<jlong>(std::min<std::uint64_t>(
        pure.projectedPixels, static_cast<std::uint64_t>(std::numeric_limits<jlong>::max())));
    values[6] = static_cast<jlong>(std::min<std::uint64_t>(
        pure.negativeComponentCount, static_cast<std::uint64_t>(std::numeric_limits<jlong>::max())));
    values[7] = static_cast<jlong>(std::min<std::uint64_t>(
        pure.overOneComponentCount, static_cast<std::uint64_t>(std::numeric_limits<jlong>::max())));
    values[8] = pure.tilesWritten;
    values[9] = static_cast<jlong>(pure.logicalWorkspacePeakBytes);
    values[10] = static_cast<jlong>(pure.logicalResidentUpperBound);
    values[11] = pure.scientificMasterIdentityVerified ? 1 : 0;
    values[12] = pure.artifactCommitted ? 1 : 0;
    values[13] = pure.representationOnly ? 1 : 0;
    values[14] = pure.scientificMasterModified ? 1 : 0;
    values[15] = pure.appearanceApplied ? 1 : 0;
    values[16] = pure.counterfactualObservationCreated ? 1 : 0;
    values[17] = pure.physicalFrameCount;
    values[18] = pure.independentEvidenceCount;

    auto out = env->NewLongArray(static_cast<jsize>(values.size()));
    if (out != nullptr) env->SetLongArrayRegion(out, 0, static_cast<jsize>(values.size()), values.data());
    return out;
}
