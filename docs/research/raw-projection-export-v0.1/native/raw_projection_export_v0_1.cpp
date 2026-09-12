#include "raw_projection_export_v0_1.h"

#include "full_frame_streaming_v0_1_internal.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <unistd.h>
#include <vector>

namespace truthraw::raw_projection_export::v0_1 {
namespace {

using streaming_v0_1::TileRect;
using streaming_v0_1::detail::Workspace;
using streaming_v0_1::detail::fill_stage2;
using streaming_v0_1::detail::vector_bytes;

constexpr std::uint16_t kTiffByte = 1;
constexpr std::uint16_t kTiffAscii = 2;
constexpr std::uint16_t kTiffShort = 3;
constexpr std::uint16_t kTiffLong = 4;
constexpr std::uint16_t kTiffRational = 5;
constexpr std::uint16_t kTiffSRational = 10;

constexpr std::uint16_t kTagImageWidth = 256;
constexpr std::uint16_t kTagImageLength = 257;
constexpr std::uint16_t kTagBitsPerSample = 258;
constexpr std::uint16_t kTagCompression = 259;
constexpr std::uint16_t kTagPhotometric = 262;
constexpr std::uint16_t kTagImageDescription = 270;
constexpr std::uint16_t kTagStripOffsets = 273;
constexpr std::uint16_t kTagOrientation = 274;
constexpr std::uint16_t kTagSamplesPerPixel = 277;
constexpr std::uint16_t kTagRowsPerStrip = 278;
constexpr std::uint16_t kTagStripByteCounts = 279;
constexpr std::uint16_t kTagPlanarConfiguration = 284;
constexpr std::uint16_t kTagSoftware = 305;
constexpr std::uint16_t kTagSampleFormat = 339;
constexpr std::uint16_t kTagCfaRepeatPatternDim = 33421;
constexpr std::uint16_t kTagCfaPattern = 33422;
constexpr std::uint16_t kTagDngVersion = 50706;
constexpr std::uint16_t kTagDngBackwardVersion = 50707;
constexpr std::uint16_t kTagUniqueCameraModel = 50708;
constexpr std::uint16_t kTagCfaPlaneColor = 50710;
constexpr std::uint16_t kTagCfaLayout = 50711;
constexpr std::uint16_t kTagBlackLevelRepeatDim = 50713;
constexpr std::uint16_t kTagBlackLevel = 50714;
constexpr std::uint16_t kTagWhiteLevel = 50717;
constexpr std::uint16_t kTagColorMatrix1 = 50721;
constexpr std::uint16_t kTagAsShotNeutral = 50728;
constexpr std::uint16_t kTagCalibrationIlluminant1 = 50778;
constexpr std::uint16_t kTagForwardMatrix1 = 50964;

constexpr std::uint16_t kPhotometricCfa = 32803;
constexpr std::uint16_t kPhotometricLinearRaw = 34892;
constexpr std::uint16_t kCompressionNone = 1;
constexpr std::uint16_t kPlanarChunky = 1;
constexpr std::uint16_t kSampleUnsignedInteger = 1;
constexpr std::uint16_t kD50LightSource = 23;
constexpr std::uint32_t kClassicTiffLimit = 0xffffffffu;

struct Entry final {
    std::uint16_t tag = 0;
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::vector<std::uint8_t> data;
    std::uint32_t externalOffset = 0;
};

void append_u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>(value & 0xffu));
    out.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
}

void append_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>(value & 0xffu));
    out.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
    out.push_back(static_cast<std::uint8_t>((value >> 16u) & 0xffu));
    out.push_back(static_cast<std::uint8_t>((value >> 24u) & 0xffu));
}

void append_i32(std::vector<std::uint8_t>& out, std::int32_t value) {
    append_u32(out, static_cast<std::uint32_t>(value));
}

void put_u16(std::vector<std::uint8_t>& out, std::size_t offset, std::uint16_t value) {
    out[offset] = static_cast<std::uint8_t>(value & 0xffu);
    out[offset + 1u] = static_cast<std::uint8_t>((value >> 8u) & 0xffu);
}

