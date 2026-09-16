#include "finalized_scientific_preview_release_v0_1.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

namespace release_v0_1 = truthraw::finalized_scientific_preview_release::v0_1;
namespace smsb_v0_1 = truthraw::scientific_master_streaming_binding::v0_1;
namespace phase2_v0_1 = truthraw::technical_backplane_phase2::v0_1;

namespace {

void require_active(bool ok, const char* expression, int line) {
    if (!ok) {
        std::cerr << "REQUIRE_FAIL line=" << line << " expr=" << expression << '\n';
        std::exit(2);
    }
}
#define REQUIRE(expr) require_active(static_cast<bool>(expr), #expr, __LINE__)

class FrameSource final : public truthraw::streaming_v0_1::IRawTileSource {
public:
    explicit FrameSource(const truthraw::DecodedDngFrame& frame) : frame_(frame) {}

    const truthraw::DngMetadata& metadata() const override { return frame_.meta; }

    std::size_t residentBytesUpperBound() const override {
        return frame_.raw.size() * sizeof(std::uint16_t) +
               frame_.gainField.size() * sizeof(float) +
               frame_.rowBias.size() * sizeof(float) +
               frame_.colBias.size() * sizeof(float);
    }

    truthraw::streaming_v0_1::StreamStatus readRawTile(
        const truthraw::TileRect& rect,
        std::uint16_t* rawOut,
        std::size_t rawCount,
        float* gainOut,
        std::size_t gainCount) override {
        const int width = rect.hx1 - rect.hx0;
        const int height = rect.hy1 - rect.hy0;
        const std::size_t count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
        if (rawOut == nullptr || rawCount != count) {
            return truthraw::streaming_v0_1::StreamStatus::error(
                truthraw::streaming_v0_1::StreamStatusCode::SourceFailed, "raw tile count mismatch");
        }
        if (frame_.meta.hasGainField && (gainOut == nullptr || gainCount != count)) {
            return truthraw::streaming_v0_1::StreamStatus::error(
                truthraw::streaming_v0_1::StreamStatusCode::SourceFailed, "gain tile count mismatch");
        }
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const std::size_t sourceIndex =
                    static_cast<std::size_t>(rect.hy0 + y) * static_cast<std::size_t>(frame_.meta.width) +
                    static_cast<std::size_t>(rect.hx0 + x);
                const std::size_t destinationIndex =
                    static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                    static_cast<std::size_t>(x);
                rawOut[destinationIndex] = frame_.raw[sourceIndex];
                if (frame_.meta.hasGainField) gainOut[destinationIndex] = frame_.gainField[sourceIndex];
            }
        }
        return truthraw::streaming_v0_1::StreamStatus::ok();
    }

    truthraw::streaming_v0_1::StreamStatus readRowBias(
        int y0, int y1, float* out, std::size_t count) override {
        if (!frame_.meta.hasResidualBlack) {
            return count == 0u
                ? truthraw::streaming_v0_1::StreamStatus::ok()
                : truthraw::streaming_v0_1::StreamStatus::error(
                    truthraw::streaming_v0_1::StreamStatusCode::SourceFailed, "unexpected row bias");
        }
        if (out == nullptr || count != static_cast<std::size_t>(y1 - y0)) {
            return truthraw::streaming_v0_1::StreamStatus::error(
                truthraw::streaming_v0_1::StreamStatusCode::SourceFailed, "row bias count mismatch");
        }
        std::copy(frame_.rowBias.begin() + y0, frame_.rowBias.begin() + y1, out);
        return truthraw::streaming_v0_1::StreamStatus::ok();
    }

    truthraw::streaming_v0_1::StreamStatus readColBias(
        int x0, int x1, float* out, std::size_t count) override {
        if (!frame_.meta.hasResidualBlack) {
            return count == 0u
                ? truthraw::streaming_v0_1::StreamStatus::ok()
                : truthraw::streaming_v0_1::StreamStatus::error(
                    truthraw::streaming_v0_1::StreamStatusCode::SourceFailed, "unexpected col bias");
        }
        if (out == nullptr || count != static_cast<std::size_t>(x1 - x0)) {
            return truthraw::streaming_v0_1::StreamStatus::error(
                truthraw::streaming_v0_1::StreamStatusCode::SourceFailed, "col bias count mismatch");
        }
        std::copy(frame_.colBias.begin() + x0, frame_.colBias.begin() + x1, out);
        return truthraw::streaming_v0_1::StreamStatus::ok();
    }

