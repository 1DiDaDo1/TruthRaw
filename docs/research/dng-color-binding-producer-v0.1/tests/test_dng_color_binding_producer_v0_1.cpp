#include "dng_color_binding_producer_v0_1.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace truthraw;
using namespace truthraw::dng_color_binding_producer_v0_1;
using namespace truthraw::scientific_preview_binding_v0_1;

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

void put16(std::vector<std::uint8_t>& b, std::size_t o, std::uint16_t v) {
    if (b.size() < o + 2) b.resize(o + 2);
    b[o] = static_cast<std::uint8_t>(v);
    b[o+1] = static_cast<std::uint8_t>(v >> 8u);
}

void put32(std::vector<std::uint8_t>& b, std::size_t o, std::uint32_t v) {
    if (b.size() < o + 4) b.resize(o + 4);
    for (int i = 0; i < 4; ++i) b[o + static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(v >> (8*i));
}

std::vector<std::uint8_t> rational(std::initializer_list<std::pair<std::uint32_t,std::uint32_t>> xs) {
    std::vector<std::uint8_t> out(xs.size() * 8u);
    std::size_t i = 0;
    for (const auto [n,d] : xs) { put32(out, 8*i, n); put32(out, 8*i+4, d); ++i; }
    return out;
}

std::vector<std::uint8_t> srational(std::initializer_list<std::pair<std::int32_t,std::int32_t>> xs) {
    std::vector<std::uint8_t> out(xs.size() * 8u);
    std::size_t i = 0;
    for (const auto [n,d] : xs) {
        put32(out, 8*i, static_cast<std::uint32_t>(n));
        put32(out, 8*i+4, static_cast<std::uint32_t>(d));
        ++i;
    }
    return out;
}

struct Entry {
    std::uint16_t tag = 0;
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::vector<std::uint8_t> data;
};

struct FixtureOptions {
    bool forward = true;
    bool includeNeutral = true;
    bool includeSecondCalibration = false;
    bool cameraCalibration = false;
    bool matchingSignatures = true;
    bool singularColorMatrix = false;
};

std::vector<std::uint8_t> make_fixture(const FixtureOptions& opt) {
    constexpr std::uint16_t RATIONAL = 5;
    constexpr std::uint16_t SRATIONAL = 10;
    constexpr std::uint16_t ASCII = 2;
    std::vector<Entry> entries;
    auto add = [&](std::uint16_t tag, std::uint16_t type, std::uint32_t count, std::vector<std::uint8_t> data) {
        entries.push_back({tag,type,count,std::move(data)});
    };

    if (!opt.singularColorMatrix) {
        add(50721, SRATIONAL, 9, srational({{1,1},{0,1},{0,1},{0,1},{1,1},{0,1},{0,1},{0,1},{1,1}}));
    } else {
        add(50721, SRATIONAL, 9, srational({{1,1},{0,1},{0,1},{1,1},{0,1},{0,1},{0,1},{0,1},{1,1}}));
    }
    if (opt.includeNeutral) add(50728, RATIONAL, 3, rational({{1,1},{1,1},{1,1}}));
    if (opt.forward) {
        add(50964, SRATIONAL, 9, srational({{96422,100000},{0,1},{0,1},{0,1},{1,1},{0,1},{0,1},{0,1},{82521,100000}}));
    }
    if (opt.cameraCalibration) {
        add(50723, SRATIONAL, 9, srational({{2,1},{0,1},{0,1},{0,1},{1,1},{0,1},{0,1},{0,1},{1,1}}));
        std::string camera = "truthraw-camera";
        std::string profile = opt.matchingSignatures ? camera : "different-profile";
        camera.push_back('\0'); profile.push_back('\0');
        add(50931, ASCII, static_cast<std::uint32_t>(camera.size()), {camera.begin(), camera.end()});
        add(50932, ASCII, static_cast<std::uint32_t>(profile.size()), {profile.begin(), profile.end()});
    }
    if (opt.includeSecondCalibration) {
        add(50722, SRATIONAL, 9, srational({{1,1},{0,1},{0,1},{0,1},{1,1},{0,1},{0,1},{0,1},{1,1}}));
    }

    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b){ return a.tag < b.tag; });
    const std::size_t ifdBytes = 2u + entries.size() * 12u + 4u;
    std::vector<std::uint8_t> out(8u + ifdBytes, 0u);
    out[0] = 'I'; out[1] = 'I'; put16(out, 2, 42); put32(out, 4, 8); put16(out, 8, static_cast<std::uint16_t>(entries.size()));
    std::size_t external = out.size();
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        const std::size_t q = 10u + 12u*i;
        put16(out, q, e.tag); put16(out, q+2, e.type); put32(out, q+4, e.count);
        if (e.data.size() <= 4u) {
            std::copy(e.data.begin(), e.data.end(), out.begin() + static_cast<std::ptrdiff_t>(q+8));
        } else {
            put32(out, q+8, static_cast<std::uint32_t>(external));
            out.insert(out.end(), e.data.begin(), e.data.end());
            external = out.size();
        }
    }
    return out;
}

