#include "scientific_master_digest_v0_1.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <limits>
#include <new>
#include <sstream>
#include <utility>

namespace truthraw::scientific_master_digest::v0_1 {
namespace {

constexpr std::size_t kMasterHeaderBytes = 48;
constexpr std::size_t kCellHeaderBytes = 32;
constexpr std::size_t kMaxDigestStateBytes = 64u * 1024u * 1024u;

constexpr std::array<std::uint32_t, 64> kSha256K = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
    0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
    0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
    0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
    0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u,
};

inline std::uint32_t rotr(std::uint32_t v, unsigned n) noexcept {
    return (v >> n) | (v << (32u - n));
}

class Sha256Hasher final {
public:
    Sha256Hasher() = default;

    void update(const std::uint8_t* data, std::size_t size) {
        if (size == 0) return;
        totalBytes_ += static_cast<std::uint64_t>(size);
        while (size > 0) {
            const std::size_t take = std::min<std::size_t>(size, block_.size() - blockUsed_);
            std::memcpy(block_.data() + blockUsed_, data, take);
            blockUsed_ += take;
            data += take;
            size -= take;
            if (blockUsed_ == block_.size()) {
                transform(block_.data());
                blockUsed_ = 0;
            }
        }
    }

    template <std::size_t N>
    void update(const std::array<std::uint8_t, N>& bytes) {
        update(bytes.data(), bytes.size());
    }

    Sha256 finalize() {
        const std::uint64_t bitCount = totalBytes_ * 8u;
        block_[blockUsed_++] = 0x80u;
        if (blockUsed_ > 56u) {
            std::fill(block_.begin() + static_cast<std::ptrdiff_t>(blockUsed_), block_.end(), 0u);
            transform(block_.data());
            blockUsed_ = 0;
        }
        std::fill(block_.begin() + static_cast<std::ptrdiff_t>(blockUsed_), block_.begin() + 56, 0u);
        for (unsigned i = 0; i < 8; ++i) {
            block_[63u - i] = static_cast<std::uint8_t>((bitCount >> (8u * i)) & 0xffu);
        }
        transform(block_.data());

        Sha256 out{};
        for (std::size_t i = 0; i < state_.size(); ++i) {
            out[i * 4u + 0u] = static_cast<std::uint8_t>((state_[i] >> 24u) & 0xffu);
            out[i * 4u + 1u] = static_cast<std::uint8_t>((state_[i] >> 16u) & 0xffu);
            out[i * 4u + 2u] = static_cast<std::uint8_t>((state_[i] >> 8u) & 0xffu);
            out[i * 4u + 3u] = static_cast<std::uint8_t>(state_[i] & 0xffu);
        }
        return out;
    }

private:
    void transform(const std::uint8_t* b) {
        std::array<std::uint32_t, 64> w{};
        for (std::size_t i = 0; i < 16; ++i) {
            w[i] = (static_cast<std::uint32_t>(b[i * 4u]) << 24u) |
                   (static_cast<std::uint32_t>(b[i * 4u + 1u]) << 16u) |
                   (static_cast<std::uint32_t>(b[i * 4u + 2u]) << 8u) |
                   static_cast<std::uint32_t>(b[i * 4u + 3u]);
        }
        for (std::size_t i = 16; i < 64; ++i) {
            const std::uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3u);
            const std::uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10u);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        std::uint32_t a = state_[0], b0 = state_[1], c = state_[2], d = state_[3];
        std::uint32_t e = state_[4], f = state_[5], g = state_[6], h = state_[7];
        for (std::size_t i = 0; i < 64; ++i) {
            const std::uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            const std::uint32_t ch = (e & f) ^ ((~e) & g);
            const std::uint32_t temp1 = h + s1 + ch + kSha256K[i] + w[i];
            const std::uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            const std::uint32_t maj = (a & b0) ^ (a & c) ^ (b0 & c);
            const std::uint32_t temp2 = s0 + maj;
            h = g; g = f; f = e; e = d + temp1;
            d = c; c = b0; b0 = a; a = temp1 + temp2;
        }
        state_[0] += a; state_[1] += b0; state_[2] += c; state_[3] += d;
        state_[4] += e; state_[5] += f; state_[6] += g; state_[7] += h;
    }

    std::array<std::uint32_t, 8> state_ = {
        0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
        0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u,
    };
    std::array<std::uint8_t, 64> block_{};
    std::size_t blockUsed_ = 0;
    std::uint64_t totalBytes_ = 0;
};

void put_u16_le(std::uint8_t* p, std::uint16_t v) noexcept {
    p[0] = static_cast<std::uint8_t>(v & 0xffu);
    p[1] = static_cast<std::uint8_t>((v >> 8u) & 0xffu);
}

void put_u32_le(std::uint8_t* p, std::uint32_t v) noexcept {
    p[0] = static_cast<std::uint8_t>(v & 0xffu);
    p[1] = static_cast<std::uint8_t>((v >> 8u) & 0xffu);
    p[2] = static_cast<std::uint8_t>((v >> 16u) & 0xffu);
    p[3] = static_cast<std::uint8_t>((v >> 24u) & 0xffu);
}