private:
    const truthraw::DecodedDngFrame& frame_;
};

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

truthraw::DecodedDngFrame make_frame(int width, int height) {
    truthraw::DecodedDngFrame frame{};
    frame.meta.width = width;
    frame.meta.height = height;
    frame.meta.cfa = truthraw::CfaPattern::BGGR;
    frame.meta.orientation = truthraw::Orientation::Rotate90CW;
    frame.meta.whiteLevel = 1023.0f;
    frame.meta.blackPhase = {64.0f, 65.0f, 66.0f, 67.0f};
    frame.meta.hasNoiseProfile = true;
    frame.meta.noiseProfile = {0.0009f, 1e-6f, 0.0010f, 1.2e-6f, 0.0011f, 1.4e-6f};
    frame.meta.cameraToXyzD50 = {
        0.62f, 0.21f, 0.08f,
        0.18f, 0.71f, 0.07f,
        0.03f, 0.12f, 0.79f,
    };
    frame.meta.hasGainField = true;
    frame.meta.hasResidualBlack = true;

    const std::size_t sampleCount =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    frame.raw.resize(sampleCount);
    frame.gainField.resize(sampleCount);
    frame.rowBias.resize(static_cast<std::size_t>(height));
    frame.colBias.resize(static_cast<std::size_t>(width));

    for (int y = 0; y < height; ++y) {
        frame.rowBias[static_cast<std::size_t>(y)] = 0.05f * static_cast<float>((y % 5) - 2);
        for (int x = 0; x < width; ++x) {
            if (y == 0) {
                frame.colBias[static_cast<std::size_t>(x)] =
                    0.03f * static_cast<float>((x % 7) - 3);
            }
            const std::size_t index =
                static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                static_cast<std::size_t>(x);
            int value = 70 + ((x * 37 + y * 53 + x * y * 3) % 900);
            if ((x + y) % 97 == 0) value = 1023;
            frame.raw[index] = static_cast<std::uint16_t>(value);
            frame.gainField[index] =
                0.94f + 0.12f * static_cast<float>((x + 2 * y) % 19) / 18.0f;
        }
    }
    return frame;
}

truthraw::scientific_preview_binding_v0_2::PreparedScientificPreviewSource prepare_source(
    truthraw::DecodedDngFrame& frame) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(frame.raw.size() * 2u + 32u);
    const char magic[] = "TRUTHRAW_FINALIZED_PREVIEW_SOURCE_V1";
    bytes.insert(bytes.end(), magic, magic + sizeof(magic) - 1u);
    for (const auto sample : frame.raw) {
        bytes.push_back(static_cast<std::uint8_t>(sample & 0xffu));
        bytes.push_back(static_cast<std::uint8_t>((sample >> 8u) & 0xffu));
    }
    SealMemorySource sealSource(std::move(bytes));

    truthraw::scientific_preview_binding_v0_1::SourceSeal seal{};
    REQUIRE(truthraw::scientific_preview_binding_v0_1::seal_source_sha256(
        sealSource, seal, 4096u));
    frame.meta.sourceId = seal.sourceEvidenceId;

    truthraw::scientific_preview_binding_v0_1::ScientificColorBindingRecord color{};
    color.authority = truthraw::scientific_preview_binding_v0_1::ColorBindingAuthority::SourceMetadataBound;
    color.sourceEvidenceId = seal.sourceEvidenceId;
    color.bindingId = "SYNTHETIC_FINALIZED_SOURCE_METADATA_D50";
    color.cameraToXyzD50 = frame.meta.cameraToXyzD50;
    color.normalized = true;
    color.validated = true;
    color.physicalFrameCount = 1u;
    color.independentEvidenceCount = 1u;

    truthraw::scientific_preview_binding_v0_2::PreparedScientificPreviewSource prepared{};
    REQUIRE(truthraw::scientific_preview_binding_v0_2::prepare_scientific_color_source(
        seal, color, prepared));
    return prepared;
}