SourceSeal seal(MemSource& source) {
    SourceSeal out;
    const auto status = seal_source_sha256(source, out, 1024);
    REQUIRE(status);
    return out;
}

technical_backplane::v0_1::State make_backplane(const SourceSeal& source) {
    technical_backplane::v0_1::State state;
    state.sourceEvidenceHash = source.sha256;
    state.scientificMasterHash.fill(0x11);
    state.zeroLineHash.fill(0x22);
    state.sceneScaleHash.fill(0x33);
    state.physicalFrameCount = 1;
    state.independentEvidenceCount = 1;
    state.roomStatus.fill(technical_backplane::v0_1::RoomStatus::Available);
    state.claimStatus = technical_backplane::v0_1::ClaimStatus::Candidate;
    return state;
}

bool near(float a, double b, double eps = 2.0e-5) { return std::abs(static_cast<double>(a) - b) <= eps; }

void test_forward_matrix_path_and_admission() {
    MemSource source(make_fixture({}));
    const auto sourceSeal = seal(source);
    ProducerResult result;
    const auto status = produce_source_metadata_color_binding(source, sourceSeal, result);
    REQUIRE(status);
    REQUIRE(result.audit.usedForwardMatrix);
    REQUIRE(result.audit.sourceVerifiedBeforeParse);
    REQUIRE(result.audit.sourceVerifiedAfterParse);
    REQUIRE(!result.audit.secondOrThirdCalibrationSeen);
    REQUIRE(result.color.authority == ColorBindingAuthority::SourceMetadataBound);
    REQUIRE(result.color.sourceEvidenceId == sourceSeal.sourceEvidenceId);
    REQUIRE(result.color.validated);
    REQUIRE(result.color.physicalFrameCount == 1);
    REQUIRE(result.color.independentEvidenceCount == 1);
    REQUIRE(near(result.color.cameraToXyzD50[0], 0.96422));
    REQUIRE(near(result.color.cameraToXyzD50[4], 1.0));
    REQUIRE(near(result.color.cameraToXyzD50[8], 0.82521));

    ScientificPreviewAdmission admitted;
    const auto admission = admit_scientific_color_preview(sourceSeal, result.color, make_backplane(sourceSeal), admitted);
    REQUIRE(admission);
    REQUIRE(admitted.claimScope == ColorClaimScope::SourceBoundPreview);
    REQUIRE(admitted.tileNativeOptions.sourceEvidenceId == sourceSeal.sourceEvidenceId);
    REQUIRE(admitted.tileNativeOptions.color.valid);
    REQUIRE(admitted.tileNativeOptions.color.bindingId == result.color.bindingId);
}

void test_color_matrix_bradford_path() {
    FixtureOptions opt; opt.forward = false;
    MemSource source(make_fixture(opt));
    const auto sourceSeal = seal(source);
    ProducerResult result;
    const auto status = produce_source_metadata_color_binding(source, sourceSeal, result);
    REQUIRE(status);
    REQUIRE(!result.audit.usedForwardMatrix);
    for (const float v : result.color.cameraToXyzD50) REQUIRE(std::isfinite(v));
    REQUIRE(std::abs(result.color.cameraToXyzD50[0] - 1.0f) > 1.0e-3f);
}