std::array<std::uint8_t, kMasterHeaderBytes> make_master_header(
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t cellColumns,
    std::uint32_t cellRows,
    std::uint32_t cellCount) {
    std::array<std::uint8_t, kMasterHeaderBytes> h{};
    constexpr char magic[8] = {'T','R','S','M','D','G','0','1'};
    std::memcpy(h.data(), magic, sizeof(magic));
    put_u16_le(h.data() + 8, kVersion);
    put_u16_le(h.data() + 10, static_cast<std::uint16_t>(kMasterHeaderBytes));
    put_u32_le(h.data() + 12, width);
    put_u32_le(h.data() + 16, height);
    put_u16_le(h.data() + 20, kChannels);
    put_u16_le(h.data() + 22, kSampleEncodingIeee754Binary32Le);
    put_u16_le(h.data() + 24, kChannelOrderCameraNativeRgb);
    put_u16_le(h.data() + 26, kCanonicalCellEdge);
    put_u16_le(h.data() + 28, kScientificRoleV47iReconstructedCameraScene);
    put_u16_le(h.data() + 30, 0u);  // flags, reserved in v0.1
    put_u32_le(h.data() + 32, cellColumns);
    put_u32_le(h.data() + 36, cellRows);
    put_u32_le(h.data() + 40, cellCount);
    put_u32_le(h.data() + 44, 0u);  // reserved, must remain zero
    return h;
}

std::array<std::uint8_t, kCellHeaderBytes> make_cell_header(
    std::uint32_t x,
    std::uint32_t y,
    std::uint16_t width,
    std::uint16_t height) {
    std::array<std::uint8_t, kCellHeaderBytes> h{};
    constexpr char magic[8] = {'T','R','S','M','C','L','0','1'};
    std::memcpy(h.data(), magic, sizeof(magic));
    put_u16_le(h.data() + 8, kVersion);
    put_u16_le(h.data() + 10, static_cast<std::uint16_t>(kCellHeaderBytes));
    put_u32_le(h.data() + 12, x);
    put_u32_le(h.data() + 16, y);
    put_u16_le(h.data() + 20, width);
    put_u16_le(h.data() + 22, height);
    put_u16_le(h.data() + 24, kChannels);
    put_u16_le(h.data() + 26, kSampleEncodingIeee754Binary32Le);
    put_u32_le(h.data() + 28, 0u);  // reserved, must remain zero
    return h;
}

bool hash_cell(const TileView& tile,
               std::uint32_t localX,
               std::uint32_t localY,
               std::uint16_t cellWidth,
               std::uint16_t cellHeight,
               Sha256& out,
               std::string& error) {
    const auto header = make_cell_header(tile.x + localX, tile.y + localY, cellWidth, cellHeight);
    Sha256Hasher hasher;
    hasher.update(header);

    std::vector<std::uint8_t> rowBytes(static_cast<std::size_t>(cellWidth) * kChannels * sizeof(float));
    for (std::uint16_t row = 0; row < cellHeight; ++row) {
        const float* src = tile.rgb +
            static_cast<std::size_t>(localY + row) * tile.rowStrideSamples +
            static_cast<std::size_t>(localX) * kChannels;
        std::uint8_t* dst = rowBytes.data();
        for (std::size_t sample = 0; sample < static_cast<std::size_t>(cellWidth) * kChannels; ++sample) {
            const float value = src[sample];
            if (!std::isfinite(value)) {
                error = "scientific master contains NaN or infinity";
                return false;
            }
            std::uint32_t bits = 0;
            static_assert(sizeof(bits) == sizeof(value), "float32 encoding requires 32-bit float");
            std::memcpy(&bits, &value, sizeof(bits));
            put_u32_le(dst + sample * sizeof(std::uint32_t), bits);
        }
        hasher.update(rowBytes.data(), rowBytes.size());
    }
    out = hasher.finalize();
    return true;
}

}  // namespace

ScientificMasterDigestAccumulator::ScientificMasterDigestAccumulator(
    std::uint32_t width,
    std::uint32_t height)
    : width_(width), height_(height) {
    if (width_ == 0 || height_ == 0) {
        fail("scientific master dimensions must be non-zero");
        return;
    }
    cellColumns_ = (width_ + kCanonicalCellEdge - 1u) / kCanonicalCellEdge;
    cellRows_ = (height_ + kCanonicalCellEdge - 1u) / kCanonicalCellEdge;
    const std::uint64_t count64 = static_cast<std::uint64_t>(cellColumns_) * cellRows_;
    const std::uint64_t bytes64 = count64 * (sizeof(Sha256) + sizeof(std::uint8_t));
    if (count64 == 0 || count64 > std::numeric_limits<std::uint32_t>::max() ||
        bytes64 > kMaxDigestStateBytes) {
        fail("scientific master digest state exceeds v0.1 bounded-state limit");
        return;
    }
    try {
        leafDigests_.resize(static_cast<std::size_t>(count64));
        seen_.assign(static_cast<std::size_t>(count64), 0u);
    } catch (const std::bad_alloc&) {
        fail("scientific master digest state allocation failed");
        return;
    }
    valid_ = true;
}