phase2_v0_1::Phase2Result build_phase2(
    truthraw::DecodedDngFrame& frame,
    const truthraw::scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    const std::shared_ptr<truthraw::IReconstructionBackend>& reconstruction) {
    FrameSource source(frame);
    smsb_v0_1::Result identity{};
    REQUIRE(smsb_v0_1::bind_scientific_master_streaming(
        source, *reconstruction, smsb_v0_1::Options{}, identity));

    phase2_v0_1::Phase2Input input{};
    input.prepared = prepared;
    input.scientificMasterHash = identity.scientificMasterHash;
    input.zeroLineGauge = identity.zeroLineGauge;
    input.sceneBinding = identity.sceneBinding;
    input.roomStatus.fill(truthraw::technical_backplane::v0_1::RoomStatus::ResearchOnly);
    input.roomStatus[0] = truthraw::technical_backplane::v0_1::RoomStatus::Available;
    input.roomStatus[1] = truthraw::technical_backplane::v0_1::RoomStatus::Available;
    input.roomStatus[2] = truthraw::technical_backplane::v0_1::RoomStatus::Available;
    input.claimStatus = truthraw::technical_backplane::v0_1::ClaimStatus::Candidate;

    phase2_v0_1::Phase2Result out{};
    REQUIRE(phase2_v0_1::finalize_phase2(input, out));
    return out;
}

truthraw::streaming_v0_1::StreamingOptions preview_options() {
    truthraw::streaming_v0_1::StreamingOptions options{};
    options.tile = {32, 7};
    options.workers = 1;
    options.hdrEnabled = true;
    options.streamScientificDiagnostics = false;
    options.sdrLutSize = 4096;
    options.memoryBudgetBytes = 0u;
    return options;
}

void test_successful_finalized_release() {
    auto frame = make_frame(130, 98);
    const auto prepared = prepare_source(frame);
    auto reconstruction = std::make_shared<truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto appearance = std::make_shared<truthraw::NeutralReferenceAppearance>();
    const auto phase2 = build_phase2(frame, prepared, reconstruction);

    FrameSource releaseSource(frame);
    truthraw::preview_surface_v0_1::BoundedSrgbPreviewSink sink(64);
    release_v0_1::ReleaseResult result{};
    const auto status = release_v0_1::release_finalized_scientific_preview(
        prepared,
        phase2.serializedBackplane,
        releaseSource,
        reconstruction,
        appearance,
        smsb_v0_1::Options{},
        preview_options(),
        sink,
        result);
    if (!status) std::cerr << "release failed: " << status.message << '\n';
    REQUIRE(status);
    REQUIRE(result.authority == release_v0_1::PreviewAuthority::FinalizedSourceBoundScientificPreview);
    REQUIRE(result.canonicalPhase2.serializedBackplane == phase2.serializedBackplane);
    REQUIRE(result.scientificIdentity.scientificMasterHash == phase2.backplane.scientificMasterHash);
    REQUIRE(result.streaming.provenance.physicalFrameCount == 1u);
    REQUIRE(result.streaming.provenance.independentEvidenceCount == 1u);
    REQUIRE(!result.streaming.provenance.scientificMasterModifiedByAppearance);
    REQUIRE(!result.streaming.provenance.counterfactualObservationCreated);
    REQUIRE(sink.finished());
    REQUIRE(!sink.argb8888().empty());
    REQUIRE(sink.width() == 48);
    REQUIRE(sink.height() == 64);

    bool nonGray = false;
    for (const std::uint32_t pixel : sink.argb8888()) {
        const std::uint32_t r = (pixel >> 16u) & 0xffu;
        const std::uint32_t g = (pixel >> 8u) & 0xffu;
        const std::uint32_t b = pixel & 0xffu;
        if (r != g || g != b) nonGray = true;
    }
    REQUIRE(nonGray);
}

void test_valid_crc_but_fabricated_master_is_rejected() {
    auto frame = make_frame(130, 98);
    const auto prepared = prepare_source(frame);
    auto reconstruction = std::make_shared<truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto appearance = std::make_shared<truthraw::NeutralReferenceAppearance>();
    auto phase2 = build_phase2(frame, prepared, reconstruction);

    phase2.backplane.scientificMasterHash[0] ^= 0x5au;
    truthraw::technical_backplane::v0_1::SerializedBackplane fabricated{};
    REQUIRE(truthraw::technical_backplane::v0_1::serialize(phase2.backplane, fabricated) ==
            truthraw::technical_backplane::v0_1::Status::Ok);

    FrameSource source(frame);
    truthraw::preview_surface_v0_1::BoundedSrgbPreviewSink sink(64);
    release_v0_1::ReleaseResult result{};
    const auto status = release_v0_1::release_finalized_scientific_preview(
        prepared, fabricated, source, reconstruction, appearance,
        smsb_v0_1::Options{}, preview_options(), sink, result);
    REQUIRE(!status);
    REQUIRE(status.code == release_v0_1::StatusCode::ScientificIdentityMismatch);
    REQUIRE(sink.argb8888().empty());
    REQUIRE(!sink.finished());
}

