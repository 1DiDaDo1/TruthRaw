#include "scientific_master_streaming_binding_v0_1.h"
#include "streaming_test_support_v0_1.h"
#include "technical_backplane_phase2_v0_1.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace truthraw::scientific_master_streaming_binding::v0_1;

namespace {

class SealMemorySource final : public truthraw::tile_dng_v0_1::IRandomAccessByteSource {
public:
    explicit SealMemorySource(std::vector<std::uint8_t> bytes) : bytes_(std::move(bytes)) {}
    std::uint64_t sizeBytes() const override { return static_cast<std::uint64_t>(bytes_.size()); }
    std::size_t residentBytesUpperBound() const override { return sizeof(*this) + bytes_.size(); }
    bool readExact(std::uint64_t offset, void* dst, std::size_t count) override {
        if (offset > bytes_.size()) return false;
        const auto start = static_cast<std::size_t>(offset);
        if (count > bytes_.size() - start) return false;
        if (count != 0u) std::memcpy(dst, bytes_.data() + start, count);
        return true;
    }
private:
    std::vector<std::uint8_t> bytes_;
};

truthraw::scientific_preview_binding_v0_2::PreparedScientificPreviewSource
prepare_source_for_phase2(const DecodedDngFrame& frame) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(frame.raw.size() * 2u + 16u);
    const char magic[] = "TRUTHRAW_SYNTH_SOURCE_V1";
    bytes.insert(bytes.end(), magic, magic + sizeof(magic) - 1u);
    for (const auto sample : frame.raw) {
        bytes.push_back(static_cast<std::uint8_t>(sample & 0xffu));
        bytes.push_back(static_cast<std::uint8_t>((sample >> 8u) & 0xffu));
    }
    SealMemorySource source(std::move(bytes));

    truthraw::scientific_preview_binding_v0_1::SourceSeal seal{};
    const auto sealStatus = truthraw::scientific_preview_binding_v0_1::seal_source_sha256(
        source, seal, 4096u);
    REQUIRE(sealStatus);

    truthraw::scientific_preview_binding_v0_1::ScientificColorBindingRecord color{};
    color.authority = truthraw::scientific_preview_binding_v0_1::ColorBindingAuthority::SourceMetadataBound;
    color.sourceEvidenceId = seal.sourceEvidenceId;
    color.bindingId = "SYNTHETIC_SOURCE_METADATA_D50";
    color.cameraToXyzD50 = frame.meta.cameraToXyzD50;
    color.normalized = true;
    color.validated = true;
    color.physicalFrameCount = 1u;
    color.independentEvidenceCount = 1u;

    truthraw::scientific_preview_binding_v0_2::PreparedScientificPreviewSource prepared{};
    const auto prepareStatus = truthraw::scientific_preview_binding_v0_2::prepare_scientific_color_source(
        seal, color, prepared);
    REQUIRE(prepareStatus);
    return prepared;
}

truthraw::scientific_master_digest::v0_1::Sha256 digest_full_latent_scene(
    const truthraw::LatentCameraSceneV02& scene) {
    truthraw::scientific_master_digest::v0_1::ScientificMasterDigestAccumulator digest(
        static_cast<std::uint32_t>(scene.width), static_cast<std::uint32_t>(scene.height));
    REQUIRE(digest.valid());
    truthraw::scientific_master_digest::v0_1::TileView tile{};
    tile.x = 0u;
    tile.y = 0u;
    tile.width = static_cast<std::uint32_t>(scene.width);
    tile.height = static_cast<std::uint32_t>(scene.height);
    tile.rgb = scene.cameraRgb.data();
    tile.rowStrideSamples = static_cast<std::size_t>(scene.width) * 3u;
    REQUIRE(digest.add_tile(tile));
    truthraw::scientific_master_digest::v0_1::Sha256 out{};
    REQUIRE(digest.finalize(out));
    return out;
}

