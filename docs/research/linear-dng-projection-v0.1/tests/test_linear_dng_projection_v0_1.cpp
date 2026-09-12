#include "linear_dng_projection_v0_1.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>

using namespace truthraw;
using namespace truthraw::linear_dng_projection::v0_1;

namespace {

#define CHECK_TRUE(expr) do { if (!(expr)) { \
    std::cerr << "CHECK failed at line " << __LINE__ << ": " #expr "\n"; return 1; } } while (0)

std::uint16_t u16(const std::vector<std::uint8_t>& b, std::size_t o) {
    return static_cast<std::uint16_t>(b.at(o)) |
           (static_cast<std::uint16_t>(b.at(o + 1)) << 8u);
}

std::uint32_t u32(const std::vector<std::uint8_t>& b, std::size_t o) {
    return static_cast<std::uint32_t>(b.at(o)) |
           (static_cast<std::uint32_t>(b.at(o + 1)) << 8u) |
           (static_cast<std::uint32_t>(b.at(o + 2)) << 16u) |
           (static_cast<std::uint32_t>(b.at(o + 3)) << 24u);
}

std::int32_t i32(const std::vector<std::uint8_t>& b, std::size_t o) {
    return static_cast<std::int32_t>(u32(b, o));
}

struct IfdValue {
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::uint32_t value = 0;
};

std::map<std::uint16_t, IfdValue> parse_ifd(const std::vector<std::uint8_t>& b) {
    std::map<std::uint16_t, IfdValue> out;
    const std::uint32_t ifd = u32(b, 4);
    const std::uint16_t n = u16(b, ifd);
    std::size_t p = ifd + 2u;
    for (std::uint16_t k = 0; k < n; ++k, p += 12u) {
        out[u16(b, p)] = {u16(b, p + 2u), u32(b, p + 4u), u32(b, p + 8u)};
    }
    return out;
}

class VectorSink final : public ISequentialByteSink {
public:
    Status append(const std::uint8_t* data, std::size_t size) override {
        if (data == nullptr && size != 0u) {
            return Status::error(StatusCode::SinkFailed, "null write");
        }
        bytes.insert(bytes.end(), data, data + size);
        return Status::ok();
    }
    std::uint64_t bytesWritten() const noexcept override { return bytes.size(); }
    std::vector<std::uint8_t> bytes;
};

class PatternSource final : public ICameraRgbTileSource {
public:
    explicit PatternSource(bool nan = false) : nan_(nan) {}
    int width() const noexcept override { return 3; }
    int height() const noexcept override { return 2; }
    Orientation orientation() const noexcept override { return Orientation::Normal; }

