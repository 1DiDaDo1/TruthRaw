#include "decoded_measurement_tile_source_v0_1.h"
#include "full_frame_streaming_v0_1.h"
#include "truthraw/core.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <span>
#include <unistd.h>

namespace bridge = truthraw::decoded_measurement_tile_source::v0_1;
namespace handoff = truthraw::decoded_measurement_handoff::v0_1;
namespace ingress = truthraw::professional_raw_ingress::v0_1;
namespace streaming = truthraw::streaming_v0_1;

#define CHECK_TRUE(expr) do { \
    if (!(expr)) { \
        std::cerr << "CHECK failed at line " << __LINE__ << ": " #expr "\n"; \
        return __LINE__; \
    } \
} while (false)

struct FdGuard {
    int fd = -1;
    ~FdGuard() { if (fd >= 0) ::close(fd); }
};

static handoff::Hash256 make_hash(std::uint8_t seed) {
    handoff::Hash256 out{};
    for (std::size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<std::uint8_t>(seed + static_cast<std::uint8_t>(i));
    }
    return out;
}

class CountingSink final : public streaming::IStreamingSink {
public:
    std::size_t residentBytesUpperBound() const override { return sizeof(*this); }

    streaming::StreamStatus beginFrame(
        int width,
        int height,
        truthraw::Orientation orientation,
        const truthraw::ExposurePlan& exposure,
        bool hdrEnabled,
        bool diagnosticsEnabled) override {
        if (begun_ || width <= 0 || height <= 0) {
            return streaming::StreamStatus::error(
                streaming::StreamStatusCode::SinkFailed, "invalid beginFrame");
        }
        width_ = width;
        height_ = height;
        orientation_ = orientation;
        exposure_ = exposure;
        hdrEnabled_ = hdrEnabled;
        diagnosticsEnabled_ = diagnosticsEnabled;
        begun_ = true;
        return streaming::StreamStatus::ok();
    }

    streaming::StreamStatus writeSdrTile(
        const truthraw::TileRect& rect,
        const float* rgb,
        std::size_t floatCount) override {
        const int w = rect.x1 - rect.x0;
        const int h = rect.y1 - rect.y0;
        if (!begun_ || !rgb || w <= 0 || h <= 0 ||
            floatCount != 3u * static_cast<std::size_t>(w) * static_cast<std::size_t>(h)) {
            return streaming::StreamStatus::error(
                streaming::StreamStatusCode::SinkFailed, "invalid SDR tile");
        }
        for (std::size_t i = 0; i < floatCount; ++i) {
            if (!std::isfinite(rgb[i])) {
                return streaming::StreamStatus::error(
                    streaming::StreamStatusCode::SinkFailed, "non-finite SDR sample");
            }
            sdrAccumulator_ += static_cast<double>(rgb[i]);
        }
        ++sdrTileWrites_;
        sdrFloats_ += floatCount;
        return streaming::StreamStatus::ok();
    }

    streaming::StreamStatus writeHalfLogGainBlock(
        const streaming::HalfStateRect& rect,
        const float* halfLogGain,
        std::size_t count) override {
        const int w = rect.x1 - rect.x0;
        const int h = rect.y1 - rect.y0;
        if (!begun_ || !halfLogGain || w <= 0 || h <= 0 ||
            count != static_cast<std::size_t>(w) * static_cast<std::size_t>(h)) {
            return streaming::StreamStatus::error(
                streaming::StreamStatusCode::SinkFailed, "invalid half-gain block");
        }
        for (std::size_t i = 0; i < count; ++i) {
            if (!std::isfinite(halfLogGain[i])) {
                return streaming::StreamStatus::error(
                    streaming::StreamStatusCode::SinkFailed, "non-finite half-gain sample");
            }
            gainAccumulator_ += static_cast<double>(halfLogGain[i]);
        }
        ++gainWrites_;
        gainFloats_ += count;
        return streaming::StreamStatus::ok();
    }

    streaming::StreamStatus writeStage2DiagnosticTile(
        const truthraw::TileRect& rect,
        const float* stage2,
        std::size_t count) override {
        const int w = rect.x1 - rect.x0;
        const int h = rect.y1 - rect.y0;
        if (!begun_ || !diagnosticsEnabled_ || !stage2 || w <= 0 || h <= 0 ||
            count != static_cast<std::size_t>(w) * static_cast<std::size_t>(h)) {
            return streaming::StreamStatus::error(
                streaming::StreamStatusCode::SinkFailed, "invalid diagnostic tile");
        }
        for (std::size_t i = 0; i < count; ++i) {
            if (!std::isfinite(stage2[i])) {
                return streaming::StreamStatus::error(
                    streaming::StreamStatusCode::SinkFailed, "non-finite diagnostic sample");
            }
            diagnosticAccumulator_ += static_cast<double>(stage2[i]);
        }
        ++diagnosticWrites_;
        diagnosticFloats_ += count;
        return streaming::StreamStatus::ok();
    }

