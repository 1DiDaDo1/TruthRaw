#pragma once
#include "truthraw/core.h"
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace truthraw {

enum class TruthRangeGaugeModeV02 : int {
    SelfGauge = 0,
    ExternalRelativeGauge = 1,
    PhysicalAbsoluteGauge = 2,
};

enum class TruthRangeSupportV02 : std::uint8_t {
    MeasuredUncensored = 0,
    MeasuredCensoredLowerBound = 1,
    ReconstructedWeak = 2,
    ReconstructedStrong = 3,
    Unknown = 4,
};

enum class TruthRangeCensorV02 : std::uint8_t {
    None = 0,
    HighClipped = 1,
    DarkNoiseLimited = 2,
};

struct TruthRangeGaugeV02 {
    TruthRangeGaugeModeV02 mode = TruthRangeGaugeModeV02::SelfGauge;
    double L0 = 1.0;
    std::string gaugeId = "unset";
    bool crossSceneComparable = false;
    bool absolutePhysicalUnits = false;
};

struct LatentSceneBindingV02 {
    std::string reconstructionBackend;
    std::string reconstructionCoreCppSha256;
    std::string reconstructionCoreHSha256;
    std::string uncertaintyModelSha256;
    std::string uncertaintyBindingSha256;
    std::string sceneScaleId;
    bool gainMapAppliedExactlyOnce = true;
    bool exposureNormalizedToCommonScene = false;
    bool gainNormalizedToCommonScene = false;
};

struct LatentCameraSceneV02 {
    int width = 0;
    int height = 0;
    CfaPattern cfa = CfaPattern::BGGR;
    std::vector<float> stage2Cfa;               // N signed Stage-2 samples
    std::vector<float> cameraRgb;               // 3*N signed reconstructed camera RGB
    std::vector<std::uint8_t> measuredChannel;  // N values: 0 R, 1 G, 2 B
    std::vector<std::uint8_t> sourceHighCensor; // N bool
    std::vector<float> sourceHighCensorLower;   // N lower bound in Stage-2 scale at source WhiteLevel
    LatentSceneBindingV02 binding;
};

struct LatentUncertaintyV02 {
    bool valid = false;
    float p50Abs = std::numeric_limits<float>::quiet_NaN();
    float p95Abs = std::numeric_limits<float>::quiet_NaN();
    std::string source = "UNRESOLVED";
};

struct TruthRangeSampleV02 {
    float muLinearSigned = 0.f;
    bool hasEstimate = false;
    double estimateEv = std::numeric_limits<double>::quiet_NaN();

    // Empirical error-quantile intervals transformed into TruthRange.
    double p50LowerEv = std::numeric_limits<double>::quiet_NaN();
    double p50UpperEv = std::numeric_limits<double>::quiet_NaN();
    double p95LowerEv = std::numeric_limits<double>::quiet_NaN();
    double p95UpperEv = std::numeric_limits<double>::quiet_NaN();

    // Direct evidence interval. A clipped measured channel has finite lower and +inf upper.
    double evidenceLowerEv = -std::numeric_limits<double>::infinity();
    double evidenceUpperEv =  std::numeric_limits<double>::infinity();

    TruthRangeSupportV02 support = TruthRangeSupportV02::Unknown;
    TruthRangeCensorV02 censor = TruthRangeCensorV02::None;
    std::string gaugeId;
    std::string uncertaintySource;
};

Status build_latent_camera_scene_v0_2(
    const DecodedDngFrame& frame,
    const TilePolicy& tile,
    IReconstructionBackend& reconstruction,
    const LatentSceneBindingV02& binding,
    LatentCameraSceneV02& out);

Status derive_self_gauge_v0_2(
    const LatentCameraSceneV02& scene,
    TruthRangeGaugeV02& gauge,
    double quantile = 0.5,
    double borderFraction = 0.10);

Status validate_gauge_for_scene_v0_2(
    const LatentCameraSceneV02& scene,
    const TruthRangeGaugeV02& gauge);

TruthRangeSampleV02 map_latent_channel_to_truthrange_v0_2(
    float muLinearSigned,
    const LatentUncertaintyV02& uncertainty,
    const TruthRangeGaugeV02& gauge,
    TruthRangeSupportV02 requestedReconstructionSupport,
    bool measuredChannel,
    bool sourceHighCensored,
    float sourceHighCensorLowerLinear);

double truthrange_ev_v0_2(double positiveL, const TruthRangeGaugeV02& gauge);

} // namespace truthraw
