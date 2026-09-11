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
    // A source-bound appearance preview may be shown before Scientific Master
    // finalization, but it must be labeled as appearance/non-final scientific state.
    bool sourceBoundAppearanceReleaseAllowed = false;
    bool scientificPreviewReleaseAllowed = false;
    bool scientificClaimAllowed = false;
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
};

// Pre-master phase. This validates the exact source/color authority and emits
// TileNative options needed for computation. It may permit a clearly labeled
// source-bound appearance preview, but cannot release a finalized scientific
// preview or claim a Scientific Master.
BindingStatus prepare_scientific_color_source(
    const SourceSeal& source,
    const ScientificColorBindingRecord& color,
    PreparedScientificPreviewSource& out) noexcept;

// Post-master phase. Only a complete valid Technical Backplane bound to the
// same source can turn a prepared computation into a finalized scientific
// preview admission.
BindingStatus finalize_scientific_color_lineage(
    const PreparedScientificPreviewSource& prepared,
    const technical_backplane::v0_1::State& backplane,
    ScientificPreviewAdmission& out) noexcept;

} // namespace truthraw::scientific_preview_binding_v0_2
