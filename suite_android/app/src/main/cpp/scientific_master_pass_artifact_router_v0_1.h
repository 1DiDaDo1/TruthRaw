#pragma once

#include "scientific_master_exact_gauge_retained_artifact_v0_3.h"

#include <cstddef>
#include <cstdint>

namespace truthraw::scientific_master_pass_artifact_router::v0_1 {

namespace smsb2 = scientific_master_streaming_binding::v0_2;
namespace ega3 = scientific_master_exact_gauge_retained_artifact::v0_3;

inline constexpr const char* kRouterId =
    "SCIENTIFIC_MASTER_PASS_ARTIFACT_ROUTER";
inline constexpr const char* kRouterVersion = "0.1";
inline constexpr const char* kExactGaugeCableId =
    "EXACT_GAUGE_RETAINED_FLOAT32_BITS";
inline constexpr const char* kExactGaugeCableVersion = "0.3";

struct Diagnostics final {
    const char* routerId = kRouterId;
    const char* routerVersion = kRouterVersion;
    const char* cableId = kExactGaugeCableId;
    const char* cableVersion = kExactGaugeCableVersion;
    const char* routeName = "unresolved";
    const char* fallbackReasonName = "none";

    std::uint64_t eligibleRetainedSamples = 0u;
    std::size_t retainedBytesUsed = 0u;
    std::size_t stage2GaugeScanPassesActuallyUsed = 0u;
    std::size_t pass2Stage2TileReadsAvoided = 0u;

    bool optimizationApplied = false;
    bool sourceValuesModified = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;

    // Router/cable selection and its timing/diagnostics are never scientific
    // evidence and cannot independently alter Scientific Master authority.
    bool isScientificEvidence = false;
    bool mayChangeScientificAuthority = false;
};

smsb2::Status bindObserved(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const smsb2::Options& options,
    smsb2::ICanonicalTileObserver& observer,
    smsb2::Result& out,
    Diagnostics& diagnostics) noexcept;

}  // namespace truthraw::scientific_master_pass_artifact_router::v0_1