    streaming::StreamStatus finishFrame() override {
        if (!begun_ || finished_ || sdrTileWrites_ == 0 || gainWrites_ == 0 ||
            (diagnosticsEnabled_ && diagnosticWrites_ == 0)) {
            return streaming::StreamStatus::error(
                streaming::StreamStatusCode::SinkFailed, "incomplete streamed frame");
        }
        finished_ = true;
        return streaming::StreamStatus::ok();
    }

    bool finished() const noexcept { return finished_; }
    std::size_t sdrTileWrites() const noexcept { return sdrTileWrites_; }
    std::size_t gainWrites() const noexcept { return gainWrites_; }
    std::size_t diagnosticWrites() const noexcept { return diagnosticWrites_; }
    std::size_t sdrFloats() const noexcept { return sdrFloats_; }
    int width() const noexcept { return width_; }
    int height() const noexcept { return height_; }
    bool hdrEnabled() const noexcept { return hdrEnabled_; }

private:
    int width_ = 0;
    int height_ = 0;
    truthraw::Orientation orientation_ = truthraw::Orientation::Normal;
    truthraw::ExposurePlan exposure_{};
    bool hdrEnabled_ = false;
    bool diagnosticsEnabled_ = false;
    bool begun_ = false;
    bool finished_ = false;
    std::size_t sdrTileWrites_ = 0;
    std::size_t gainWrites_ = 0;
    std::size_t diagnosticWrites_ = 0;
    std::size_t sdrFloats_ = 0;
    std::size_t gainFloats_ = 0;
    std::size_t diagnosticFloats_ = 0;
    double sdrAccumulator_ = 0.0;
    double gainAccumulator_ = 0.0;
    double diagnosticAccumulator_ = 0.0;
};

static std::uint16_t synthetic_sample(std::uint32_t x, std::uint32_t y) {
    std::uint32_t v = 700u + ((x * 193u + y * 271u + x * y * 7u) % 14500u);
    if ((x + 3u * y) % 101u == 0u) v = 16383u;
    return static_cast<std::uint16_t>(v);
}

