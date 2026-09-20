#pragma once

#include "truthraw/core.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <utility>

namespace truthraw::streaming_v0_1 {

enum class StreamStatusCode : int {
    Ok = 0,
    InvalidArgument,
    SourceFailed,
    SinkFailed,
    BudgetExceeded,
    BackendFailed,
    UnsupportedExecution,
};

struct StreamStatus {
    StreamStatusCode code = StreamStatusCode::Ok;
    std::string message;
    explicit operator bool() const { return code == StreamStatusCode::Ok; }
    static StreamStatus ok() { return {}; }
    static StreamStatus error(StreamStatusCode c, std::string m) {
        StreamStatus s; s.code = c; s.message = std::move(m); return s;
    }
};

struct StreamingOptions {
    TilePolicy tile{128, 16};
    int workers = 1;
    bool hdrEnabled = true;
    bool streamScientificDiagnostics = false;
    int sdrLutSize = 4096;
    std::size_t memoryBudgetBytes = 0; // 0 = caller does not impose a logical resident ceiling.
};

struct HalfStateRect {
    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;
};

class IRawTileSource {
public:
    virtual ~IRawTileSource() = default;
    virtual const DngMetadata& metadata() const = 0;
    virtual std::size_t residentBytesUpperBound() const = 0;

    // The caller owns the destination buffers. The source must fill the exact
    // halo rectangle in row-major order. gainOut is null when hasGainField=false.
    virtual StreamStatus readRawTile(
        const TileRect& rect,
        std::uint16_t* rawOut,
        std::size_t rawCount,
        float* gainOut,
        std::size_t gainCount) = 0;

    virtual StreamStatus readRowBias(int y0, int y1, float* out, std::size_t count) = 0;
    virtual StreamStatus readColBias(int x0, int x1, float* out, std::size_t count) = 0;
};

class IStreamingSink {
public:
    virtual ~IStreamingSink() = default;
    virtual std::size_t residentBytesUpperBound() const = 0;
    virtual StreamStatus beginFrame(
        int width,
        int height,
        Orientation orientation,
        const ExposurePlan& exposure,
        bool hdrEnabled,
        bool diagnosticsEnabled) = 0;
    // Optional scene-linear edit tap. Called after color conversion and the
    // selected appearance backend, but before exposure LUT, negative clamp,
    // max-RGB normalization and display transfer. Existing sinks may ignore it.
    virtual StreamStatus writeExtendedLinearTile(
        const TileRect&,
        const float*,
        std::size_t) {
        return StreamStatus::ok();
    }

    virtual StreamStatus writeSdrTile(
        const TileRect& coreRect,
        const float* rgb,
        std::size_t floatCount) = 0;
    virtual StreamStatus writeHalfLogGainBlock(
        const HalfStateRect& rect,
        const float* halfLogGain,
        std::size_t count) = 0;
    virtual StreamStatus writeStage2DiagnosticTile(
        const TileRect& coreRect,
        const float* stage2,
        std::size_t count) = 0;
    virtual StreamStatus finishFrame() = 0;
};

struct StreamingMemoryInfo {
    std::size_t sourceResidentUpperBound = 0;
    std::size_t sinkResidentUpperBound = 0;
    std::size_t logicalWorkspacePeakBytes = 0;
    std::size_t logicalResidentUpperBound = 0;
    bool adapterOwnsFullRawFrame = false;
    bool adapterOwnsFullSdrFrame = false;
    bool adapterOwnsFullHalfGainFrame = false;
    bool adapterOwnsFullDiagnosticFrame = false;
};

struct StreamingProvenance {
    bool scientificMasterModifiedByAppearance = false;
    bool gainMapAppliedExactlyOnce = true;
    bool fullFrameRawCopiedByAdapter = false;
    bool fullFrameSdrAllocatedByAdapter = false;
    bool halfResolutionStateRecomputedTileLocal = true;
    bool counterfactualObservationCreated = false;
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
    std::string reconstructionBackend;
    std::string appearanceBackend;
};

struct StreamingResult {
    StreamStatus status;
    int width = 0;
    int height = 0;
    Orientation orientation = Orientation::Normal;
    ExposurePlan exposure;
    StreamingMemoryInfo memory;
    StreamingProvenance provenance;
    std::uint64_t stage2Over1Count = 0;
    std::uint64_t clippedCount = 0;
    std::size_t tilesProcessedPass1 = 0;
    std::size_t tilesProcessedPass2 = 0;
};

struct StreamingPlan {
    int width = 0;
    int height = 0;
    int halfWidth = 0;
    int halfHeight = 0;
    int tileCount = 0;
    std::size_t logicalWorkspaceUpperBound = 0;
    std::size_t logicalResidentUpperBound = 0;
    bool megapixelIndependentWorkspace = true;
};

StreamStatus plan_streaming_frame(
    const DngMetadata& metadata,
    const StreamingOptions& options,
    const IReconstructionBackend& reconstruction,
    const IAppearanceBackend& appearance,
    std::size_t sourceResidentUpperBound,
    std::size_t sinkResidentUpperBound,
    StreamingPlan& plan);

class StreamingTruthRawProcessor {
public:
    StreamingTruthRawProcessor(
        std::shared_ptr<IReconstructionBackend> reconstruction,
        std::shared_ptr<IAppearanceBackend> appearance);

    StreamStatus process(
        IRawTileSource& source,
        IStreamingSink& sink,
        const StreamingOptions& options,
        StreamingResult& result) const;

private:
    std::shared_ptr<IReconstructionBackend> reconstruction_;
    std::shared_ptr<IAppearanceBackend> appearance_;
};

} // namespace truthraw::streaming_v0_1