void test_wrong_source_is_rejected_before_preview() {
    auto frame = make_frame(130, 98);
    const auto prepared = prepare_source(frame);
    auto reconstruction = std::make_shared<truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto appearance = std::make_shared<truthraw::NeutralReferenceAppearance>();
    const auto phase2 = build_phase2(frame, prepared, reconstruction);

    auto wrongFrame = frame;
    wrongFrame.meta.sourceId = "sha256:ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff";
    FrameSource source(wrongFrame);
    truthraw::preview_surface_v0_1::BoundedSrgbPreviewSink sink(64);
    release_v0_1::ReleaseResult result{};
    const auto status = release_v0_1::release_finalized_scientific_preview(
        prepared, phase2.serializedBackplane, source, reconstruction, appearance,
        smsb_v0_1::Options{}, preview_options(), sink, result);
    REQUIRE(!status);
    REQUIRE(status.code == release_v0_1::StatusCode::SourceIdentityMismatch);
    REQUIRE(sink.argb8888().empty());
}

void test_wrong_color_is_rejected_before_preview() {
    auto frame = make_frame(130, 98);
    const auto prepared = prepare_source(frame);
    auto reconstruction = std::make_shared<truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto appearance = std::make_shared<truthraw::NeutralReferenceAppearance>();
    const auto phase2 = build_phase2(frame, prepared, reconstruction);

    auto wrongFrame = frame;
    wrongFrame.meta.cameraToXyzD50[0] += 0.125f;
    FrameSource source(wrongFrame);
    truthraw::preview_surface_v0_1::BoundedSrgbPreviewSink sink(64);
    release_v0_1::ReleaseResult result{};
    const auto status = release_v0_1::release_finalized_scientific_preview(
        prepared, phase2.serializedBackplane, source, reconstruction, appearance,
        smsb_v0_1::Options{}, preview_options(), sink, result);
    REQUIRE(!status);
    REQUIRE(status.code == release_v0_1::StatusCode::ColorIdentityMismatch);
    REQUIRE(sink.argb8888().empty());
}

void test_corrupt_serialized_backplane_is_rejected() {
    auto frame = make_frame(130, 98);
    const auto prepared = prepare_source(frame);
    auto reconstruction = std::make_shared<truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto appearance = std::make_shared<truthraw::NeutralReferenceAppearance>();
    const auto phase2 = build_phase2(frame, prepared, reconstruction);

    auto corrupt = phase2.serializedBackplane;
    corrupt[40] ^= 0x01u;
    FrameSource source(frame);
    truthraw::preview_surface_v0_1::BoundedSrgbPreviewSink sink(64);
    release_v0_1::ReleaseResult result{};
    const auto status = release_v0_1::release_finalized_scientific_preview(
        prepared, corrupt, source, reconstruction, appearance,
        smsb_v0_1::Options{}, preview_options(), sink, result);
    REQUIRE(!status);
    REQUIRE(status.code == release_v0_1::StatusCode::BackplaneRejected);
    REQUIRE(sink.argb8888().empty());
}

}  // namespace

int main() {
    test_successful_finalized_release();
    test_valid_crc_but_fabricated_master_is_rejected();
    test_wrong_source_is_rejected_before_preview();
    test_wrong_color_is_rejected_before_preview();
    test_corrupt_serialized_backplane_is_rejected();

    std::cout << "FINALIZED_SCIENTIFIC_PREVIEW_RELEASE_V0_1_PASS\n";
    std::cout << "valid_phase2_opens_preview=1\n";
    std::cout << "fabricated_valid_crc_master_blocked=1\n";
    std::cout << "source_mismatch_blocked_before_preview=1\n";
    std::cout << "color_mismatch_blocked_before_preview=1\n";
    std::cout << "corrupt_backplane_blocked_before_preview=1\n";
    std::cout << "physicalFrameCount=1\n";
    std::cout << "independentEvidenceCount=1\n";
    return 0;
}
