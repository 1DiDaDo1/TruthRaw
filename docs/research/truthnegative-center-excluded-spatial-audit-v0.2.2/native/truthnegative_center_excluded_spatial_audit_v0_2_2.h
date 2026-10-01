#pragma once

#include "full_frame_streaming_v0_1.h"
#include "truthnegative_center_excluded_spatial_audit_v0_2_1.h"
#include "truthnegative_n2_cfa_audit_v0_1.h"

namespace truthraw::truthnegative_center_excluded_spatial_audit::v0_2_2 {

namespace stream = truthraw::streaming_v0_1;
namespace v01 = truthraw::truthnegative_n2_cfa_audit::v0_1;
namespace v021 =
    truthraw::truthnegative_center_excluded_spatial_audit::v0_2_1;

/**
 * Performance-only execution path for the v0.2.1 scientific contract.
 *
 * It consumes sparse corrected-sample coordinates recorded during the already
 * completed v0.1 pass instead of rerunning v0.1 per reporting tile.
 *
 * IMPORTANT: output is the exact v0.2.1 Result type. The implementation must
 * preserve all v0.2.1 metrics and audit identity byte-for-byte.
 */
bool runSparseReference(
    stream::IRawTileSource& source,
    const v021::Binding& binding,
    const v01::Result& referenceV01,
    v021::Result& out) noexcept;

} // namespace truthraw::truthnegative_center_excluded_spatial_audit::v0_2_2
