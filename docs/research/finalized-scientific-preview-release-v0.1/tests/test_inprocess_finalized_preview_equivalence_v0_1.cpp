#define main truthraw_existing_release_tests_main
#include "test_finalized_scientific_preview_release_v0_1.cpp"
#undef main

int main() {
    auto frame = make_frame(130, 98);
    const auto prepared = prepare_source(frame);
    auto reconstruction = std::make_shared<truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction>();

    std::array<truthraw::technical_backplane::v0_1::RoomStatus,
               truthraw::technical_backplane::v0_1::kRoomCount> rooms{};
    rooms.fill(truthraw::technical_backplane::v0_1::RoomStatus::ResearchOnly);
    rooms[0] = truthraw::technical_backplane::v0_1::RoomStatus::Available;
    rooms[1] = truthraw::technical_backplane::v0_1::RoomStatus::Available;
    rooms[2] = truthraw::technical_backplane::v0_1::RoomStatus::Available;

    FrameSource inProcessSource(frame);
    truthraw::preview_surface_v0_1::BoundedSrgbPreviewSink inProcessSink(64);
    release_v0_1::ReleaseResult inProcess{};
    const auto inProcessStatus = release_v0_1::create_and_release_finalized_scientific_preview(
        prepared,
        inProcessSource,
        reconstruction,
        std::make_shared<truthraw::NeutralReferenceAppearance>(),
        smsb_v0_1::Options{},
        preview_options(),
        rooms,
        truthraw::technical_backplane::v0_1::ClaimStatus::Candidate,
        inProcessSink,
        inProcess);
    if (!inProcessStatus) {
        std::cerr << "in-process release failed: " << inProcessStatus.message << '\n';
    }
    REQUIRE(inProcessStatus);
    REQUIRE(inProcess.authority ==
            release_v0_1::PreviewAuthority::FinalizedSourceBoundScientificPreview);

    const auto persistedPhase2 = build_phase2(frame, prepared, reconstruction);
    FrameSource persistedSource(frame);
    truthraw::preview_surface_v0_1::BoundedSrgbPreviewSink persistedSink(64);
    release_v0_1::ReleaseResult persisted{};
    const auto persistedStatus = release_v0_1::release_finalized_scientific_preview(
        prepared,
        persistedPhase2.serializedBackplane,
        persistedSource,
        reconstruction,
        std::make_shared<truthraw::NeutralReferenceAppearance>(),
        smsb_v0_1::Options{},
        preview_options(),
        persistedSink,
        persisted);
    if (!persistedStatus) {
        std::cerr << "persisted release failed: " << persistedStatus.message << '\n';
    }
    REQUIRE(persistedStatus);

    REQUIRE(inProcess.authority == persisted.authority);
    REQUIRE(inProcess.scientificIdentity.scientificMasterHash ==
            persisted.scientificIdentity.scientificMasterHash);
    REQUIRE(inProcess.scientificIdentity.zeroLineGauge.L0 ==
            persisted.scientificIdentity.zeroLineGauge.L0);
    REQUIRE(inProcess.scientificIdentity.zeroLineGauge.gaugeId ==
            persisted.scientificIdentity.zeroLineGauge.gaugeId);
    REQUIRE(inProcess.scientificIdentity.sceneBinding.sceneScaleId ==
            persisted.scientificIdentity.sceneBinding.sceneScaleId);
    REQUIRE(inProcess.canonicalPhase2.serializedBackplane ==
            persisted.canonicalPhase2.serializedBackplane);

    REQUIRE(inProcessSink.width() == persistedSink.width());
    REQUIRE(inProcessSink.height() == persistedSink.height());
    REQUIRE(inProcessSink.argb8888() == persistedSink.argb8888());
    REQUIRE(inProcessSink.writtenPixelCount() == persistedSink.writtenPixelCount());

    REQUIRE(inProcess.streaming.provenance.physicalFrameCount == 1u);
    REQUIRE(inProcess.streaming.provenance.independentEvidenceCount == 1u);
    REQUIRE(!inProcess.streaming.provenance.scientificMasterModifiedByAppearance);
    REQUIRE(!inProcess.streaming.provenance.counterfactualObservationCreated);

    std::cout << "INPROCESS_FINALIZED_PREVIEW_EQUIVALENCE_V0_1_PASS\n";
    std::cout << "pixel_identity=1\n";
    std::cout << "scientific_master_identity=1\n";
    std::cout << "backplane_identity=1\n";
    std::cout << "authority_identity=1\n";
    return 0;
}
