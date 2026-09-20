#include <jni.h>

#include "dng_color_binding_producer_v0_2.h"
#include "full_frame_streaming_v0_1_internal.h"
#include "raw_source_adapter_bridge_common.h"
#include "open_scene_canonical_v0_70.h"
#include "truthraw_sha256_v0_69.h"
#include "truthraw_ordered_parallel_executor_v0_1.h"
#include "scientific_master_digest_v0_1.h"
#include "scientific_master_streaming_binding_v0_2.h"
#include "scientific_preview_source_binding_v0_1.h"
#include "scientific_preview_source_binding_v0_2.h"
#include "technical_backplane_phase2_v0_1.h"
#include "technical_backplane_v0_1.h"
#include "tile_native_dng_source_v0_1.h"
#include "truthraw/core.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <unistd.h>
#include <vector>

namespace {

using truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction;
using truthraw::TileRect;
using truthraw::dng_color_binding_producer_v0_2::ProducerResult;
using truthraw::scientific_preview_binding_v0_1::ColorClaimScope;
using truthraw::scientific_preview_binding_v0_1::SourceSeal;
using truthraw::scientific_preview_binding_v0_2::PreparedScientificPreviewSource;
using truthraw::tile_dng_v0_1::PosixFdByteSource;

namespace digest = truthraw::scientific_master_digest::v0_1;
namespace adapter = truthraw::multivendor_raw_source_adapter::v0_1;
namespace streaming = truthraw::streaming_v0_1;
namespace canonical_scene = truthraw::open_scene_canonical::v0_70;
namespace sha = truthraw::sha256_v0_69;
namespace ordered = truthraw::ordered_parallel_executor::v0_1;

constexpr jlong kMagic = 0x54525253; // TRRS
constexpr std::size_t kPacketLongs = 25u;
constexpr std::size_t kHeaderBytes = 8192u;
constexpr std::uint32_t kCore = 64u;
constexpr int kRestorationRadius = 2;
constexpr int kMinSupport = 3;

enum class RestorationRole : std::uint8_t {
    PreserveScientificMaster = 0,
    AestheticReintegrationOnly = 1,
    UnresolvedLoss = 2,
};

bool write_all(int fd, const std::uint8_t* data, std::size_t size) noexcept {
    std::size_t offset = 0u;
    while (offset < size) {
        const ssize_t n = ::write(fd, data + offset, size - offset);
        if (n <= 0) return false;
        offset += static_cast<std::size_t>(n);
    }
    return true;
}

void append_u32_le(std::vector<std::uint8_t>& out, std::uint32_t v) {
    out.push_back(static_cast<std::uint8_t>(v & 0xffu));
    out.push_back(static_cast<std::uint8_t>((v >> 8u) & 0xffu));
    out.push_back(static_cast<std::uint8_t>((v >> 16u) & 0xffu));
    out.push_back(static_cast<std::uint8_t>((v >> 24u) & 0xffu));
}

std::string hex_bytes(std::span<const std::uint8_t> bytes) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out(bytes.size() * 2u, '0');
    for (std::size_t i = 0u; i < bytes.size(); ++i) {
        out[2u * i] = kHex[(bytes[i] >> 4u) & 0x0fu];
        out[2u * i + 1u] = kHex[bytes[i] & 0x0fu];
    }
    return out;
}

jlong clamp_jlong(std::uint64_t value) noexcept {
    const auto cap = static_cast<std::uint64_t>(std::numeric_limits<jlong>::max());
    return static_cast<jlong>(std::min(value, cap));
}

jlongArray packet(JNIEnv* env, jlong status) {
    std::array<jlong, kPacketLongs> values{};
    values[0] = kMagic;
    values[1] = status;
    auto out = env->NewLongArray(static_cast<jsize>(values.size()));
    if (out != nullptr) {
        env->SetLongArrayRegion(out, 0, static_cast<jsize>(values.size()), values.data());
    }
    return out;
}

