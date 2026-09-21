#pragma once

#include "scientific_master_linear_dng_projection_v0_1.h"
#include "full_frame_streaming_v0_1.h"
#include "truthraw/core.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace truthraw::advanced_render_edit::v0_1 {

namespace float_dng =
    truthraw::scientific_master_linear_dng_projection::v0_1;

inline constexpr std::uint32_t kFlagLight = 1u << 0u;
inline constexpr std::uint32_t kFlagHdr = 1u << 1u;
inline constexpr std::uint32_t kFlagDetail = 1u << 2u;
inline constexpr std::uint32_t kFlagRestoration = 1u << 3u;
inline constexpr std::uint32_t kAllowedFlags =
    kFlagLight | kFlagHdr | kFlagDetail | kFlagRestoration;

// Returns the exact storage transform used for the Render/Edit DNG:
// extended linear-sRGB derivative -> XYZ-D50 Float32 LinearRaw.
const std::array<float, 9>& linear_srgb_to_xyz_d50_matrix() noexcept;

// Read-only deterministic derivative source. The inherited method name is a
// historical ABI name from the Float32 DNG writer; this implementation returns
// the declared developed extended-linear sRGB derivative, NOT camera-native
// Scientific Master RGB.
//
// Pipeline:
// Scientific Master replay -> authorized camera->XYZ-D50 -> linear-sRGB
// -> optional v4.7j Detail -> optional aesthetic Restoration
// -> optional Open-World Light.
//
// Natural HDR is intentionally recipe/preview-only here while v0.84 output
// authority still contains UNKNOWN channels. Output Acutance v4.7k is excluded
// because it belongs to final-size output, not an editable master.
class ExtendedLinearSrgbTileSource final
    : public float_dng::IScientificMasterTileSource {
public:
    ExtendedLinearSrgbTileSource(
        streaming_v0_1::IRawTileSource& source,
        IReconstructionBackend& reconstruction,
        const std::array<float, 9>& cameraToXyzD50,
        std::uint32_t flags,
        const ExposurePlan& exposure) noexcept;
    ~ExtendedLinearSrgbTileSource() override;

    ExtendedLinearSrgbTileSource(const ExtendedLinearSrgbTileSource&) = delete;
    ExtendedLinearSrgbTileSource& operator=(const ExtendedLinearSrgbTileSource&) = delete;

    std::size_t residentBytesUpperBound() const noexcept override;

    float_dng::Status readCameraNativeTile(
        std::uint32_t x,
        std::uint32_t y,
        std::uint32_t width,
        std::uint32_t height,
        float* rgb,
        std::size_t floatCount) noexcept override;

    std::uint32_t flags() const noexcept;
    bool appearanceBakedIntoPrimary() const noexcept;
    bool hdrBakedIntoPrimary() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Hashes the derivative in the same canonical 64x64 float grid used by the DNG
// writer. The writer will replay the source again and reject the artifact unless
// it reproduces this exact digest.
float_dng::Status compute_projected_raster_sha256(
    float_dng::IScientificMasterTileSource& source,
    std::uint32_t width,
    std::uint32_t height,
    float_dng::Hash256& out) noexcept;

}  // namespace truthraw::advanced_render_edit::v0_1