bool ScientificMasterDigestAccumulator::fail(std::string message) {
    valid_ = false;
    if (error_.empty()) error_ = std::move(message);
    return false;
}

bool ScientificMasterDigestAccumulator::add_tile(const TileView& tile) {
    if (!valid_) return false;
    if (finalized_) return fail("cannot add tile after finalization");
    if (tile.rgb == nullptr || tile.width == 0 || tile.height == 0) {
        return fail("tile payload and dimensions must be non-empty");
    }
    if ((tile.x % kCanonicalCellEdge) != 0 || (tile.y % kCanonicalCellEdge) != 0) {
        return fail("tile origin is not aligned to the canonical 64x64 cell grid");
    }
    const std::uint64_t right = static_cast<std::uint64_t>(tile.x) + tile.width;
    const std::uint64_t bottom = static_cast<std::uint64_t>(tile.y) + tile.height;
    if (right > width_ || bottom > height_) {
        return fail("tile extends outside scientific master bounds");
    }
    if (right != width_ && (tile.width % kCanonicalCellEdge) != 0) {
        return fail("non-edge tile width must be a multiple of the canonical cell edge");
    }
    if (bottom != height_ && (tile.height % kCanonicalCellEdge) != 0) {
        return fail("non-edge tile height must be a multiple of the canonical cell edge");
    }
    const std::size_t minStride = static_cast<std::size_t>(tile.width) * kChannels;
    if (tile.rowStrideSamples < minStride) {
        return fail("tile row stride is smaller than width * 3");
    }

    for (std::uint32_t localY = 0; localY < tile.height; localY += kCanonicalCellEdge) {
        const std::uint32_t globalY = tile.y + localY;
        const auto cellH = static_cast<std::uint16_t>(
            std::min<std::uint32_t>(kCanonicalCellEdge, height_ - globalY));
        for (std::uint32_t localX = 0; localX < tile.width; localX += kCanonicalCellEdge) {
            const std::uint32_t globalX = tile.x + localX;
            const auto cellW = static_cast<std::uint16_t>(
                std::min<std::uint32_t>(kCanonicalCellEdge, width_ - globalX));
            if (localX + cellW > tile.width || localY + cellH > tile.height) {
                return fail("runtime tile splits a canonical digest cell");
            }
            const std::uint32_t cellX = globalX / kCanonicalCellEdge;
            const std::uint32_t cellY = globalY / kCanonicalCellEdge;
            const std::size_t index = static_cast<std::size_t>(cellY) * cellColumns_ + cellX;
            if (index >= seen_.size()) return fail("canonical cell index overflow");
            if (seen_[index] != 0u) return fail("duplicate canonical digest cell");

            Sha256 leaf{};
            std::string hashError;
            if (!hash_cell(tile, localX, localY, cellW, cellH, leaf, hashError)) {
                return fail(std::move(hashError));
            }
            leafDigests_[index] = leaf;
            seen_[index] = 1u;
            ++cellsReceived_;
        }
    }
    return true;
}

bool ScientificMasterDigestAccumulator::finalize(Sha256& outDigest) {
    if (!valid_) return false;
    if (finalized_) return fail("scientific master digest already finalized");
    if (cellsReceived_ != leafDigests_.size()) {
        return fail("scientific master digest is incomplete: one or more canonical cells are missing");
    }
    if (std::find(seen_.begin(), seen_.end(), 0u) != seen_.end()) {
        return fail("scientific master digest coverage map is incomplete");
    }

    const auto header = make_master_header(width_, height_, cellColumns_, cellRows_, cellsReceived_);
    Sha256Hasher hasher;
    hasher.update(header);
    for (const auto& leaf : leafDigests_) hasher.update(leaf);
    outDigest = hasher.finalize();
    finalized_ = true;
    return true;
}

DigestMetrics ScientificMasterDigestAccumulator::metrics() const noexcept {
    DigestMetrics m{};
    m.width = width_;
    m.height = height_;
    m.cellColumns = cellColumns_;
    m.cellRows = cellRows_;
    m.cellCount = static_cast<std::uint32_t>(leafDigests_.size());
    m.cellsReceived = cellsReceived_;
    m.residentBytesUpperBound = sizeof(*this) +
        leafDigests_.capacity() * sizeof(Sha256) +
        seen_.capacity() * sizeof(std::uint8_t) +
        static_cast<std::size_t>(kCanonicalCellEdge) * kChannels * sizeof(float);
    return m;
}

std::string to_hex(const Sha256& digest) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (const auto b : digest) out << std::setw(2) << static_cast<unsigned>(b);
    return out.str();
}

}  // namespace truthraw::scientific_master_digest::v0_1
