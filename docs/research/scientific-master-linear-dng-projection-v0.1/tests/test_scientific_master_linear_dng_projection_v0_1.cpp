#include "scientific_master_linear_dng_projection_v0_1.h"
#include "scientific_master_digest_v0_1.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace projection = truthraw::scientific_master_linear_dng_projection::v0_1;
namespace digest_v0_1 = truthraw::scientific_master_digest::v0_1;

namespace {

#define REQUIRE(expr) do { \
    if (!(expr)) { \
        std::cerr << "REQUIRE failed: " #expr " at " << __FILE__ << ':' << __LINE__ << '\n'; \
        std::exit(2); \
    } \
} while (false)

std::uint16_t u16(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    REQUIRE(offset + 2u <= bytes.size());
    return static_cast<std::uint16_t>(bytes[offset]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[offset + 1u]) << 8u);
}

std::uint32_t u32(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    REQUIRE(offset + 4u <= bytes.size());
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1u]) << 8u) |
           (static_cast<std::uint32_t>(bytes[offset + 2u]) << 16u) |
           (static_cast<std::uint32_t>(bytes[offset + 3u]) << 24u);
}

float f32(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    const std::uint32_t bits = u32(bytes, offset);
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

class SyntheticMasterSource final : public projection::IScientificMasterTileSource {
public:
    SyntheticMasterSource(std::uint32_t width, std::uint32_t height)
        : width_(width), height_(height) {}

    std::size_t residentBytesUpperBound() const noexcept override { return 0u; }

    static std::array<float, 3> pixel(std::uint32_t x, std::uint32_t y) noexcept {
        return {
            0.02f + static_cast<float>(x) * 0.003f,
            0.03f + static_cast<float>(y) * 0.004f,
            0.01f + static_cast<float>(x + y) * 0.002f,
        };
    }

    projection::Status readCameraNativeTile(
        std::uint32_t x,
        std::uint32_t y,
        std::uint32_t width,
        std::uint32_t height,
        float* rgb,
        std::size_t floatCount) noexcept override {
        if (rgb == nullptr || width == 0u || height == 0u ||
            x + width > width_ || y + height > height_ ||
            floatCount != static_cast<std::size_t>(width) * height * 3u) {
            return projection::Status::error(
                projection::StatusCode::SourceFailed, "invalid synthetic tile request");
        }
        maxTileFloats = std::max(maxTileFloats, floatCount);
        for (std::uint32_t ly = 0u; ly < height; ++ly) {
            for (std::uint32_t lx = 0u; lx < width; ++lx) {
                const auto p = pixel(x + lx, y + ly);
                const std::size_t i =
                    (static_cast<std::size_t>(ly) * width + lx) * 3u;
                rgb[i + 0u] = p[0];
                rgb[i + 1u] = p[1];
                rgb[i + 2u] = p[2];
            }
        }
        ++calls;
        return projection::Status::ok();
    }

    std::size_t maxTileFloats = 0u;
    std::size_t calls = 0u;

private:
    std::uint32_t width_;
    std::uint32_t height_;
};

class MemoryTransactionSink final : public projection::ITransactionalByteSink {
public:
    std::size_t residentBytesUpperBound() const noexcept override {
        return pending.capacity() + committed.capacity();
    }

    bool begin(std::uint64_t expectedBytes) noexcept override {
        expected = expectedBytes;
        pending.clear();
        committed.clear();
        begun = true;
        aborted = false;
        try {
            pending.reserve(static_cast<std::size_t>(expectedBytes));
            return true;
        } catch (...) {
            return false;
        }
    }

    bool write(const std::uint8_t* data, std::size_t size) noexcept override {
        if (!begun || data == nullptr) return false;
        try {
            pending.insert(pending.end(), data, data + size);
            return pending.size() <= expected;
        } catch (...) {
            return false;
        }
    }

    bool commit() noexcept override {
        if (!begun || pending.size() != expected) return false;
        committed.swap(pending);
        begun = false;
        return true;
    }

    void abort() noexcept override {
        pending.clear();
        committed.clear();
        begun = false;
        aborted = true;
    }

    std::uint64_t expected = 0u;
    bool begun = false;
    bool aborted = false;
    std::vector<std::uint8_t> pending;
    std::vector<std::uint8_t> committed;
};

projection::Hash256 compute_master_hash(
    SyntheticMasterSource& source,
    std::uint32_t width,
    std::uint32_t height) {
    digest_v0_1::ScientificMasterDigestAccumulator digest(width, height);
    REQUIRE(digest.valid());
    std::vector<float> tile;
    for (std::uint32_t y = 0u; y < height; y += projection::kCanonicalTileEdge) {
        const std::uint32_t h = std::min(projection::kCanonicalTileEdge, height - y);
        for (std::uint32_t x = 0u; x < width; x += projection::kCanonicalTileEdge) {
            const std::uint32_t w = std::min(projection::kCanonicalTileEdge, width - x);
            tile.resize(static_cast<std::size_t>(w) * h * 3u);
            REQUIRE(source.readCameraNativeTile(x, y, w, h, tile.data(), tile.size()));
            digest_v0_1::TileView view{};
            view.x = x;
            view.y = y;
            view.width = w;
            view.height = h;
            view.rgb = tile.data();
            view.rowStrideSamples = static_cast<std::size_t>(w) * 3u;
            REQUIRE(digest.add_tile(view));
        }
    }
    projection::Hash256 hash{};
    REQUIRE(digest.finalize(hash));
    return hash;
}

projection::ProjectionDescriptor descriptor_for(
    std::uint32_t width,
    std::uint32_t height,
    const projection::Hash256& master) {
    projection::ProjectionDescriptor d{};
    d.width = width;
    d.height = height;
    d.orientation = 6u;
    for (std::size_t i = 0u; i < d.sealedSourceSha256.size(); ++i) {
        d.sealedSourceSha256[i] = static_cast<std::uint8_t>(0x80u + i);
    }
    d.scientificMasterSha256 = master;
    d.sourceEvidenceId = "sha256:test-source";
    d.colorBindingId = "independent-test-color-binding";
    return d;
}

struct Entry final {
    std::uint16_t type = 0u;
    std::uint32_t count = 0u;
    std::uint32_t valueOrOffset = 0u;
    std::size_t entryOffset = 0u;
};

std::map<std::uint16_t, Entry> parse_ifd(const std::vector<std::uint8_t>& bytes) {
    REQUIRE(bytes.size() >= 8u);
    REQUIRE(bytes[0] == 'I' && bytes[1] == 'I');
    REQUIRE(u16(bytes, 2u) == 42u);
    const std::uint32_t ifdOffset = u32(bytes, 4u);
    REQUIRE(ifdOffset + 2u <= bytes.size());
    const std::uint16_t count = u16(bytes, ifdOffset);
    std::map<std::uint16_t, Entry> entries;
    for (std::uint16_t i = 0u; i < count; ++i) {
        const std::size_t off = static_cast<std::size_t>(ifdOffset) + 2u + 12u * i;
        REQUIRE(off + 12u <= bytes.size());
        entries.emplace(u16(bytes, off), Entry{u16(bytes, off + 2u),
                                               u32(bytes, off + 4u),
                                               u32(bytes, off + 8u), off});
    }
    return entries;
}

std::size_t type_size(std::uint16_t type) {
    switch (type) {
        case 1u: case 2u: return 1u;
        case 3u: return 2u;
        case 4u: return 4u;
        case 5u: case 10u: return 8u;
        default: return 0u;
    }
}

std::size_t payload_offset(const Entry& e) {
    const std::size_t bytes = type_size(e.type) * e.count;
    REQUIRE(bytes != 0u);
    return bytes <= 4u ? e.entryOffset + 8u : e.valueOrOffset;
}

std::string bytes_as_string(
    const std::vector<std::uint8_t>& bytes,
    std::size_t offset,
    std::size_t count) {
    REQUIRE(offset + count <= bytes.size());
    return std::string(reinterpret_cast<const char*>(bytes.data() + offset), count);
}

void verify_dng_structure(
    const std::vector<std::uint8_t>& bytes,
    const projection::ProjectionDescriptor& descriptor,
    const std::array<float, 9>& matrix) {
    const auto entries = parse_ifd(bytes);
    REQUIRE(entries.count(262u) == 1u);
    REQUIRE(u16(bytes, payload_offset(entries.at(262u))) == projection::kPhotometricLinearRaw);
    REQUIRE(entries.count(258u) == 1u);
    const auto bitsOff = payload_offset(entries.at(258u));
    REQUIRE(u16(bytes, bitsOff + 0u) == 32u);
    REQUIRE(u16(bytes, bitsOff + 2u) == 32u);
    REQUIRE(u16(bytes, bitsOff + 4u) == 32u);

    REQUIRE(entries.count(339u) == 1u);
    const auto fmtOff = payload_offset(entries.at(339u));
    REQUIRE(u16(bytes, fmtOff + 0u) == 3u);
    REQUIRE(u16(bytes, fmtOff + 2u) == 3u);
    REQUIRE(u16(bytes, fmtOff + 4u) == 3u);

    REQUIRE(entries.count(50706u) == 1u);
    const auto versionOff = payload_offset(entries.at(50706u));
    REQUIRE(bytes[versionOff + 0u] == 1u);
    REQUIRE(bytes[versionOff + 1u] == 4u);
    REQUIRE(bytes[versionOff + 2u] == 0u);
    REQUIRE(bytes[versionOff + 3u] == 0u);

    REQUIRE(entries.count(50721u) == 1u);
    const auto cmOff = payload_offset(entries.at(50721u));
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            const std::size_t off = cmOff + static_cast<std::size_t>(row * 3 + col) * 8u;
            const std::int32_t num = static_cast<std::int32_t>(u32(bytes, off));
            const std::int32_t den = static_cast<std::int32_t>(u32(bytes, off + 4u));
            REQUIRE(num == (row == col ? 1 : 0));
            REQUIRE(den == 1);
        }
    }

    REQUIRE(entries.count(50778u) == 1u);
    REQUIRE(u16(bytes, payload_offset(entries.at(50778u))) ==
            projection::kCalibrationIlluminantD50);

    REQUIRE(entries.count(324u) == 1u);
    REQUIRE(entries.at(324u).count == 2u);
    const auto offsetsOff = payload_offset(entries.at(324u));
    const std::uint32_t tile0 = u32(bytes, offsetsOff + 0u);
    const std::uint32_t tile1 = u32(bytes, offsetsOff + 4u);
    REQUIRE(tile1 > tile0);
    REQUIRE(tile1 - tile0 == projection::kCanonicalTileEdge *
                                  projection::kCanonicalTileEdge * 3u * 4u);

    const auto p = SyntheticMasterSource::pixel(0u, 0u);
    const double r = static_cast<double>(p[0]);
    const double g = static_cast<double>(p[1]);
    const double b = static_cast<double>(p[2]);
    const float expectedX = static_cast<float>(
        static_cast<double>(matrix[0]) * r +
        static_cast<double>(matrix[1]) * g +
        static_cast<double>(matrix[2]) * b);
    const float expectedY = static_cast<float>(
        static_cast<double>(matrix[3]) * r +
        static_cast<double>(matrix[4]) * g +
        static_cast<double>(matrix[5]) * b);
    const float expectedZ = static_cast<float>(
        static_cast<double>(matrix[6]) * r +
        static_cast<double>(matrix[7]) * g +
        static_cast<double>(matrix[8]) * b);
    const float actualX = f32(bytes, tile0 + 0u);
    const float actualY = f32(bytes, tile0 + 4u);
    const float actualZ = f32(bytes, tile0 + 8u);
    REQUIRE(std::memcmp(&expectedX, &actualX, sizeof(float)) == 0);
    REQUIRE(std::memcmp(&expectedY, &actualY, sizeof(float)) == 0);
    REQUIRE(std::memcmp(&expectedZ, &actualZ, sizeof(float)) == 0);

    REQUIRE(entries.count(50740u) == 1u);
    const auto privateOff = payload_offset(entries.at(50740u));
    const auto privateText = bytes_as_string(
        bytes, privateOff, entries.at(50740u).count);
    REQUIRE(privateText.find("LINEAR_DNG_XYZ_D50_COMPATIBILITY_PROJECTION") != std::string::npos);
    REQUIRE(privateText.find("representation_only=1") != std::string::npos);
    REQUIRE(privateText.find("scientific_master_sha256=") != std::string::npos);
    REQUIRE(privateText.find(descriptor.colorBindingId) != std::string::npos);
}

