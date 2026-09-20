#define main truthraw_existing_release_v0_1_tests_main
#include "../../finalized-scientific-preview-release-v0.1/tests/test_finalized_scientific_preview_release_v0_1.cpp"
#undef main

#include "finalized_scientific_preview_release_v0_2.h"

#include <cstring>
#include <iostream>
#include <memory>

namespace release_v0_2 = truthraw::finalized_scientific_preview_release::v0_2;
namespace smsb_v0_2 = truthraw::scientific_master_streaming_binding::v0_2;

namespace {

class CountingReleaseSource final : public truthraw::streaming_v0_1::IRawTileSource {
public:
    explicit CountingReleaseSource(const truthraw::DecodedDngFrame& frame) : inner_(frame) {}

    const truthraw::DngMetadata& metadata() const override { return inner_.metadata(); }
    std::size_t residentBytesUpperBound() const override { return inner_.residentBytesUpperBound(); }

    truthraw::streaming_v0_1::StreamStatus readRawTile(
        const truthraw::TileRect& rect,
        std::uint16_t* rawOut,
        std::size_t rawCount,
        float* gainOut,
        std::size_t gainCount) override {
        ++rawTileCalls;
        return inner_.readRawTile(rect, rawOut, rawCount, gainOut, gainCount);
    }

    truthraw::streaming_v0_1::StreamStatus readRowBias(
        int y0, int y1, float* out, std::size_t count) override {
        return inner_.readRowBias(y0, y1, out, count);
    }

    truthraw::streaming_v0_1::StreamStatus readColBias(
        int x0, int x1, float* out, std::size_t count) override {
        return inner_.readColBias(x0, x1, out, count);
    }