void test_streaming_equals_full_latent_and_phase2() {
    auto frame = make_frame(130, 70);
    auto reconstruction = std::make_shared<truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction>();

    FrameSource source(frame);
    Result streaming{};
    Options options{};
    const auto status = bind_scientific_master_streaming(source, *reconstruction, options, streaming);
    if (!status) std::cerr << "streaming bind failed: " << status.message << '\n';
    REQUIRE(status);
    REQUIRE(streaming.physicalFrameCount == 1u);
    REQUIRE(streaming.independentEvidenceCount == 1u);
    REQUIRE(streaming.stage2GaugeScanPasses == 4u);
    REQUIRE(streaming.masterTilesProcessed == 6u);
    REQUIRE(streaming.selfGaugeEligibleSamples > 0u);
    REQUIRE(streaming.logicalWorkspacePeakBytes > 0u);
    REQUIRE(streaming.logicalResidentUpperBound >= streaming.logicalWorkspacePeakBytes);

    truthraw::LatentSceneBindingV02 fullBinding{};
    fullBinding.reconstructionBackend = reconstruction->name();
    fullBinding.sceneScaleId = "TRUTHRANGE_SELF_GAUGE_STAGE2_V0_2";
    fullBinding.gainMapAppliedExactlyOnce = true;
    fullBinding.exposureNormalizedToCommonScene = false;
    fullBinding.gainNormalizedToCommonScene = false;

    truthraw::LatentCameraSceneV02 fullScene{};
    const truthraw::TilePolicy tilePolicy{64, reconstruction->requiredHalo()};
    const auto fullStatus = truthraw::build_latent_camera_scene_v0_2(
        frame, tilePolicy, *reconstruction, fullBinding, fullScene);
    REQUIRE(fullStatus);

    const auto fullDigest = digest_full_latent_scene(fullScene);
    REQUIRE(fullDigest == streaming.scientificMasterHash);

    truthraw::TruthRangeGaugeV02 fullGauge{};
    const auto gaugeStatus = truthraw::derive_self_gauge_v0_2(
        fullScene, fullGauge, 0.5, 0.10);
    REQUIRE(gaugeStatus);
    REQUIRE(fullGauge.gaugeId == streaming.zeroLineGauge.gaugeId);
    REQUIRE(fullGauge.mode == streaming.zeroLineGauge.mode);
    REQUIRE(fullGauge.L0 == streaming.zeroLineGauge.L0);
    REQUIRE(!streaming.zeroLineGauge.crossSceneComparable);
    REQUIRE(!streaming.zeroLineGauge.absolutePhysicalUnits);

    const auto prepared = prepare_source_for_phase2(frame);
    truthraw::technical_backplane_phase2::v0_1::Phase2Input phase2{};
    phase2.prepared = prepared;
    phase2.scientificMasterHash = streaming.scientificMasterHash;
    phase2.zeroLineGauge = streaming.zeroLineGauge;
    phase2.sceneBinding = streaming.sceneBinding;
    phase2.roomStatus.fill(truthraw::technical_backplane::v0_1::RoomStatus::ResearchOnly);
    phase2.roomStatus[0] = truthraw::technical_backplane::v0_1::RoomStatus::Available;
    phase2.roomStatus[1] = truthraw::technical_backplane::v0_1::RoomStatus::Available;
    phase2.roomStatus[2] = truthraw::technical_backplane::v0_1::RoomStatus::Available;
    phase2.claimStatus = truthraw::technical_backplane::v0_1::ClaimStatus::Candidate;

    truthraw::technical_backplane_phase2::v0_1::Phase2Result finalized{};
    const auto phase2Status = truthraw::technical_backplane_phase2::v0_1::finalize_phase2(
        phase2, finalized);
    if (!phase2Status) std::cerr << "phase2 failed: " << phase2Status.message << '\n';
    REQUIRE(phase2Status);
    REQUIRE(finalized.backplane.scientificMasterHash == streaming.scientificMasterHash);
    REQUIRE(finalized.backplane.sourceEvidenceHash == prepared.source.sha256);
    REQUIRE(finalized.admission.sourceSeal.sha256 == prepared.source.sha256);
    REQUIRE(finalized.admission.claimScope ==
            truthraw::scientific_preview_binding_v0_1::ColorClaimScope::SourceBoundPreview);
}

void test_color_metadata_does_not_change_scientific_identity() {
    auto frameA = make_frame(130, 70);
    auto frameB = frameA;
    frameB.meta.cameraToXyzD50 = {
        1.7f, -0.4f, 0.2f,
        0.1f,  0.8f, 0.1f,
       -0.2f,  0.3f, 1.4f,
    };
    auto reconstruction = std::make_shared<truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction>();
    FrameSource sourceA(frameA);
    FrameSource sourceB(frameB);
    Result a{};
    Result b{};
    REQUIRE(bind_scientific_master_streaming(sourceA, *reconstruction, Options{}, a));
    REQUIRE(bind_scientific_master_streaming(sourceB, *reconstruction, Options{}, b));
    REQUIRE(a.scientificMasterHash == b.scientificMasterHash);
    REQUIRE(a.zeroLineGauge.L0 == b.zeroLineGauge.L0);
}

void test_budget_fail_closed() {
    auto frame = make_frame(130, 70);
    auto reconstruction = std::make_shared<truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction>();
    FrameSource source(frame);
    Options options{};
    options.memoryBudgetBytes = 1024u;
    Result result{};
    const auto status = bind_scientific_master_streaming(source, *reconstruction, options, result);
    REQUIRE(!status);
    REQUIRE(status.code == StatusCode::BudgetExceeded);
}

}  // namespace

int main() {
    test_streaming_equals_full_latent_and_phase2();
    test_color_metadata_does_not_change_scientific_identity();
    test_budget_fail_closed();

    std::cout << "SCIENTIFIC_MASTER_STREAMING_BINDING_V0_1_PASS\n";
    std::cout << "streaming_master_equals_full_latent=1\n";
    std::cout << "streaming_self_gauge_equals_truthrange_v0_2=1\n";
    std::cout << "phase2_finalization_from_streaming_identity=1\n";
    std::cout << "full_frame_master_materialized_by_streaming_binding=0\n";
    std::cout << "physicalFrameCount=1\n";
    std::cout << "independentEvidenceCount=1\n";
    return 0;
}
