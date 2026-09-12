#pragma once

#include "linear_dng_projection_v0_1.h"
#include "scientific_master_digest_v0_1.h"

#include <cstddef>
#include <memory>

namespace truthraw::linear_dng_projection::v0_1 {

struct ScientificExportAudit final {
    Audit projection{};
    std::size_t reconstructedTiles = 0;
    std::size_t reconstructionWorkspacePeakBytes = 0;
    bool sourceIdentityChecked = false;
    bool scientificMasterDigestReverified = false;
    bool transactionCommitted = false;
    bool appearanceApplied = false;
    bool cameraToXyzApplied = false;
};

// Re-runs the validated camera-native Scientific Master reconstruction as a
// bounded 64x64 tile stream and writes that stream to a transactional
// compatibility-DNG sink. The reconstructed RGB is hashed again with the
// canonical Scientific Master digest while exporting. Bytes are committed only
// after that digest exactly equals the admitted Scientific Master hash.
//
// No camera_to_xyz(), appearance, tone mapping or preview/JPEG pixels enter
// this path. The extra pass changes execution cost only, never evidence count.
Status write_linear_dng_from_finalized_scientific_source(
    streaming_v0_1::IRawTileSource& source,
    std::shared_ptr<IReconstructionBackend> reconstruction,
    ITransactionalByteSink& sink,
    const ProjectionAdmission& admission,
    const Options& options,
    ScientificExportAudit& out) noexcept;

}  // namespace truthraw::linear_dng_projection::v0_1
