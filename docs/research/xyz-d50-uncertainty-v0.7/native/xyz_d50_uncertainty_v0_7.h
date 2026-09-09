#pragma once

#include "camera_rgb_covariance_v0_6.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace truthraw_v07 {

enum class MatrixAuthorityV07 : std::uint8_t {
    Unresolved = 0,
    SourceBoundMetadataDerived = 1,
    IndependentTargetCalibration = 2,
    ExplicitResearchFixture = 3,
};

enum class XyzPropagationKnowledgeV07 : std::uint8_t {
    Unresolved = 0,
    ComponentVarianceBounds = 1,
    ExactFullCovariance = 2,
};

struct StatusV07 {
    bool ok = true;
    std::string message;
    explicit operator bool() const { return ok; }
    static StatusV07 success() { return {}; }
    static StatusV07 error(std::string m) { return {false, std::move(m)}; }
};

struct CameraToXyzD50MatrixV07 {
    // Row-major 3x3 linear transform from the exact camera-RGB scene-linear
    // coordinate used by v0.6 to XYZ with D50 PCS semantics.
    std::array<double, 9> m {
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN()
    };
    MatrixAuthorityV07 authority = MatrixAuthorityV07::Unresolved;
    std::string sourceId;
    bool declaredLinearSceneTransform = false;
    bool declaredOutputXyzD50 = false;
};

struct VarianceOuterBoundV07 {
    bool valid = false;
    double lower = std::numeric_limits<double>::quiet_NaN();
    double upper = std::numeric_limits<double>::quiet_NaN();
    bool exact = false;
};

struct PixelXyzD50UncertaintyV07 {
    // Exact XYZ marginal variances when known; otherwise NaN with mask bit clear.
    std::array<double, 3> variance {
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN()
    };
    std::uint8_t varianceKnownMask = 0; // X,Y,Z bits 0,1,2

    // Exact output covariance pair order XY, XZ, YZ. Unknown remains NaN.
    std::array<double, 3> covariance {
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN()
    };
    std::uint8_t covarianceKnownMask = 0; // XY,XZ,YZ bits 0,1,2

    std::array<VarianceOuterBoundV07, 3> varianceBounds;
    bool fullPsdCertified = false;
    XyzPropagationKnowledgeV07 knowledge = XyzPropagationKnowledgeV07::Unresolved;
    std::string matrixSourceId;
    std::string claimBoundary =
        "XYZ(D50) uncertainty only. Matrix provenance is caller-supplied and not calibrated here. "
        "Unknown camera-RGB correlation is never replaced by independence.";
};

struct XyzD50UncertaintyTileV07 {
    int originX = 0;
    int originY = 0;
    int width = 0;
    int height = 0;
    std::vector<PixelXyzD50UncertaintyV07> pixels;
    std::string matrixSourceId;
    std::string claimBoundary;
};

StatusV07 validate_camera_to_xyz_d50_matrix_v0_7(
    const CameraToXyzD50MatrixV07& matrix);

StatusV07 propagate_camera_rgb_uncertainty_to_xyz_d50_v0_7(
    const truthraw_v06::PixelCameraRgbCovarianceV06& cameraRgb,
    const CameraToXyzD50MatrixV07& matrix,
    PixelXyzD50UncertaintyV07& out);

StatusV07 propagate_camera_rgb_tile_to_xyz_d50_v0_7(
    const truthraw_v06::CameraRgbCovarianceTileV06& cameraRgb,
    const CameraToXyzD50MatrixV07& matrix,
    XyzD50UncertaintyTileV07& out);

const char* xyz_propagation_knowledge_name_v0_7(
    XyzPropagationKnowledgeV07 knowledge);

const char* matrix_authority_name_v0_7(MatrixAuthorityV07 authority);

} // namespace truthraw_v07
