#include "scientific_dng_export_v0_1.h"
#include "scientific_master_streaming_binding_v0_2.h"
#include "streaming_test_support_v0_1.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

using namespace truthraw;
using namespace truthraw::scientific_dng_export::v0_1;

class VectorSink final : public ISequentialByteSink {
public:
    bool write(const void* data, std::size_t bytes) override {
        const auto* p = static_cast<const std::uint8_t*>(data);
        bytes_.insert(bytes_.end(), p, p + bytes);
        return true;
    }
    bool flush() override { return true; }
    std::uint64_t bytesWritten() const noexcept override { return bytes_.size(); }
    const std::vector<std::uint8_t>& bytes() const noexcept { return bytes_; }
private:
    std::vector<std::uint8_t> bytes_;
};

std::uint16_t u16(const std::vector<std::uint8_t>& b, std::size_t at) {
    REQUIRE(at + 2u <= b.size());
    return static_cast<std::uint16_t>(b[at]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(b[at + 1u]) << 8u);
}

std::uint32_t u32(const std::vector<std::uint8_t>& b, std::size_t at) {
    REQUIRE(at + 4u <= b.size());
    return static_cast<std::uint32_t>(b[at]) |
           (static_cast<std::uint32_t>(b[at + 1u]) << 8u) |
           (static_cast<std::uint32_t>(b[at + 2u]) << 16u) |
           (static_cast<std::uint32_t>(b[at + 3u]) << 24u);
}

std::uint32_t tag_value(const std::vector<std::uint8_t>& b,
                        std::uint16_t wanted,
                        std::uint16_t* typeOut = nullptr,
                        std::uint32_t* countOut = nullptr) {
    REQUIRE(b.size() >= 8u);
    REQUIRE(b[0] == 'I' && b[1] == 'I');
    REQUIRE(u16(b, 2u) == 42u);
    const std::uint32_t ifd = u32(b, 4u);
    const std::uint16_t count = u16(b, ifd);
    for (std::uint16_t i = 0; i < count; ++i) {
        const std::size_t at = static_cast<std::size_t>(ifd) + 2u + 12u * i;
        if (u16(b, at) == wanted) {
            if (typeOut) *typeOut = u16(b, at + 2u);
            if (countOut) *countOut = u32(b, at + 4u);
            return u32(b, at + 8u);
        }
    }
    std::cerr << "missing tag " << wanted << "\n";
    std::exit(2);
}

std::string source_id_from_digest(const std::array<std::uint8_t, 32>& digest) {
    static constexpr char hex[] = "0123456789abcdef";
    std::string out = "sha256:";
    for (const auto b : digest) {
        out.push_back(hex[b >> 4]);
        out.push_back(hex[b & 0x0fu]);
    }
    return out;
}

scientific_preview_binding_v0_1::SourceSeal test_source_seal() {
    scientific_preview_binding_v0_1::SourceSeal seal{};
    for (std::size_t i = 0; i < seal.sha256.size(); ++i) {
        seal.sha256[i] = static_cast<std::uint8_t>(0x21u + 3u * i);
    }
    seal.byteLength = 25369034u;
    seal.sourceEvidenceId = source_id_from_digest(seal.sha256);
    return seal;
}

scientific_preview_binding_v0_2::PreparedScientificPreviewSource prepared_for(
    const DngMetadata& m,
    scientific_preview_binding_v0_1::ColorBindingAuthority authority =
        scientific_preview_binding_v0_1::ColorBindingAuthority::SourceMetadataBound) {
    auto seal = test_source_seal();
    REQUIRE(m.sourceId == seal.sourceEvidenceId);

    scientific_preview_binding_v0_1::ScientificColorBindingRecord color{};
    color.authority = authority;
    color.sourceEvidenceId = seal.sourceEvidenceId;
    color.bindingId = "TEST_SOURCE_METADATA_D50_BINDING";
    color.cameraToXyzD50 = m.cameraToXyzD50;
    color.normalized = true;
    color.validated = true;
    color.physicalFrameCount = 1u;
    color.independentEvidenceCount = 1u;

    scientific_preview_binding_v0_2::PreparedScientificPreviewSource prepared{};
    const auto status = scientific_preview_binding_v0_2::prepare_scientific_color_source(
        seal, color, prepared);
    REQUIRE(static_cast<bool>(status));
    return prepared;
}

