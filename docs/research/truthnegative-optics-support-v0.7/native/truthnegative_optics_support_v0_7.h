#pragma once

#include "free_world_pixel_resolve_2d_v0_2.h"
#include "truthraw_sha256_v0_69.h"

#include <cstdint>
#include <string>
#include <vector>

namespace truthraw::truthnegative_optics_support::v0_7 {

namespace free_world = truthraw::free_world_pixel_resolve_2d::v0_2;
using Digest = truthraw::sha256_v0_69::Digest;

inline constexpr const char* kSchemaName =
    "TruthNegativeOpticsSupport/0.7";
inline constexpr const char* kMethodId =
    "CALIBRATION_BOUND_PSF_SUPPORT_PROPAGATION_V0_7";

enum class CalibrationAuthority : std::uint8_t {
    Measured = 1u,
    CalibratedEstimate = 2u,
    Inferred = 3u,
    Unknown = 4u,
};

struct CalibrationInput final {
    Digest sourceEvidenceSha256{};
    Digest lensIdentitySha256{};
    Digest sensorIdentitySha256{};
    Digest calibrationEvidenceSha256{};
    CalibrationAuthority authority = CalibrationAuthority::Unknown;

    std::uint32_t kernelWidth = 0u;
    std::uint32_t kernelHeight = 0u;
    std::vector<double> psfKernel;

    double mtf50XCyclesPerPixel = 0.0;
    double mtf50YCyclesPerPixel = 0.0;
};

struct CalibrationState final {
    CalibrationInput input{};
    Digest stateSha256{};
    std::vector<double> normalizedPsfKernel;
    bool scientificSupportUseAllowed = false;
    bool deconvolutionAllowed = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
    std::string methodId = kMethodId;
};

struct EffectiveSupport final {
    std::vector<free_world::FootprintContribution> footprint;
    Digest calibrationStateSha256{};
    Digest supportSha256{};
    double weightSum = 0.0;
    bool numericSceneValueChanged = false;
    bool authorityUpgraded = false;
    bool deconvolutionApplied = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

bool finalizeCalibration(
    const CalibrationInput& input,
    CalibrationState& out) noexcept;

bool propagateSupport(
    const CalibrationState& calibration,
    std::uint32_t sourceWidth,
    std::uint32_t sourceHeight,
    const free_world::ResolvedPixel& query,
    EffectiveSupport& out) noexcept;

const char* toString(CalibrationAuthority authority) noexcept;
const char* schema_name() noexcept;

}  // namespace truthraw::truthnegative_optics_support::v0_7
