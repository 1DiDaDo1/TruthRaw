#include "dng_color_binding_producer_v0_2.h"
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
using namespace truthraw::scientific_preview_binding_v0_1;
namespace v1 = truthraw::dng_color_binding_producer_v0_1;
namespace v2 = truthraw::dng_color_binding_producer_v0_2;

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
    for (int i = 0; i < 4; ++i) {
        b[o + static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(v >> (8*i));
    }
}

std::vector<std::uint8_t> rational(
    std::initializer_list<std::pair<std::uint32_t,std::uint32_t>> xs) {
    std::vector<std::uint8_t> out(xs.size() * 8u);
    std::size_t i = 0;
    for (const auto& [n,d] : xs) {
        put32(out, 8*i, n);
        put32(out, 8*i+4, d);
        ++i;
    }
    return out;
}

std::vector<std::uint8_t> srational(
    std::initializer_list<std::pair<std::int32_t,std::int32_t>> xs) {
    std::vector<std::uint8_t> out(xs.size() * 8u);
    std::size_t i = 0;
    for (const auto& [n,d] : xs) {
        put32(out, 8*i, static_cast<std::uint32_t>(n));
        put32(out, 8*i+4, static_cast<std::uint32_t>(d));
        ++i;
    }
    return out;
}

std::vector<std::uint8_t> short_value(std::uint16_t value) {
    std::vector<std::uint8_t> out(2u, 0u);
    put16(out, 0, value);
    return out;
}

struct Entry {
    std::uint16_t tag = 0;
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::vector<std::uint8_t> data;
};

struct FixtureOptions {
    bool dual = false;
    bool swapDualOrder = false;
    bool includeForward1 = true;
    bool includeForward2 = true;
    bool includeIlluminant2 = true;
    bool includeColorMatrix2 = true;
    bool triple = false;
    bool customSecondIlluminant = false;
    bool dualCameraCalibration = false;
    bool matchingSignatures = true;
};

std::vector<std::uint8_t> identity_matrix() {
    return srational({
        {1,1},{0,1},{0,1},
        {0,1},{1,1},{0,1},
        {0,1},{0,1},{1,1}});
}

std::vector<std::uint8_t> forward_low() {
    return srational({
        {90000,100000},{6422,100000},{0,1},
        {0,1},{1,1},{0,1},
        {0,1},{0,1},{82521,100000}});
}

std::vector<std::uint8_t> forward_high() {
    return srational({
        {80000,100000},{16422,100000},{0,1},
        {5000,100000},{95000,100000},{0,1},
        {0,1},{10000,100000},{72521,100000}});
}

