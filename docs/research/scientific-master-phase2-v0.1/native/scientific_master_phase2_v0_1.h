#pragma once

#include "full_frame_streaming_v0_1.h"
#include "scientific_master_digest_v0_1.h"

#include <cstddef>
#include <memory>

namespace truthraw::scientific_master_phase2::v0_1 {

struct Phase2StreamingResult final {
    streaming_v0_1::StreamingResult streaming;
    scientific_master_digest::v0_1::Sha256 scientificMasterHash{};
    scientific_master_digest::v0_1::DigestMetrics digestMetrics{};
    std::size_t digestResidentBytesUpperBound = 0;
    bool scientificMasterHashFinalized = false;
};

// Research integration layer: identical v0.1 streaming science/output path plus
// deterministic capture of the reconstructed camera-native Scientific Master
// immediately before camera->XYZ and appearance processing.
class StreamingScientificMasterProcessor final {
public:
    StreamingScientificMasterProcessor(
        std::shared_ptr<IReconstructionBackend> reconstruction,
        std::shared_ptr<IAppearanceBackend> appearance);

    streaming_v0_1::StreamStatus process(
        streaming_v0_1::IRawTileSource& source,
        streaming_v0_1::IStreamingSink& sink,
        const streaming_v0_1::StreamingOptions& options,
        Phase2StreamingResult& result) const;

private:
    std::shared_ptr<IReconstructionBackend> reconstruction_;
    std::shared_ptr<IAppearanceBackend> appearance_;
};

}  // namespace truthraw::scientific_master_phase2::v0_1
