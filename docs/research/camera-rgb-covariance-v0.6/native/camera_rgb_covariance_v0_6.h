#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>
#include <utility>

namespace truthraw_v06 {

enum class MarginalSourceV06 : std::uint8_t {
    Unresolved = 0,
    NoiseProfileGaussianEquivalent = 1,
    BackendErrorQuantilesOnly = 2,
    FutureCertifiedVariance = 3,
};

enum class CovarianceKnowledgeV06 : std::uint8_t {
    NoVarianceKnown = 0,
    PartialDiagonalOnly = 1,
    FullDiagonalOffDiagonalUnknown = 2,
    PartialOffDiagonalCertified = 3,
    FullCovarianceCertifiedPsd = 4,
};

enum class RgbPairV06 : std::uint8_t {
    RG = 0,
    RB = 1,
    GB = 2,
};

struct StatusV06 {
    bool ok = true;
    std::string message;
    explicit operator bool() const { return ok; }
    static StatusV06 success() { return {}; }
    static StatusV06 error(std::string m) { return {false, std::move(m)}; }
};

struct MarginalChannelKnowledgeV06 {
    bool sigmaEquivalentKnown = false;
    float sigmaEquivalent = std::numeric_limits<float>::quiet_NaN();
    bool p50AbsKnown = false;
    float p50Abs = std::numeric_limits<float>::quiet_NaN();
    bool p95AbsKnown = false;
    float p95Abs = std::numeric_limits<float>::quiet_NaN();
    bool topologyCertified = false;
    MarginalSourceV06 source = MarginalSourceV06::Unresolved;
};

struct PixelCameraRgbCovarianceV06 {
    // Camera-RGB marginal variances. Unknown values MUST remain NaN; zero is never a default assumption.
    std::array<float, 3> variance {
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN()
    };
    std::uint8_t varianceKnownMask = 0; // bits R,G,B => 0,1,2

    // Pair order RG, RB, GB. Unknown values MUST remain NaN.
    std::array<float, 3> covariance {
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN()
    };
    std::uint8_t covarianceKnownMask = 0; // bits RG,RB,GB => 0,1,2

    std::uint8_t topologyCertifiedMask = 0; // bits R,G,B for marginal topology only
    bool fullPsdCertified = false;
};

struct CameraRgbCovarianceTileV06 {
    int originX = 0;
    int originY = 0;
    int width = 0;
    int height = 0;
    std::vector<PixelCameraRgbCovarianceV06> pixels;
    std::string covarianceStatus = "UNRESOLVED";
    std::string claimBoundary =
        "Unknown covariance is represented as unknown/NaN, never as zero. Error quantiles alone do not imply variance. ";
};

struct CovarianceOuterBoundV06 {
    bool valid = false;
    float lower = std::numeric_limits<float>::quiet_NaN();
    float upper = std::numeric_limits<float>::quiet_NaN();
    bool exact = false;
};

StatusV06 build_pixel_covariance_from_marginals_v0_6(
    const std::array<MarginalChannelKnowledgeV06, 3>& marginal,
    PixelCameraRgbCovarianceV06& out);

StatusV06 set_certified_covariance_v0_6(
    PixelCameraRgbCovarianceV06& io,
    RgbPairV06 pair,
    float covariance);

StatusV06 validate_pixel_covariance_v0_6(
    const PixelCameraRgbCovarianceV06& v);

CovarianceKnowledgeV06 covariance_knowledge_v0_6(
    const PixelCameraRgbCovarianceV06& v);

bool can_exactly_propagate_linear_covariance_v0_6(
    const PixelCameraRgbCovarianceV06& v);

CovarianceOuterBoundV06 cauchy_outer_bound_v0_6(
    const PixelCameraRgbCovarianceV06& v,
    RgbPairV06 pair);

StatusV06 scale_pixel_covariance_v0_6(
    PixelCameraRgbCovarianceV06& io,
    float scalar);

StatusV06 init_covariance_tile_v0_6(
    int originX,
    int originY,
    int width,
    int height,
    CameraRgbCovarianceTileV06& out);

const char* covariance_knowledge_name_v0_6(CovarianceKnowledgeV06 s);

} // namespace truthraw_v06
