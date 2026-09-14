#pragma once

#include "full_frame_streaming_v0_1.h"

#include <cstddef>

namespace truthraw::streaming_v0_2 {

struct MultiWorkerTelemetry {
    int effectiveWorkers = 0;
    std::size_t queueDepth = 0;
    std::size_t maxReadyPackets = 0;
    bool orderedCommit = true;
};

// Research candidate only. Source access is serialized internally; expensive
// reconstruction/appearance/HDR tile work may overlap across workers. Sink
// commit remains canonical tile-index order. Worker count changes execution
// resources only and must not change scientific authority.
streaming_v0_1::StreamStatus process_multiworker_streaming(
    streaming_v0_1::IRawTileSource& source,
    streaming_v0_1::IStreamingSink& sink,
    IReconstructionBackend& reconstruction,
    const IAppearanceBackend& appearance,
    streaming_v0_1::StreamingOptions options,
    streaming_v0_1::StreamingResult& result,
    MultiWorkerTelemetry& telemetry);

} // namespace truthraw::streaming_v0_2