    Status readCameraRgbTile(int x0, int y0, int x1, int y1,
                             float* out, std::size_t count) override {
        const std::size_t expected = static_cast<std::size_t>(x1 - x0) *
                                     static_cast<std::size_t>(y1 - y0) * 3u;
        if (!out || count != expected) {
            return Status::error(StatusCode::SourceFailed, "bad destination");
        }
        std::size_t i = 0;
        for (int y = y0; y < y1; ++y) {
            for (int x = x0; x < x1; ++x) {
                const int pixel = y * width() + x;
                float r = 0.1f * static_cast<float>(pixel + 1);
                float g = 0.5f;
                float b = 0.75f;
                if (x == 0 && y == 0) r = -0.25f;
                if (x == 2 && y == 1) b = 1.25f;
                if (nan_ && x == 1 && y == 0) g = std::numeric_limits<float>::quiet_NaN();
                out[i++] = r;
                out[i++] = g;
                out[i++] = b;
            }
        }
        return Status::ok();
    }
private:
    bool nan_ = false;
};

ProjectionAdmission direct_admission() {
    ProjectionAdmission a;
    a.source.sha256.fill(0x11u);
    a.source.byteLength = 1234;
    a.source.sourceEvidenceId = "sha256:1111111111111111111111111111111111111111111111111111111111111111";
    a.scientificMasterHash.fill(0x22u);
    a.cameraToXyzD50 = {1.f,0.f,0.f, 0.f,1.f,0.f, 0.f,0.f,1.f};
    a.colorAuthority = scientific_preview_binding_v0_1::ColorBindingAuthority::SourceMetadataBound;
    a.strongerPhysicalColorClaim = false;
    return a;
}

void make_finalized_fixture(
    scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    finalized_scientific_preview_release::v0_2::ReleaseResult& release) {
    prepared = {};
    release = {};
    prepared.source.sha256.fill(0x31u);
    prepared.source.byteLength = 4096;
    prepared.source.sourceEvidenceId =
        "sha256:3131313131313131313131313131313131313131313131313131313131313131";
    prepared.color.authority =
        scientific_preview_binding_v0_1::ColorBindingAuthority::SourceMetadataBound;
    prepared.color.sourceEvidenceId = prepared.source.sourceEvidenceId;
    prepared.color.bindingId = "fixture-color";
    prepared.color.cameraToXyzD50 = {1.f,0.f,0.f, 0.f,1.f,0.f, 0.f,0.f,1.f};
    prepared.color.validated = true;
    prepared.mainHouseComputeAllowed = true;
    prepared.physicalFrameCount = 1;
    prepared.independentEvidenceCount = 1;

    release.authority = finalized_scientific_preview_release::v0_2::
        PreviewAuthority::FinalizedSourceBoundScientificPreview;
    release.scientificIdentity.scientificMasterHash.fill(0x52u);
    release.scientificIdentity.physicalFrameCount = 1;
    release.scientificIdentity.independentEvidenceCount = 1;
    release.canonicalPhase2.admission.sourceSeal = prepared.source;
    release.canonicalPhase2.backplane.sourceEvidenceHash = prepared.source.sha256;
    release.canonicalPhase2.backplane.scientificMasterHash =
        release.scientificIdentity.scientificMasterHash;
    release.canonicalPhase2.backplane.physicalFrameCount = 1;
    release.canonicalPhase2.backplane.independentEvidenceCount = 1;
}

int test_admission() {
    scientific_preview_binding_v0_2::PreparedScientificPreviewSource prepared;
    finalized_scientific_preview_release::v0_2::ReleaseResult release;
    make_finalized_fixture(prepared, release);

    ProjectionAdmission admitted;
    auto s = admit_linear_dng_projection(prepared, release, admitted);
    CHECK_TRUE(static_cast<bool>(s));
    CHECK_TRUE(admitted.source.sha256 == prepared.source.sha256);
    CHECK_TRUE(admitted.scientificMasterHash == release.scientificIdentity.scientificMasterHash);
    CHECK_TRUE(!admitted.strongerPhysicalColorClaim);
    CHECK_TRUE(admitted.colorAuthority ==
               scientific_preview_binding_v0_1::ColorBindingAuthority::SourceMetadataBound);

    release.canonicalPhase2.backplane.scientificMasterHash[0] ^= 1u;
    s = admit_linear_dng_projection(prepared, release, admitted);
    CHECK_TRUE(s.code == StatusCode::ScientificIdentityMismatch);

    make_finalized_fixture(prepared, release);
    release.authority = finalized_scientific_preview_release::v0_2::
        PreviewAuthority::FinalizedIndependentlyCalibratedScientificPreview;
    prepared.color.authority = scientific_preview_binding_v0_1::
        ColorBindingAuthority::IndependentCalibration;
    s = admit_linear_dng_projection(prepared, release, admitted);
    CHECK_TRUE(static_cast<bool>(s));
    CHECK_TRUE(admitted.strongerPhysicalColorClaim);
    return 0;
}

int test_dng_writer() {
    PatternSource source;
    VectorSink sink;
    auto admission = direct_admission();
    Options options;
    options.tileEdge = 2;
    Audit audit;
    const auto s = write_linear_dng(source, sink, admission, options, audit);
    CHECK_TRUE(static_cast<bool>(s));
    CHECK_TRUE(audit.cameraRgbSamplesRead == 18u);
    CHECK_TRUE(audit.negativeSamplesClamped == 1u);
    CHECK_TRUE(audit.overrangeSamplesClamped == 1u);
    CHECK_TRUE(audit.quantizedSamples == 18u);
    CHECK_TRUE(audit.tilesWritten == 2u);
    CHECK_TRUE(audit.outputBytes == sink.bytes.size());
    CHECK_TRUE(audit.boundedCompatibilityProjection);
    CHECK_TRUE(!audit.scientificMasterModified);
    CHECK_TRUE(!audit.createsEvidence);
    CHECK_TRUE(!audit.fullFrameMaterialized);
    CHECK_TRUE(audit.sourceMetadataColorOnly);
    CHECK_TRUE(!audit.strongerPhysicalColorClaim);

    const auto& b = sink.bytes;
    CHECK_TRUE(b.size() > 300u);
    CHECK_TRUE(b[0] == 'I' && b[1] == 'I');
    CHECK_TRUE(u16(b, 2) == 42u);
    CHECK_TRUE(u32(b, 4) == 8u);
    CHECK_TRUE(u16(b, 8) == 23u);

    const auto ifd = parse_ifd(b);
    CHECK_TRUE(ifd.at(256).value == 3u);
    CHECK_TRUE(ifd.at(257).value == 2u);
    CHECK_TRUE(ifd.at(259).value == 1u);
    CHECK_TRUE(ifd.at(262).value == 34892u);
    CHECK_TRUE(ifd.at(274).value == 1u);
    CHECK_TRUE(ifd.at(277).value == 3u);
    CHECK_TRUE(ifd.at(284).value == 1u);
    CHECK_TRUE(ifd.at(322).value == 2u);
    CHECK_TRUE(ifd.at(323).value == 2u);
    CHECK_TRUE(ifd.at(324).count == 2u);
    CHECK_TRUE(ifd.at(325).count == 2u);
    CHECK_TRUE(ifd.at(50706).value == 0x00000401u);
    CHECK_TRUE(ifd.at(50707).value == 0x00000401u);
    CHECK_TRUE(ifd.at(50778).value == 23u);

    const auto bitsOff = ifd.at(258).value;
    CHECK_TRUE(u16(b, bitsOff) == 16u && u16(b, bitsOff + 2u) == 16u &&
               u16(b, bitsOff + 4u) == 16u);
    const auto sampleOff = ifd.at(339).value;
    CHECK_TRUE(u16(b, sampleOff) == 1u && u16(b, sampleOff + 2u) == 1u &&
               u16(b, sampleOff + 4u) == 1u);
    const auto whiteOff = ifd.at(50717).value;
    CHECK_TRUE(u16(b, whiteOff) == 65535u && u16(b, whiteOff + 2u) == 65535u &&
               u16(b, whiteOff + 4u) == 65535u);

    const auto cmOff = ifd.at(50721).value;
    for (int i = 0; i < 9; ++i) {
        const double value = static_cast<double>(i32(b, cmOff + i * 8u)) /
                             static_cast<double>(i32(b, cmOff + i * 8u + 4u));
        const double expected = (i == 0 || i == 4 || i == 8) ? 1.0 : 0.0;
        CHECK_TRUE(std::abs(value - expected) < 1.0e-6);
    }
    const auto neutralOff = ifd.at(50728).value;
    const double n0 = static_cast<double>(u32(b, neutralOff)) /
                      static_cast<double>(u32(b, neutralOff + 4u));
    const double n1 = static_cast<double>(u32(b, neutralOff + 8u)) /
                      static_cast<double>(u32(b, neutralOff + 12u));
    const double n2 = static_cast<double>(u32(b, neutralOff + 16u)) /
                      static_cast<double>(u32(b, neutralOff + 20u));
    CHECK_TRUE(std::abs(n0 - 0.96422) < 2.0e-6);
    CHECK_TRUE(std::abs(n1 - 1.0) < 1.0e-6);
    CHECK_TRUE(std::abs(n2 - 0.82521) < 2.0e-6);

    const auto tileOffsetsTable = ifd.at(324).value;
    const auto tile0 = u32(b, tileOffsetsTable);
    const auto tile1 = u32(b, tileOffsetsTable + 4u);
    CHECK_TRUE(tile1 > tile0);
    CHECK_TRUE(u16(b, tile0) == 0u); // negative R clamped
    // Tile 1, valid pixel (x=2,y=1), blue channel is at padded tile row 1, col 0.
    const std::size_t highSampleByte = tile1 + ((1u * 2u + 0u) * 3u + 2u) * 2u;
    CHECK_TRUE(u16(b, highSampleByte) == 65535u);

    const auto desc = ifd.at(270);
    const std::string descText(reinterpret_cast<const char*>(b.data() + desc.value));
    CHECK_TRUE(descText.find("LINEAR_DNG_COMPATIBILITY_PROJECTION") != std::string::npos);
    CHECK_TRUE(descText.find("creates_evidence=0") != std::string::npos);
    return 0;
}

int test_fail_closed() {
    auto admission = direct_admission();
    PatternSource source;
    VectorSink sink;
    Options options;
    options.tileEdge = 2;
    options.clampToLinearReferenceRange = false;
    Audit audit;
    auto s = write_linear_dng(source, sink, admission, options, audit);
    CHECK_TRUE(s.code == StatusCode::InvalidArgument);

    PatternSource nanSource(true);
    VectorSink sink2;
    options.clampToLinearReferenceRange = true;
    s = write_linear_dng(nanSource, sink2, admission, options, audit);
    CHECK_TRUE(s.code == StatusCode::NonFiniteSample);

    VectorSink sink3;
    admission.cameraToXyzD50 = {1.f,0.f,0.f, 1.f,0.f,0.f, 0.f,0.f,1.f};
    s = write_linear_dng(source, sink3, admission, options, audit);
    CHECK_TRUE(s.code == StatusCode::SingularColorMatrix);
    return 0;
}

} // namespace

int main() {
    if (test_admission() != 0) return 1;
    if (test_dng_writer() != 0) return 1;
    if (test_fail_closed() != 0) return 1;
    std::cout << "LINEAR_DNG_PROJECTION_V0_1_PASS\n";
    std::cout << "role=LINEAR_DNG_COMPATIBILITY_PROJECTION\n";
    std::cout << "creates_evidence=0\n";
    std::cout << "scientific_master_modified=0\n";
    return 0;
}
