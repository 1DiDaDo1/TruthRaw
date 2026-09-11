#include "scientific_preview_source_binding_v0_1.h"

#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

using namespace truthraw::scientific_preview_binding_v0_1;
using truthraw::technical_backplane::v0_1::State;
using truthraw::tile_dng_v0_1::IRandomAccessByteSource;

#define REQUIRE(x) do { if (!(x)) throw std::runtime_error(std::string("REQUIRE failed: ") + #x); } while (0)

namespace {
struct MemSource final : IRandomAccessByteSource {
    std::vector<std::uint8_t> bytes;
    explicit MemSource(std::vector<std::uint8_t> value) : bytes(std::move(value)) {}
    std::uint64_t sizeBytes() const override { return bytes.size(); }
    std::size_t residentBytesUpperBound() const override { return sizeof(*this) + bytes.capacity(); }
    bool readExact(std::uint64_t offset, void* dst, std::size_t count) override {
        if (offset > bytes.size() || count > bytes.size() - offset) return false;
        std::memcpy(dst, bytes.data() + static_cast<std::size_t>(offset), count);
        return true;
    }
};

State valid_backplane(const SourceSeal& seal) {
    State b;
    b.sourceEvidenceHash = seal.sha256;
    b.scientificMasterHash.fill(0x21u);
    b.zeroLineHash.fill(0x32u);
    b.sceneScaleHash.fill(0x43u);
    b.physicalFrameCount = 1;
    b.independentEvidenceCount = 1;
    return b;
}

ScientificColorBindingRecord source_bound_record(const SourceSeal& seal) {
    ScientificColorBindingRecord c;
    c.authority = ColorBindingAuthority::SourceMetadataBound;
    c.sourceEvidenceId = seal.sourceEvidenceId;
    c.bindingId = "fixture-source-metadata-binding-v0.1";
    c.cameraToXyzD50 = {0.71f,0.18f,0.11f, 0.22f,0.72f,0.06f, 0.02f,0.10f,0.88f};
    c.normalized = true;
    c.validated = true;
    return c;
}
} // namespace

int main() {
    MemSource abc({'a','b','c'});
    SourceSeal seal;
    auto status = seal_source_sha256(abc, seal, 4096);
    REQUIRE(status);
    REQUIRE(seal.byteLength == 3);
    REQUIRE(seal.sourceEvidenceId == "sha256:ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    REQUIRE(is_canonical_source_evidence_id(seal.sourceEvidenceId));
    REQUIRE(seal.hashWorkspacePeakBytes <= 4096u + 128u);

    auto backplane = valid_backplane(seal);
    auto color = source_bound_record(seal);
    ScientificPreviewAdmission admitted;
    status = admit_scientific_color_preview(seal, color, backplane, admitted);
    REQUIRE(status);
    REQUIRE(admitted.claimScope == ColorClaimScope::SourceBoundPreview);
    REQUIRE(admitted.tileNativeOptions.sourceEvidenceId == seal.sourceEvidenceId);
    REQUIRE(admitted.tileNativeOptions.color.valid);
    REQUIRE(admitted.tileNativeOptions.color.bindingId == color.bindingId);
    REQUIRE(admitted.tileNativeOptions.color.cameraToXyzD50 == color.cameraToXyzD50);

    auto sentinel = color;
    sentinel.authority = ColorBindingAuthority::PreviewSentinel;
    sentinel.bindingId = "ui-preview-parser-sentinel-not-scientific-v0.2";
    status = admit_scientific_color_preview(seal, sentinel, backplane, admitted);
    REQUIRE(!status && status.code == BindingStatusCode::UnauthorizedColorBinding);

    auto unvalidated = color;
    unvalidated.validated = false;
    status = admit_scientific_color_preview(seal, unvalidated, backplane, admitted);
    REQUIRE(!status && status.code == BindingStatusCode::UnauthorizedColorBinding);

    auto wrongSource = color;
    wrongSource.sourceEvidenceId = "sha256:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    status = admit_scientific_color_preview(seal, wrongSource, backplane, admitted);
    REQUIRE(!status && status.code == BindingStatusCode::BindingSourceMismatch);

    auto badMatrix = color;
    badMatrix.cameraToXyzD50[4] = std::numeric_limits<float>::quiet_NaN();
    status = admit_scientific_color_preview(seal, badMatrix, backplane, admitted);
    REQUIRE(!status && status.code == BindingStatusCode::InvalidMatrix);

    auto wrongBackplane = backplane;
    wrongBackplane.sourceEvidenceHash[0] ^= 0x01u;
    status = admit_scientific_color_preview(seal, color, wrongBackplane, admitted);
    REQUIRE(!status && status.code == BindingStatusCode::BackplaneSourceMismatch);

    auto wrongEvidenceCount = color;
    wrongEvidenceCount.independentEvidenceCount = 2;
    status = admit_scientific_color_preview(seal, wrongEvidenceCount, backplane, admitted);
    REQUIRE(!status && status.code == BindingStatusCode::EvidenceInvariantViolation);

    auto independent = color;
    independent.authority = ColorBindingAuthority::IndependentCalibration;
    independent.bindingId = "fixture-independent-calibration-v0.1";
    status = admit_scientific_color_preview(seal, independent, backplane, admitted);
    REQUIRE(status);
    REQUIRE(admitted.claimScope == ColorClaimScope::IndependentlyCalibratedPreview);

    auto gatehouse = color;
    gatehouse.authority = ColorBindingAuthority::GatehouseCertifiedMetadata;
    gatehouse.bindingId = "fixture-gatehouse-certified-metadata-v0.1";
    status = admit_scientific_color_preview(seal, gatehouse, backplane, admitted);
    REQUIRE(status);
    REQUIRE(admitted.claimScope == ColorClaimScope::SourceBoundPreview);

    status = reverify_source_sha256(abc, seal, 4096);
    REQUIRE(status);
    abc.bytes[1] = 'x';
    status = reverify_source_sha256(abc, seal, 4096);
    REQUIRE(!status && status.code == BindingStatusCode::SourceSealMismatch);

    REQUIRE(!is_canonical_source_evidence_id("sha256:BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD"));
    REQUIRE(!is_canonical_source_evidence_id("ui-preview-ephemeral-not-evidence-v0.2"));

    std::cout << "SCIENTIFIC_PREVIEW_SOURCE_BINDING_V0_1_PASS\n";
    std::cout << "source_evidence_id=" << seal.sourceEvidenceId << "\n";
    std::cout << "hash_workspace_peak_bytes=" << seal.hashWorkspacePeakBytes << "\n";
    std::cout << "sentinel_scientific_color=BLOCKED\n";
    std::cout << "tampered_source=BLOCKED\n";
    std::cout << "frame_count=1\n";
    std::cout << "evidence_count=1\n";
    return 0;
}