void test_transactional_projection_and_identity_gate() {
    constexpr std::uint32_t width = 66u;
    constexpr std::uint32_t height = 50u;
    SyntheticMasterSource hashingSource(width, height);
    const auto master = compute_master_hash(hashingSource, width, height);
    const auto descriptor = descriptor_for(width, height, master);
    const std::array<float, 9> matrix = {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.5f, 0.0f,
        0.0f, 0.0f, 0.75f,
    };

    SyntheticMasterSource sourceA(width, height);
    MemoryTransactionSink sinkA;
    projection::Result resultA{};
    const auto statusA = projection::write_xyz_d50_linear_dng_projection(
        sourceA, descriptor, matrix, sinkA, resultA);
    if (!statusA) std::cerr << "projection A failed: " << statusA.message << '\n';
    REQUIRE(statusA);
    REQUIRE(resultA.scientificMasterIdentityVerified);
    REQUIRE(resultA.artifactCommitted);
    REQUIRE(resultA.representationOnly);
    REQUIRE(!resultA.scientificMasterModified);
    REQUIRE(!resultA.appearanceApplied);
    REQUIRE(!resultA.counterfactualObservationCreated);
    REQUIRE(resultA.physicalFrameCount == 1u);
    REQUIRE(resultA.independentEvidenceCount == 1u);
    REQUIRE(resultA.projectedPixels == static_cast<std::uint64_t>(width) * height);
    REQUIRE(resultA.tilesWritten == 2u);
    REQUIRE(sourceA.maxTileFloats <=
            projection::kCanonicalTileEdge * projection::kCanonicalTileEdge * 3u);
    REQUIRE(!sinkA.committed.empty());
    verify_dng_structure(sinkA.committed, descriptor, matrix);

    SyntheticMasterSource sourceB(width, height);
    MemoryTransactionSink sinkB;
    projection::Result resultB{};
    const auto statusB = projection::write_xyz_d50_linear_dng_projection(
        sourceB, descriptor, matrix, sinkB, resultB);
    REQUIRE(statusB);
    REQUIRE(sinkA.committed == sinkB.committed);

    auto wrong = descriptor;
    wrong.scientificMasterSha256[0] ^= 0x01u;
    SyntheticMasterSource sourceWrong(width, height);
    MemoryTransactionSink sinkWrong;
    projection::Result wrongResult{};
    const auto wrongStatus = projection::write_xyz_d50_linear_dng_projection(
        sourceWrong, wrong, matrix, sinkWrong, wrongResult);
    REQUIRE(!wrongStatus);
    REQUIRE(wrongStatus.code == projection::StatusCode::ScientificMasterMismatch);
    REQUIRE(sinkWrong.aborted);
    REQUIRE(sinkWrong.committed.empty());
    REQUIRE(!wrongResult.artifactCommitted);

    const std::array<float, 9> singular = {
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
    };
    SyntheticMasterSource sourceSingular(width, height);
    MemoryTransactionSink sinkSingular;
    projection::Result singularResult{};
    const auto singularStatus = projection::write_xyz_d50_linear_dng_projection(
        sourceSingular, descriptor, singular, sinkSingular, singularResult);
    REQUIRE(!singularStatus);
    REQUIRE(singularStatus.code == projection::StatusCode::InvalidColorTransform);
    REQUIRE(sinkSingular.committed.empty());
}

}  // namespace

int main() {
    test_transactional_projection_and_identity_gate();
    std::cout << "SCIENTIFIC_MASTER_LINEAR_DNG_PROJECTION_V0_1_PASS\n";
    std::cout << "photometric_linear_raw=34892\n";
    std::cout << "sample_format_ieee_float32=1\n";
    std::cout << "scientific_master_digest_gate=1\n";
    std::cout << "transactional_abort_on_master_mismatch=1\n";
    std::cout << "representation_only=1\n";
    std::cout << "physical_frame_count=1\n";
    std::cout << "independent_evidence_count=1\n";
    return 0;
}
