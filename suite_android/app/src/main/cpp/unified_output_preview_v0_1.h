#pragma once

#include "scientific_master_linear_dng_projection_v0_1.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace truthraw::unified_output_preview::v0_1 {

namespace float_dng =
    truthraw::scientific_master_linear_dng_projection::v0_1;

inline constexpr std::uint32_t kMagic = 0x31504f55u; // UOP1
inline constexpr std::uint32_t kMaxEdgeHardLimit = 1024u;

enum class SourceSpace : std::uint8_t {
    CameraNative = 1,
    LinearSrgb = 2,
    XyzD50 = 3,
};

struct Descriptor final {
    std::uint32_t sourceWidth = 0u;
    std::uint32_t sourceHeight = 0u;
    std::uint32_t maxEdge = 384u;
    SourceSpace sourceSpace = SourceSpace::CameraNative;
    std::array<float,9> cameraToXyzD50{1,0,0,0,1,0,0,0,1};
    std::string outputRole;
};

struct Result final {
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    std::uint64_t sampledPrimaryPixels = 0u;
    std::uint64_t negativeDisplayClampedComponents = 0u;
    std::uint64_t overOneDisplayClampedComponents = 0u;
    std::vector<std::uint32_t> argb8888;
    bool primaryTileSourceUsedDirectly = false;
    bool appearanceAddedByPreview = false;
    bool scientificWritebackAllowed = false;
};

bool render(
    float_dng::IScientificMasterTileSource& primary,
    const Descriptor& descriptor,
    Result& out) noexcept;

bool write_uop1_fd(
    int fd,
    const Descriptor& descriptor,
    const Result& result) noexcept;

const char* schema_name() noexcept;
const char* source_space_name(SourceSpace space) noexcept;

} // namespace truthraw::unified_output_preview::v0_1