void test_camera_calibration_signature_mismatch_uses_identity() {
    FixtureOptions opt; opt.cameraCalibration = true; opt.matchingSignatures = false;
    MemSource source(make_fixture(opt));
    const auto sourceSeal = seal(source);
    ProducerResult result;
    REQUIRE(produce_source_metadata_color_binding(source, sourceSeal, result));
    REQUIRE(result.audit.cameraCalibrationPresent);
    REQUIRE(!result.audit.cameraCalibrationSignatureMatched);
    REQUIRE(!result.audit.cameraCalibrationApplied);
    REQUIRE(near(result.color.cameraToXyzD50[0], 0.96422));
}

void test_matching_camera_calibration_is_applied() {
    FixtureOptions opt; opt.cameraCalibration = true; opt.matchingSignatures = true;
    MemSource source(make_fixture(opt));
    const auto sourceSeal = seal(source);
    ProducerResult result;
    REQUIRE(produce_source_metadata_color_binding(source, sourceSeal, result));
    REQUIRE(result.audit.cameraCalibrationPresent);
    REQUIRE(result.audit.cameraCalibrationSignatureMatched);
    REQUIRE(result.audit.cameraCalibrationApplied);
}

void test_multiple_calibrations_fail_closed() {
    FixtureOptions opt; opt.includeSecondCalibration = true;
    MemSource source(make_fixture(opt));
    const auto sourceSeal = seal(source);
    ProducerResult result;
    const auto status = produce_source_metadata_color_binding(source, sourceSeal, result);
    REQUIRE(!status);
    REQUIRE(status.code == ProducerStatusCode::MultipleCalibrationsUnsupported);
}

void test_missing_neutral_fails_closed() {
    FixtureOptions opt; opt.includeNeutral = false;
    MemSource source(make_fixture(opt));
    const auto sourceSeal = seal(source);
    ProducerResult result;
    const auto status = produce_source_metadata_color_binding(source, sourceSeal, result);
    REQUIRE(!status);
    REQUIRE(status.code == ProducerStatusCode::MissingAsShotNeutral);
}

void test_singular_color_matrix_fails_closed_without_forward() {
    FixtureOptions opt; opt.forward = false; opt.singularColorMatrix = true;
    MemSource source(make_fixture(opt));
    const auto sourceSeal = seal(source);
    ProducerResult result;
    const auto status = produce_source_metadata_color_binding(source, sourceSeal, result);
    REQUIRE(!status);
    REQUIRE(status.code == ProducerStatusCode::SingularMatrix);
}

void test_source_tamper_is_rejected() {
    MemSource source(make_fixture({}));
    const auto sourceSeal = seal(source);
    REQUIRE(source.bytes.size() > 32u);
    source.bytes.back() ^= 0x01u;
    ProducerResult result;
    const auto status = produce_source_metadata_color_binding(source, sourceSeal, result);
    REQUIRE(!status);
    REQUIRE(status.code == ProducerStatusCode::SourceSealMismatch);
}

} // namespace

int main() {
    try {
        test_forward_matrix_path_and_admission();
        test_color_matrix_bradford_path();
        test_camera_calibration_signature_mismatch_uses_identity();
        test_matching_camera_calibration_is_applied();
        test_multiple_calibrations_fail_closed();
        test_missing_neutral_fails_closed();
        test_singular_color_matrix_fails_closed_without_forward();
        test_source_tamper_is_rejected();
        std::cout << "DNG_COLOR_BINDING_PRODUCER_V0_1_PASS\n";
        std::cout << "authority=SOURCE_METADATA_BOUND\n";
        std::cout << "multi_calibration=AUTO_DISABLED_FAIL_CLOSED\n";
        std::cout << "source_reverify=BEFORE_AND_AFTER_PARSE\n";
        std::cout << "physical_frame_count=1 independent_evidence_count=1\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
