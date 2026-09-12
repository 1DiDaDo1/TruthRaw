#include "linear_dng_compatibility_projection_v0_1.h"

#include "full_frame_streaming_v0_1_internal.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include <unistd.h>

namespace truthraw::linear_dng_compatibility_projection::v0_1 {
namespace {

using scientific_preview_binding_v0_1::ColorBindingAuthority;
using scientific_preview_binding_v0_1::ScientificColorBindingRecord;
using streaming_v0_1::detail::Workspace;
using streaming_v0_1::detail::fill_stage2;
using streaming_v0_1::detail::vector_bytes;

constexpr std::uint16_t kTiffByte = 1;
constexpr std::uint16_t kTiffAscii = 2;
constexpr std::uint16_t kTiffShort = 3;
constexpr std::uint16_t kTiffLong = 4;
constexpr std::uint16_t kTiffRational = 5;
constexpr std::uint16_t kTiffSRational = 10;
constexpr std::uint16_t kPhotometricLinearRaw = 34892;
constexpr std::uint16_t kCalibrationIlluminantD50 = 23;
constexpr std::uint32_t kClassicTiffIfdOffset = 8u;
constexpr std::size_t kIfdEntryCount = 28u;
constexpr std::int64_t kMatrixDenominator = 10000000ll;

struct IfdEntry final {
    std::uint16_t tag = 0;
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::array<std::uint8_t, 4> value{};
};

bool safe_add_u64(std::uint64_t a, std::uint64_t b, std::uint64_t& out) noexcept {
    if (a > std::numeric_limits<std::uint64_t>::max() - b) return false;
    out = a + b;
    return true;
}

bool safe_mul_u64(std::uint64_t a, std::uint64_t b, std::uint64_t& out) noexcept {
    if (a != 0u && b > std::numeric_limits<std::uint64_t>::max() / a) return false;
    out = a * b;
    return true;
}

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

void append_s32(std::vector<std::uint8_t>& out, std::int32_t value) {
    append_u32(out, static_cast<std::uint32_t>(value));
}

void put_u32(std::vector<std::uint8_t>& out, std::size_t offset, std::uint32_t value) {
    out[offset + 0u] = static_cast<std::uint8_t>(value & 0xffu);
    out[offset + 1u] = static_cast<std::uint8_t>((value >> 8u) & 0xffu);
    out[offset + 2u] = static_cast<std::uint8_t>((value >> 16u) & 0xffu);
    out[offset + 3u] = static_cast<std::uint8_t>((value >> 24u) & 0xffu);
}

void align_extra(std::vector<std::uint8_t>& extra, std::size_t alignment) {
    while ((extra.size() % alignment) != 0u) extra.push_back(0u);
}

std::uint32_t add_extra(
    std::vector<std::uint8_t>& extra,
    std::uint32_t extraBase,
    const std::vector<std::uint8_t>& bytes,
    std::size_t alignment = 4u) {
    align_extra(extra, alignment);
    const std::uint64_t absolute =
        static_cast<std::uint64_t>(extraBase) + static_cast<std::uint64_t>(extra.size());
    if (absolute > std::numeric_limits<std::uint32_t>::max()) return 0u;
    const auto offset = static_cast<std::uint32_t>(absolute);
    extra.insert(extra.end(), bytes.begin(), bytes.end());
    return offset;
}

std::uint32_t add_zero_extra(
    std::vector<std::uint8_t>& extra,
    std::uint32_t extraBase,
    std::size_t count,
    std::size_t alignment = 4u) {
    align_extra(extra, alignment);
    const std::uint64_t absolute =
        static_cast<std::uint64_t>(extraBase) + static_cast<std::uint64_t>(extra.size());
    if (absolute > std::numeric_limits<std::uint32_t>::max()) return 0u;
    const auto offset = static_cast<std::uint32_t>(absolute);
    extra.resize(extra.size() + count, 0u);
    return offset;
}

IfdEntry inline_short(std::uint16_t tag, std::uint16_t value) {
    IfdEntry e;
    e.tag = tag;
    e.type = kTiffShort;
    e.count = 1u;
    e.value[0] = static_cast<std::uint8_t>(value & 0xffu);
    e.value[1] = static_cast<std::uint8_t>((value >> 8u) & 0xffu);
    return e;
}

IfdEntry inline_long(std::uint16_t tag, std::uint32_t value) {
    IfdEntry e;
    e.tag = tag;
    e.type = kTiffLong;
    e.count = 1u;
    e.value[0] = static_cast<std::uint8_t>(value & 0xffu);
    e.value[1] = static_cast<std::uint8_t>((value >> 8u) & 0xffu);
    e.value[2] = static_cast<std::uint8_t>((value >> 16u) & 0xffu);
    e.value[3] = static_cast<std::uint8_t>((value >> 24u) & 0xffu);
    return e;
}

IfdEntry inline_bytes4(std::uint16_t tag, std::array<std::uint8_t, 4> bytes) {
    IfdEntry e;
    e.tag = tag;
    e.type = kTiffByte;
    e.count = 4u;
    e.value = bytes;
    return e;
}

IfdEntry offset_entry(
    std::uint16_t tag,
    std::uint16_t type,
    std::uint32_t count,
    std::uint32_t offset) {
    IfdEntry e = inline_long(tag, offset);
    e.type = type;
    e.count = count;
    return e;
}

bool write_all(int fd, const void* data, std::size_t bytes) noexcept {
    const auto* p = static_cast<const std::uint8_t*>(data);
    std::size_t done = 0u;
    while (done < bytes) {
        const ssize_t written = ::write(fd, p + done, bytes - done);
        if (written < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (written == 0) return false;
        done += static_cast<std::size_t>(written);
    }
    return true;
}

bool matrix_inverse(const std::array<float, 9>& in, std::array<double, 9>& out) noexcept {
    const double a = in[0], b = in[1], c = in[2];
    const double d = in[3], e = in[4], f = in[5];
    const double g = in[6], h = in[7], i = in[8];
    const double det =
        a * (e * i - f * h) -
        b * (d * i - f * g) +
        c * (d * h - e * g);
    if (!std::isfinite(det) || std::abs(det) <= 1.0e-12) return false;
    out = {
        (e*i-f*h)/det, (c*h-b*i)/det, (b*f-c*e)/det,
        (f*g-d*i)/det, (a*i-c*g)/det, (c*d-a*f)/det,
        (d*h-e*g)/det, (b*g-a*h)/det, (a*e-b*d)/det,
    };
    for (const double v : out) {
        if (!std::isfinite(v) || std::abs(v) > 128.0) return false;
    }
    return true;
}

bool append_srational_matrix(
    std::vector<std::uint8_t>& bytes,
    const std::array<double, 9>& matrix) {
    for (const double value : matrix) {
        if (!std::isfinite(value)) return false;
        const double scaled = value * static_cast<double>(kMatrixDenominator);
        if (scaled < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
            scaled > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
            return false;
        }
        append_s32(bytes, static_cast<std::int32_t>(std::llround(scaled)));
        append_s32(bytes, static_cast<std::int32_t>(kMatrixDenominator));
    }
    return true;
}

bool exact_color_match(
    const DngMetadata& metadata,
    const ScientificColorBindingRecord& color) noexcept {
    for (std::size_t index = 0u; index < 9u; ++index) {
        if (!std::isfinite(metadata.cameraToXyzD50[index]) ||
            !std::isfinite(color.cameraToXyzD50[index]) ||
            metadata.cameraToXyzD50[index] != color.cameraToXyzD50[index]) {
            return false;
        }
    }
    return true;
}

bool authority_allowed(ColorBindingAuthority authority) noexcept {
    return authority == ColorBindingAuthority::SourceMetadataBound ||
           authority == ColorBindingAuthority::GatehouseCertifiedMetadata ||
           authority == ColorBindingAuthority::IndependentCalibration;
}

std::size_t estimated_workspace_bytes(
    const DngMetadata& metadata,
    int tileEdge,
    int halo) noexcept {
    const int haloWidth = std::min(metadata.width, tileEdge + 2 * halo);
    const int haloHeight = std::min(metadata.height, tileEdge + 2 * halo);
    const std::size_t haloSamples =
        static_cast<std::size_t>(haloWidth) * static_cast<std::size_t>(haloHeight);
    const std::size_t coreSamples =
        static_cast<std::size_t>(std::min(metadata.width, tileEdge)) *
        static_cast<std::size_t>(std::min(metadata.height, tileEdge));
    std::size_t bytes = haloSamples * sizeof(std::uint16_t); // raw
    bytes += haloSamples * sizeof(float);                    // stage2
    if (metadata.hasGainField) bytes += haloSamples * sizeof(float);
    if (metadata.hasResidualBlack) {
        bytes += static_cast<std::size_t>(haloHeight + haloWidth) * sizeof(float);
    }
    bytes += 3u * coreSamples * sizeof(float);               // reconstructed camera RGB
    bytes += static_cast<std::size_t>(tileEdge) * static_cast<std::size_t>(tileEdge) *
             3u * sizeof(std::uint16_t);                     // encoded output tile
    return bytes;
}

Status build_prefix(
    const DngMetadata& metadata,
    const ScientificColorBindingRecord& color,
    int tileEdge,
    std::uint32_t tileCount,
    std::uint32_t tileByteCount,
    std::vector<std::uint8_t>& prefix,
    std::uint32_t& pixelDataOffset) {
    const std::uint32_t extraBase =
        kClassicTiffIfdOffset + 2u + static_cast<std::uint32_t>(kIfdEntryCount * 12u) + 4u;
    std::vector<std::uint8_t> extra;

    std::vector<std::uint8_t> bits;
    for (int index = 0; index < 3; ++index) append_u16(bits, 16u);
    const std::uint32_t bitsOffset = add_extra(extra, extraBase, bits, 2u);

    std::vector<std::uint8_t> sampleFormat;
    for (int index = 0; index < 3; ++index) append_u16(sampleFormat, 1u);
    const std::uint32_t sampleFormatOffset = add_extra(extra, extraBase, sampleFormat, 2u);

    std::vector<std::uint8_t> blackLevel;
    for (int index = 0; index < 3; ++index) append_u16(blackLevel, 0u);
    const std::uint32_t blackLevelOffset = add_extra(extra, extraBase, blackLevel, 2u);

    std::vector<std::uint8_t> whiteLevel;
    for (int index = 0; index < 3; ++index) append_u32(whiteLevel, 65535u);
    const std::uint32_t whiteLevelOffset = add_extra(extra, extraBase, whiteLevel);

    std::vector<std::uint8_t> cropOrigin;
    append_u32(cropOrigin, 0u);
    append_u32(cropOrigin, 0u);
    const std::uint32_t cropOriginOffset = add_extra(extra, extraBase, cropOrigin);

    std::vector<std::uint8_t> cropSize;
    append_u32(cropSize, static_cast<std::uint32_t>(metadata.width));
    append_u32(cropSize, static_cast<std::uint32_t>(metadata.height));
    const std::uint32_t cropSizeOffset = add_extra(extra, extraBase, cropSize);

    std::vector<std::uint8_t> activeArea;
    append_u32(activeArea, 0u);
    append_u32(activeArea, 0u);
    append_u32(activeArea, static_cast<std::uint32_t>(metadata.height));
    append_u32(activeArea, static_cast<std::uint32_t>(metadata.width));
    const std::uint32_t activeAreaOffset = add_extra(extra, extraBase, activeArea);

    std::array<double, 9> inverse{};
    if (!matrix_inverse(color.cameraToXyzD50, inverse)) {
        return Status::error(StatusCode::InvalidArgument,
                             "cameraToXyzD50 cannot be inverted for DNG ColorMatrix1");
    }
    std::vector<std::uint8_t> colorMatrix;
    if (!append_srational_matrix(colorMatrix, inverse)) {
        return Status::error(StatusCode::InvalidArgument,
                             "ColorMatrix1 rational encoding failed");
    }
    const std::uint32_t colorMatrixOffset = add_extra(extra, extraBase, colorMatrix);

    std::array<double, 9> forward{};
    for (std::size_t index = 0u; index < 9u; ++index) {
        forward[index] = static_cast<double>(color.cameraToXyzD50[index]);
    }
    std::vector<std::uint8_t> forwardMatrix;
    if (!append_srational_matrix(forwardMatrix, forward)) {
        return Status::error(StatusCode::InvalidArgument,
                             "ForwardMatrix1 rational encoding failed");
    }
    const std::uint32_t forwardMatrixOffset = add_extra(extra, extraBase, forwardMatrix);

    std::vector<std::uint8_t> neutral;
    for (int index = 0; index < 3; ++index) {
        append_u32(neutral, 1u);
        append_u32(neutral, 1u);
    }
    const std::uint32_t neutralOffset = add_extra(extra, extraBase, neutral);

    const std::string model = "TruthRaw Reconstructed Linear v0.1";
    const std::vector<std::uint8_t> modelBytes(model.begin(), model.end());
    std::vector<std::uint8_t> modelTerminated = modelBytes;
    modelTerminated.push_back(0u);
    const std::uint32_t modelOffset = add_extra(extra, extraBase, modelTerminated, 1u);

    const std::string software = "TruthRaw Linear DNG Compatibility Projection v0.1";
    std::vector<std::uint8_t> softwareBytes(software.begin(), software.end());
    softwareBytes.push_back(0u);
    const std::uint32_t softwareOffset = add_extra(extra, extraBase, softwareBytes, 1u);

    const std::string description =
        "TruthRaw reconstructed camera-native RGB compatibility projection; "
        "not measured sensor CFA; no appearance/tone/sRGB applied; source-bound color authority.";
    std::vector<std::uint8_t> descriptionBytes(description.begin(), description.end());
    descriptionBytes.push_back(0u);
    const std::uint32_t descriptionOffset = add_extra(extra, extraBase, descriptionBytes, 1u);

    align_extra(extra, 4u);
    const std::size_t tileOffsetsRelative = extra.size();
    const std::uint32_t tileOffsetsOffset =
        add_zero_extra(extra, extraBase, static_cast<std::size_t>(tileCount) * 4u);

    std::vector<std::uint8_t> tileByteCounts;
    tileByteCounts.reserve(static_cast<std::size_t>(tileCount) * 4u);
    for (std::uint32_t index = 0u; index < tileCount; ++index) {
        append_u32(tileByteCounts, tileByteCount);
    }
    const std::uint32_t tileByteCountsOffset = add_extra(extra, extraBase, tileByteCounts);

    align_extra(extra, 4u);
    const std::uint64_t pixelOffset64 =
        static_cast<std::uint64_t>(extraBase) + static_cast<std::uint64_t>(extra.size());
    if (pixelOffset64 > std::numeric_limits<std::uint32_t>::max()) {
        return Status::error(StatusCode::ClassicTiffLimitExceeded,
                             "DNG metadata exceeds classic-TIFF offset range");
    }
    pixelDataOffset = static_cast<std::uint32_t>(pixelOffset64);

    for (std::uint32_t index = 0u; index < tileCount; ++index) {
        const std::uint64_t tileOffset64 =
            static_cast<std::uint64_t>(pixelDataOffset) +
            static_cast<std::uint64_t>(index) * static_cast<std::uint64_t>(tileByteCount);
        if (tileOffset64 > std::numeric_limits<std::uint32_t>::max()) {
            return Status::error(StatusCode::ClassicTiffLimitExceeded,
                                 "DNG tile offset exceeds classic-TIFF limit");
        }
        put_u32(extra,
                tileOffsetsRelative + static_cast<std::size_t>(index) * 4u,
                static_cast<std::uint32_t>(tileOffset64));
    }

    std::vector<IfdEntry> entries;
    entries.reserve(kIfdEntryCount);
    entries.push_back(inline_long(254u, 0u));
    entries.push_back(inline_long(256u, static_cast<std::uint32_t>(metadata.width)));
    entries.push_back(inline_long(257u, static_cast<std::uint32_t>(metadata.height)));
    entries.push_back(offset_entry(258u, kTiffShort, 3u, bitsOffset));
    entries.push_back(inline_short(259u, 1u));
    entries.push_back(inline_short(262u, kPhotometricLinearRaw));
    entries.push_back(offset_entry(270u, kTiffAscii,
                                   static_cast<std::uint32_t>(descriptionBytes.size()),
                                   descriptionOffset));
    entries.push_back(inline_short(274u, static_cast<std::uint16_t>(metadata.orientation)));
    entries.push_back(inline_short(277u, 3u));
    entries.push_back(inline_short(284u, 1u));
    entries.push_back(offset_entry(305u, kTiffAscii,
                                   static_cast<std::uint32_t>(softwareBytes.size()),
                                   softwareOffset));
    entries.push_back(inline_long(322u, static_cast<std::uint32_t>(tileEdge)));
    entries.push_back(inline_long(323u, static_cast<std::uint32_t>(tileEdge)));
    entries.push_back(offset_entry(324u, kTiffLong, tileCount, tileOffsetsOffset));
    entries.push_back(offset_entry(325u, kTiffLong, tileCount, tileByteCountsOffset));
    entries.push_back(offset_entry(339u, kTiffShort, 3u, sampleFormatOffset));
    entries.push_back(inline_bytes4(50706u, {1u, 4u, 0u, 0u}));
    entries.push_back(inline_bytes4(50707u, {1u, 3u, 0u, 0u}));
    entries.push_back(offset_entry(50708u, kTiffAscii,
                                   static_cast<std::uint32_t>(modelTerminated.size()),
                                   modelOffset));
    entries.push_back(offset_entry(50714u, kTiffShort, 3u, blackLevelOffset));
    entries.push_back(offset_entry(50717u, kTiffLong, 3u, whiteLevelOffset));
    entries.push_back(offset_entry(50719u, kTiffLong, 2u, cropOriginOffset));
    entries.push_back(offset_entry(50720u, kTiffLong, 2u, cropSizeOffset));
    entries.push_back(offset_entry(50721u, kTiffSRational, 9u, colorMatrixOffset));
    entries.push_back(offset_entry(50728u, kTiffRational, 3u, neutralOffset));
    entries.push_back(inline_short(50778u, kCalibrationIlluminantD50));
    entries.push_back(offset_entry(50829u, kTiffLong, 4u, activeAreaOffset));
    entries.push_back(offset_entry(50964u, kTiffSRational, 9u, forwardMatrixOffset));

    if (entries.size() != kIfdEntryCount) {
        return Status::error(StatusCode::InvalidArgument,
                             "internal DNG IFD entry-count mismatch");
    }
    std::sort(entries.begin(), entries.end(),
              [](const IfdEntry& a, const IfdEntry& b) { return a.tag < b.tag; });

    prefix.clear();
    prefix.reserve(static_cast<std::size_t>(pixelDataOffset));
    prefix.push_back('I');
    prefix.push_back('I');
    append_u16(prefix, 42u);
    append_u32(prefix, kClassicTiffIfdOffset);
    append_u16(prefix, static_cast<std::uint16_t>(entries.size()));
    for (const auto& entry : entries) {
        append_u16(prefix, entry.tag);
        append_u16(prefix, entry.type);
        append_u32(prefix, entry.count);
        prefix.insert(prefix.end(), entry.value.begin(), entry.value.end());
    }
    append_u32(prefix, 0u); // no next IFD
    prefix.insert(prefix.end(), extra.begin(), extra.end());

    if (prefix.size() != static_cast<std::size_t>(pixelDataOffset)) {
        return Status::error(StatusCode::InvalidArgument,
                             "internal DNG prefix-size mismatch");
    }
    return Status::ok();
}

} // namespace

Status write_linear_dng(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const ScientificColorBindingRecord& color,
    int outputFd,
    const Options& options,
    Result& out) noexcept {
    out = {};
    if (outputFd < 0 || options.tileEdge < 16 || options.tileEdge > 512 ||
        (options.tileEdge % 16) != 0) {
        return Status::error(StatusCode::InvalidArgument,
                             "invalid output fd or DNG tile edge");
    }
    const auto& metadata = source.metadata();
    if (metadata.width <= 1 || metadata.height <= 1 ||
        metadata.width > 65535 || metadata.height > 65535) {
        return Status::error(StatusCode::InvalidArgument,
                             "invalid/unsupported Linear DNG dimensions");
    }
    if (!color.validated || !color.normalized || !authority_allowed(color.authority) ||
        color.physicalFrameCount != 1u || color.independentEvidenceCount != 1u) {
        return Status::error(StatusCode::UnauthorizedColorBinding,
                             "Linear DNG requires validated 1/1 scientific color binding");
    }
    if (!exact_color_match(metadata, color)) {
        return Status::error(StatusCode::ColorBindingMismatch,
                             "source cameraToXyzD50 differs from finalized color binding");
    }
    const int halo = reconstruction.requiredHalo();
    if (halo < 0 || halo > options.tileEdge / 2) {
        return Status::error(StatusCode::InvalidArgument,
                             "reconstruction halo is invalid for DNG projection");
    }

    const std::size_t workspaceEstimate =
        estimated_workspace_bytes(metadata, options.tileEdge, halo);
    const std::size_t sourceResident = source.residentBytesUpperBound();
    if (workspaceEstimate > std::numeric_limits<std::size_t>::max() - sourceResident) {
        return Status::error(StatusCode::BudgetExceeded,
                             "Linear DNG resident accounting overflow");
    }
    const std::size_t residentEstimate = workspaceEstimate + sourceResident;
    if (options.memoryBudgetBytes != 0u && residentEstimate > options.memoryBudgetBytes) {
        return Status::error(StatusCode::BudgetExceeded,
                             "Linear DNG logical resident estimate exceeds caller budget");
    }

    const std::uint32_t tilesX = static_cast<std::uint32_t>(
        (metadata.width + options.tileEdge - 1) / options.tileEdge);
    const std::uint32_t tilesY = static_cast<std::uint32_t>(
        (metadata.height + options.tileEdge - 1) / options.tileEdge);
    std::uint64_t tileCount64 = 0u;
    if (!safe_mul_u64(tilesX, tilesY, tileCount64) ||
        tileCount64 == 0u || tileCount64 > std::numeric_limits<std::uint32_t>::max()) {
        return Status::error(StatusCode::ClassicTiffLimitExceeded,
                             "Linear DNG tile count exceeds classic-TIFF bounds");
    }
    const auto tileCount = static_cast<std::uint32_t>(tileCount64);

    std::uint64_t tileByteCount64 = 0u;
    if (!safe_mul_u64(static_cast<std::uint64_t>(options.tileEdge),
                      static_cast<std::uint64_t>(options.tileEdge),
                      tileByteCount64) ||
        !safe_mul_u64(tileByteCount64, 3u * sizeof(std::uint16_t), tileByteCount64) ||
        tileByteCount64 > std::numeric_limits<std::uint32_t>::max()) {
        return Status::error(StatusCode::ClassicTiffLimitExceeded,
                             "Linear DNG tile byte size overflow");
    }
    const auto tileByteCount = static_cast<std::uint32_t>(tileByteCount64);

    std::vector<std::uint8_t> prefix;
    std::uint32_t pixelDataOffset = 0u;
    auto prefixStatus = build_prefix(
        metadata, color, options.tileEdge, tileCount, tileByteCount,
        prefix, pixelDataOffset);
    if (!prefixStatus) return prefixStatus;

    std::uint64_t pixelBytes = 0u;
    std::uint64_t outputBytes = 0u;
    if (!safe_mul_u64(tileCount, tileByteCount, pixelBytes) ||
        !safe_add_u64(pixelDataOffset, pixelBytes, outputBytes) ||
        outputBytes > std::numeric_limits<std::uint32_t>::max()) {
        return Status::error(StatusCode::ClassicTiffLimitExceeded,
                             "Linear DNG file exceeds classic-TIFF 4 GiB limit");
    }

    if (::ftruncate(outputFd, 0) != 0 || ::lseek(outputFd, 0, SEEK_SET) < 0) {
        return Status::error(StatusCode::OutputIoFailed,
                             "destination document is not writable/seekable");
    }
    if (!write_all(outputFd, prefix.data(), prefix.size())) {
        return Status::error(StatusCode::OutputIoFailed,
                             "failed writing Linear DNG metadata prefix");
    }

    Workspace workspace{};
    std::vector<std::uint8_t> encodedTile(static_cast<std::size_t>(tileByteCount), 0u);
    std::size_t workspacePeak = workspaceEstimate;
    std::uint64_t clippedLow = 0u;
    std::uint64_t clippedHigh = 0u;
    std::uint64_t samplesWritten = 0u;
    std::uint64_t tilesWritten = 0u;

    for (std::uint32_t tileY = 0u; tileY < tilesY; ++tileY) {
        for (std::uint32_t tileX = 0u; tileX < tilesX; ++tileX) {
            TileRect tile{};
            tile.x0 = static_cast<int>(tileX) * options.tileEdge;
            tile.y0 = static_cast<int>(tileY) * options.tileEdge;
            tile.x1 = std::min(metadata.width, tile.x0 + options.tileEdge);
            tile.y1 = std::min(metadata.height, tile.y0 + options.tileEdge);
            tile.hx0 = std::max(0, tile.x0 - halo);
            tile.hy0 = std::max(0, tile.y0 - halo);
            tile.hx1 = std::min(metadata.width, tile.x1 + halo);
            tile.hy1 = std::min(metadata.height, tile.y1 + halo);

            const auto filled = fill_stage2(source, tile, workspace);
            if (!filled) {
                return Status::error(StatusCode::SourceFailed,
                                     "Stage-2 tile read failed: " + filled.message);
            }
            const int tileWidth = tile.hx1 - tile.hx0;
            const int tileHeight = tile.hy1 - tile.hy0;
            const int coreWidth = tile.x1 - tile.x0;
            const int coreHeight = tile.y1 - tile.y0;
            const std::size_t coreSamples =
                static_cast<std::size_t>(coreWidth) * static_cast<std::size_t>(coreHeight);
            workspace.cam.resize(3u * coreSamples);
            const auto reconstructed = reconstruction.reconstructTile(
                workspace.stage2.data(), tileWidth, tileHeight,
                tile.hx0, tile.hy0,
                tile.x0, tile.y0, coreWidth, coreHeight,
                metadata.cfa, workspace.cam.data());
            if (!reconstructed) {
                return Status::error(StatusCode::ReconstructionFailed,
                                     "camera-native reconstruction failed: " + reconstructed.message);
            }

            std::fill(encodedTile.begin(), encodedTile.end(), 0u);
            for (int y = 0; y < coreHeight; ++y) {
                for (int x = 0; x < coreWidth; ++x) {
                    const std::size_t sourcePixel =
                        static_cast<std::size_t>(y) * static_cast<std::size_t>(coreWidth) +
                        static_cast<std::size_t>(x);
                    const std::size_t destinationPixel =
                        static_cast<std::size_t>(y) * static_cast<std::size_t>(options.tileEdge) +
                        static_cast<std::size_t>(x);
                    for (std::size_t channel = 0u; channel < 3u; ++channel) {
                        const float sample = workspace.cam[3u * sourcePixel + channel];
                        if (!std::isfinite(sample)) {
                            return Status::error(StatusCode::NonFiniteScientificSample,
                                                 "non-finite reconstructed sample cannot be projected");
                        }
                        float bounded = sample;
                        if (bounded < 0.0f) {
                            ++clippedLow;
                            bounded = 0.0f;
                        } else if (bounded > 1.0f) {
                            ++clippedHigh;
                            bounded = 1.0f;
                        }
                        const auto quantized = static_cast<std::uint16_t>(
                            std::floor(static_cast<double>(bounded) * 65535.0 + 0.5));
                        const std::size_t byteIndex =
                            (3u * destinationPixel + channel) * sizeof(std::uint16_t);
                        encodedTile[byteIndex + 0u] =
                            static_cast<std::uint8_t>(quantized & 0xffu);
                        encodedTile[byteIndex + 1u] =
                            static_cast<std::uint8_t>((quantized >> 8u) & 0xffu);
                        ++samplesWritten;
                    }
                }
            }

            if (!write_all(outputFd, encodedTile.data(), encodedTile.size())) {
                return Status::error(StatusCode::OutputIoFailed,
                                     "failed writing Linear DNG image tile");
            }
            ++tilesWritten;
            workspacePeak = std::max(
                workspacePeak,
                vector_bytes(workspace) + encodedTile.capacity() * sizeof(std::uint8_t));
            if (options.memoryBudgetBytes != 0u &&
                sourceResident <= std::numeric_limits<std::size_t>::max() - workspacePeak &&
                sourceResident + workspacePeak > options.memoryBudgetBytes) {
                return Status::error(StatusCode::BudgetExceeded,
                                     "Linear DNG observed logical resident bound exceeds budget");
            }
        }
    }

    if (::fsync(outputFd) != 0) {
        return Status::error(StatusCode::OutputIoFailed,
                             "failed flushing Linear DNG destination");
    }

    out.width = metadata.width;
    out.height = metadata.height;
    out.outputBytes = outputBytes;
    out.tilesWritten = tilesWritten;
    out.samplesWritten = samplesWritten;
    out.clippedBelowZero = clippedLow;
    out.clippedAboveOne = clippedHigh;
    out.logicalWorkspacePeakBytes = workspacePeak;
    out.fullFrameMaterialized = false;
    out.appearanceApplied = false;
    out.scientificMasterModified = false;
    out.physicalFrameCount = color.physicalFrameCount;
    out.independentEvidenceCount = color.independentEvidenceCount;
    return Status::ok();
}

const char* status_name(StatusCode code) noexcept {
    switch (code) {
        case StatusCode::Ok: return "OK";
        case StatusCode::InvalidArgument: return "INVALID_ARGUMENT";
        case StatusCode::UnauthorizedColorBinding: return "UNAUTHORIZED_COLOR_BINDING";
        case StatusCode::ColorBindingMismatch: return "COLOR_BINDING_MISMATCH";
        case StatusCode::BudgetExceeded: return "BUDGET_EXCEEDED";
        case StatusCode::OutputIoFailed: return "OUTPUT_IO_FAILED";
        case StatusCode::ClassicTiffLimitExceeded: return "CLASSIC_TIFF_LIMIT_EXCEEDED";
        case StatusCode::SourceFailed: return "SOURCE_FAILED";
        case StatusCode::ReconstructionFailed: return "RECONSTRUCTION_FAILED";
        case StatusCode::NonFiniteScientificSample: return "NON_FINITE_SCIENTIFIC_SAMPLE";
    }
    return "UNKNOWN";
}

} // namespace truthraw::linear_dng_compatibility_projection::v0_1
