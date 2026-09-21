#pragma once

#include "scientific_master_linear_dng_projection_v0_1.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace truthraw::truthnegative_dense_projection::v0_3 {

namespace float_dng = truthraw::scientific_master_linear_dng_projection::v0_1;

inline constexpr std::uint32_t kScale = 4u;
inline constexpr const char* kMethodId =
    "PIXEL_CENTER_BILINEAR_F32_EXACT_ORDER_V0_3";
inline constexpr const char* kAuthority =
    "RECONSTRUCTED_DENSE_SUPPORT";
inline constexpr std::uint32_t kMeasuredTargetClaimCount = 0u;

struct Geometry final {
    std::uint32_t sourceWidth = 0u;
    std::uint32_t sourceHeight = 0u;
    std::uint32_t targetWidth = 0u;
    std::uint32_t targetHeight = 0u;
};

struct Result final {
    Geometry geometry{};
    float_dng::Hash256 projectedRasterSha256{};
    std::uint64_t projectedPixels = 0u;
    std::uint64_t negativeComponentCount = 0u;
    std::uint64_t overOneComponentCount = 0u;
    std::size_t logicalResidentUpperBound = 0u;
    bool projectedRasterIdentityAvailable = false;
    bool createsNewEvidence = false;
    bool impliesPhysicalSensorGeometry = false;
    std::uint32_t measuredTargetClaimCount = 0u;
    std::uint32_t physicalFrameCount = 1u;
    std::uint32_t independentEvidenceCount = 1u;
    std::string methodId = kMethodId;
    std::string targetAuthority = kAuthority;
};

class DenseProjectionTileSource final : public float_dng::IScientificMasterTileSource {
public:
    DenseProjectionTileSource(
        float_dng::IScientificMasterTileSource& scientificMaster,
        std::uint32_t sourceWidth,
        std::uint32_t sourceHeight) noexcept;
    ~DenseProjectionTileSource() override;

    DenseProjectionTileSource(const DenseProjectionTileSource&) = delete;
    DenseProjectionTileSource& operator=(const DenseProjectionTileSource&) = delete;

    Geometry geometry() const noexcept;
    bool valid() const noexcept;
    const std::string& error() const noexcept;

    std::size_t residentBytesUpperBound() const noexcept override;

    float_dng::Status readCameraNativeTile(
        std::uint32_t x,
        std::uint32_t y,
        std::uint32_t width,
        std::uint32_t height,
        float* rgb,
        std::size_t floatCount) noexcept override;

private:
    struct Impl;
    float_dng::IScientificMasterTileSource& source_;
    std::unique_ptr<Impl> impl_;
};

float_dng::Status compute_projected_raster_identity(
    DenseProjectionTileSource& source,
    Result& out) noexcept;

std::string authority_manifest(const Result& result);

}  // namespace truthraw::truthnegative_dense_projection::v0_3