jlong binding_status(const truthraw::scientific_preview_binding_v0_1::BindingStatus& s) {
    return 2000 + static_cast<jlong>(s.code);
}
jlong producer_status(const truthraw::dng_color_binding_producer_v0_2::ProducerStatus& s) {
    return 2100 + static_cast<jlong>(s.code);
}
jlong adapter_status(const adapter::AdapterStatus& s) {
    return 7000 + static_cast<jlong>(s.code);
}
jlong science_status(const truthraw::scientific_master_streaming_binding::v0_2::Status& s) {
    return 8000 + static_cast<jlong>(s.code);
}
jlong phase2_status(const truthraw::technical_backplane_phase2::v0_1::Status& s) {
    return 9000 + static_cast<jlong>(s.code);
}
bool finite_rgb(const float* p) noexcept {
    return std::isfinite(p[0]) && std::isfinite(p[1]) && std::isfinite(p[2]);
}

bool write_header(
    int fd,
    const SourceSeal& sourceSeal,
    const truthraw::scientific_master_streaming_binding::v0_2::Result& scientific,
    const truthraw::technical_backplane_phase2::v0_1::Phase2Result& phase2,
    const digest::Sha256& derivativeHash,
    int width,
    int height,
    truthraw::Orientation orientation,
    std::uint64_t tileCount,
    std::uint64_t preservedPixels,
    std::uint64_t censoredPixels,
    std::uint64_t restoredPixels,
    std::uint64_t unresolvedPixels,
    std::uint64_t changedComponents,
    std::uint64_t payloadBytes,
    std::uint64_t roleBytes,
    std::uint64_t totalBytes,
    const canonical_scene::Summary& openScene,
    const sha::Digest& roleMaskHash) noexcept {
    std::string text;
    text.reserve(3000u);
    text += "magic=TRUTHRAW_FULLRES_RESTORATION_V0_67\n";
    text += "container_version=1\n";
    text += "role=FULL_RESOLUTION_RETREATABLE_RESTORATION_DERIVATIVE\n";
    text += "pixel_domain=CAMERA_NATIVE_SCIENTIFIC_MASTER_RGB\n";
    text += "sample_encoding=IEEE754_BINARY32_LE\n";
    text += "layout=CANONICAL_64X64_CELL_SEQUENCE_PLUS_PIXEL_ROLE_MASK\n";
    text += "width=" + std::to_string(width) + "\n";
    text += "height=" + std::to_string(height) + "\n";
    text += "orientation=" + std::to_string(static_cast<int>(orientation)) + "\n";
    text += "header_bytes=" + std::to_string(kHeaderBytes) + "\n";
    text += "tile_count=" + std::to_string(tileCount) + "\n";
    text += "source_sha256=" + hex_bytes(sourceSeal.sha256) + "\n";
    text += "scientific_master_sha256=" + digest::to_hex(scientific.scientificMasterHash) + "\n";
    text += "restoration_derivative_rgb_sha256=" + digest::to_hex(derivativeHash) + "\n";
    text += "zero_line_sha256=" + hex_bytes(phase2.zeroLineHash) + "\n";
    text += "scene_scale_sha256=" + hex_bytes(phase2.sceneScaleHash) + "\n";
    text += "technical_backplane_serialized_hex=" + hex_bytes(phase2.serializedBackplane) + "\n";
    text += "binding_extension=TRUTHRAW_TRR_CANONICAL_OPEN_SCENE_ROLEMASK_V0_71\n";
    text += "open_scene_canonical_schema=" + std::string(canonical_scene::schema_name()) + "\n";
    text += "open_scene_semantic_parent_region=" + std::string(canonical_scene::semantic_parent_region()) + "\n";
    text += "open_scene_semantic_parent_stream=" + std::string(canonical_scene::semantic_parent_stream()) + "\n";
    text += "dynamic_authority_artifact_sha256=" + sha::hex(openScene.dynamicAuthoritySha256) + "\n";
    text += "open_scene_state_sha256=" + sha::hex(openScene.contentSha256) + "\n";
    text += "open_scene_policy_sha256=" + sha::hex(openScene.policySha256) + "\n";
    text += "open_scene_artifact_sha256=" + sha::hex(openScene.artifactSha256) + "\n";
    text += "restoration_role_mask_sha256=" + sha::hex(roleMaskHash) + "\n";
    text += "open_scene_counterfactual_pixels=" + std::to_string(openScene.counterfactualPixelCount) + "\n";
    text += "open_scene_scientific_writeback_pixels=" + std::to_string(openScene.scientificWritebackPixelCount) + "\n";
    text += "open_scene_creates_new_evidence=0\n";
    text += "restoration_algorithm=WEIGHTED_NEIGHBOUR_REINTEGRATION_RADIUS_2_MIN_SUPPORT_3\n";
    text += "restoration_radius=2\n";
    text += "restoration_min_support=3\n";
    text += "condition_trigger=SOURCE_CFA_SAMPLE_AT_OR_ABOVE_WHITELEVEL\n";
    text += "support_excludes_censored_source_sites=1\n";
    text += "role_0=PRESERVE_SCIENTIFIC_MASTER\n";
    text += "role_1=AESTHETIC_REINTEGRATION_ONLY\n";
    text += "role_2=UNRESOLVED_LOSS\n";
    text += "preserved_pixels=" + std::to_string(preservedPixels) + "\n";
    text += "censored_source_pixels=" + std::to_string(censoredPixels) + "\n";
    text += "restored_pixels=" + std::to_string(restoredPixels) + "\n";
    text += "unresolved_pixels=" + std::to_string(unresolvedPixels) + "\n";
    text += "changed_components=" + std::to_string(changedComponents) + "\n";
    text += "payload_bytes=" + std::to_string(payloadBytes) + "\n";
    text += "restoration_role_bytes=" + std::to_string(roleBytes) + "\n";
    text += "total_bytes=" + std::to_string(totalBytes) + "\n";
    text += "full_resolution=1\n";
    text += "retreatable=1\n";
    text += "provenance_bound=1\n";
    text += "scientific_master_replay_verified=1\n";
    text += "scientific_master_modified=0\n";
    text += "scientific_writeback_allowed=0\n";
    text += "creates_new_evidence=0\n";
    text += "creates_second_scientific_world=0\n";
    text += "physical_frame_count=1\n";
    text += "independent_evidence_count=1\n";
    text += "END_HEADER\n";

    if (text.size() > kHeaderBytes) return false;
    std::array<std::uint8_t, kHeaderBytes> header{};
    std::copy(text.begin(), text.end(), header.begin());
    if (::lseek(fd, 0, SEEK_SET) < 0) return false;
    return write_all(fd, header.data(), header.size());
}

} // namespace