std::vector<std::uint8_t> make_fixture(const FixtureOptions& opt) {
    constexpr std::uint16_t SHORT = 3;
    constexpr std::uint16_t RATIONAL = 5;
    constexpr std::uint16_t SRATIONAL = 10;
    constexpr std::uint16_t ASCII = 2;

    std::vector<Entry> entries;
    auto add = [&](std::uint16_t tag,
                   std::uint16_t type,
                   std::uint32_t count,
                   std::vector<std::uint8_t> data) {
        entries.push_back({tag,type,count,std::move(data)});
    };

    add(50721, SRATIONAL, 9, identity_matrix());

    // D55 XYZ (Y=1) as AsShotNeutral. With identity endpoint color matrices
    // NeutralToXY resolves directly to the D55 chromaticity, making the
    // inverse-temperature interpolation weight independently testable.
    add(50728, RATIONAL, 3, rational({
        {956797,1000000},
        {1,1},
        {921481,1000000}}));

    if (!opt.dual) {
        if (opt.includeForward1) {
            add(50964, SRATIONAL, 9, srational({
                {96422,100000},{0,1},{0,1},
                {0,1},{1,1},{0,1},
                {0,1},{0,1},{82521,100000}}));
        }
    } else {
        const std::uint16_t illuminant1 = opt.swapDualOrder ? 21u : 23u; // D65 / D50
        std::uint16_t illuminant2 = opt.swapDualOrder ? 23u : 21u;       // D50 / D65
        if (opt.customSecondIlluminant) illuminant2 = 255u;

        add(50778, SHORT, 1, short_value(illuminant1));
        if (opt.includeIlluminant2) add(50779, SHORT, 1, short_value(illuminant2));
        if (opt.includeColorMatrix2) add(50722, SRATIONAL, 9, identity_matrix());

        if (opt.includeForward1) {
            add(50964, SRATIONAL, 9,
                opt.swapDualOrder ? forward_high() : forward_low());
        }
        if (opt.includeForward2) {
            add(50965, SRATIONAL, 9,
                opt.swapDualOrder ? forward_low() : forward_high());
        }

        if (opt.dualCameraCalibration) {
            add(50723, SRATIONAL, 9, srational({
                {11,10},{0,1},{0,1},
                {0,1},{1,1},{0,1},
                {0,1},{0,1},{1,1}}));
            add(50724, SRATIONAL, 9, srational({
                {9,10},{0,1},{0,1},
                {0,1},{1,1},{0,1},
                {0,1},{0,1},{1,1}}));
            std::string camera = "truthraw-camera";
            std::string profile = opt.matchingSignatures ? camera : "different-profile";
            camera.push_back('\0');
            profile.push_back('\0');
            add(50931, ASCII, static_cast<std::uint32_t>(camera.size()),
                {camera.begin(), camera.end()});
            add(50932, ASCII, static_cast<std::uint32_t>(profile.size()),
                {profile.begin(), profile.end()});
        }
    }

    if (opt.triple) {
        add(52531, SRATIONAL, 9, identity_matrix());
    }

    std::sort(entries.begin(), entries.end(),
              [](const Entry& a, const Entry& b){ return a.tag < b.tag; });
    const std::size_t ifdBytes = 2u + entries.size() * 12u + 4u;
    std::vector<std::uint8_t> out(8u + ifdBytes, 0u);
    out[0] = 'I';
    out[1] = 'I';
    put16(out, 2, 42);
    put32(out, 4, 8);
    put16(out, 8, static_cast<std::uint16_t>(entries.size()));

    std::size_t external = out.size();
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        const std::size_t q = 10u + 12u*i;
        put16(out, q, e.tag);
        put16(out, q+2, e.type);
        put32(out, q+4, e.count);
        if (e.data.size() <= 4u) {
            std::copy(e.data.begin(), e.data.end(),
                      out.begin() + static_cast<std::ptrdiff_t>(q+8));
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

bool near(double a, double b, double eps = 2.0e-5) {
    return std::abs(a - b) <= eps;
}

void test_single_illuminant_is_exact_v01_delegate() {
    FixtureOptions opt;
    opt.dual = false;
    MemSource source(make_fixture(opt));
    const auto sourceSeal = seal(source);

    v1::ProducerResult oldResult;
    REQUIRE(v1::produce_source_metadata_color_binding(source, sourceSeal, oldResult));

    v2::ProducerResult newResult;
    REQUIRE(v2::produce_source_metadata_color_binding(source, sourceSeal, newResult));
    REQUIRE(newResult.audit.delegatedSingleIlluminantV01);
    REQUIRE(!newResult.audit.dualIlluminantUsed);
    REQUIRE(newResult.color.bindingId == oldResult.color.bindingId);
    REQUIRE(newResult.color.sourceEvidenceId == oldResult.color.sourceEvidenceId);
    REQUIRE(newResult.color.cameraToXyzD50 == oldResult.color.cameraToXyzD50);
    REQUIRE(newResult.color.authority == oldResult.color.authority);
}

void test_dual_d50_d65_interpolates_inverse_temperature() {
    FixtureOptions opt;
    opt.dual = true;
    MemSource source(make_fixture(opt));
    const auto sourceSeal = seal(source);

    v2::ProducerResult result;
    const auto status = v2::produce_source_metadata_color_binding(source, sourceSeal, result);
    if (!status) std::cerr << "dual status=" << v2::status_name(status.code) << " " << status.message << '\n';
    REQUIRE(status);
    REQUIRE(result.audit.dualIlluminantUsed);
    REQUIRE(!result.audit.delegatedSingleIlluminantV01);
    REQUIRE(result.audit.sourceVerifiedBeforeParse);
    REQUIRE(result.audit.sourceVerifiedAfterParse);
    REQUIRE(result.audit.calibrationIlluminant1 == 23u);
    REQUIRE(result.audit.calibrationIlluminant2 == 21u);
    REQUIRE(near(result.audit.calibrationTemperatureLowK, 5000.0));
    REQUIRE(near(result.audit.calibrationTemperatureHighK, 6500.0));
    REQUIRE(result.audit.resolvedWhiteTemperatureK > 5400.0);
    REQUIRE(result.audit.resolvedWhiteTemperatureK < 5600.0);
    const double expected =
        (1.0 / result.audit.resolvedWhiteTemperatureK - 1.0 / 6500.0) /
        (1.0 / 5000.0 - 1.0 / 6500.0);
    REQUIRE(near(result.audit.interpolationWeightLow, expected, 1.0e-10));
    REQUIRE(result.audit.interpolationWeightLow > 0.55);
    REQUIRE(result.audit.interpolationWeightLow < 0.67);
    REQUIRE(result.audit.neutralSolveIterations >= 2u);
    REQUIRE(result.audit.neutralSolveIterations <= 30u);
    REQUIRE(result.audit.usedForwardMatrix);
    REQUIRE(!result.audit.usedSingleForwardMatrixAcrossTemperatures);
    REQUIRE(result.color.authority == ColorBindingAuthority::SourceMetadataBound);
    REQUIRE(result.color.bindingId.find("dng-ifd0-source-metadata-v0.2:dual-ict:") == 0u);
    REQUIRE(result.color.physicalFrameCount == 1u);
    REQUIRE(result.color.independentEvidenceCount == 1u);
    for (float value : result.color.cameraToXyzD50) REQUIRE(std::isfinite(value));

    ScientificPreviewAdmission admission;
    REQUIRE(admit_scientific_color_preview(
        sourceSeal, result.color, make_backplane(sourceSeal), admission));
    REQUIRE(admission.claimScope == ColorClaimScope::SourceBoundPreview);
}

void test_dual_order_swap_is_invariant() {
    FixtureOptions normalOpt;
    normalOpt.dual = true;
    MemSource normal(make_fixture(normalOpt));
    const auto normalSeal = seal(normal);
    v2::ProducerResult a;
    REQUIRE(v2::produce_source_metadata_color_binding(normal, normalSeal, a));

    FixtureOptions swappedOpt = normalOpt;
    swappedOpt.swapDualOrder = true;
    MemSource swapped(make_fixture(swappedOpt));
    const auto swappedSeal = seal(swapped);
    v2::ProducerResult b;
    REQUIRE(v2::produce_source_metadata_color_binding(swapped, swappedSeal, b));

    REQUIRE(near(a.audit.calibrationTemperatureLowK, b.audit.calibrationTemperatureLowK));
    REQUIRE(near(a.audit.calibrationTemperatureHighK, b.audit.calibrationTemperatureHighK));
    REQUIRE(near(a.audit.resolvedWhiteTemperatureK, b.audit.resolvedWhiteTemperatureK, 1.0e-6));
    REQUIRE(near(a.audit.interpolationWeightLow, b.audit.interpolationWeightLow, 1.0e-10));
    for (std::size_t i = 0; i < 9; ++i) {
        REQUIRE(near(a.color.cameraToXyzD50[i], b.color.cameraToXyzD50[i], 2.0e-6));
    }
}

void test_one_forward_matrix_is_explicitly_audited() {
    FixtureOptions opt;
    opt.dual = true;
    opt.includeForward2 = false;
    MemSource source(make_fixture(opt));
    const auto sourceSeal = seal(source);
    v2::ProducerResult result;
    REQUIRE(v2::produce_source_metadata_color_binding(source, sourceSeal, result));
    REQUIRE(result.audit.usedForwardMatrix);
    REQUIRE(result.audit.usedSingleForwardMatrixAcrossTemperatures);
}

void test_dual_camera_calibration_signature_rule() {
    FixtureOptions mismatchOpt;
    mismatchOpt.dual = true;
    mismatchOpt.dualCameraCalibration = true;
    mismatchOpt.matchingSignatures = false;
    MemSource mismatch(make_fixture(mismatchOpt));
    const auto mismatchSeal = seal(mismatch);
    v2::ProducerResult mismatchResult;
    REQUIRE(v2::produce_source_metadata_color_binding(mismatch, mismatchSeal, mismatchResult));
    REQUIRE(mismatchResult.audit.cameraCalibrationPresent);
    REQUIRE(!mismatchResult.audit.cameraCalibrationSignatureMatched);
    REQUIRE(!mismatchResult.audit.cameraCalibrationApplied);

    FixtureOptions matchOpt = mismatchOpt;
    matchOpt.matchingSignatures = true;
    MemSource match(make_fixture(matchOpt));
    const auto matchSeal = seal(match);
    v2::ProducerResult matchResult;
    REQUIRE(v2::produce_source_metadata_color_binding(match, matchSeal, matchResult));
    REQUIRE(matchResult.audit.cameraCalibrationPresent);
    REQUIRE(matchResult.audit.cameraCalibrationSignatureMatched);
    REQUIRE(matchResult.audit.cameraCalibrationApplied);
}

void test_incomplete_dual_fails_closed() {
    FixtureOptions opt;
    opt.dual = true;
    opt.includeIlluminant2 = false;
    MemSource source(make_fixture(opt));
    const auto sourceSeal = seal(source);
    v2::ProducerResult result;
    const auto status = v2::produce_source_metadata_color_binding(source, sourceSeal, result);
    REQUIRE(!status);
    REQUIRE(status.code == v2::ProducerStatusCode::IncompleteDualCalibration);
}

void test_custom_illuminant_fails_closed_until_illuminant_data_support() {
    FixtureOptions opt;
    opt.dual = true;
    opt.customSecondIlluminant = true;
    MemSource source(make_fixture(opt));
    const auto sourceSeal = seal(source);
    v2::ProducerResult result;
    const auto status = v2::produce_source_metadata_color_binding(source, sourceSeal, result);
    REQUIRE(!status);
    REQUIRE(status.code == v2::ProducerStatusCode::UnsupportedCalibrationIlluminant);
}

void test_triple_calibration_fails_closed() {
    FixtureOptions opt;
    opt.dual = true;
    opt.triple = true;
    MemSource source(make_fixture(opt));
    const auto sourceSeal = seal(source);
    v2::ProducerResult result;
    const auto status = v2::produce_source_metadata_color_binding(source, sourceSeal, result);
    REQUIRE(!status);
    REQUIRE(status.code == v2::ProducerStatusCode::TripleCalibrationUnsupported);
}

void test_source_tamper_is_rejected() {
    FixtureOptions opt;
    opt.dual = true;
    MemSource source(make_fixture(opt));
    const auto sourceSeal = seal(source);
    REQUIRE(source.bytes.size() > 32u);
    source.bytes.back() ^= 0x01u;
    v2::ProducerResult result;
    const auto status = v2::produce_source_metadata_color_binding(source, sourceSeal, result);
    REQUIRE(!status);
    REQUIRE(status.code == v2::ProducerStatusCode::SourceSealMismatch);
}

} // namespace

int main() {
    try {
        test_single_illuminant_is_exact_v01_delegate();
        test_dual_d50_d65_interpolates_inverse_temperature();
        test_dual_order_swap_is_invariant();
        test_one_forward_matrix_is_explicitly_audited();
        test_dual_camera_calibration_signature_rule();
        test_incomplete_dual_fails_closed();
        test_custom_illuminant_fails_closed_until_illuminant_data_support();
        test_triple_calibration_fails_closed();
        test_source_tamper_is_rejected();
        std::cout << "DNG_COLOR_BINDING_PRODUCER_V0_2_PASS\n";
        std::cout << "single_illuminant=EXACT_V0_1_DELEGATION\n";
        std::cout << "dual_interpolation=INVERSE_CORRELATED_COLOR_TEMPERATURE\n";
        std::cout << "neutral_to_xy=ITERATIVE_MAX_30_CONVERGENCE_1E-7\n";
        std::cout << "triple_calibration=FAIL_CLOSED\n";
        std::cout << "custom_illuminant_255=FAIL_CLOSED_UNTIL_ILLUMINANT_DATA\n";
        std::cout << "authority=SOURCE_METADATA_BOUND\n";
        std::cout << "physical_frame_count=1 independent_evidence_count=1\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