void put_u32(std::vector<std::uint8_t>& out, std::size_t offset, std::uint32_t value) {
    out[offset] = static_cast<std::uint8_t>(value & 0xffu);
    out[offset + 1u] = static_cast<std::uint8_t>((value >> 8u) & 0xffu);
    out[offset + 2u] = static_cast<std::uint8_t>((value >> 16u) & 0xffu);
    out[offset + 3u] = static_cast<std::uint8_t>((value >> 24u) & 0xffu);
}

std::size_t align4(std::size_t value) noexcept {
    return (value + 3u) & ~std::size_t(3u);
}

Entry short_entry(std::uint16_t tag, std::initializer_list<std::uint16_t> values) {
    Entry e{};
    e.tag = tag;
    e.type = kTiffShort;
    e.count = static_cast<std::uint32_t>(values.size());
    for (const auto value : values) append_u16(e.data, value);
    return e;
}

Entry long_entry(std::uint16_t tag, const std::vector<std::uint32_t>& values) {
    Entry e{};
    e.tag = tag;
    e.type = kTiffLong;
    e.count = static_cast<std::uint32_t>(values.size());
    for (const auto value : values) append_u32(e.data, value);
    return e;
}

Entry byte_entry(std::uint16_t tag, std::initializer_list<std::uint8_t> values) {
    Entry e{};
    e.tag = tag;
    e.type = kTiffByte;
    e.count = static_cast<std::uint32_t>(values.size());
    e.data.assign(values.begin(), values.end());
    return e;
}

Entry ascii_entry(std::uint16_t tag, const std::string& text) {
    Entry e{};
    e.tag = tag;
    e.type = kTiffAscii;
    e.count = static_cast<std::uint32_t>(text.size() + 1u);
    e.data.assign(text.begin(), text.end());
    e.data.push_back(0u);
    return e;
}

bool rational_entry(std::uint16_t tag,
                    const std::vector<double>& values,
                    bool signedValues,
                    Entry& out) noexcept {
    out = {};
    out.tag = tag;
    out.type = signedValues ? kTiffSRational : kTiffRational;
    out.count = static_cast<std::uint32_t>(values.size());
    constexpr double scale = 1000000.0;
    for (const double value : values) {
        if (!std::isfinite(value)) return false;
        if (signedValues) {
            const double scaled = std::round(value * scale);
            if (scaled < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
                scaled > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
                return false;
            }
            append_i32(out.data, static_cast<std::int32_t>(scaled));
            append_i32(out.data, 1000000);
        } else {
            if (value < 0.0) return false;
            const double scaled = std::round(value * scale);
            if (scaled > static_cast<double>(std::numeric_limits<std::uint32_t>::max())) {
                return false;
            }
            append_u32(out.data, static_cast<std::uint32_t>(scaled));
            append_u32(out.data, 1000000u);
        }
    }
    return true;
}

bool invert3(const std::array<float, 9>& in, std::array<double, 9>& out) noexcept {
    const double a = in[0], b = in[1], c = in[2];
    const double d = in[3], e = in[4], f = in[5];
    const double g = in[6], h = in[7], i = in[8];
    const double det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
    if (!std::isfinite(det) || std::abs(det) <= 1.0e-12) return false;
    out = {
        (e*i-f*h)/det, (c*h-b*i)/det, (b*f-c*e)/det,
        (f*g-d*i)/det, (a*i-c*g)/det, (c*d-a*f)/det,
        (d*h-e*g)/det, (b*g-a*h)/det, (a*e-b*d)/det,
    };
    for (const double value : out) if (!std::isfinite(value)) return false;
    return true;
}

std::array<std::uint8_t, 4> cfa_pattern(CfaPattern pattern) noexcept {
    switch (pattern) {
        case CfaPattern::BGGR: return {2u, 1u, 1u, 0u};
        case CfaPattern::RGGB: return {0u, 1u, 1u, 2u};
        case CfaPattern::GRBG: return {1u, 0u, 2u, 1u};
        case CfaPattern::GBRG: return {1u, 2u, 0u, 1u};
    }
    return {2u, 1u, 1u, 0u};
}