extern "C" JNIEXPORT jlongArray JNICALL
Java_com_truthraw_adaptiveui_FullResRestorationNativeBridge_exportFullResRestoration(
    JNIEnv* env,
    jobject,
    jint sourceFd,
    jint outputFd,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes,
    jint requestedWorkers) {
    if (sourceFd < 0 || outputFd < 0 ||
        maxSourceResidentBytes <= 0 || maxLogicalResidentBytes <= 0 ||
        requestedWorkers <= 0 || requestedWorkers > 8) {
        return packet(env, -1);
    }
    if (::ftruncate(outputFd, 0) != 0 || ::lseek(outputFd, 0, SEEK_SET) < 0) {
        return packet(env, -2);
    }

    auto bytes = std::make_shared<PosixFdByteSource>(static_cast<int>(sourceFd));
    SourceSeal sourceSeal;
    const auto sealed =
        truthraw::scientific_preview_binding_v0_1::seal_source_sha256(*bytes, sourceSeal);
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
    if (!prepared.mainHouseComputeAllowed ||
        prepared.physicalFrameCount != 1u ||
        prepared.independentEvidenceCount != 1u) {
        return packet(env, -3);
    }

    const auto preVerified =
        truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes, sourceSeal);
    if (!preVerified) return packet(env, binding_status(preVerified));

    auto openOptions = prepared.tileNativeOptions;
    openOptions.maxResidentBytes = static_cast<std::size_t>(maxSourceResidentBytes);
    truthraw::android_raw_adapter_bridge::v0_1::OpenedDngSource openedSource;
    const auto opened = truthraw::android_raw_adapter_bridge::v0_1::openDngViaAdapter(
        bytes, sourceSeal, openOptions, openedSource);
    if (!opened) return packet(env, adapter_status(opened));
    auto& source = openedSource.source;

    auto reconstruction = std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();

    truthraw::scientific_master_streaming_binding::v0_2::Options scientificOptions;
    scientificOptions.memoryBudgetBytes = static_cast<std::size_t>(maxLogicalResidentBytes);
    truthraw::scientific_master_streaming_binding::v0_2::Result scientific;
    const auto scientificStatus =
        truthraw::scientific_master_streaming_binding::v0_2::bind_scientific_master_streaming(
            *source, *reconstruction, scientificOptions, scientific);
    if (!scientificStatus) return packet(env, science_status(scientificStatus));

    truthraw::technical_backplane_phase2::v0_1::Phase2Input phaseInput;
    phaseInput.prepared = prepared;
    phaseInput.scientificMasterHash = scientific.scientificMasterHash;
    phaseInput.zeroLineGauge = scientific.zeroLineGauge;
    phaseInput.sceneBinding = scientific.sceneBinding;
    phaseInput.roomStatus.fill(
        truthraw::technical_backplane::v0_1::RoomStatus::ResearchOnly);
    phaseInput.claimStatus = truthraw::technical_backplane::v0_1::ClaimStatus::Candidate;

    truthraw::technical_backplane_phase2::v0_1::Phase2Result phase2;
    const auto phaseStatus =
        truthraw::technical_backplane_phase2::v0_1::finalize_phase2(phaseInput, phase2);
    if (!phaseStatus) return packet(env, phase2_status(phaseStatus));
    if (phase2.admission.claimScope == ColorClaimScope::None ||
        phase2.backplane.sourceEvidenceHash != sourceSeal.sha256 ||
        phase2.backplane.scientificMasterHash != scientific.scientificMasterHash ||
        phase2.backplane.forbiddenFlags != 0u ||
        scientific.physicalFrameCount != 1u ||
        scientific.independentEvidenceCount != 1u) {
        return packet(env, -4);
    }

    const int width = source->metadata().width;
    const int height = source->metadata().height;
    if (width <= 0 || height <= 0) return packet(env, -5);

    canonical_scene::Binding openSceneBinding{};
    openSceneBinding.sourceEvidenceSha256 = sourceSeal.sha256;
    openSceneBinding.scientificMasterSha256 = scientific.scientificMasterHash;
    openSceneBinding.width = static_cast<std::uint32_t>(width);
    openSceneBinding.height = static_cast<std::uint32_t>(height);
    openSceneBinding.physicalFrameCount = scientific.physicalFrameCount;
    openSceneBinding.independentEvidenceCount = scientific.independentEvidenceCount;
    openSceneBinding.colourBindingId = produced.color.bindingId;
    canonical_scene::Summary openSceneSummary{};
    if (!canonical_scene::build_from_source(*source, openSceneBinding, openSceneSummary) ||
        openSceneSummary.counterfactualPixelCount != 0u ||
        openSceneSummary.scientificWritebackPixelCount != 0u ||
        openSceneSummary.createsNewEvidence ||
        openSceneSummary.chunkingChangesScientificIdentity) {
        return packet(env, -18);
    }

    std::array<std::uint8_t, kHeaderBytes> blank{};
    if (!write_all(outputFd, blank.data(), blank.size())) {
        (void)::ftruncate(outputFd, 0);
        return packet(env, -6);
    }

    digest::ScientificMasterDigestAccumulator baseDigest(
        static_cast<std::uint32_t>(width),
        static_cast<std::uint32_t>(height));
    digest::ScientificMasterDigestAccumulator derivativeDigest(
        static_cast<std::uint32_t>(width),
        static_cast<std::uint32_t>(height));
    if (!baseDigest.valid() || !derivativeDigest.valid()) {
        (void)::ftruncate(outputFd, 0);
        return packet(env, -7);
    }

    sha::Hasher roleMaskHasher;

    std::uint64_t tileCount = 0u;
    std::uint64_t preservedPixels = 0u;
    std::uint64_t censoredPixels = 0u;
    std::uint64_t restoredPixels = 0u;
    std::uint64_t unresolvedPixels = 0u;
    std::uint64_t changedComponents = 0u;
    std::uint64_t payloadBytes = 0u;
    std::uint64_t roleBytes = 0u;
    std::uint64_t bytesWritten = kHeaderBytes;

    const int reconstructionHalo = reconstruction->requiredHalo();
    if (reconstructionHalo < 0) {
        (void)::ftruncate(outputFd, 0);
        return packet(env, -8);
    }

    struct RestorationWorkerContext final {
        truthraw::android_raw_adapter_bridge::v0_1::OpenedDngSource opened;
        std::shared_ptr<ResearchEdgeAwareMeasuredPreservingReconstruction> reconstruction;
        streaming::detail::Workspace workspace;
        std::vector<std::uint16_t> raw;
        std::vector<float> gain;
    };

    struct RestorationTileResult final {
        int x = 0;
        int y = 0;
        int coreW = 0;
        int coreH = 0;
        std::vector<float> baseCore;
        std::vector<float> restoredCore;
        std::vector<std::uint8_t> roles;
        std::vector<std::uint8_t> record;
        std::uint64_t preservedPixels = 0u;
        std::uint64_t censoredPixels = 0u;
        std::uint64_t restoredPixels = 0u;
        std::uint64_t unresolvedPixels = 0u;
        std::uint64_t changedComponents = 0u;
    };

    const auto coreTiles = truthraw::make_tiles(
        width,
        height,
        truthraw::TilePolicy{static_cast<int>(kCore), 0});
    if (coreTiles.empty()) {
        (void)::ftruncate(outputFd, 0);
        return packet(env, -25);
    }

    constexpr std::size_t kPerWorkerEnvelopeBytes = 2u * 1024u * 1024u;
    const int memoryWorkerCap = std::max(
        1,
        std::min(
            8,
            static_cast<int>(
                static_cast<std::size_t>(maxLogicalResidentBytes) /
                kPerWorkerEnvelopeBytes)));
    const int workerCount = std::max(
        1,
        std::min(
            {static_cast<int>(requestedWorkers),
             memoryWorkerCap,
             static_cast<int>(coreTiles.size())}));

    std::vector<RestorationWorkerContext> workerContexts(
        static_cast<std::size_t>(workerCount));
    for (int worker = 0; worker < workerCount; ++worker) {
        auto& ctx = workerContexts[static_cast<std::size_t>(worker)];
        const auto workerOpened =
            truthraw::android_raw_adapter_bridge::v0_1::openDngViaAdapter(
                bytes,
                sourceSeal,
                openOptions,
                ctx.opened);
        if (!workerOpened || !ctx.opened.source) {
            (void)::ftruncate(outputFd, 0);
            return packet(env, adapter_status(workerOpened));
        }
        const auto& wm = ctx.opened.source->metadata();
        const auto& rm = source->metadata();
        if (wm.width != rm.width ||
            wm.height != rm.height ||
            wm.cfa != rm.cfa ||
            wm.orientation != rm.orientation ||
            wm.whiteLevel != rm.whiteLevel ||
            wm.hasGainField != rm.hasGainField ||
            wm.hasResidualBlack != rm.hasResidualBlack) {
            (void)::ftruncate(outputFd, 0);
            return packet(env, -26);
        }
        ctx.reconstruction =
            std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
    }

    const auto parallelStatus = ordered::run<RestorationTileResult>(
        coreTiles.size(),
        workerCount,
        static_cast<std::size_t>(workerCount) * 2u,
        [&](std::size_t tileIndex,
            std::size_t workerIndex,
            RestorationTileResult& result) -> ordered::Status {
            auto& ctx = workerContexts[workerIndex];
            auto& workerSource = ctx.opened.source;
            const auto& core = coreTiles[tileIndex];

            result.x = core.x0;
            result.y = core.y0;
            result.coreW = core.x1 - core.x0;
            result.coreH = core.y1 - core.y0;

            const int ex0 = std::max(0, result.x - kRestorationRadius);
            const int ey0 = std::max(0, result.y - kRestorationRadius);
            const int ex1 = std::min(
                width,
                result.x + result.coreW + kRestorationRadius);
            const int ey1 = std::min(
                height,
                result.y + result.coreH + kRestorationRadius);
            const int expW = ex1 - ex0;
            const int expH = ey1 - ey0;
            const std::size_t expPixels =
                static_cast<std::size_t>(expW) *
                static_cast<std::size_t>(expH);

            TileRect reconTile{};
            reconTile.x0 = ex0;
            reconTile.y0 = ey0;
            reconTile.x1 = ex1;
            reconTile.y1 = ey1;
            reconTile.hx0 = std::max(0, ex0 - reconstructionHalo);
            reconTile.hy0 = std::max(0, ey0 - reconstructionHalo);
            reconTile.hx1 = std::min(width, ex1 + reconstructionHalo);
            reconTile.hy1 = std::min(height, ey1 + reconstructionHalo);

            const auto fill =
                streaming::detail::fill_stage2(
                    *workerSource,
                    reconTile,
                    ctx.workspace);
            if (!fill) {
                return ordered::Status::error(
                    std::string("Restoration fill_stage2 failed: ") +
                    fill.message);
            }

            ctx.workspace.cam.resize(expPixels * 3u);
            const auto reconStatus = ctx.reconstruction->reconstructTile(
                ctx.workspace.stage2.data(),
                reconTile.hx1 - reconTile.hx0,
                reconTile.hy1 - reconTile.hy0,
                reconTile.hx0,
                reconTile.hy0,
                reconTile.x0,
                reconTile.y0,
                expW,
                expH,
                workerSource->metadata().cfa,
                ctx.workspace.cam.data());
            if (!reconStatus) {
                return ordered::Status::error(
                    std::string("Restoration reconstruction failed: ") +
                    reconStatus.message);
            }

            ctx.raw.resize(expPixels);
            if (workerSource->metadata().hasGainField) {
                ctx.gain.resize(expPixels);
            } else {
                ctx.gain.clear();
            }
            TileRect rawRect{ex0, ey0, ex1, ey1, ex0, ey0, ex1, ey1};
            const auto rawStatus = workerSource->readRawTile(
                rawRect,
                ctx.raw.data(),
                ctx.raw.size(),
                workerSource->metadata().hasGainField
                    ? ctx.gain.data()
                    : nullptr,
                workerSource->metadata().hasGainField
                    ? ctx.gain.size()
                    : 0u);
            if (!rawStatus) {
                return ordered::Status::error(
                    std::string("Restoration raw tile read failed: ") +
                    rawStatus.message);
            }

            const std::size_t corePixels =
                static_cast<std::size_t>(result.coreW) *
                static_cast<std::size_t>(result.coreH);
            result.baseCore.resize(corePixels * 3u);
            result.restoredCore.resize(corePixels * 3u);
            result.roles.assign(
                corePixels,
                static_cast<std::uint8_t>(
                    RestorationRole::PreserveScientificMaster));

            for (int cy = 0; cy < result.coreH; ++cy) {
                for (int cx = 0; cx < result.coreW; ++cx) {
                    const int gx = result.x + cx;
                    const int gy = result.y + cy;
                    const int lx = gx - ex0;
                    const int ly = gy - ey0;
                    const std::size_t epi =
                        static_cast<std::size_t>(ly) *
                            static_cast<std::size_t>(expW) +
                        static_cast<std::size_t>(lx);
                    const std::size_t cpi =
                        static_cast<std::size_t>(cy) *
                            static_cast<std::size_t>(result.coreW) +
                        static_cast<std::size_t>(cx);
                    const float* base =
                        ctx.workspace.cam.data() + 3u * epi;
                    if (!finite_rgb(base)) {
                        return ordered::Status::error(
                            "Restoration reconstructed non-finite RGB");
                    }
                    for (std::size_t channel = 0u; channel < 3u; ++channel) {
                        result.baseCore[3u * cpi + channel] = base[channel];
                        result.restoredCore[3u * cpi + channel] = base[channel];
                    }

                    const bool censored =
                        static_cast<float>(ctx.raw[epi]) >=
                        workerSource->metadata().whiteLevel;
                    if (!censored) {
                        ++result.preservedPixels;
                        continue;
                    }

                    ++result.censoredPixels;
                    double sumR = 0.0;
                    double sumG = 0.0;
                    double sumB = 0.0;
                    double sumW = 0.0;
                    int support = 0;
                    for (int radius = 1;
                         radius <= kRestorationRadius && support < kMinSupport;
                         ++radius) {
                        for (int dy = -radius; dy <= radius; ++dy) {
                            for (int dx = -radius; dx <= radius; ++dx) {
                                if (dx == 0 && dy == 0) continue;
                                if (std::max(std::abs(dx), std::abs(dy)) != radius) {
                                    continue;
                                }
                                const int nx = lx + dx;
                                const int ny = ly + dy;
                                if (nx < 0 || ny < 0 || nx >= expW || ny >= expH) {
                                    continue;
                                }
                                const std::size_t npi =
                                    static_cast<std::size_t>(ny) *
                                        static_cast<std::size_t>(expW) +
                                    static_cast<std::size_t>(nx);
                                if (static_cast<float>(ctx.raw[npi]) >=
                                    workerSource->metadata().whiteLevel) {
                                    continue;
                                }
                                const float* neighbour =
                                    ctx.workspace.cam.data() + 3u * npi;
                                if (!finite_rgb(neighbour)) continue;
                                const double weight =
                                    1.0 /
                                    std::sqrt(
                                        static_cast<double>(dx * dx + dy * dy));
                                sumR +=
                                    weight * static_cast<double>(neighbour[0]);
                                sumG +=
                                    weight * static_cast<double>(neighbour[1]);
                                sumB +=
                                    weight * static_cast<double>(neighbour[2]);
                                sumW += weight;
                                ++support;
                            }
                        }
                    }

                    if (support >= kMinSupport && sumW > 0.0) {
                        result.restoredCore[3u * cpi] =
                            static_cast<float>(sumR / sumW);
                        result.restoredCore[3u * cpi + 1u] =
                            static_cast<float>(sumG / sumW);
                        result.restoredCore[3u * cpi + 2u] =
                            static_cast<float>(sumB / sumW);
                        result.roles[cpi] =
                            static_cast<std::uint8_t>(
                                RestorationRole::AestheticReintegrationOnly);
                        ++result.restoredPixels;
                        for (std::size_t channel = 0u;
                             channel < 3u;
                             ++channel) {
                            if (std::bit_cast<std::uint32_t>(
                                    result.restoredCore[3u * cpi + channel]) !=
                                std::bit_cast<std::uint32_t>(
                                    result.baseCore[3u * cpi + channel])) {
                                ++result.changedComponents;
                            }
                        }
                    } else {
                        result.roles[cpi] =
                            static_cast<std::uint8_t>(
                                RestorationRole::UnresolvedLoss);
                        ++result.unresolvedPixels;
                    }
                }
            }

            result.record.clear();
            result.record.reserve(
                16u +
                result.restoredCore.size() * 4u +
                result.roles.size());
            append_u32_le(
                result.record,
                static_cast<std::uint32_t>(result.x));
            append_u32_le(
                result.record,
                static_cast<std::uint32_t>(result.y));
            append_u32_le(
                result.record,
                static_cast<std::uint32_t>(result.coreW));
            append_u32_le(
                result.record,
                static_cast<std::uint32_t>(result.coreH));
            for (float value : result.restoredCore) {
                if (!std::isfinite(value)) {
                    return ordered::Status::error(
                        "Restoration derivative contains non-finite RGB");
                }
                append_u32_le(
                    result.record,
                    std::bit_cast<std::uint32_t>(value));
            }
            result.record.insert(
                result.record.end(),
                result.roles.begin(),
                result.roles.end());
            return ordered::Status::success();
        },
        [&](std::size_t tileIndex,
            const RestorationTileResult& result) -> ordered::Status {
            const auto& expected = coreTiles[tileIndex];
            if (result.x != expected.x0 ||
                result.y != expected.y0 ||
                result.coreW != expected.x1 - expected.x0 ||
                result.coreH != expected.y1 - expected.y0) {
                return ordered::Status::error(
                    "Restoration canonical tile ordering mismatch");
            }

            digest::TileView baseView{};
            baseView.x = static_cast<std::uint32_t>(result.x);
            baseView.y = static_cast<std::uint32_t>(result.y);
            baseView.width = static_cast<std::uint32_t>(result.coreW);
            baseView.height = static_cast<std::uint32_t>(result.coreH);
            baseView.rgb = result.baseCore.data();
            baseView.rowStrideSamples =
                static_cast<std::size_t>(result.coreW) * 3u;
            if (!baseDigest.add_tile(baseView)) {
                return ordered::Status::error(
                    "Restoration Scientific Master digest commit failed");
            }

            digest::TileView derivativeView = baseView;
            derivativeView.rgb = result.restoredCore.data();
            if (!derivativeDigest.add_tile(derivativeView)) {
                return ordered::Status::error(
                    "Restoration derivative digest commit failed");
            }

            roleMaskHasher.update(
                result.roles.data(),
                result.roles.size());
            if (!write_all(
                    outputFd,
                    result.record.data(),
                    result.record.size())) {
                return ordered::Status::error(
                    "Restoration canonical record write failed");
            }

            preservedPixels += result.preservedPixels;
            censoredPixels += result.censoredPixels;
            restoredPixels += result.restoredPixels;
            unresolvedPixels += result.unresolvedPixels;
            changedComponents += result.changedComponents;
            payloadBytes +=
                static_cast<std::uint64_t>(
                    result.restoredCore.size()) * 4u;
            roleBytes +=
                static_cast<std::uint64_t>(result.roles.size());
            bytesWritten +=
                static_cast<std::uint64_t>(result.record.size());
            ++tileCount;
            return ordered::Status::success();
        });

    if (!parallelStatus) {
        (void)::ftruncate(outputFd, 0);
        return packet(env, -27);
    }

    digest::Sha256 baseHash{};
    digest::Sha256 derivativeHash{};
    if (!baseDigest.finalize(baseHash) ||
        !derivativeDigest.finalize(derivativeHash) ||
        baseHash != scientific.scientificMasterHash) {
        (void)::ftruncate(outputFd, 0);
        return packet(env, -14);
    }

    const std::uint64_t totalPixels =
        static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height);
    if (preservedPixels + censoredPixels != totalPixels ||
        restoredPixels + unresolvedPixels != censoredPixels ||
        roleBytes != totalPixels) {
        (void)::ftruncate(outputFd, 0);
        return packet(env, -15);
    }

    const auto roleMaskHash = roleMaskHasher.finalize();

    if (!write_header(
            outputFd,
            sourceSeal,
            scientific,
            phase2,
            derivativeHash,
            width,
            height,
            source->metadata().orientation,
            tileCount,
            preservedPixels,
            censoredPixels,
            restoredPixels,
            unresolvedPixels,
            changedComponents,
            payloadBytes,
            roleBytes,
            bytesWritten,
            openSceneSummary,
            roleMaskHash)) {
        (void)::ftruncate(outputFd, 0);
        return packet(env, -16);
    }

    if (::ftruncate(outputFd, static_cast<off_t>(bytesWritten)) != 0 ||
        ::fsync(outputFd) != 0) {
        (void)::ftruncate(outputFd, 0);
        return packet(env, -17);
    }

    const auto postVerified =
        truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(
            *bytes, sourceSeal);
    if (!postVerified) {
        (void)::ftruncate(outputFd, 0);
        return packet(env, binding_status(postVerified));
    }

    std::array<jlong, kPacketLongs> values{};
    values[0] = kMagic;
    values[1] = 0;
    values[2] = width;
    values[3] = height;
    values[4] = clamp_jlong(bytesWritten);
    values[5] = clamp_jlong(tileCount);
    values[6] = clamp_jlong(preservedPixels);
    values[7] = clamp_jlong(censoredPixels);
    values[8] = clamp_jlong(restoredPixels);
    values[9] = clamp_jlong(unresolvedPixels);
    values[10] = clamp_jlong(changedComponents);
    values[11] = clamp_jlong(payloadBytes);
    values[12] = clamp_jlong(roleBytes);
    values[13] = 1; // full resolution
    values[14] = 1; // retreatable
    values[15] = 1; // provenance bound
    values[16] = 1; // master replay verified
    values[17] = 0; // master modified
    values[18] = 0; // scientific writeback
    values[19] = 0; // creates new evidence
    values[20] = 0; // second scientific world
    values[21] = 1; // physical frame count
    values[22] = 1; // independent evidence count
    values[23] = 1; // FULLRES_RESTORATION_V0_67
    values[24] = workerCount; // runtime only; not embedded in artifact identity

    auto out = env->NewLongArray(static_cast<jsize>(values.size()));
    if (out != nullptr) {
        env->SetLongArrayRegion(out, 0, static_cast<jsize>(values.size()), values.data());
    }
    return out;
}
