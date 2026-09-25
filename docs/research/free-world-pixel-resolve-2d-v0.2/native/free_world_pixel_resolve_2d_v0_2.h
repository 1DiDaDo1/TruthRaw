#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace truthraw::free_world_pixel_resolve_2d::v0_2 {

inline constexpr const char* kMethodId =
    "FREE_WORLD_PIXEL_RESOLVE_2D_AREA_BILINEAR_F64_V0_2";
inline constexpr const char* kCreationRole =
    "AREA_INTEGRATED_CONTINUOUS_PROJECTION";
inline constexpr std::uint32_t kMeasuredTargetClaimCount = 0u;

enum class SourceCreationRole : std::uint8_t {
    Unknown = 0u,
    SourceMeasuredCfa = 1u,
    ScientificReconstruction = 2u,
    DenseProjection = 3u,
    RestorationDerivative = 4u,
};

enum class SourceAuthority : std::uint8_t {
    CalibratedEstimate = 0u,
    Reconstructed = 1u,
    Censored = 2u,
    Unknown = 3u,
};

enum class BoundDomain : std::uint8_t {
    None = 0u,
    SourceRawCode = 1u,
    SceneLinear = 2u,
};

enum class ResolvedAuthority : std::uint8_t {
    Reconstructed = 0u,
    Censored = 1u,
    Unknown = 2u,
};

struct SourceChannelState final {
    double value = 0.0;
    SourceCreationRole role = SourceCreationRole::Unknown;
    SourceAuthority authority = SourceAuthority::Unknown;
    bool uncertaintyKnown = false;
    double p95Uncertainty = 0.0;
    bool boundKnown = false;
    double lowerBound = 0.0;
    BoundDomain boundDomain = BoundDomain::None;
    std::uint8_t contributionMask = 0u;
};

struct SourcePixel final {
    std::array<SourceChannelState, 3u> channel{};
};

class IScenePlaneSource {
public:
    virtual ~IScenePlaneSource() = default;
    virtual std::uint32_t width() const noexcept = 0;
    virtual std::uint32_t height() const noexcept = 0;
    virtual bool readPixel(
        std::uint32_t x,
        std::uint32_t y,
        SourcePixel& out) const noexcept = 0;
};

struct AxisContribution final {
    std::uint32_t index = 0u;
    double weight = 0.0;
};

struct FootprintContribution final {
    std::uint32_t x = 0u;
    std::uint32_t y = 0u;
    double weight = 0.0;
};

struct ChannelSupportSummary final {
    double calibratedEstimateWeight = 0.0;
    double reconstructedWeight = 0.0;
    double censoredWeight = 0.0;
    double unknownWeight = 0.0;

    double sourceMeasuredCfaWeight = 0.0;
    double scientificReconstructionWeight = 0.0;
    double denseProjectionWeight = 0.0;
    double restorationDerivativeWeight = 0.0;
    double unknownRoleWeight = 0.0;

    bool uncertaintyKnown = false;
    double p95Uncertainty = 0.0;

    bool boundKnown = false;
    double lowerBound = 0.0;
    BoundDomain boundDomain = BoundDomain::None;

    std::uint8_t contributionMask = 0u;
    ResolvedAuthority authority = ResolvedAuthority::Unknown;
};

struct ResolvedPixel final {
    std::array<double, 3u> sceneLinear{};
    std::array<ChannelSupportSummary, 3u> support{};
    std::vector<FootprintContribution> footprint;
    bool createsNewEvidence = false;
    std::uint32_t measuredTargetClaimCount = kMeasuredTargetClaimCount;
    std::uint32_t physicalFrameCount = 1u;
    std::uint32_t independentEvidenceCount = 1u;
    std::string methodId = kMethodId;
    std::string creationRole = kCreationRole;
};

struct Geometry final {
    std::uint32_t sourceWidth = 0u;
    std::uint32_t sourceHeight = 0u;
    std::uint32_t targetWidth = 0u;
    std::uint32_t targetHeight = 0u;
};

struct ViewDisplayPolicy final {
    std::string appearanceModelId;
    std::string displayTargetId;
};

struct OutputIntentBinding final {
    std::string sceneStateId;
    std::string appearanceModelId;
    std::string displayTargetId;
    bool scientificSceneMutationAllowed = false;
};

class Resolver final {
public:
    Resolver(
        const IScenePlaneSource& source,
        std::uint32_t targetWidth,
        std::uint32_t targetHeight) noexcept;

    bool valid() const noexcept;
    const std::string& error() const noexcept;
    Geometry geometry() const noexcept;

    bool resolvePixel(
        std::uint32_t targetX,
        std::uint32_t targetY,
        ResolvedPixel& out) const noexcept;

    bool resolveRaster(
        std::vector<ResolvedPixel>& out,
        bool includeFootprints) const noexcept;

private:
    const IScenePlaneSource& source_;
    Geometry geometry_{};
    bool valid_ = false;
    std::string error_;
};

std::vector<AxisContribution> axisAreaWeights(
    std::uint32_t sourceCount,
    std::uint32_t targetIndex,
    std::uint32_t targetCount);

OutputIntentBinding bindOutputIntent(
    const std::string& sceneStateId,
    const ViewDisplayPolicy& policy);

const char* toString(SourceCreationRole role) noexcept;
const char* toString(SourceAuthority authority) noexcept;
const char* toString(BoundDomain domain) noexcept;
const char* toString(ResolvedAuthority authority) noexcept;

}  // namespace truthraw::free_world_pixel_resolve_2d::v0_2
