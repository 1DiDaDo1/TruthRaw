#pragma once

#include "full_frame_streaming_v0_1.h"
#include "truthraw_sha256_v0_69.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace truthraw::universal_local_model_bank_holdout::v0_1 {

namespace stream = truthraw::streaming_v0_1;
using Digest = truthraw::sha256_v0_69::Digest;

inline constexpr const char* kSchemaName =
    "D.RAW/UniversalLocalModelBankHoldoutAudit/0.1";
inline constexpr std::uint32_t kLatticeFractionBits = 20u;
inline constexpr std::uint64_t kLatticeUnitsPerSourcePixel =
    1ull << kLatticeFractionBits;
inline constexpr std::uint32_t kHoldoutPeriod = 32u;
inline constexpr int kSupportRadius = 8;
inline constexpr int kCoreTileExtent = 256;

enum class ModelId : std::uint32_t {
    None = 0u,
    RobustMedianConstant = 1u,
    DirectionalLine = 2u,
    AffinePlane = 3u,
    QuadraticSurface = 4u,
};

struct Binding final {
    Digest sourceEvidenceSha256{};
    Digest scientificMasterSha256{};
    Digest authorityFieldSha256{};
    Digest truthNegativeStateSha256{};
};

struct ModelPrediction final {
    ModelId model = ModelId::None;
    bool valid = false;
    double estimate = 0.0;
    double residualRms = 0.0;
    double selectionScore = 0.0;
    double predictionVariance = 0.0;
    std::uint32_t supportSamples = 0u;
    std::int32_t directionDx = 0;
    std::int32_t directionDy = 0;
};

struct HoldoutRecord final {
    std::uint32_t x = 0u;
    std::uint32_t y = 0u;
    std::uint32_t cfaPhase = 0u;
    std::uint32_t channel = 0u;
    double actual = 0.0;
    std::uint32_t supportSamples = 0u;
    ModelPrediction robustConstant{};
    ModelPrediction directional{};
    ModelPrediction affine{};
    ModelPrediction quadratic{};
    ModelPrediction selected{};
    bool baselineValid = false;
    double baselineEstimate = 0.0;
    double baselineAbsError = 0.0;
    double selectedAbsError = 0.0;
    ModelId oracleBestModel = ModelId::None;
    double oracleBestAbsError = 0.0;
    double selectorRegretAbsError = 0.0;
};

struct ModelAggregate final {
    std::uint64_t valid = 0u;
    std::uint64_t selected = 0u;
    std::uint64_t lowerAbsErrorThanBaseline = 0u;
    long double absErrorSum = 0.0L;
    long double squaredErrorSum = 0.0L;
    long double signedErrorSum = 0.0L;
    long double residualRmsSum = 0.0L;
};

struct Metrics final {
    std::uint64_t holdouts = 0u;
    std::uint64_t targetCensoredSkipped = 0u;
    std::uint64_t noModelSelected = 0u;
    std::uint64_t baselineValid = 0u;
    std::uint64_t selectedValid = 0u;
    std::uint64_t selectedLowerAbsErrorThanBaseline = 0u;
    std::uint64_t baselineLowerAbsErrorThanSelected = 0u;
    std::uint64_t equalSelectedBaselineAbsError = 0u;
    std::array<std::uint64_t,4u> holdoutCfaPhase{};
    std::array<std::uint64_t,4u> selectedCfaPhase{};
    std::array<ModelAggregate,5u> model{};
    long double baselineAbsErrorSum = 0.0L;
    long double baselineSquaredErrorSum = 0.0L;
    long double baselineSignedErrorSum = 0.0L;
    long double selectedAbsErrorSum = 0.0L;
    long double selectedSquaredErrorSum = 0.0L;
    long double selectedSignedErrorSum = 0.0L;
    long double selectorRegretAbsErrorSum = 0.0L;
    double maxSelectorRegretAbsError = 0.0;
};

struct Report final {
    std::string json{};
    Digest jsonSha256{};
    Digest holdoutStreamSha256{};
    Metrics global{};
    std::vector<HoldoutRecord> holdouts{};
    bool targetValueUsedByModels = false;
    bool targetValueUsedBySelector = false;
    bool holdoutErrorUsedBySelector = false;
    bool postRevealOracleUsedBySelector = false;
    bool lensCalibrationUsed = false;
    bool cameraModelUsed = false;
    bool vendorMappingUsed = false;
    bool measuredAnchorsModified = false;
    bool unanchoredValuesPromotedToMeasured = false;
    bool modelBankAppliedToScientificMaster = false;
    bool candidateApplied = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

bool run(
    stream::IRawTileSource& source,
    const Binding& binding,
    Report& out) noexcept;

} // namespace truthraw::universal_local_model_bank_holdout::v0_1