int cfa_channel(CfaPattern pattern, int x, int y) noexcept {
    const int phase = ((y & 1) << 1) | (x & 1);
    switch (pattern) {
        case CfaPattern::BGGR: {
            constexpr int map[4] = {2, 1, 1, 0};
            return map[phase];
        }
        case CfaPattern::RGGB: {
            constexpr int map[4] = {0, 1, 1, 2};
            return map[phase];
        }
        case CfaPattern::GRBG: {
            constexpr int map[4] = {1, 0, 2, 1};
            return map[phase];
        }
        case CfaPattern::GBRG: {
            constexpr int map[4] = {1, 2, 0, 1};
            return map[phase];
        }
    }
    return 1;
}

bool write_all(int fd, const std::uint8_t* data, std::size_t size) noexcept {
    std::size_t written = 0u;
    while (written < size) {
        const ssize_t n = ::write(fd, data + written, size - written);
        if (n <= 0) return false;
        written += static_cast<std::size_t>(n);
    }
    return true;
}

bool checked_u32(std::uint64_t value, std::uint32_t& out) noexcept {
    if (value > static_cast<std::uint64_t>(kClassicTiffLimit)) return false;
    out = static_cast<std::uint32_t>(value);
    return true;
}

Status build_dng_header(const DngMetadata& metadata,
                        ProjectionKind kind,
                        int rowsPerStrip,
                        const std::vector<std::uint32_t>& stripByteCounts,
                        std::vector<std::uint8_t>& header) noexcept {
    if (kind != ProjectionKind::ReconstructedCfaDng16 && kind != ProjectionKind::LinearDng16) {
        return Status::error(StatusCode::UnsupportedProjection, "DNG header requested for non-DNG projection");
    }
    const bool linear = kind == ProjectionKind::LinearDng16;
    const int samples = linear ? 3 : 1;
    std::array<double, 9> xyzToCamera{};
    if (!invert3(metadata.cameraToXyzD50, xyzToCamera)) {
        return Status::error(StatusCode::InvalidColorMatrix, "cameraToXyzD50 cannot be inverted for DNG ColorMatrix1");
    }

    std::vector<Entry> entries;
    entries.reserve(28u);
    entries.push_back(long_entry(kTagImageWidth, {static_cast<std::uint32_t>(metadata.width)}));
    entries.push_back(long_entry(kTagImageLength, {static_cast<std::uint32_t>(metadata.height)}));
    if (linear) entries.push_back(short_entry(kTagBitsPerSample, {16u, 16u, 16u}));
    else entries.push_back(short_entry(kTagBitsPerSample, {16u}));
    entries.push_back(short_entry(kTagCompression, {kCompressionNone}));
    entries.push_back(short_entry(kTagPhotometric, {linear ? kPhotometricLinearRaw : kPhotometricCfa}));
    entries.push_back(ascii_entry(kTagImageDescription,
        linear
            ? "TruthRaw LINEAR_DNG_COMPATIBILITY_PROJECTION_V0_1; reconstructed; projection-only; not measured sensor evidence"
            : "TruthRaw RECONSTRUCTED_CFA_PROJECTION_V0_1; remosaiced; projection-only; not measured sensor evidence"));

    Entry stripOffsets = long_entry(kTagStripOffsets, std::vector<std::uint32_t>(stripByteCounts.size(), 0u));
    entries.push_back(std::move(stripOffsets));
    entries.push_back(short_entry(kTagOrientation, {static_cast<std::uint16_t>(metadata.orientation)}));
    entries.push_back(short_entry(kTagSamplesPerPixel, {static_cast<std::uint16_t>(samples)}));
    entries.push_back(long_entry(kTagRowsPerStrip, {static_cast<std::uint32_t>(rowsPerStrip)}));
    entries.push_back(long_entry(kTagStripByteCounts, stripByteCounts));
    entries.push_back(short_entry(kTagPlanarConfiguration, {kPlanarChunky}));
    entries.push_back(ascii_entry(kTagSoftware, "TruthRaw raw-projection-export-v0.1"));
    if (linear) entries.push_back(short_entry(kTagSampleFormat, {kSampleUnsignedInteger, kSampleUnsignedInteger, kSampleUnsignedInteger}));
    else entries.push_back(short_entry(kTagSampleFormat, {kSampleUnsignedInteger}));

    entries.push_back(byte_entry(kTagDngVersion, {1u, 4u, 0u, 0u}));
    entries.push_back(byte_entry(kTagDngBackwardVersion, {1u, 1u, 0u, 0u}));
    entries.push_back(ascii_entry(kTagUniqueCameraModel,
        linear ? "TruthRaw Linear DNG Projection" : "TruthRaw Reconstructed CFA DNG Projection"));

    if (!linear) {
        const auto cfa = cfa_pattern(metadata.cfa);
        entries.push_back(byte_entry(kTagCfaPlaneColor, {0u, 1u, 2u}));
        entries.push_back(short_entry(kTagCfaLayout, {1u}));
        entries.push_back(short_entry(kTagCfaRepeatPatternDim, {2u, 2u}));
        entries.push_back(byte_entry(kTagCfaPattern, {cfa[0], cfa[1], cfa[2], cfa[3]}));
        entries.push_back(short_entry(kTagBlackLevelRepeatDim, {2u, 2u}));
        entries.push_back(short_entry(kTagBlackLevel, {0u, 0u, 0u, 0u}));
        entries.push_back(long_entry(kTagWhiteLevel, {65535u}));
    } else {
        entries.push_back(short_entry(kTagBlackLevel, {0u, 0u, 0u}));
        entries.push_back(long_entry(kTagWhiteLevel, {65535u, 65535u, 65535u}));
    }

    Entry colorMatrix{};
    if (!rational_entry(kTagColorMatrix1,
                        std::vector<double>(xyzToCamera.begin(), xyzToCamera.end()),
                        true, colorMatrix)) {
        return Status::error(StatusCode::InvalidColorMatrix, "ColorMatrix1 rational encoding failed");
    }
    entries.push_back(std::move(colorMatrix));

    Entry neutral{};
    if (!rational_entry(kTagAsShotNeutral, {1.0, 1.0, 1.0}, false, neutral)) {
        return Status::error(StatusCode::InvalidColorMatrix, "AsShotNeutral encoding failed");
    }
    entries.push_back(std::move(neutral));
    entries.push_back(short_entry(kTagCalibrationIlluminant1, {kD50LightSource}));

    Entry forward{};
    std::vector<double> cameraToXyz;
    cameraToXyz.reserve(9u);
    for (const float value : metadata.cameraToXyzD50) {
        if (!std::isfinite(value)) {
            return Status::error(StatusCode::InvalidColorMatrix, "cameraToXyzD50 contains non-finite value");
        }
        cameraToXyz.push_back(static_cast<double>(value));
    }
    if (!rational_entry(kTagForwardMatrix1, cameraToXyz, true, forward)) {
        return Status::error(StatusCode::InvalidColorMatrix, "ForwardMatrix1 rational encoding failed");
    }
    entries.push_back(std::move(forward));

    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return a.tag < b.tag; });
    if (entries.size() > std::numeric_limits<std::uint16_t>::max()) {
        return Status::error(StatusCode::TiffLayoutOverflow, "too many TIFF entries");
    }

    const std::size_t ifdOffset = 8u;
    const std::size_t ifdBytes = 2u + entries.size() * 12u + 4u;
    std::size_t cursor = align4(ifdOffset + ifdBytes);
    for (auto& entry : entries) {
        if (entry.data.size() > 4u) {
            std::uint32_t external = 0u;
            if (!checked_u32(cursor, external)) {
                return Status::error(StatusCode::TiffLayoutOverflow, "TIFF metadata offset exceeds classic TIFF range");
            }
            entry.externalOffset = external;
            cursor = align4(cursor + entry.data.size());
        }
    }
    const std::size_t pixelDataOffset = align4(cursor);
    std::uint64_t totalPixelBytes = 0u;
    for (const auto byteCount : stripByteCounts) totalPixelBytes += byteCount;
    if (static_cast<std::uint64_t>(pixelDataOffset) + totalPixelBytes > kClassicTiffLimit) {
        return Status::error(StatusCode::TiffLayoutOverflow, "projected DNG exceeds classic TIFF 4 GiB range");
    }

    auto offsetsIt = std::find_if(entries.begin(), entries.end(), [](const Entry& e) { return e.tag == kTagStripOffsets; });
    if (offsetsIt == entries.end()) {
        return Status::error(StatusCode::TiffLayoutOverflow, "internal StripOffsets entry missing");
    }
    offsetsIt->data.clear();
    std::uint64_t nextOffset = pixelDataOffset;
    for (const auto byteCount : stripByteCounts) {
        std::uint32_t encoded = 0u;
        if (!checked_u32(nextOffset, encoded)) {
            return Status::error(StatusCode::TiffLayoutOverflow, "strip offset exceeds classic TIFF range");
        }
        append_u32(offsetsIt->data, encoded);
        nextOffset += byteCount;
    }

    header.assign(pixelDataOffset, 0u);
    header[0] = 'I';
    header[1] = 'I';
    put_u16(header, 2u, 42u);
    put_u32(header, 4u, static_cast<std::uint32_t>(ifdOffset));
    put_u16(header, ifdOffset, static_cast<std::uint16_t>(entries.size()));

    std::size_t entryOffset = ifdOffset + 2u;
    for (const auto& entry : entries) {
        put_u16(header, entryOffset, entry.tag);
        put_u16(header, entryOffset + 2u, entry.type);
        put_u32(header, entryOffset + 4u, entry.count);
        if (entry.data.size() <= 4u) {
            std::copy(entry.data.begin(), entry.data.end(), header.begin() + static_cast<std::ptrdiff_t>(entryOffset + 8u));
        } else {
            put_u32(header, entryOffset + 8u, entry.externalOffset);
            const std::size_t off = entry.externalOffset;
            if (off > header.size() || entry.data.size() > header.size() - off) {
                return Status::error(StatusCode::TiffLayoutOverflow, "TIFF metadata payload outside header");
            }
            std::copy(entry.data.begin(), entry.data.end(), header.begin() + static_cast<std::ptrdiff_t>(off));
        }
        entryOffset += 12u;
    }
    put_u32(header, entryOffset, 0u);
    return Status::ok();
}