    std::size_t rawTileCalls = 0u;

private:
    FrameSource inner_;
};

bool equal_double_bits_v0_2(double a, double b) {
    return std::memcmp(&a, &b, sizeof(double)) == 0;
}

std::array<truthraw::technical_backplane::v0_1::RoomStatus,
           truthraw::technical_backplane::v0_1::kRoomCount> release_rooms() {
    std::array<truthraw::technical_backplane::v0_1::RoomStatus,
               truthraw::technical_backplane::v0_1::kRoomCount> rooms{};
    rooms.fill(truthraw::technical_backplane::v0_1::RoomStatus::ResearchOnly);
    rooms[0] = truthraw::technical_backplane::v0_1::RoomStatus::Available;
    rooms[1] = truthraw::technical_backplane::v0_1::RoomStatus::Available;
    rooms[2] = truthraw::technical_backplane::v0_1::RoomStatus::Available;
    return rooms;
}

void require_scientific_identity_equal(
    const smsb_v0_1::Result& oldResult,
    const smsb_v0_2::Result& newResult) {
    REQUIRE(oldResult.scientificMasterHash == newResult.scientificMasterHash);
    REQUIRE(oldResult.zeroLineGauge.mode == newResult.zeroLineGauge.mode);
    REQUIRE(oldResult.zeroLineGauge.gaugeId == newResult.zeroLineGauge.gaugeId);
    REQUIRE(equal_double_bits_v0_2(oldResult.zeroLineGauge.L0, newResult.zeroLineGauge.L0));
    REQUIRE(oldResult.zeroLineGauge.crossSceneComparable ==
            newResult.zeroLineGauge.crossSceneComparable);
    REQUIRE(oldResult.zeroLineGauge.absolutePhysicalUnits ==
            newResult.zeroLineGauge.absolutePhysicalUnits);
    REQUIRE(oldResult.sceneBinding.reconstructionBackend ==
            newResult.sceneBinding.reconstructionBackend);
    REQUIRE(oldResult.sceneBinding.reconstructionCoreCppSha256 ==
            newResult.sceneBinding.reconstructionCoreCppSha256);
    REQUIRE(oldResult.sceneBinding.reconstructionCoreHSha256 ==
            newResult.sceneBinding.reconstructionCoreHSha256);
    REQUIRE(oldResult.sceneBinding.uncertaintyModelSha256 ==
            newResult.sceneBinding.uncertaintyModelSha256);
    REQUIRE(oldResult.sceneBinding.uncertaintyBindingSha256 ==
            newResult.sceneBinding.uncertaintyBindingSha256);
    REQUIRE(oldResult.sceneBinding.sceneScaleId == newResult.sceneBinding.sceneScaleId);
    REQUIRE(oldResult.sceneBinding.gainMapAppliedExactlyOnce ==
            newResult.sceneBinding.gainMapAppliedExactlyOnce);
    REQUIRE(oldResult.sceneBinding.exposureNormalizedToCommonScene ==
            newResult.sceneBinding.exposureNormalizedToCommonScene);
    REQUIRE(oldResult.sceneBinding.gainNormalizedToCommonScene ==
            newResult.sceneBinding.gainNormalizedToCommonScene);
    REQUIRE(oldResult.selfGaugeEligibleSamples == newResult.selfGaugeEligibleSamples);
    REQUIRE(oldResult.masterTilesProcessed == newResult.masterTilesProcessed);
    REQUIRE(oldResult.physicalFrameCount == newResult.physicalFrameCount);
    REQUIRE(oldResult.independentEvidenceCount == newResult.independentEvidenceCount);
    REQUIRE(oldResult.stage2GaugeScanPasses == 4u);
    REQUIRE(newResult.stage2GaugeScanPasses == 2u);
}

void test_end_to_end_v0_1_v0_2_bit_exact_equivalence() {
    auto frame = make_frame(130, 98);
    const auto prepared = prepare_source(frame);
    const auto rooms = release_rooms();

    CountingReleaseSource oldSource(frame);
    CountingReleaseSource newSource(frame);
    auto oldReconstruction =
        std::make_shared<truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto newReconstruction =
        std::make_shared<truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction>();

    truthraw::preview_surface_v0_1::BoundedSrgbPreviewSink oldSink(64);
    truthraw::preview_surface_v0_1::BoundedSrgbPreviewSink newSink(64);
    release_v0_1::ReleaseResult oldResult{};
    release_v0_2::ReleaseResult newResult{};

    const auto oldStatus = release_v0_1::create_and_release_finalized_scientific_preview(
        prepared,
        oldSource,
        oldReconstruction,
        std::make_shared<truthraw::NeutralReferenceAppearance>(),
        smsb_v0_1::Options{},
        preview_options(),
        rooms,
        truthraw::technical_backplane::v0_1::ClaimStatus::Candidate,
        oldSink,
        oldResult);
    const auto newStatus = release_v0_2::create_and_release_finalized_scientific_preview(
        prepared,
        newSource,
        newReconstruction,
        std::make_shared<truthraw::NeutralReferenceAppearance>(),
        smsb_v0_2::Options{},
        preview_options(),
        rooms,
        truthraw::technical_backplane::v0_1::ClaimStatus::Candidate,
        newSink,
        newResult);

    if (!oldStatus) std::cerr << "v0.1 release failed: " << oldStatus.message << '\n';
    if (!newStatus) std::cerr << "v0.2 release failed: " << newStatus.message << '\n';
    REQUIRE(oldStatus);
    REQUIRE(newStatus);

    REQUIRE(oldResult.authority ==
            release_v0_1::PreviewAuthority::FinalizedSourceBoundScientificPreview);
    REQUIRE(newResult.authority ==
            release_v0_2::PreviewAuthority::FinalizedSourceBoundScientificPreview);
    require_scientific_identity_equal(oldResult.scientificIdentity, newResult.scientificIdentity);
    REQUIRE(oldResult.canonicalPhase2.serializedBackplane ==
            newResult.canonicalPhase2.serializedBackplane);
    REQUIRE(oldResult.canonicalPhase2.serializedBackplane.size() == 180u);

    REQUIRE(oldSink.width() == newSink.width());
    REQUIRE(oldSink.height() == newSink.height());
    REQUIRE(oldSink.writtenPixelCount() == newSink.writtenPixelCount());
    REQUIRE(oldSink.argb8888() == newSink.argb8888());

    REQUIRE(oldResult.streaming.provenance.physicalFrameCount == 1u);
    REQUIRE(newResult.streaming.provenance.physicalFrameCount == 1u);
    REQUIRE(oldResult.streaming.provenance.independentEvidenceCount == 1u);
    REQUIRE(newResult.streaming.provenance.independentEvidenceCount == 1u);
    REQUIRE(!oldResult.streaming.provenance.scientificMasterModifiedByAppearance);
    REQUIRE(!newResult.streaming.provenance.scientificMasterModifiedByAppearance);
    REQUIRE(!oldResult.streaming.provenance.counterfactualObservationCreated);
    REQUIRE(!newResult.streaming.provenance.counterfactualObservationCreated);

    REQUIRE(oldSource.rawTileCalls > newSource.rawTileCalls);
    REQUIRE(oldSource.rawTileCalls - newSource.rawTileCalls ==
            oldResult.scientificIdentity.masterTilesProcessed * 2u);
}

void test_v0_2_accepts_v0_1_persisted_backplane_without_identity_drift() {
    auto frame = make_frame(130, 98);
    const auto prepared = prepare_source(frame);
    auto reconstruction =
        std::make_shared<truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction>();
    const auto oldPhase2 = build_phase2(frame, prepared, reconstruction);

    CountingReleaseSource source(frame);
    truthraw::preview_surface_v0_1::BoundedSrgbPreviewSink sink(64);
    release_v0_2::ReleaseResult result{};
    const auto status = release_v0_2::release_finalized_scientific_preview(
        prepared,
        oldPhase2.serializedBackplane,
        source,
        reconstruction,
        std::make_shared<truthraw::NeutralReferenceAppearance>(),
        smsb_v0_2::Options{},
        preview_options(),
        sink,
        result);

    if (!status) std::cerr << "v0.2 persisted release failed: " << status.message << '\n';
    REQUIRE(status);
    REQUIRE(result.canonicalPhase2.serializedBackplane == oldPhase2.serializedBackplane);
    REQUIRE(result.scientificIdentity.scientificMasterHash == oldPhase2.backplane.scientificMasterHash);
    REQUIRE(result.scientificIdentity.stage2GaugeScanPasses == 2u);
    REQUIRE(sink.finished());
    REQUIRE(!sink.argb8888().empty());
}

}  // namespace