technical_backplane_phase2::v0_1::Phase2Result finalize_lineage(
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    const scientific_master_streaming_binding::v0_2::Result& scientific,
    const scientific_master_digest::v0_1::Sha256* masterOverride = nullptr) {
    technical_backplane_phase2::v0_1::Phase2Input input{};
    input.prepared = prepared;
    input.scientificMasterHash =
        masterOverride != nullptr ? *masterOverride : scientific.scientificMasterHash;
    input.zeroLineGauge = scientific.zeroLineGauge;
    input.sceneBinding = scientific.sceneBinding;
    input.roomStatus.fill(technical_backplane::v0_1::RoomStatus::ResearchOnly);
    input.roomStatus[0] = technical_backplane::v0_1::RoomStatus::Available;
    input.roomStatus[1] = technical_backplane::v0_1::RoomStatus::Available;
    input.roomStatus[2] = technical_backplane::v0_1::RoomStatus::Available;
    input.claimStatus = technical_backplane::v0_1::ClaimStatus::Candidate;

    technical_backplane_phase2::v0_1::Phase2Result finalized{};
    const auto status = technical_backplane_phase2::v0_1::finalize_phase2(input, finalized);
    REQUIRE(static_cast<bool>(status));
    return finalized;
}

void check_common_dng(const std::vector<std::uint8_t>& b, int w, int h) {
    REQUIRE(tag_value(b, 256u) == static_cast<std::uint32_t>(w));
    REQUIRE(tag_value(b, 257u) == static_cast<std::uint32_t>(h));
    REQUIRE(tag_value(b, 259u) == 1u);
    REQUIRE(tag_value(b, 322u) == 64u);
    REQUIRE(tag_value(b, 323u) == 64u);
    REQUIRE(tag_value(b, 274u) == 1u);
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    const std::uint32_t dngv = tag_value(b, 50706u, &type, &count);
    REQUIRE(type == 1u && count == 4u);
    REQUIRE((dngv & 0xffu) == 1u);
    REQUIRE(((dngv >> 8u) & 0xffu) == 4u);
    REQUIRE(tag_value(b, 50778u) == 23u);
    REQUIRE(tag_value(b, 50721u, &type, &count) > 0u && type == 10u && count == 9u);
    REQUIRE(tag_value(b, 50728u, &type, &count) > 0u && type == 5u && count == 3u);
    REQUIRE(tag_value(b, 50964u, &type, &count) > 0u && type == 10u && count == 9u);
}

} // namespace