std::uint16_t quantize(float value, Result& result, bool& valid) noexcept {
    if (!std::isfinite(value)) {
        valid = false;
        return 0u;
    }
    double x = static_cast<double>(value);
    if (x < 0.0) {
        ++result.clippedLowSamples;
        x = 0.0;
    } else if (x > 1.0) {
        ++result.clippedHighSamples;
        x = 1.0;
    }
    ++result.projectedSamples;
    const double q = std::floor(x * 65535.0 + 0.5);
    return static_cast<std::uint16_t>(std::clamp(q, 0.0, 65535.0));
}

std::size_t logical_bytes(const streaming_v0_1::IRawTileSource& source,
                          const Workspace& workspace,
                          const std::vector<std::uint8_t>& strip,
                          const std::vector<std::uint8_t>& header) noexcept {
    const std::size_t a = source.residentBytesUpperBound();
    const std::size_t b = vector_bytes(workspace);
    if (a > std::numeric_limits<std::size_t>::max() - b) return std::numeric_limits<std::size_t>::max();
    std::size_t total = a + b;
    if (total > std::numeric_limits<std::size_t>::max() - strip.capacity()) return std::numeric_limits<std::size_t>::max();
    total += strip.capacity();
    if (total > std::numeric_limits<std::size_t>::max() - header.capacity()) return std::numeric_limits<std::size_t>::max();
    return total + header.capacity();
}

}  // namespace

