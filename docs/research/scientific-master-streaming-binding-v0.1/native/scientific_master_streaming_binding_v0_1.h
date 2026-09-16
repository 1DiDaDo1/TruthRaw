#pragma once

#include "full_frame_streaming_v0_1.h"
#include "scientific_master_digest_v0_1.h"
#include "truthrange_latent_v0_2.h"

#include <cstddef>
#include <cstdint>
#include <string>

// Full-Frame Streaming v0.1 refers to the parent-namespace TileRect
// unqualified. Expose the same type through its nested namespace for this new
// integration layer without changing validated historical headers.
namespace truthraw::streaming_v0_1 {
using ::truthraw::TileRect;
}

namespace truthraw::scientific_master_streaming_binding::v0_1 {

using Hash256 = scientific_master_digest::v0_1::Sha256;

inline constexpr double kSelfGaugeQuantile = 0.5;
inline constexpr double kSelfGaugeBorderFraction = 0.10;
inline constexpr int kCanonicalCore = scientific_master_digest::v0_1::kCanonicalCellEdge;

enum class StatusCode : std::uint8_t {
    Ok = 0,
    InvalidArgument,
    SourceFailed,
    ReconstructionFailed,
    DigestFailed,
    GaugeFailed,
    BudgetExceeded,
};

struct Status final {
    StatusCode code = StatusCode::Ok;
    std::string message;
    explicit operator bool() const noexcept { return code == StatusCode::Ok; }
    static Status ok() { return {}; }
    static Status error(StatusCode code, std::string message) {
        Status out;
        out.code = code;
        out.message = std::move(message);
        return out;
    }
};

struct Options final {
    // 0 means no caller-imposed logical resident ceiling.
    std::size_t memoryBudgetBytes = 0;
};

struct Result final {
    Hash256 scientificMasterHash{};
    TruthRangeGaugeV02 zeroLineGauge{};
    LatentSceneBindingV02 sceneBinding{};

    std::uint64_t selfGaugeEligibleSamples = 0;
    std::size_t masterTilesProcessed = 0;
    std::size_t stage2GaugeScanPasses = 0;
    std::size_t logicalWorkspacePeakBytes = 0;
    std::size_t logicalResidentUpperBound = 0;

    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
};

// Builds the Scientific Master identity from the exact camera-native RGB
// reconstruction before camera_to_xyz()/appearance. The scientific pass uses a
// fixed 64x64 canonical core grid independent of preview/runtime tile policy.
//
// In the same lineage, it derives the exact TruthRange v0.2 self-gauge median
// from positive, finite, uncensored Stage-2 evidence inside the canonical 10%
// border exclusion. The median is selected with bounded-memory float32 radix
// counting; no full-frame Stage-2 or RGB master is materialized.
Status bind_scientific_master_streaming(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const Options& options,
    Result& out) noexcept;

const char* status_name(StatusCode code) noexcept;

}  // namespace truthraw::scientific_master_streaming_binding::v0_1
