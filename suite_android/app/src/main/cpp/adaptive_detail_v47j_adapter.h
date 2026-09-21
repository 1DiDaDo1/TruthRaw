#pragma once

#include "truthraw/core.h"

namespace truthraw::adaptive_detail_v47j_adapter {

float noise_sigma_2pct_from_metadata(const DngMetadata& metadata) noexcept;

// Compatibility adapter for the frozen v4.7i IAppearanceBackend ABI.
// The scientific v4.7i reconstruction translation unit remains untouched.
class AdaptiveDetailedCrispAppearanceV47j final : public IAppearanceBackend {
public:
    explicit AdaptiveDetailedCrispAppearanceV47j(
        float noiseSigmaAt2Pct,
        float detailMix = 1.0f) noexcept
        : noiseSigmaAt2Pct_(noiseSigmaAt2Pct),
          detailMix_(detailMix) {}

    AppearanceProfile profile() const override {
        return AppearanceProfile::ExternalProfile;
    }

    const char* name() const override {
        return "adaptive_detailed_crisp_multiband_hard_edge_guard_v47j";
    }

    int requiredHalo() const override { return 5; }

    const char* colorFidelityPolicy() const override {
        return "luminance_only_rgb_direction_preserved_no_semantic_segmentation";
    }

    Status applyTile(
        const float* neutralRgbTile,
        int tileW,
        int tileH,
        int coreX0,
        int coreY0,
        int coreW,
        int coreH,
        float* coreAppearanceRgb) const override;

    float noiseSigmaAt2Pct() const noexcept { return noiseSigmaAt2Pct_; }
    float detailMix() const noexcept { return detailMix_; }

private:
    float noiseSigmaAt2Pct_ = 0.0f;
    float detailMix_ = 1.0f;
};

}  // namespace truthraw::adaptive_detail_v47j_adapter