int main() {
    constexpr std::uint32_t width = 66;
    constexpr std::uint32_t height = 50;

    char path[] = "/tmp/truthraw_main_house_e2e_XXXXXX";
    FdGuard store{::mkstemp(path)};
    CHECK_TRUE(store.fd >= 0);
    CHECK_TRUE(::unlink(path) == 0);

    handoff::Descriptor descriptor{};
    descriptor.width = width;
    descriptor.height = height;
    descriptor.sourceBitDepth = 14;
    descriptor.topology = ingress::MeasurementTopology::Bayer2x2;
    descriptor.evidenceClass = ingress::EvidenceClass::LosslessDecodedCertified;
    descriptor.sourceCompression = ingress::CompressionSemantics::LosslessVerified;
    descriptor.cfa2x2 = {
        handoff::CfaColor::Blue,
        handoff::CfaColor::Green,
        handoff::CfaColor::Green,
        handoff::CfaColor::Red};
    descriptor.sourceEvidenceHash = make_hash(11);
    descriptor.decoderAuditHash = make_hash(123);

    handoff::Writer writer;
    CHECK_TRUE(writer.begin(store.fd, descriptor) == handoff::Status::Ok);

    std::array<std::uint16_t, 257> chunk{};
    std::uint64_t linear = 0;
    const std::uint64_t total = static_cast<std::uint64_t>(width) * height;
    while (linear < total) {
        const std::size_t count = static_cast<std::size_t>(
            std::min<std::uint64_t>(chunk.size(), total - linear));
        for (std::size_t i = 0; i < count; ++i) {
            const std::uint64_t index = linear + i;
            const auto y = static_cast<std::uint32_t>(index / width);
            const auto x = static_cast<std::uint32_t>(index % width);
            chunk[i] = synthetic_sample(x, y);
        }
        CHECK_TRUE(writer.append_samples(
            std::span<const std::uint16_t>(chunk.data(), count)) == handoff::Status::Ok);
        linear += count;
    }
    CHECK_TRUE(writer.seal() == handoff::Status::Ok);

    handoff::Reader reader;
    CHECK_TRUE(reader.open(store.fd) == handoff::Status::Ok);
    CHECK_TRUE(reader.verify_payload_integrity() == handoff::Status::Ok);

    truthraw::DngMetadata metadata{};
    metadata.width = static_cast<int>(width);
    metadata.height = static_cast<int>(height);
    metadata.cfa = truthraw::CfaPattern::BGGR;
    metadata.orientation = truthraw::Orientation::Normal;
    metadata.whiteLevel = 16383.0f;
    metadata.blackPhase = {512.0f, 513.0f, 514.0f, 515.0f};
    metadata.hasNoiseProfile = true;
    metadata.noiseProfile = {0.0009f, 1.0e-6f, 0.0010f, 1.2e-6f, 0.0011f, 1.4e-6f};
    metadata.cameraToXyzD50 = {
        0.62f, 0.21f, 0.08f,
        0.18f, 0.71f, 0.07f,
        0.03f, 0.12f, 0.79f};
    metadata.hasGainField = false;
    metadata.hasResidualBlack = false;
    metadata.sourceId = "detached-gatehouse-fixture";

    bridge::BindingAuthority authority{};
    authority.metadataBoundToOriginalSource = true;
    authority.metadataSemanticsVerified = true;
    authority.decodedStoreBoundToOriginalSource = true;
    authority.gatehouseDetached = true;

    bridge::DecodedMeasurementTileSource source;
    CHECK_TRUE(source.bind(reader, metadata, authority) == bridge::BindStatus::Ok);
    CHECK_TRUE(source.residentBytesUpperBound() < 64u * 1024u);

    auto reconstruction = std::make_shared<truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto appearance = std::make_shared<truthraw::SkinSafeDetailedCrispAppearance>();
    streaming::StreamingTruthRawProcessor processor(reconstruction, appearance);

    CountingSink sink;
    CHECK_TRUE(sink.residentBytesUpperBound() < 64u * 1024u);

    streaming::StreamingOptions options{};
    options.tile = {16, 7};
    options.workers = 1;
    options.hdrEnabled = true;
    options.streamScientificDiagnostics = true;
    options.sdrLutSize = 4096;
    options.memoryBudgetBytes = 8u * 1024u * 1024u;

    streaming::StreamingResult result{};
    const auto status = processor.process(source, sink, options, result);
    if (!status) {
        std::cerr << "processor failure: " << status.message << "\n";
        return 200;
    }

    CHECK_TRUE(sink.finished());
    CHECK_TRUE(sink.width() == static_cast<int>(width));
    CHECK_TRUE(sink.height() == static_cast<int>(height));
    CHECK_TRUE(sink.hdrEnabled());
    CHECK_TRUE(sink.sdrTileWrites() > 1);
    CHECK_TRUE(sink.gainWrites() > 0);
    CHECK_TRUE(sink.diagnosticWrites() > 1);
    CHECK_TRUE(sink.sdrFloats() == 3u * static_cast<std::size_t>(width) * height);

    CHECK_TRUE(result.status);
    CHECK_TRUE(result.width == static_cast<int>(width));
    CHECK_TRUE(result.height == static_cast<int>(height));
    CHECK_TRUE(result.tilesProcessedPass1 > 1);
    CHECK_TRUE(result.tilesProcessedPass1 == result.tilesProcessedPass2);
    CHECK_TRUE(result.memory.sourceResidentUpperBound == source.residentBytesUpperBound());
    CHECK_TRUE(result.memory.sinkResidentUpperBound == sink.residentBytesUpperBound());
    CHECK_TRUE(result.memory.logicalResidentUpperBound <= options.memoryBudgetBytes);
    CHECK_TRUE(!result.memory.adapterOwnsFullRawFrame);
    CHECK_TRUE(!result.memory.adapterOwnsFullSdrFrame);
    CHECK_TRUE(!result.memory.adapterOwnsFullHalfGainFrame);
    CHECK_TRUE(!result.memory.adapterOwnsFullDiagnosticFrame);

    CHECK_TRUE(!result.provenance.scientificMasterModifiedByAppearance);
    CHECK_TRUE(result.provenance.gainMapAppliedExactlyOnce);
    CHECK_TRUE(!result.provenance.fullFrameRawCopiedByAdapter);
    CHECK_TRUE(!result.provenance.fullFrameSdrAllocatedByAdapter);
    CHECK_TRUE(result.provenance.halfResolutionStateRecomputedTileLocal);
    CHECK_TRUE(!result.provenance.counterfactualObservationCreated);
    CHECK_TRUE(result.provenance.physicalFrameCount == 1u);
    CHECK_TRUE(result.provenance.independentEvidenceCount == 1u);
    CHECK_TRUE(!result.provenance.reconstructionBackend.empty());
    CHECK_TRUE(!result.provenance.appearanceBackend.empty());

    std::cout << "DECODED_MEASUREMENT_MAIN_HOUSE_E2E_V0_1_PASS\n";
    std::cout << "source_resident_upper_bound=" << result.memory.sourceResidentUpperBound << "\n";
    std::cout << "sink_resident_upper_bound=" << result.memory.sinkResidentUpperBound << "\n";
    std::cout << "logical_workspace_peak_bytes=" << result.memory.logicalWorkspacePeakBytes << "\n";
    std::cout << "logical_resident_upper_bound=" << result.memory.logicalResidentUpperBound << "\n";
    std::cout << "tiles_pass1=" << result.tilesProcessedPass1 << "\n";
    std::cout << "tiles_pass2=" << result.tilesProcessedPass2 << "\n";
    std::cout << "full_frame_source_buffer=0\n";
    std::cout << "full_frame_sink_buffer=0\n";
    std::cout << "libraw_in_main_house=0\n";
    std::cout << "physical_frame_count=" << result.provenance.physicalFrameCount << "\n";
    std::cout << "independent_evidence_count=" << result.provenance.independentEvidenceCount << "\n";
    return 0;
}
