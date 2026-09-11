#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace truthraw::scientific_master_digest::v0_1 {

inline constexpr std::uint16_t kVersion = 1;
inline constexpr std::uint16_t kCanonicalCellEdge = 64;
inline constexpr std::uint16_t kChannels = 3;
inline constexpr std::uint16_t kSampleEncodingIeee754Binary32Le = 1;
inline constexpr std::uint16_t kChannelOrderCameraNativeRgb = 1;
inline constexpr std::uint16_t kScientificRoleV47iReconstructedCameraScene = 1;
inline constexpr std::size_t kSha256Bytes = 32;

using Sha256 = std::array<std::uint8_t, kSha256Bytes>;

struct TileView final {
    std::uint32_t x = 0;
    std::uint32_t y = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    const float* rgb = nullptr;
    // Number of float samples between adjacent rows. Must be >= width * 3.
    std::size_t rowStrideSamples = 0;
};

struct DigestMetrics final {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t cellColumns = 0;
    std::uint32_t cellRows = 0;
    std::uint32_t cellCount = 0;
    std::uint32_t cellsReceived = 0;
    std::size_t residentBytesUpperBound = 0;
};

// Deterministic Scientific Master identity for the camera-native reconstructed
// scene before XYZ conversion and before any appearance/tone/output transform.
//
// The digest is hierarchical and resource-profile invariant:
//   leaf = SHA256(canonical cell header || exact float32 little-endian RGB bits)
//   root = SHA256(master header || leaf digests in canonical row-major cell order)
//
// add_tile() accepts any partition made from the fixed 64x64 canonical grid.
// Runtime tile order may vary; runtime tile size may vary; duplicate/missing cells,
// non-finite samples and non-canonical boundaries fail closed.
class ScientificMasterDigestAccumulator final {
public:
    ScientificMasterDigestAccumulator(std::uint32_t width, std::uint32_t height);

    bool valid() const noexcept { return valid_; }
    const std::string& error() const noexcept { return error_; }

    bool add_tile(const TileView& tile);
    bool finalize(Sha256& outDigest);

    DigestMetrics metrics() const noexcept;

private:
    bool fail(std::string message);

    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
    std::uint32_t cellColumns_ = 0;
    std::uint32_t cellRows_ = 0;
    std::uint32_t cellsReceived_ = 0;
    bool valid_ = false;
    bool finalized_ = false;
    std::string error_;
    std::vector<Sha256> leafDigests_;
    std::vector<std::uint8_t> seen_;
};

std::string to_hex(const Sha256& digest);

}  // namespace truthraw::scientific_master_digest::v0_1
