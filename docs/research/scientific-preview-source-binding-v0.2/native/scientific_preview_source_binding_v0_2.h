#pragma once

#include "scientific_preview_source_binding_v0_1.h"

namespace truthraw::scientific_preview_binding_v0_2 {

using scientific_preview_binding_v0_1::BindingStatus;
using scientific_preview_binding_v0_1::BindingStatusCode;
using scientific_preview_binding_v0_1::ColorClaimScope;
using scientific_preview_binding_v0_1::ScientificColorBindingRecord;
using scientific_preview_binding_v0_1::ScientificPreviewAdmission;
using scientific_preview_binding_v0_1::SourceSeal;

struct PreparedScientificPreviewSource {
    SourceSeal source{};
    ScientificColorBindingRecord color{};
    tile_dng_v0_1::OpenOptions tileNativeOptions{};
    ColorClaimScope eventualClaimScope = ColorClaimScope::None;
    bool mainHouseComputeAllowed = false;
    bool previewReleaseAllowed = false;
    bool scientificClaimAllowed = false;
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
};

// Pre-master phase. This validates the exact source/color authority and emits
// TileNative options needed for computation, but deliberately cannot release a
// scientific preview or claim a finalized Scientific Master.
BindingStatus prepare_scientific_color_source(
    const SourceSeal& source,
    const ScientificColorBindingRecord& color,
    PreparedScientificPreviewSource& out) noexcept;

// Post-master phase. Only a complete valid Technical Backplane bound to the
// same source can turn a prepared computation into a releasable preview.
BindingStatus finalize_scientific_color_lineage(
    const PreparedScientificPreviewSource& prepared,
    const technical_backplane::v0_1::State& backplane,
    ScientificPreviewAdmission& out) noexcept;

} // namespace truthraw::scientific_preview_binding_v0_2