Status export_projection(streaming_v0_1::IRawTileSource& source,
                         IReconstructionBackend& reconstruction,
                         int outputFd,
                         ProjectionKind kind,
                         const Options& options,
                         Result& out) noexcept {
    out = {};
    out.kind = kind;
    const auto& metadata = source.metadata();
    if (outputFd < 0 || metadata.width <= 1 || metadata.height <= 1 || options.rowsPerStrip <= 0) {
        return Status::error(StatusCode::InvalidArgument, "invalid output fd, dimensions or rowsPerStrip");
    }
    if (kind != ProjectionKind::RawSensorCfa16 &&
        kind != ProjectionKind::ReconstructedCfaDng16 &&
        kind != ProjectionKind::LinearDng16) {
        return Status::error(StatusCode::UnsupportedProjection, "unsupported projection kind");
    }
    const int halo = reconstruction.requiredHalo();
    if (halo < 0) return Status::error(StatusCode::InvalidArgument, "reconstruction backend returned negative halo");
    if (::ftruncate(outputFd, 0) != 0 || ::lseek(outputFd, 0, SEEK_SET) < 0) {
        return Status::error(StatusCode::OutputNotSeekable, "output document is not truncatable/seekable");
    }

    const int samples = kind == ProjectionKind::LinearDng16 ? 3 : 1;
    const std::uint32_t stripCount = static_cast<std::uint32_t>(
        (metadata.height + options.rowsPerStrip - 1) / options.rowsPerStrip);
    std::vector<std::uint32_t> stripByteCounts;
    stripByteCounts.reserve(stripCount);
    for (int y0 = 0; y0 < metadata.height; y0 += options.rowsPerStrip) {
        const int rows = std::min(options.rowsPerStrip, metadata.height - y0);
        const std::uint64_t bytes = static_cast<std::uint64_t>(metadata.width) *
                                    static_cast<std::uint64_t>(rows) *
                                    static_cast<std::uint64_t>(samples) * 2u;
        std::uint32_t encoded = 0u;
        if (!checked_u32(bytes, encoded)) {
            return Status::error(StatusCode::TiffLayoutOverflow, "strip byte count exceeds classic TIFF range");
        }
        stripByteCounts.push_back(encoded);
    }

    std::vector<std::uint8_t> header;
    if (kind != ProjectionKind::RawSensorCfa16) {
        const auto headerStatus = build_dng_header(metadata, kind, options.rowsPerStrip, stripByteCounts, header);
        if (!headerStatus) return headerStatus;
        if (!write_all(outputFd, header.data(), header.size())) {
            return Status::error(StatusCode::OutputWriteFailed, "failed writing DNG header");
        }
        out.outputBytes += header.size();
    }

    Workspace workspace{};
    std::vector<std::uint8_t> strip;
    std::uint32_t stripsWritten = 0u;
    for (int y0 = 0; y0 < metadata.height; y0 += options.rowsPerStrip) {
        const int y1 = std::min(metadata.height, y0 + options.rowsPerStrip);
        TileRect tile{};
        tile.x0 = 0;
        tile.y0 = y0;
        tile.x1 = metadata.width;
        tile.y1 = y1;
        tile.hx0 = 0;
        tile.hy0 = std::max(0, y0 - halo);
        tile.hx1 = metadata.width;
        tile.hy1 = std::min(metadata.height, y1 + halo);

        const auto filled = fill_stage2(source, tile, workspace);
        if (!filled) {
            return Status::error(StatusCode::SourceFailed, "Stage-2 source read failed during projection: " + filled.message);
        }
        const int tileWidth = tile.hx1 - tile.hx0;
        const int tileHeight = tile.hy1 - tile.hy0;
        const int coreWidth = tile.x1 - tile.x0;
        const int coreHeight = tile.y1 - tile.y0;
        const std::size_t corePixels = static_cast<std::size_t>(coreWidth) * static_cast<std::size_t>(coreHeight);
        if (corePixels > std::numeric_limits<std::size_t>::max() / 3u) {
            return Status::error(StatusCode::BudgetExceeded, "projection core size overflow");
        }
        workspace.cam.resize(corePixels * 3u);
        const auto reconstructed = reconstruction.reconstructTile(
            workspace.stage2.data(), tileWidth, tileHeight,
            tile.hx0, tile.hy0,
            tile.x0, tile.y0, coreWidth, coreHeight,
            metadata.cfa, workspace.cam.data());
        if (!reconstructed) {
            return Status::error(StatusCode::ReconstructionFailed,
                                 "camera-native reconstruction failed during projection: " + reconstructed.message);
        }

        const std::size_t sampleCount = corePixels * static_cast<std::size_t>(samples);
        if (sampleCount > std::numeric_limits<std::size_t>::max() / 2u) {
            return Status::error(StatusCode::BudgetExceeded, "projection strip size overflow");
        }
        strip.resize(sampleCount * 2u);
        bool valid = true;
        std::size_t cursor = 0u;
        for (int y = y0; y < y1; ++y) {
            const int localY = y - y0;
            for (int x = 0; x < metadata.width; ++x) {
                const std::size_t pixel = static_cast<std::size_t>(localY) * static_cast<std::size_t>(coreWidth) +
                                          static_cast<std::size_t>(x);
                if (samples == 1) {
                    const int channel = cfa_channel(metadata.cfa, x, y);
                    const std::uint16_t q = quantize(workspace.cam[pixel * 3u + static_cast<std::size_t>(channel)], out, valid);
                    if (!valid) return Status::error(StatusCode::NonFiniteScientificSample, "non-finite camera-native sample in CFA projection");
                    strip[cursor++] = static_cast<std::uint8_t>(q & 0xffu);
                    strip[cursor++] = static_cast<std::uint8_t>((q >> 8u) & 0xffu);
                } else {
                    for (int channel = 0; channel < 3; ++channel) {
                        const std::uint16_t q = quantize(workspace.cam[pixel * 3u + static_cast<std::size_t>(channel)], out, valid);
                        if (!valid) return Status::error(StatusCode::NonFiniteScientificSample, "non-finite camera-native sample in LinearRaw projection");
                        strip[cursor++] = static_cast<std::uint8_t>(q & 0xffu);
                        strip[cursor++] = static_cast<std::uint8_t>((q >> 8u) & 0xffu);
                    }
                }
            }
        }
        if (cursor != strip.size()) {
            return Status::error(StatusCode::OutputWriteFailed, "internal projection strip length mismatch");
        }

        const std::size_t resident = logical_bytes(source, workspace, strip, header);
        out.logicalWorkspacePeakBytes = std::max(out.logicalWorkspacePeakBytes, resident);
        if (options.memoryBudgetBytes != 0u && resident > options.memoryBudgetBytes) {
            return Status::error(StatusCode::BudgetExceeded, "projection writer exceeded caller memory budget");
        }
        if (!write_all(outputFd, strip.data(), strip.size())) {
            return Status::error(StatusCode::OutputWriteFailed, "failed writing projected strip");
        }
        out.outputBytes += strip.size();
        ++stripsWritten;
    }

    out.width = metadata.width;
    out.height = metadata.height;
    out.samplesPerPixel = samples;
    out.stripsWritten = stripsWritten;
    out.fullScientificMasterMaterialized = false;
    out.sourcePixelsClaimedMeasured = false;
    out.projectionOnly = true;
    out.physicalFrameCount = 1u;
    out.independentEvidenceCount = 1u;
    if (::fsync(outputFd) != 0 && errno != EINVAL && errno != EROFS) {
        return Status::error(StatusCode::OutputWriteFailed, "output fsync failed");
    }
    return Status::ok();
}

