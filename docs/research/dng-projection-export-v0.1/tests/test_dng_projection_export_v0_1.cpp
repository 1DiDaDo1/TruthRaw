#include "dng_projection_export_v0_1.h"
#include "scientific_master_streaming_binding_v0_2.h"
#include "streaming_test_support_v0_1.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

namespace {

using truthraw::dng_projection_export::v0_1::AuthorityContext;
using truthraw::dng_projection_export::v0_1::ISequentialByteSink;
using truthraw::dng_projection_export::v0_1::Options;
using truthraw::dng_projection_export::v0_1::ProjectionKind;
using truthraw::dng_projection_export::v0_1::Result;
using truthraw::dng_projection_export::v0_1::StatusCode;

void check(bool ok, const char* expr, int line) {
    if (!ok) {
        std::cerr << "CHECK_FAIL line=" << line << " expr=" << expr << "\n";
        std::exit(2);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

class VectorSink final : public ISequentialByteSink {
public:
    bool writeExact(const void* data, std::size_t bytes) noexcept override {
        const auto* p = static_cast<const std::uint8_t*>(data);
        try {
            data_.insert(data_.end(), p, p + bytes);
            return true;
        } catch (...) {
            return false;
        }
    }
    const std::vector<std::uint8_t>& data() const { return data_; }
private:
    std::vector<std::uint8_t> data_;
};

std::uint16_t u16(const std::vector<std::uint8_t>& b, std::size_t o) {
    CHECK(o + 2u <= b.size());
    return static_cast<std::uint16_t>(b[o]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(b[o + 1u]) << 8u);
}

std::uint32_t u32(const std::vector<std::uint8_t>& b, std::size_t o) {
    CHECK(o + 4u <= b.size());
    return static_cast<std::uint32_t>(b[o]) |
           (static_cast<std::uint32_t>(b[o + 1u]) << 8u) |
           (static_cast<std::uint32_t>(b[o + 2u]) << 16u) |
           (static_cast<std::uint32_t>(b[o + 3u]) << 24u);
}

struct FoundTag {
    bool found = false;
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::uint32_t valueOrOffset = 0;
};

FoundTag find_tag(const std::vector<std::uint8_t>& b, std::uint16_t wanted) {
    CHECK(b.size() >= 10u);
    CHECK(b[0] == 'I' && b[1] == 'I' && u16(b, 2u) == 42u && u32(b, 4u) == 8u);
    const std::uint16_t n = u16(b, 8u);
    for (std::uint16_t i = 0; i < n; ++i) {
        const std::size_t o = 10u + static_cast<std::size_t>(i) * 12u;
        CHECK(o + 12u <= b.size());
        const std::uint16_t id = u16(b, o);
        if (id == wanted) {
            return {true, u16(b, o + 2u), u32(b, o + 4u), u32(b, o + 8u)};
        }
    }
    return {};
}

AuthorityContext make_authority(
    const truthraw::DecodedDngFrame& frame,
    const truthraw::scientific_master_streaming_binding::v0_2::Result& scientific) {
    AuthorityContext a;
    for (std::size_t i = 0; i < a.prepared.source.sha256.size(); ++i) {
        a.prepared.source.sha256[i] = static_cast<std::uint8_t>(0x20u + i);
    }
    a.prepared.source.byteLength = 123456u;
    a.prepared.source.sourceEvidenceId = "sha256:test_projection_source";
    a.prepared.mainHouseComputeAllowed = true;
    a.prepared.sourceBoundAppearanceReleaseAllowed = true;
    a.prepared.scientificPreviewReleaseAllowed = false;
    a.prepared.scientificClaimAllowed = false;
    a.prepared.physicalFrameCount = 1u;
    a.prepared.independentEvidenceCount = 1u;
    a.prepared.eventualClaimScope =
        truthraw::scientific_preview_binding_v0_1::ColorClaimScope::SourceBoundPreview;

    a.color.authority = truthraw::scientific_preview_binding_v0_1::ColorBindingAuthority::SourceMetadataBound;
    a.color.sourceEvidenceId = a.prepared.source.sourceEvidenceId;
    a.color.bindingId = "dngmeta:test_projection_source";
    a.color.cameraToXyzD50 = frame.meta.cameraToXyzD50;
    a.color.normalized = true;
    a.color.validated = true;
    a.color.physicalFrameCount = 1u;
    a.color.independentEvidenceCount = 1u;
    a.prepared.color = a.color;

    a.scientificIdentity = scientific;
    a.phase2.backplane.sourceEvidenceHash = a.prepared.source.sha256;
    a.phase2.backplane.scientificMasterHash = scientific.scientificMasterHash;
    a.phase2.backplane.zeroLineHash.fill(0x31u);
    a.phase2.backplane.sceneScaleHash.fill(0x42u);
    a.phase2.backplane.physicalFrameCount = 1u;
    a.phase2.backplane.independentEvidenceCount = 1u;
    a.phase2.backplane.roomStatus.fill(truthraw::technical_backplane::v0_1::RoomStatus::ResearchOnly);
    a.phase2.backplane.claimStatus = truthraw::technical_backplane::v0_1::ClaimStatus::Candidate;
    a.phase2.backplane.forbiddenFlags = 0u;
    a.phase2.admission.sourceSeal = a.prepared.source;
    a.phase2.admission.claimScope =
        truthraw::scientific_preview_binding_v0_1::ColorClaimScope::SourceBoundPreview;
    return a;
}

} // namespace

int main() {
    auto frame = make_frame(66, 50);
    FrameSource scientificSource(frame);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction reconstruction;
    truthraw::scientific_master_streaming_binding::v0_2::Options scientificOptions;
    scientificOptions.memoryBudgetBytes = 32u * 1024u * 1024u;
    truthraw::scientific_master_streaming_binding::v0_2::Result scientific;
    const auto bound = truthraw::scientific_master_streaming_binding::v0_2::bind_scientific_master_streaming(
        scientificSource, reconstruction, scientificOptions, scientific);
    CHECK(bound);
    CHECK(scientific.masterTilesProcessed > 0u);
    CHECK(scientific.stage2GaugeScanPasses == 2u);

    const AuthorityContext authority = make_authority(frame, scientific);
    Options options;
    options.memoryBudgetBytes = 32u * 1024u * 1024u;
    options.stripRows = 64;

    FrameSource linearSource(frame);
    VectorSink linearSink;
    Result linearResult;
    auto status = truthraw::dng_projection_export::v0_1::export_projection(
        linearSource, reconstruction, authority, ProjectionKind::LinearDng16,
        linearSink, options, linearResult);
    CHECK(status);
    CHECK(linearResult.scientificMasterDigestVerified);
    CHECK(!linearResult.fullScientificMasterMaterialized);
    CHECK(linearResult.compatibilityProjection);
    CHECK(linearResult.physicalFrameCount == 1u);
    CHECK(linearResult.independentEvidenceCount == 1u);
    CHECK(linearResult.bytesWritten == linearSink.data().size());
    CHECK(linearSink.data().size() > 1000u);
    const auto linearPhoto = find_tag(linearSink.data(), 262u);
    const auto linearSpp = find_tag(linearSink.data(), 277u);
    const auto linearForward = find_tag(linearSink.data(), 50964u);
    CHECK(linearPhoto.found && linearPhoto.type == 3u &&
          static_cast<std::uint16_t>(linearPhoto.valueOrOffset & 0xffffu) == 34892u);
    CHECK(linearSpp.found && static_cast<std::uint16_t>(linearSpp.valueOrOffset & 0xffffu) == 3u);
    CHECK(linearForward.found && linearForward.type == 10u && linearForward.count == 9u);

    FrameSource cfaSource(frame);
    VectorSink cfaSink;
    Result cfaResult;
    status = truthraw::dng_projection_export::v0_1::export_projection(
        cfaSource, reconstruction, authority, ProjectionKind::CfaDng16,
        cfaSink, options, cfaResult);
    CHECK(status);
    CHECK(cfaResult.scientificMasterDigestVerified);
    CHECK(cfaResult.compatibilityProjection);
    const auto cfaPhoto = find_tag(cfaSink.data(), 262u);
    const auto cfaSpp = find_tag(cfaSink.data(), 277u);
    const auto cfaPattern = find_tag(cfaSink.data(), 33422u);
    CHECK(cfaPhoto.found && static_cast<std::uint16_t>(cfaPhoto.valueOrOffset & 0xffffu) == 32803u);
    CHECK(cfaSpp.found && static_cast<std::uint16_t>(cfaSpp.valueOrOffset & 0xffffu) == 1u);
    CHECK(cfaPattern.found && cfaPattern.type == 1u && cfaPattern.count == 4u);
    CHECK((cfaPattern.valueOrOffset & 0xffffffffu) == 0x00010102u); // BGGR = 2,1,1,0 little-endian.

    FrameSource rawSensorSource(frame);
    VectorSink rawSensorSink;
    Result rawSensorResult;
    status = truthraw::dng_projection_export::v0_1::export_projection(
        rawSensorSource, reconstruction, authority, ProjectionKind::ScientificRawSensorF32,
        rawSensorSink, options, rawSensorResult);
    CHECK(status);
    CHECK(rawSensorResult.scientificMasterDigestVerified);
    CHECK(!rawSensorResult.compatibilityProjection);
    CHECK(rawSensorSink.data().size() == 128u + 66u * 50u * 3u * sizeof(float));
    const char expectedMagic[8] = {'T','R','R','A','W','S','0','1'};
    CHECK(std::equal(expectedMagic, expectedMagic + 8, rawSensorSink.data().begin()));

    AuthorityContext tampered = authority;
    tampered.scientificIdentity.scientificMasterHash[0] ^= 0x01u;
    FrameSource rejectedSource(frame);
    VectorSink rejectedSink;
    Result rejectedResult;
    status = truthraw::dng_projection_export::v0_1::export_projection(
        rejectedSource, reconstruction, tampered, ProjectionKind::LinearDng16,
        rejectedSink, options, rejectedResult);
    CHECK(!status);
    CHECK(status.code == StatusCode::AuthorityRejected);
    CHECK(rejectedSink.data().empty());

    std::cout << "DNG_PROJECTION_EXPORT_V0_1_PASS\n"
              << "master_tiles=" << scientific.masterTilesProcessed << "\n"
              << "linear_bytes=" << linearResult.bytesWritten << "\n"
              << "cfa_bytes=" << cfaResult.bytesWritten << "\n"
              << "rawsensor_bytes=" << rawSensorResult.bytesWritten << "\n"
              << "digest_verified=1\n"
              << "tampered_identity_rejected=1\n";
    return 0;
}
