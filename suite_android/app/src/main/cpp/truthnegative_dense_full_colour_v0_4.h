#pragma once

#include "scientific_master_digest_v0_1.h"
#include "scientific_master_linear_dng_projection_v0_1.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace truthraw::truthnegative_dense_full_colour::v0_4 {

namespace float_dng =
    truthraw::scientific_master_linear_dng_projection::v0_1;
namespace digest = truthraw::scientific_master_digest::v0_1;

inline constexpr std::uint32_t kScale = 4u;
inline constexpr std::uint32_t kCamera5SourceWidth = 4080u;
inline constexpr std::uint32_t kCamera5SourceHeight = 3072u;
inline constexpr std::uint32_t kCamera5TargetWidth = 16320u;
inline constexpr std::uint32_t kCamera5TargetHeight = 12288u;

struct Contract final {
    std::uint32_t sourceWidth = 0u;
    std::uint32_t sourceHeight = 0u;
    std::uint32_t targetWidth = 0u;
    std::uint32_t targetHeight = 0u;
    std::uint32_t scale = kScale;
    bool createsNewEvidence = false;
    bool impliesPhysicalSensorGeometry = false;
    bool sourceMasterAnchorValuesPreserved = true;
    bool appearanceApplied = false;
    std::uint32_t physicalFrameCount = 1u;
    std::uint32_t independentEvidenceCount = 1u;
};

class DenseFullColourTileSource final
    : public float_dng::IScientificMasterTileSource {
public:
    DenseFullColourTileSource(
        float_dng::IScientificMasterTileSource& source,
        std::uint32_t sourceWidth,
        std::uint32_t sourceHeight) noexcept;
    ~DenseFullColourTileSource() override;

    DenseFullColourTileSource(const DenseFullColourTileSource&) = delete;
    DenseFullColourTileSource& operator=(const DenseFullColourTileSource&) = delete;

    bool valid() const noexcept;
    const std::string& error() const noexcept;
    const Contract& contract() const noexcept;

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

float_dng::Status compute_projected_raster_sha256(
    float_dng::IScientificMasterTileSource& source,
    std::uint32_t width,
    std::uint32_t height,
    float_dng::Hash256& out) noexcept;

std::string projection_manifest(const Contract& contract);

}  // namespace truthraw::truthnegative_dense_full_colour::v0_4
