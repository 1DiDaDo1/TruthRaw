#pragma once

#include "dng_color_binding_producer_v0_2.h"
#include "raw_source_adapter_bridge_common.h"
#include "scientific_master_f64_reconstruction_v0_1.h"
#include "scientific_master_streaming_binding_v0_2.h"
#include "scientific_preview_source_binding_v0_1.h"
#include "scientific_preview_source_binding_v0_2.h"
#include "streaming_scientific_master_tile_source_v0_1.h"
#include "technical_backplane_phase2_v0_1.h"
#include "truthnegative_continuous_v0_5.h"
#include "truthnegative_dense_local_field_adapter_v0_4.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace truthraw::android_truthnegative_pipeline::v0_1 {

struct Status final {
    int code = 0;
    std::string message;
    explicit operator bool() const noexcept { return code == 0; }
};

struct Context final {
    std::shared_ptr<tile_dng_v0_1::IRandomAccessByteSource> bytes;
    scientific_preview_binding_v0_1::SourceSeal sourceSeal{};
    dng_color_binding_producer_v0_2::ProducerResult produced{};
    scientific_preview_binding_v0_2::PreparedScientificPreviewSource prepared{};
    android_raw_adapter_bridge::v0_1::OpenedDngSource openedSource{};
    std::shared_ptr<
        scientific_master_f64_reconstruction_v0_1::
            ResearchEdgeAwareMeasuredPreservingReconstructionF64>
        reconstruction;
    scientific_master_streaming_binding::v0_2::Result scientific{};
    technical_backplane_phase2::v0_1::Phase2Result phase2{};

    std::unique_ptr<
        scientific_master_linear_dng_projection::v0_1::
            StreamingScientificMasterTileSource>
        masterSource;
    std::unique_ptr<
        truthnegative_dense_local_field_adapter::v0_4::
            SourceFieldAdapter>
        fieldSource;

    truthnegative_continuous::v0_5::AuthorityFieldSummary authorityField{};
    truthnegative_continuous::v0_5::State truthNegativeState{};

    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
};

Status prepare(
    int sourceFd,
    std::size_t maxSourceResidentBytes,
    std::size_t maxLogicalResidentBytes,
    Context& out) noexcept;

bool reverify(const Context& context) noexcept;

}  // namespace truthraw::android_truthnegative_pipeline::v0_1
