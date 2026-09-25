#pragma once

#include "truthnegative_n2_cfa_audit_v0_1.h"
#include "truthraw/core.h"
#include "truthraw_sha256_v0_69.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace truthraw::truthnegative_n2_full_colour_candidate::v0_1 {

namespace audit = truthraw::truthnegative_n2_cfa_audit::v0_1;
using Digest = truthraw::sha256_v0_69::Digest;

struct Input final {
    const float* stage2 = nullptr;
    int tileWidth = 0;
    int tileHeight = 0;
    int globalHx0 = 0;
    int globalHy0 = 0;
    int coreX0 = 0;
    int coreY0 = 0;
    int coreWidth = 0;
    int coreHeight = 0;
    CfaPattern cfa = CfaPattern::BGGR;
    const audit::Result* n2Audit = nullptr;
    // Optional conservative closure: an admitted correction is suppressed when
    // it lies within this Chebyshev radius of any protected core output pixel.
    // Android binds this to backend.requiredHalo().
    int reconstructionInfluenceRadius = 0;
    bool closeProtectionOverReconstructionSupport = false;
};

struct Result final {
    std::vector<float> baselineCameraRgb{};
    std::vector<float> candidateCameraRgb{};
    Digest candidateIdentitySha256{};
    std::uint64_t correctedStage2Sites = 0u;
    std::uint64_t supportGuardSuppressedStage2Sites = 0u;
    std::uint64_t protectedCorePixels = 0u;
    std::uint64_t protectedCoreChangedRgbChannels = 0u;
    std::uint64_t changedRgbChannels = 0u;
    double maxAbsRgbDelta = 0.0;
    bool supportGuardApplied = false;
    bool sourceStage2Modified = false;
    bool scientificWritebackAllowed = false;
    bool createsNewEvidence = false;
};

bool reconstruct(
    const Input& input,
    IReconstructionBackend& backend,
    Result& out) noexcept;

} // namespace truthraw::truthnegative_n2_full_colour_candidate::v0_1
