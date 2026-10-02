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
#include "drawnegative_v0_1.h"
#include "truthnegative_dense_local_field_adapter_v0_4.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

namespace truthraw::android_truthnegative_pipeline::v0_1 {

struct Status final {
    int code = 0;
    std::string message;
    explicit operator bool() const noexcept { return code == 0; }
};

struct PreparationTiming final {
    double duplicateAndByteSourceMs = 0.0;
    double sealSourceMs = 0.0;
    double colorBindingMs = 0.0;
    double prepareColorSourceMs = 0.0;
    double preOpenReverifyMs = 0.0;
    double openDngAdapterMs = 0.0;
    double bindScientificMasterMs = 0.0;
    double finalizePhase2Ms = 0.0;
    double summarizeAuthorityFieldMs = 0.0;
    double authorityFusedFinalizeMs = 0.0;
    bool authorityFusedIntoScientificMasterPass = false;
    bool authorityReplayPassPerformed = true;
    bool authorityDirectRecordStreamingActive = false;
    bool authorityTemporaryRecordVectorUsed = true;
    bool authorityDirectByteEncodingActive = false;
    bool authorityGenericRecordValidationBypassed = false;
    bool authorityPixelTripletEncodingActive = false;
    std::size_t authorityCanonicalRecordBytes = 0u;
    std::size_t authorityCanonicalPixelTripletBytes = 0u;
    std::size_t authorityHashBatchRecordCapacity = 0u;
    std::size_t authorityHashBatchBytes = 0u;
    std::uint64_t authorityDirectByteRecordCount = 0u;
    std::uint64_t authorityGenericFallbackRecordCount = 0u;
    std::uint64_t authorityDirectPixelTripletCount = 0u;
    std::uint64_t authorityGenericFallbackPixelCount = 0u;
    bool authorityShaDirectBlockTransportActive = false;
    std::uint64_t authorityShaDirectInputBlockTransformCount = 0u;
    std::uint64_t authorityShaBufferedInputBlockTransformCount = 0u;
    std::uint64_t authorityShaDirectInputBytes = 0u;
    double authorityDirectRecordStreamMs = 0.0;
    std::uint64_t authorityDirectRecordStreamTileCount = 0u;
    std::uint64_t authorityDirectRecordStreamRecordCount = 0u;
    std::size_t authorityAccumulatorResidentBytesUpperBound = 0u;
    double finalizeTruthNegativeMs = 0.0;
    double finalizeDrawNegativeMs = 0.0;
    double totalMs = 0.0;
};

struct SharedAcquireTiming final {
    double probeSealMs = 0.0;
    double cacheLookupMs = 0.0;
    double prepareTotalMs = 0.0;
    bool cacheHit = false;
    PreparationTiming preparation{};
};

struct Context final {
    // Serialize callers that reuse the same process-local prepared context.
    // Scientific/sample values remain read-only; this protects mutable
    // transport/audit counters inside the tile source from concurrent access.
    std::shared_ptr<std::mutex> useMutex =
        std::make_shared<std::mutex>();

    // Own a duplicated descriptor so a shared prepared context stays valid
    // after the Java ParcelFileDescriptor used to create it is closed.
    std::shared_ptr<int> ownedSourceFd;
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
    drawnegative::v0_1::State drawNegativeState{};

    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
};

Status prepare(
    int sourceFd,
    std::size_t maxSourceResidentBytes,
    std::size_t maxLogicalResidentBytes,
    Context& out,
    PreparationTiming* timing = nullptr) noexcept;

/**
 * Acquire a process-local, single-source prepared context.
 *
 * The current source is SHA-256 sealed before a cache hit is accepted.
 * Only the expensive immutable preparation is shared; each caller must still
 * run its own audit and source re-verification. The cache holds at most one
 * prepared source and therefore cannot merge evidence across observations.
 */
Status acquireShared(
    int sourceFd,
    std::size_t maxSourceResidentBytes,
    std::size_t maxLogicalResidentBytes,
    std::shared_ptr<Context>& out,
    bool& cacheHit,
    SharedAcquireTiming* timing = nullptr) noexcept;

void clearSharedCache() noexcept;

bool reverify(const Context& context) noexcept;

}  // namespace truthraw::android_truthnegative_pipeline::v0_1
