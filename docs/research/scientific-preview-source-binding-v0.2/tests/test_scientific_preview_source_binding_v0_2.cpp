#include "scientific_preview_source_binding_v0_2.h"

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace truthraw;
using namespace truthraw::scientific_preview_binding_v0_1;
using namespace truthraw::scientific_preview_binding_v0_2;

#define REQUIRE(x) do { if (!(x)) throw std::runtime_error(std::string("REQUIRE failed: ") + #x); } while (0)

namespace {

struct MemSource final : tile_dng_v0_1::IRandomAccessByteSource {
    std::vector<std::uint8_t> bytes;
    explicit MemSource(std::vector<std::uint8_t> in) : bytes(std::move(in)) {}
    std::uint64_t sizeBytes() const override { return bytes.size(); }
    std::size_t residentBytesUpperBound() const override { return sizeof(*this) + bytes.capacity(); }
    bool readExact(std::uint64_t offset, void* dst, std::size_t count) override {
        if (offset > bytes.size() || count > bytes.size() - offset) return false;
        std::memcpy(dst, bytes.data() + offset, count);
        return true;
    }
};

SourceSeal make_source_seal() {
    MemSource source(std::vector<std::uint8_t>{'t','w','o','-','p','h','a','s','e'});
    SourceSeal seal;
    REQUIRE(seal_source_sha256(source, seal, 1024));
    return seal;
}

ScientificColorBindingRecord make_color(const SourceSeal& source) {
    ScientificColorBindingRecord color;
    color.authority = ColorBindingAuthority::SourceMetadataBound;
    color.sourceEvidenceId = source.sourceEvidenceId;
    color.bindingId = "dng-source-bound-test";
    color.cameraToXyzD50 = {0.96f,0.01f,0.01f, 0.02f,1.00f,0.01f, 0.01f,0.01f,0.82f};
    color.normalized = true;
    color.validated = true;
    color.physicalFrameCount = 1;
    color.independentEvidenceCount = 1;
    return color;
}

technical_backplane::v0_1::State complete_backplane(const SourceSeal& source) {
    technical_backplane::v0_1::State state;
    state.sourceEvidenceHash = source.sha256;
    state.scientificMasterHash.fill(0x41);
    state.zeroLineHash.fill(0x42);
    state.sceneScaleHash.fill(0x43);
    state.physicalFrameCount = 1;
    state.independentEvidenceCount = 1;
    state.roomStatus.fill(technical_backplane::v0_1::RoomStatus::Available);
    state.claimStatus = technical_backplane::v0_1::ClaimStatus::Candidate;
    return state;
}

void test_prepare_allows_compute_and_labeled_appearance_only() {
    const auto source = make_source_seal();
    const auto color = make_color(source);
    PreparedScientificPreviewSource prepared;
    const auto status = prepare_scientific_color_source(source, color, prepared);
    REQUIRE(status);
    REQUIRE(prepared.mainHouseComputeAllowed);
    REQUIRE(prepared.sourceBoundAppearanceReleaseAllowed);
    REQUIRE(!prepared.scientificPreviewReleaseAllowed);
    REQUIRE(!prepared.scientificClaimAllowed);
    REQUIRE(prepared.eventualClaimScope == ColorClaimScope::SourceBoundPreview);
    REQUIRE(prepared.tileNativeOptions.color.valid);
    REQUIRE(prepared.tileNativeOptions.sourceEvidenceId == source.sourceEvidenceId);
    REQUIRE(prepared.physicalFrameCount == 1u);
    REQUIRE(prepared.independentEvidenceCount == 1u);
}

void test_sentinel_cannot_prepare() {
    const auto source = make_source_seal();
    auto color = make_color(source);
    color.authority = ColorBindingAuthority::PreviewSentinel;
    PreparedScientificPreviewSource prepared;
    const auto status = prepare_scientific_color_source(source, color, prepared);
    REQUIRE(!status);
    REQUIRE(status.code == BindingStatusCode::UnauthorizedColorBinding);
}

void test_wrong_source_cannot_prepare() {
    const auto source = make_source_seal();
    auto color = make_color(source);
    color.sourceEvidenceId = "sha256:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    PreparedScientificPreviewSource prepared;
    const auto status = prepare_scientific_color_source(source, color, prepared);
    REQUIRE(!status);
    REQUIRE(status.code == BindingStatusCode::BindingSourceMismatch);
}

void test_incomplete_backplane_cannot_finalize() {
    const auto source = make_source_seal();
    PreparedScientificPreviewSource prepared;
    REQUIRE(prepare_scientific_color_source(source, make_color(source), prepared));
    auto backplane = complete_backplane(source);
    backplane.scientificMasterHash.fill(0);
    ScientificPreviewAdmission finalized;
    const auto status = finalize_scientific_color_lineage(prepared, backplane, finalized);
    REQUIRE(!status);
    REQUIRE(status.code == BindingStatusCode::BackplaneRejected);
}

void test_complete_backplane_finalizes_same_identity() {
    const auto source = make_source_seal();
    PreparedScientificPreviewSource prepared;
    REQUIRE(prepare_scientific_color_source(source, make_color(source), prepared));
    ScientificPreviewAdmission finalized;
    const auto status = finalize_scientific_color_lineage(prepared, complete_backplane(source), finalized);
    REQUIRE(status);
    REQUIRE(finalized.claimScope == prepared.eventualClaimScope);
    REQUIRE(finalized.sourceSeal.sourceEvidenceId == source.sourceEvidenceId);
    REQUIRE(finalized.tileNativeOptions.sourceEvidenceId == prepared.tileNativeOptions.sourceEvidenceId);
    REQUIRE(finalized.tileNativeOptions.color.bindingId == prepared.tileNativeOptions.color.bindingId);
}

void test_backplane_source_mismatch_fails() {
    const auto source = make_source_seal();
    PreparedScientificPreviewSource prepared;
    REQUIRE(prepare_scientific_color_source(source, make_color(source), prepared));
    auto backplane = complete_backplane(source);
    backplane.sourceEvidenceHash[0] ^= 0xffu;
    ScientificPreviewAdmission finalized;
    const auto status = finalize_scientific_color_lineage(prepared, backplane, finalized);
    REQUIRE(!status);
    REQUIRE(status.code == BindingStatusCode::BackplaneSourceMismatch);
}

} // namespace

int main() {
    try {
        test_prepare_allows_compute_and_labeled_appearance_only();
        test_sentinel_cannot_prepare();
        test_wrong_source_cannot_prepare();
        test_incomplete_backplane_cannot_finalize();
        test_complete_backplane_finalizes_same_identity();
        test_backplane_source_mismatch_fails();
        std::cout << "SCIENTIFIC_PREVIEW_SOURCE_BINDING_V0_2_PASS\n";
        std::cout << "pre_master_compute=ALLOWED\n";
        std::cout << "source_bound_appearance_release=ALLOWED_WITH_LABEL\n";
        std::cout << "pre_master_scientific_preview_release=BLOCKED\n";
        std::cout << "pre_master_scientific_claim=BLOCKED\n";
        std::cout << "post_master_backplane=REQUIRED\n";
        std::cout << "physical_frame_count=1 independent_evidence_count=1\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