int truthraw_existing_release_v0_2_tests_main() {
    test_end_to_end_v0_1_v0_2_bit_exact_equivalence();
    test_v0_2_accepts_v0_1_persisted_backplane_without_identity_drift();
    std::cout << "FINALIZED_SCIENTIFIC_PREVIEW_RELEASE_V0_2_PASS\n";
    std::cout << "scientific_master_identity=1\n";
    std::cout << "zero_line_bit_identity=1\n";
    std::cout << "backplane_180_byte_identity=1\n";
    std::cout << "preview_pixel_identity=1\n";
    std::cout << "stage2_gauge_scan_passes_old=4\n";
    std::cout << "stage2_gauge_scan_passes_new=2\n";
    return 0;
}


#include "finalized_scientific_preview_release_v0_3.h"

namespace release_v0_3 = truthraw::finalized_scientific_preview_release::v0_3;
namespace smsb_v0_3 = truthraw::scientific_master_streaming_binding::v0_3;

namespace {

bool equal_double_bits_v0_3(double a, double b) {
    return std::memcmp(&a, &b, sizeof(double)) == 0;
}

void require_v2_v3_identity_equal(
    const smsb_v0_2::Result& v2,
    const smsb_v0_3::Result& v3) {
    REQUIRE(v2.scientificMasterHash == v3.scientificMasterHash);
    REQUIRE(v2.zeroLineGauge.mode == v3.zeroLineGauge.mode);
    REQUIRE(v2.zeroLineGauge.gaugeId == v3.zeroLineGauge.gaugeId);
    REQUIRE(equal_double_bits_v0_3(v2.zeroLineGauge.L0, v3.zeroLineGauge.L0));
    REQUIRE(v2.zeroLineGauge.crossSceneComparable == v3.zeroLineGauge.crossSceneComparable);
    REQUIRE(v2.zeroLineGauge.absolutePhysicalUnits == v3.zeroLineGauge.absolutePhysicalUnits);
    REQUIRE(v2.sceneBinding.reconstructionBackend == v3.sceneBinding.reconstructionBackend);
    REQUIRE(v2.sceneBinding.reconstructionCoreCppSha256 == v3.sceneBinding.reconstructionCoreCppSha256);
    REQUIRE(v2.sceneBinding.reconstructionCoreHSha256 == v3.sceneBinding.reconstructionCoreHSha256);
    REQUIRE(v2.sceneBinding.uncertaintyModelSha256 == v3.sceneBinding.uncertaintyModelSha256);
    REQUIRE(v2.sceneBinding.uncertaintyBindingSha256 == v3.sceneBinding.uncertaintyBindingSha256);
    REQUIRE(v2.sceneBinding.sceneScaleId == v3.sceneBinding.sceneScaleId);
    REQUIRE(v2.sceneBinding.gainMapAppliedExactlyOnce == v3.sceneBinding.gainMapAppliedExactlyOnce);
    REQUIRE(v2.sceneBinding.exposureNormalizedToCommonScene == v3.sceneBinding.exposureNormalizedToCommonScene);
    REQUIRE(v2.sceneBinding.gainNormalizedToCommonScene == v3.sceneBinding.gainNormalizedToCommonScene);
    REQUIRE(v2.selfGaugeEligibleSamples == v3.selfGaugeEligibleSamples);
    REQUIRE(v2.masterTilesProcessed == v3.masterTilesProcessed);
    REQUIRE(v2.physicalFrameCount == 1u && v3.physicalFrameCount == 1u);
    REQUIRE(v2.independentEvidenceCount == 1u && v3.independentEvidenceCount == 1u);
    REQUIRE(v2.stage2GaugeScanPasses == 2u && v3.stage2GaugeScanPasses == 2u);
}

void test_v0_2_v0_3_end_to_end_identity() {
    auto frame = make_frame(2113, 193);
    const auto prepared = prepare_source(frame);
    const auto rooms = release_rooms();

    CountingReleaseSource oldSource(frame);
    CountingReleaseSource newSource(frame);

    auto oldReconstruction =
        std::make_shared<truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto newReconstruction =
        std::make_shared<truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction>();

    truthraw::preview_surface_v0_1::BoundedSrgbPreviewSink oldSink(96);
    truthraw::preview_surface_v0_1::BoundedSrgbPreviewSink newSink(96);
    release_v0_2::ReleaseResult oldResult{};
    release_v0_3::ReleaseResult newResult{};

    const auto oldStatus = release_v0_2::create_and_release_finalized_scientific_preview(
        prepared, oldSource, oldReconstruction,
        std::make_shared<truthraw::NeutralReferenceAppearance>(),
        smsb_v0_2::Options{}, preview_options(), rooms,
        truthraw::technical_backplane::v0_1::ClaimStatus::Candidate,
        oldSink, oldResult);
    const auto newStatus = release_v0_3::create_and_release_finalized_scientific_preview(
        prepared, newSource, newReconstruction,
        std::make_shared<truthraw::NeutralReferenceAppearance>(),
        smsb_v0_3::Options{}, preview_options(), rooms,
        truthraw::technical_backplane::v0_1::ClaimStatus::Candidate,
        newSink, newResult);

    if (!oldStatus) std::cerr << "v0.2 release failed: " << oldStatus.message << '\n';
    if (!newStatus) std::cerr << "v0.3 release failed: " << newStatus.message << '\n';
    REQUIRE(oldStatus);
    REQUIRE(newStatus);

    REQUIRE(oldResult.authority == release_v0_2::PreviewAuthority::FinalizedSourceBoundScientificPreview);
    REQUIRE(newResult.authority == release_v0_3::PreviewAuthority::FinalizedSourceBoundScientificPreview);
    require_v2_v3_identity_equal(oldResult.scientificIdentity, newResult.scientificIdentity);
    REQUIRE(oldResult.canonicalPhase2.serializedBackplane ==
            newResult.canonicalPhase2.serializedBackplane);
    REQUIRE(oldResult.canonicalPhase2.serializedBackplane.size() == 180u);

    REQUIRE(oldSink.width() == newSink.width());
    REQUIRE(oldSink.height() == newSink.height());
    REQUIRE(oldSink.writtenPixelCount() == newSink.writtenPixelCount());
    REQUIRE(oldSink.argb8888() == newSink.argb8888());

    REQUIRE(oldResult.streaming.provenance.physicalFrameCount == 1u);
    REQUIRE(newResult.streaming.provenance.physicalFrameCount == 1u);
    REQUIRE(oldResult.streaming.provenance.independentEvidenceCount == 1u);
    REQUIRE(newResult.streaming.provenance.independentEvidenceCount == 1u);
    REQUIRE(!newResult.streaming.provenance.scientificMasterModifiedByAppearance);
    REQUIRE(!newResult.streaming.provenance.counterfactualObservationCreated);
    REQUIRE(newSource.rawTileCalls < oldSource.rawTileCalls);
}

void test_v0_3_accepts_v0_2_backplane_exactly() {
    auto frame = make_frame(130, 98);
    const auto prepared = prepare_source(frame);
    const auto rooms = release_rooms();

    CountingReleaseSource buildSource(frame);
    auto buildReconstruction =
        std::make_shared<truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction>();
    truthraw::preview_surface_v0_1::BoundedSrgbPreviewSink buildSink(64);
    release_v0_2::ReleaseResult built{};
    const auto builtStatus = release_v0_2::create_and_release_finalized_scientific_preview(
        prepared, buildSource, buildReconstruction,
        std::make_shared<truthraw::NeutralReferenceAppearance>(),
        smsb_v0_2::Options{}, preview_options(), rooms,
        truthraw::technical_backplane::v0_1::ClaimStatus::Candidate,
        buildSink, built);
    REQUIRE(builtStatus);

    CountingReleaseSource verifySource(frame);
    auto verifyReconstruction =
        std::make_shared<truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction>();
    truthraw::preview_surface_v0_1::BoundedSrgbPreviewSink verifySink(64);
    release_v0_3::ReleaseResult verified{};
    const auto status = release_v0_3::release_finalized_scientific_preview(
        prepared,
        built.canonicalPhase2.serializedBackplane,
        verifySource,
        verifyReconstruction,
        std::make_shared<truthraw::NeutralReferenceAppearance>(),
        smsb_v0_3::Options{},
        preview_options(),
        verifySink,
        verified);
    if (!status) std::cerr << "v0.3 persisted release failed: " << status.message << '\n';
    REQUIRE(status);
    REQUIRE(verified.canonicalPhase2.serializedBackplane ==
            built.canonicalPhase2.serializedBackplane);
    require_v2_v3_identity_equal(built.scientificIdentity, verified.scientificIdentity);
    REQUIRE(buildSink.argb8888() == verifySink.argb8888());
}

}  // namespace

int main() {
    test_v0_2_v0_3_end_to_end_identity();
    test_v0_3_accepts_v0_2_backplane_exactly();
    std::cout << "FINALIZED_SCIENTIFIC_PREVIEW_RELEASE_V0_3_PASS\n";
    std::cout << "scientific_master_sha_identity=1\n";
    std::cout << "zero_line_bit_identity=1\n";
    std::cout << "backplane_180_byte_identity=1\n";
    std::cout << "preview_pixel_identity=1\n";
    std::cout << "source_calls_reduced=1\n";
    return 0;
}
