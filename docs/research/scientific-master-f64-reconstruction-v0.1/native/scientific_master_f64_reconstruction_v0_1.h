#pragma once

#include "truthraw/core.h"

namespace truthraw::scientific_master_f64_reconstruction_v0_1 {

// Parallel mixed-precision research backend over the frozen v4.7i ABI.
//
// Input/output storage remains Float32. Branch-sensitive directional,
// weighting, support-bound and colour-difference computations are Float64.
// The measured CFA component is written directly from the original Float32
// Stage-2 sample to preserve its exact bit pattern.
class ResearchEdgeAwareMeasuredPreservingReconstructionF64 final
    : public IReconstructionBackend {
public:
    ReconstructionQuality quality() const override {
        return ReconstructionQuality::ResearchBackend;
    }

    const char* name() const override {
        return "research_edge_aware_support_limited_measured_preserving_f64_v0_1";
    }

    int requiredHalo() const override { return 3; }

    Status reconstructTile(
        const float* stage2FullTile,
        int tileW,
        int tileH,
        int globalHx0,
        int globalHy0,
        int coreX0,
        int coreY0,
        int coreW,
        int coreH,
        CfaPattern cfa,
        float* coreCameraRgb) override;
};

} // namespace truthraw::scientific_master_f64_reconstruction_v0_1
