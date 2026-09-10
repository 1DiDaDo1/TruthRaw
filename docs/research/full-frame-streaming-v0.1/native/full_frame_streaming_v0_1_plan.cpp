#include "full_frame_streaming_v0_1.h"
#include "full_frame_streaming_v0_1_internal.h"

namespace truthraw::streaming_v0_1 {
using namespace detail;

StreamStatus plan_streaming_frame(
    const DngMetadata& m,
    const StreamingOptions& o,
    const IReconstructionBackend& reconstruction,
    const IAppearanceBackend& appearance,
    std::size_t sourceResident,
    std::size_t sinkResident,
    StreamingPlan& plan) {
    plan = StreamingPlan{};
    std::string why;
    if (!valid_options(m, o, reconstruction, appearance, why))
        return StreamStatus::error(StreamStatusCode::InvalidArgument, why);
    auto tiles = make_tiles(m.width, m.height, o.tile);
    if (tiles.empty()) return StreamStatus::error(StreamStatusCode::InvalidArgument, "no tiles");
    bool ok = true;
    const std::size_t workspace = estimate_workspace(m, o, appearance, ok);
    if (!ok) return StreamStatus::error(StreamStatusCode::InvalidArgument, "workspace size overflow");
    std::size_t resident = workspace;
    resident = checked_add(resident, sourceResident, ok);
    resident = checked_add(resident, sinkResident, ok);
    if (!ok) return StreamStatus::error(StreamStatusCode::InvalidArgument, "resident size overflow");
    if (o.memoryBudgetBytes != 0 && resident > o.memoryBudgetBytes)
        return StreamStatus::error(StreamStatusCode::BudgetExceeded, "logical resident upper bound exceeds budget");
    plan.width = m.width;
    plan.height = m.height;
    plan.halfWidth = (m.width + 1) / 2;
    plan.halfHeight = (m.height + 1) / 2;
    plan.tileCount = int(tiles.size());
    plan.logicalWorkspaceUpperBound = workspace;
    plan.logicalResidentUpperBound = resident;
    return StreamStatus::ok();
}

} // namespace truthraw::streaming_v0_1