const char* status_name(StatusCode code) noexcept {
    switch (code) {
        case StatusCode::Ok: return "OK";
        case StatusCode::InvalidArgument: return "INVALID_ARGUMENT";
        case StatusCode::UnsupportedProjection: return "UNSUPPORTED_PROJECTION";
        case StatusCode::InvalidColorMatrix: return "INVALID_COLOR_MATRIX";
        case StatusCode::SourceFailed: return "SOURCE_FAILED";
        case StatusCode::ReconstructionFailed: return "RECONSTRUCTION_FAILED";
        case StatusCode::NonFiniteScientificSample: return "NONFINITE_SCIENTIFIC_SAMPLE";
        case StatusCode::BudgetExceeded: return "BUDGET_EXCEEDED";
        case StatusCode::OutputNotSeekable: return "OUTPUT_NOT_SEEKABLE";
        case StatusCode::OutputWriteFailed: return "OUTPUT_WRITE_FAILED";
        case StatusCode::TiffLayoutOverflow: return "TIFF_LAYOUT_OVERFLOW";
    }
    return "UNKNOWN";
}

const char* projection_name(ProjectionKind kind) noexcept {
    switch (kind) {
        case ProjectionKind::RawSensorCfa16: return "RECONSTRUCTED_CFA_RAWSENSOR_PROJECTION_V0_1";
        case ProjectionKind::ReconstructedCfaDng16: return "RECONSTRUCTED_CFA_DNG_PROJECTION_V0_1";
        case ProjectionKind::LinearDng16: return "LINEAR_DNG_COMPATIBILITY_PROJECTION_V0_1";
    }
    return "UNKNOWN_PROJECTION";
}

}  // namespace truthraw::raw_projection_export::v0_1