int main() {
    auto frame = make_frame(66, 50);
    frame.meta.sourceId = test_source_seal().sourceEvidenceId;
    FrameSource source(frame);
    ResearchEdgeAwareMeasuredPreservingReconstruction reconstruction;

    scientific_master_streaming_binding::v0_2::Result scientific{};
    scientific_master_streaming_binding::v0_2::Options scientificOptions{};
    const auto bound = scientific_master_streaming_binding::v0_2::bind_scientific_master_streaming(
        source, reconstruction, scientificOptions, scientific);
    REQUIRE(static_cast<bool>(bound));
    REQUIRE(scientific.stage2GaugeScanPasses == 2u);

    const auto prepared = prepared_for(frame.meta);
    const auto finalized = finalize_lineage(prepared, scientific);
    REQUIRE(finalized.backplane.scientificMasterHash == scientific.scientificMasterHash);
    REQUIRE(finalized.admission.sourceSeal.sha256 == prepared.source.sha256);

    VectorSink linearSink;
    Result linear{};
    Options linearOptions{};
    linearOptions.role = ProjectionRole::LinearRawCompatibility;
    const auto linearStatus = export_scientific_dng(
        source, reconstruction, prepared, finalized,
        linearSink, linearOptions, linear);
    REQUIRE(static_cast<bool>(linearStatus));
    REQUIRE(linear.finalizedLineageValidated);
    REQUIRE(linear.scientificMasterIdentityMatched);
    REQUIRE(linear.replayedScientificMasterHash == scientific.scientificMasterHash);
    REQUIRE(linear.physicalFrameCount == 1u && linear.independentEvidenceCount == 1u);
    REQUIRE(linear.sourceMetadataBoundColor);
    REQUIRE(!linear.independentPhysicalColor);
    REQUIRE(!linear.fullScientificMasterMaterialized);
    REQUIRE(linear.width == 66u && linear.height == 50u);
    REQUIRE(linear.tileCount == 2u);
    REQUIRE(linear.bytesWritten == linearSink.bytes().size());
    check_common_dng(linearSink.bytes(), 66, 50);
    REQUIRE(tag_value(linearSink.bytes(), 262u) == 34892u);
    REQUIRE(tag_value(linearSink.bytes(), 277u) == 3u);
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    REQUIRE(tag_value(linearSink.bytes(), 258u, &type, &count) > 0u && type == 3u && count == 3u);
    REQUIRE(tag_value(linearSink.bytes(), 339u, &type, &count) > 0u && type == 3u && count == 3u);
    REQUIRE(tag_value(linearSink.bytes(), 50717u, &type, &count) > 0u && type == 4u && count == 3u);

    VectorSink cfaSink;
    Result cfa{};
    Options cfaOptions{};
    cfaOptions.role = ProjectionRole::ReconstructedCfaCompatibility;
    const auto cfaStatus = export_scientific_dng(
        source, reconstruction, prepared, finalized,
        cfaSink, cfaOptions, cfa);
    REQUIRE(static_cast<bool>(cfaStatus));
    REQUIRE(cfa.finalizedLineageValidated);
    REQUIRE(cfa.replayedScientificMasterHash == scientific.scientificMasterHash);
    REQUIRE(cfa.tileCount == 2u);
    check_common_dng(cfaSink.bytes(), 66, 50);
    REQUIRE(tag_value(cfaSink.bytes(), 262u) == 32803u);
    REQUIRE(tag_value(cfaSink.bytes(), 277u) == 1u);
    REQUIRE(tag_value(cfaSink.bytes(), 33421u, &type, &count) != 0u && type == 3u && count == 2u);
    REQUIRE(tag_value(cfaSink.bytes(), 33422u, &type, &count) != 0u && type == 1u && count == 4u);
    REQUIRE(tag_value(cfaSink.bytes(), 50710u, &type, &count) != 0u && type == 1u && count == 3u);

    auto wrongHash = scientific.scientificMasterHash;
    wrongHash[0] ^= 0x01u;
    const auto wrongFinalized = finalize_lineage(prepared, scientific, &wrongHash);
    VectorSink wrongSink;
    Result wrong{};
    const auto wrongStatus = export_scientific_dng(
        source, reconstruction, prepared, wrongFinalized,
        wrongSink, linearOptions, wrong);
    REQUIRE(!static_cast<bool>(wrongStatus));
    REQUIRE(wrongStatus.code ==
            scientific_dng_export::v0_1::StatusCode::ScientificIdentityMismatch);
    REQUIRE(wrongSink.bytes().empty());

    auto mismatchedLineage = finalized;
    mismatchedLineage.admission.tileNativeOptions.color.bindingId = "OTHER_COLOR_BINDING";
    VectorSink mismatchSink;
    Result mismatch{};
    const auto mismatchStatus = export_scientific_dng(
        source, reconstruction, prepared, mismatchedLineage,
        mismatchSink, linearOptions, mismatch);
    REQUIRE(!static_cast<bool>(mismatchStatus));
    REQUIRE(mismatchStatus.code == scientific_dng_export::v0_1::StatusCode::InvalidAuthority);
    REQUIRE(mismatchSink.bytes().empty());

    auto unauthorized = prepared;
    unauthorized.color.authority = scientific_preview_binding_v0_1::ColorBindingAuthority::PreviewSentinel;
    VectorSink unauthorizedSink;
    Result unauthorizedResult{};
    const auto unauthorizedStatus = export_scientific_dng(
        source, reconstruction, unauthorized, finalized,
        unauthorizedSink, linearOptions, unauthorizedResult);
    REQUIRE(!static_cast<bool>(unauthorizedStatus));
    REQUIRE(unauthorizedStatus.code == scientific_dng_export::v0_1::StatusCode::InvalidAuthority);
    REQUIRE(unauthorizedSink.bytes().empty());

    std::cout << "SCIENTIFIC_DNG_EXPORT_V0_1_PASS\n"
              << "linear_bytes=" << linear.bytesWritten << "\n"
              << "cfa_bytes=" << cfa.bytesWritten << "\n"
              << "tiles=" << linear.tileCount << "\n"
              << "phase2_lineage_validated=1\n"
              << "master_identity_match=1\n"
              << "source_bound_not_full_physical=1\n";
    return 0;
}
