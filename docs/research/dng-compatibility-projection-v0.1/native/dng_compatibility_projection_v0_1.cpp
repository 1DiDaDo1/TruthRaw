#include "dng_compatibility_projection_v0_1.h"

#include "full_frame_streaming_v0_1_internal.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace truthraw::dng_compatibility_projection::v0_1 {
namespace {

using streaming_v0_1::detail::Workspace;
using streaming_v0_1::detail::fill_stage2;
using streaming_v0_1::detail::vector_bytes;

constexpr std::uint16_t kTypeByte = 1;
constexpr std::uint16_t kTypeAscii = 2;
constexpr std::uint16_t kTypeShort = 3;
constexpr std::uint16_t kTypeLong = 4;
constexpr std::uint16_t kTypeRational = 5;
constexpr std::uint16_t kTypeSRational = 10;

constexpr std::uint16_t kTagNewSubFileType = 254;
constexpr std::uint16_t kTagImageWidth = 256;
constexpr std::uint16_t kTagImageLength = 257;
constexpr std::uint16_t kTagBitsPerSample = 258;
constexpr std::uint16_t kTagCompression = 259;
constexpr std::uint16_t kTagPhotometricInterpretation = 262;
constexpr std::uint16_t kTagImageDescription = 270;
constexpr std::uint16_t kTagOrientation = 274;
constexpr std::uint16_t kTagSamplesPerPixel = 277;
constexpr std::uint16_t kTagPlanarConfiguration = 284;
constexpr std::uint16_t kTagSoftware = 305;
constexpr std::uint16_t kTagTileWidth = 322;
constexpr std::uint16_t kTagTileLength = 323;
constexpr std::uint16_t kTagTileOffsets = 324;
constexpr std::uint16_t kTagTileByteCounts = 325;
constexpr std::uint16_t kTagSampleFormat = 339;
constexpr std::uint16_t kTagCfaRepeatPatternDim = 33421;
constexpr std::uint16_t kTagCfaPattern = 33422;

constexpr std::uint16_t kTagDngVersion = 50706;
constexpr std::uint16_t kTagDngBackwardVersion = 50707;
constexpr std::uint16_t kTagUniqueCameraModel = 50708;
constexpr std::uint16_t kTagCfaPlaneColor = 50710;
constexpr std::uint16_t kTagCfaLayout = 50711;
constexpr std::uint16_t kTagColorMatrix1 = 50721;
constexpr std::uint16_t kTagAsShotWhiteXy = 50729;
constexpr std::uint16_t kTagDngPrivateData = 50740;
constexpr std::uint16_t kTagCalibrationIlluminant1 = 50778;

constexpr std::uint16_t kCompressionUncompressed = 1;
constexpr std::uint16_t kPhotometricCfa = 32803;
constexpr std::uint16_t kPhotometricLinearRaw = 34892;
constexpr std::uint16_t kSampleFormatIeeeFloat = 3;
constexpr std::uint16_t kCalibrationIlluminantD50 = 23;
constexpr std::uint16_t kCfaLayoutRectangular = 1;

struct IfdEntry final {
    std::uint16_t tag = 0;
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::vector<std::uint8_t> value;
    std::uint32_t externalOffset = 0;
};

void append_u16(std::vector<std::uint8_t>& out, std::uint16_t v) {
    out.push_back(static_cast<std::uint8_t>(v & 0xffu));
    out.push_back(static_cast<std::uint8_t>((v >> 8u) & 0xffu));
}

void append_u32(std::vector<std::uint8_t>& out, std::uint32_t v) {
    out.push_back(static_cast<std::uint8_t>(v & 0xffu));
    out.push_back(static_cast<std::uint8_t>((v >> 8u) & 0xffu));
    out.push_back(static_cast<std::uint8_t>((v >> 16u) & 0xffu));
    out.push_back(static_cast<std::uint8_t>((v >> 24u) & 0xffu));
}

void write_u16_at(std::vector<std::uint8_t>& out, std::size_t offset, std::uint16_t v) {
    out[offset] = static_cast<std::uint8_t>(v & 0xffu);
    out[offset + 1u] = static_cast<std::uint8_t>((v >> 8u) & 0xffu);
}

void write_u32_at(std::vector<std::uint8_t>& out, std::size_t offset, std::uint32_t v) {
    out[offset] = static_cast<std::uint8_t>(v & 0xffu);
    out[offset + 1u] = static_cast<std::uint8_t>((v >> 8u) & 0xffu);
    out[offset + 2u] = static_cast<std::uint8_t>((v >> 16u) & 0xffu);
    out[offset + 3u] = static_cast<std::uint8_t>((v >> 24u) & 0xffu);
}

std::size_t align4(std::size_t v) noexcept {
    return (v + 3u) & ~std::size_t(3u);
}

bool is_zero_hash(const Hash256& h) noexcept {
    for (const auto b : h) if (b != 0u) return false;
    return true;
}

std::vector<std::uint8_t> short_value(std::uint16_t v) {
    std::vector<std::uint8_t> out;
    append_u16(out, v);
    return out;
}

std::vector<std::uint8_t> long_value(std::uint32_t v) {
    std::vector<std::uint8_t> out;
    append_u32(out, v);
    return out;
}

std::vector<std::uint8_t> shorts_value(std::initializer_list<std::uint16_t> values) {
    std::vector<std::uint8_t> out;
    out.reserve(values.size() * 2u);
    for (const auto v : values) append_u16(out, v);
    return out;
}

std::vector<std::uint8_t> bytes_value(std::initializer_list<std::uint8_t> values) {
    return std::vector<std::uint8_t>(values);
}

std::vector<std::uint8_t> ascii_value(const std::string& value) {
    std::vector<std::uint8_t> out(value.begin(), value.end());
    out.push_back(0u);
    return out;
}

std::vector<std::uint8_t> rational_value(double value,
                                         std::uint32_t denominator = 1000000000u) {
    const double scaled = std::round(value * static_cast<double>(denominator));
    const auto numerator = static_cast<std::uint32_t>(std::clamp(
        scaled, 0.0, static_cast<double>(std::numeric_limits<std::uint32_t>::max())));
    std::vector<std::uint8_t> out;
    append_u32(out, numerator);
    append_u32(out, denominator);
    return out;
}

std::vector<std::uint8_t> srational_matrix(const std::array<double,9>& matrix) {
    constexpr std::int32_t denominator = 1000000;
    std::vector<std::uint8_t> out;
    out.reserve(9u * 8u);
    for (const double value : matrix) {
        const double scaled = std::round(value * static_cast<double>(denominator));
        const auto numerator = static_cast<std::int32_t>(std::clamp(
            scaled,
            static_cast<double>(std::numeric_limits<std::int32_t>::min()),
            static_cast<double>(std::numeric_limits<std::int32_t>::max())));
        append_u32(out, static_cast<std::uint32_t>(numerator));
        append_u32(out, static_cast<std::uint32_t>(denominator));
    }
    return out;
}

bool invert3x3(const std::array<float,9>& in, std::array<double,9>& out) noexcept {
    const double a = in[0], b = in[1], c = in[2];
    const double d = in[3], e = in[4], f = in[5];
    const double g = in[6], h = in[7], i = in[8];
    const double A = e*i - f*h;
    const double B = -(d*i - f*g);
    const double C = d*h - e*g;
    const double D = -(b*i - c*h);
    const double E = a*i - c*g;
    const double F = -(a*h - b*g);
    const double G = b*f - c*e;
    const double H = -(a*f - c*d);
    const double I = a*e - b*d;
    const double det = a*A + b*B + c*C;
    if (!std::isfinite(det) || std::abs(det) < 1e-12) return false;
    const double inv = 1.0 / det;
    out = {A*inv, D*inv, G*inv,
           B*inv, E*inv, H*inv,
           C*inv, F*inv, I*inv};
    for (const double v : out) if (!std::isfinite(v)) return false;
    return true;
}

std::array<std::uint8_t,4> cfa_pattern_bytes(CfaPattern cfa) noexcept {
    switch (cfa) {
        case CfaPattern::BGGR: return {2u,1u,1u,0u};
        case CfaPattern::RGGB: return {0u,1u,1u,2u};
        case CfaPattern::GRBG: return {1u,0u,2u,1u};
        case CfaPattern::GBRG: return {1u,2u,0u,1u};
    }
    return {2u,1u,1u,0u};
}

int measured_channel(CfaPattern cfa, int x, int y) noexcept {
    const int p = ((y & 1) << 1) | (x & 1);
    switch (cfa) {
        case CfaPattern::BGGR: { constexpr int m[4] = {2,1,1,0}; return m[p]; }
        case CfaPattern::RGGB: { constexpr int m[4] = {0,1,1,2}; return m[p]; }
        case CfaPattern::GRBG: { constexpr int m[4] = {1,0,2,1}; return m[p]; }
        case CfaPattern::GBRG: { constexpr int m[4] = {1,2,0,1}; return m[p]; }
    }
    return 1;
}

const char* authority_name_local(
    scientific_preview_binding_v0_1::ColorBindingAuthority authority) noexcept {
    using scientific_preview_binding_v0_1::ColorBindingAuthority;
    switch (authority) {
        case ColorBindingAuthority::Unverified: return "UNVERIFIED";
        case ColorBindingAuthority::PreviewSentinel: return "PREVIEW_SENTINEL";
        case ColorBindingAuthority::SourceMetadataBound: return "SOURCE_METADATA_BOUND";
        case ColorBindingAuthority::GatehouseCertifiedMetadata: return "GATEHOUSE_CERTIFIED_METADATA";
        case ColorBindingAuthority::IndependentCalibration: return "INDEPENDENT_CALIBRATION";
    }
    return "UNKNOWN";
}

bool valid_authority(const ProjectionMetadata& metadata,
                     const DngMetadata& sourceMetadata) noexcept {
    using scientific_preview_binding_v0_1::ColorBindingAuthority;
    const auto authority = metadata.color.authority;
    const bool authorized =
        authority == ColorBindingAuthority::SourceMetadataBound ||
        authority == ColorBindingAuthority::GatehouseCertifiedMetadata ||
        authority == ColorBindingAuthority::IndependentCalibration;
    return authorized && metadata.color.validated &&
        metadata.color.physicalFrameCount == 1u &&
        metadata.color.independentEvidenceCount == 1u &&
        metadata.sourceSeal.byteLength != 0u &&
        !metadata.sourceSeal.sourceEvidenceId.empty() &&
        metadata.color.sourceEvidenceId == metadata.sourceSeal.sourceEvidenceId &&
        sourceMetadata.sourceId == metadata.sourceSeal.sourceEvidenceId &&
        !metadata.color.bindingId.empty();
}

std::string private_payload(ProjectionRole role,
                            const ProjectionMetadata& metadata) {
    std::string text = "TruthRaw";
    text.push_back('\0');
    text += "format=TRUTHRAW_DNG_COMPATIBILITY_PROJECTION_V0_1\n";
    text += "role=";
    text += role_name(role);
    text += "\nsource_sha256=";
    text += scientific_master_digest::v0_1::to_hex(metadata.sourceSeal.sha256);
    text += "\nscientific_master_sha256=";
    text += scientific_master_digest::v0_1::to_hex(metadata.expectedScientificMasterHash);
    text += "\nsource_evidence_id=" + metadata.sourceSeal.sourceEvidenceId;
    text += "\ncolor_binding_id=" + metadata.color.bindingId;
    text += "\ncolor_authority=";
    text += authority_name_local(metadata.color.authority);
    text += "\nphysical_frame_count=1\nindependent_evidence_count=1\n";
    text += "projection_is_evidence=false\nfull_physical_color_promoted=false\n";
    text += "scientific_master_modified=false\n";
    return text;
}

Status build_header(ProjectionRole role,
                    const DngMetadata& sourceMetadata,
                    const ProjectionMetadata& metadata,
                    std::uint32_t channels,
                    std::uint32_t tileCount,
                    std::uint32_t tileBytes,
                    std::vector<std::uint8_t>& header,
                    std::uint32_t& pixelOffsetOut) {
    std::array<double,9> xyzToCamera{};
    if (!invert3x3(metadata.color.cameraToXyzD50, xyzToCamera)) {
        return Status::error(StatusCode::InvalidColorMatrix,
                             "cameraToXyzD50 is singular or non-finite");
    }

    std::vector<IfdEntry> entries;
    auto add = [&](std::uint16_t tag, std::uint16_t type,
                   std::uint32_t count, std::vector<std::uint8_t> value) {
        entries.push_back({tag, type, count, std::move(value), 0u});
    };

    add(kTagNewSubFileType, kTypeLong, 1u, long_value(0u));
    add(kTagImageWidth, kTypeLong, 1u,
        long_value(static_cast<std::uint32_t>(sourceMetadata.width)));
    add(kTagImageLength, kTypeLong, 1u,
        long_value(static_cast<std::uint32_t>(sourceMetadata.height)));
    if (channels == 3u) {
        add(kTagBitsPerSample, kTypeShort, 3u, shorts_value({32u,32u,32u}));
    } else {
        add(kTagBitsPerSample, kTypeShort, 1u, short_value(32u));
    }
    add(kTagCompression, kTypeShort, 1u, short_value(kCompressionUncompressed));
    add(kTagPhotometricInterpretation, kTypeShort, 1u,
        short_value(role == ProjectionRole::LinearScientificMaster ?
                    kPhotometricLinearRaw : kPhotometricCfa));
    const std::string description = role == ProjectionRole::LinearScientificMaster ?
        "TruthRaw Linear Scientific Master compatibility projection; derived, not evidence" :
        "TruthRaw measured-preserving CFA compatibility projection; derived, not original sensor code evidence";
    add(kTagImageDescription, kTypeAscii,
        static_cast<std::uint32_t>(description.size() + 1u), ascii_value(description));
    add(kTagOrientation, kTypeShort, 1u,
        short_value(static_cast<std::uint16_t>(sourceMetadata.orientation)));
    add(kTagSamplesPerPixel, kTypeShort, 1u,
        short_value(static_cast<std::uint16_t>(channels)));
    add(kTagPlanarConfiguration, kTypeShort, 1u, short_value(1u));
    add(kTagSoftware, kTypeAscii, 9u, ascii_value("TruthRaw"));
    add(kTagTileWidth, kTypeLong, 1u, long_value(kTileEdge));
    add(kTagTileLength, kTypeLong, 1u, long_value(kTileEdge));

    std::vector<std::uint8_t> tileOffsets(static_cast<std::size_t>(tileCount) * 4u, 0u);
    std::vector<std::uint8_t> tileByteCounts;
    tileByteCounts.reserve(static_cast<std::size_t>(tileCount) * 4u);
    for (std::uint32_t i = 0; i < tileCount; ++i) append_u32(tileByteCounts, tileBytes);
    add(kTagTileOffsets, kTypeLong, tileCount, std::move(tileOffsets));
    add(kTagTileByteCounts, kTypeLong, tileCount, std::move(tileByteCounts));

    if (channels == 3u) {
        add(kTagSampleFormat, kTypeShort, 3u,
            shorts_value({kSampleFormatIeeeFloat,kSampleFormatIeeeFloat,kSampleFormatIeeeFloat}));
    } else {
        add(kTagSampleFormat, kTypeShort, 1u, short_value(kSampleFormatIeeeFloat));
        const auto pattern = cfa_pattern_bytes(sourceMetadata.cfa);
        add(kTagCfaRepeatPatternDim, kTypeShort, 2u, shorts_value({2u,2u}));
        add(kTagCfaPattern, kTypeByte, 4u,
            bytes_value({pattern[0],pattern[1],pattern[2],pattern[3]}));
    }

    add(kTagDngVersion, kTypeByte, 4u, bytes_value({1u,4u,0u,0u}));
    add(kTagDngBackwardVersion, kTypeByte, 4u, bytes_value({1u,4u,0u,0u}));
    const std::string model = role == ProjectionRole::LinearScientificMaster ?
        "TruthRaw Source-Bound Linear Scientific Master" :
        "TruthRaw Source-Bound Measured-Preserving CFA";
    add(kTagUniqueCameraModel, kTypeAscii,
        static_cast<std::uint32_t>(model.size() + 1u), ascii_value(model));
    if (channels == 1u) {
        add(kTagCfaPlaneColor, kTypeByte, 3u, bytes_value({0u,1u,2u}));
        add(kTagCfaLayout, kTypeShort, 1u, short_value(kCfaLayoutRectangular));
    }
    add(kTagColorMatrix1, kTypeSRational, 9u, srational_matrix(xyzToCamera));
    std::vector<std::uint8_t> whiteXy = rational_value(metadata.asShotWhiteX);
    const auto whiteY = rational_value(metadata.asShotWhiteY);
    whiteXy.insert(whiteXy.end(), whiteY.begin(), whiteY.end());
    add(kTagAsShotWhiteXy, kTypeRational, 2u, std::move(whiteXy));
    const auto privateText = private_payload(role, metadata);
    std::vector<std::uint8_t> privateBytes(privateText.begin(), privateText.end());
    add(kTagDngPrivateData, kTypeByte,
        static_cast<std::uint32_t>(privateBytes.size()), std::move(privateBytes));
    add(kTagCalibrationIlluminant1, kTypeShort, 1u,
        short_value(kCalibrationIlluminantD50));

    std::sort(entries.begin(), entries.end(),
              [](const IfdEntry& a, const IfdEntry& b) { return a.tag < b.tag; });
    if (entries.size() > std::numeric_limits<std::uint16_t>::max()) {
        return Status::error(StatusCode::NumericOverflow, "too many TIFF IFD entries");
    }

    const std::size_t ifdOffset = 8u;
    const std::size_t ifdBytes = 2u + entries.size() * 12u + 4u;
    std::size_t cursor = align4(ifdOffset + ifdBytes);
    for (auto& entry : entries) {
        if (entry.value.size() > 4u) {
            if (cursor > std::numeric_limits<std::uint32_t>::max()) {
                return Status::error(StatusCode::NumericOverflow, "TIFF metadata offset overflow");
            }
            entry.externalOffset = static_cast<std::uint32_t>(cursor);
            if (entry.value.size() > std::numeric_limits<std::size_t>::max() - cursor) {
                return Status::error(StatusCode::NumericOverflow, "TIFF metadata size overflow");
            }
            cursor = align4(cursor + entry.value.size());
        }
    }
    const std::size_t pixelOffset = align4(cursor);
    const std::uint64_t totalBytes = static_cast<std::uint64_t>(pixelOffset) +
        static_cast<std::uint64_t>(tileBytes) * static_cast<std::uint64_t>(tileCount);
    if (pixelOffset > std::numeric_limits<std::uint32_t>::max() ||
        totalBytes > std::numeric_limits<std::uint32_t>::max()) {
        return Status::error(StatusCode::NumericOverflow,
                             "classic TIFF/DNG projection exceeds 32-bit offset space");
    }

    for (auto& entry : entries) {
        if (entry.tag != kTagTileOffsets) continue;
        for (std::uint32_t tile = 0; tile < tileCount; ++tile) {
            const auto offset = static_cast<std::uint32_t>(
                pixelOffset + static_cast<std::size_t>(tile) * tileBytes);
            write_u32_at(entry.value, static_cast<std::size_t>(tile) * 4u, offset);
        }
    }

    header.assign(pixelOffset, 0u);
    header[0] = 'I'; header[1] = 'I';
    write_u16_at(header, 2u, 42u);
    write_u32_at(header, 4u, static_cast<std::uint32_t>(ifdOffset));
    write_u16_at(header, ifdOffset, static_cast<std::uint16_t>(entries.size()));

    std::size_t entryOffset = ifdOffset + 2u;
    for (const auto& entry : entries) {
        write_u16_at(header, entryOffset, entry.tag);
        write_u16_at(header, entryOffset + 2u, entry.type);
        write_u32_at(header, entryOffset + 4u, entry.count);
        if (entry.value.size() <= 4u) {
            std::copy(entry.value.begin(), entry.value.end(),
                      header.begin() + static_cast<std::ptrdiff_t>(entryOffset + 8u));
        } else {
            write_u32_at(header, entryOffset + 8u, entry.externalOffset);
            std::copy(entry.value.begin(), entry.value.end(),
                      header.begin() + static_cast<std::ptrdiff_t>(entry.externalOffset));
        }
        entryOffset += 12u;
    }
    write_u32_at(header, entryOffset, 0u);
    pixelOffsetOut = static_cast<std::uint32_t>(pixelOffset);
    return Status::ok();
}

bool write_sink(ISequentialByteSink& sink, const void* data, std::size_t count,
                std::uint64_t& bytesWritten) {
    if (count == 0u) return true;
    if (!sink.write(data, count)) return false;
    bytesWritten += static_cast<std::uint64_t>(count);
    return true;
}

Status validate_inputs(streaming_v0_1::IRawTileSource& source,
                       IReconstructionBackend& reconstruction,
                       const ProjectionMetadata& metadata) {
    const auto& m = source.metadata();
    if (m.width <= 1 || m.height <= 1 || reconstruction.requiredHalo() < 0) {
        return Status::error(StatusCode::InvalidArgument,
                             "invalid source dimensions or reconstruction halo");
    }
    if (is_zero_hash(metadata.expectedScientificMasterHash)) {
        return Status::error(StatusCode::InvalidArgument,
                             "expected Scientific Master hash is zero");
    }
    if (!valid_authority(metadata, m)) {
        return Status::error(StatusCode::InvalidAuthority,
                             "projection source/color authority is not finalized source-bound lineage");
    }
    if (!(metadata.asShotWhiteX > 0.0) || !(metadata.asShotWhiteY > 0.0) ||
        !std::isfinite(metadata.asShotWhiteX) || !std::isfinite(metadata.asShotWhiteY) ||
        metadata.asShotWhiteX + metadata.asShotWhiteY >= 1.0) {
        return Status::error(StatusCode::InvalidWhitePoint,
                             "invalid resolved AsShotWhiteXY");
    }
    std::array<double,9> inverse{};
    if (!invert3x3(metadata.color.cameraToXyzD50, inverse)) {
        return Status::error(StatusCode::InvalidColorMatrix,
                             "invalid cameraToXyzD50 compatibility matrix");
    }
    return Status::ok();
}

Status write_projection(ProjectionRole role,
                        streaming_v0_1::IRawTileSource& source,
                        IReconstructionBackend& reconstruction,
                        const ProjectionMetadata& metadata,
                        ISequentialByteSink& sink,
                        Result& out) noexcept {
    out = {};
    out.role = role;
    out.colorAuthority = metadata.color.authority;
    out.physicalFrameCount = 1u;
    out.independentEvidenceCount = 1u;
    out.fullFrameMaterialized = false;
    out.projectionIsEvidence = false;
    out.colorAuthorityPromoted = false;

    const auto valid = validate_inputs(source, reconstruction, metadata);
    if (!valid) return valid;

    const auto& m = source.metadata();
    const std::uint32_t columns =
        (static_cast<std::uint32_t>(m.width) + kTileEdge - 1u) / kTileEdge;
    const std::uint32_t rows =
        (static_cast<std::uint32_t>(m.height) + kTileEdge - 1u) / kTileEdge;
    if (columns == 0u || rows == 0u ||
        columns > std::numeric_limits<std::uint32_t>::max() / rows) {
        return Status::error(StatusCode::NumericOverflow, "tile-count overflow");
    }
    const std::uint32_t tileCount = columns * rows;
    const std::uint32_t channels =
        role == ProjectionRole::LinearScientificMaster ? 3u : 1u;
    constexpr std::uint32_t sampleBytes = 4u;
    const std::uint64_t tileBytes64 = static_cast<std::uint64_t>(kTileEdge) * kTileEdge *
        channels * sampleBytes;
    if (tileBytes64 > std::numeric_limits<std::uint32_t>::max()) {
        return Status::error(StatusCode::NumericOverflow, "tile payload size overflow");
    }
    const auto tileBytes = static_cast<std::uint32_t>(tileBytes64);

    std::vector<std::uint8_t> header;
    std::uint32_t pixelOffset = 0u;
    const auto headerStatus = build_header(role, m, metadata, channels,
                                           tileCount, tileBytes, header, pixelOffset);
    if (!headerStatus) return headerStatus;
    (void)pixelOffset;

    if (!write_sink(sink, header.data(), header.size(), out.bytesWritten)) {
        return Status::error(StatusCode::SinkFailed, "failed writing DNG header");
    }

    scientific_master_digest::v0_1::ScientificMasterDigestAccumulator digest(
        static_cast<std::uint32_t>(m.width), static_cast<std::uint32_t>(m.height));
    if (!digest.valid()) {
        return Status::error(StatusCode::DigestFailed,
                             "Scientific Master digest initialization failed: " + digest.error());
    }

    const int halo = reconstruction.requiredHalo();
    Workspace workspace{};
    std::vector<std::uint8_t> payload(tileBytes, 0u);

    for (int y0 = 0; y0 < m.height; y0 += static_cast<int>(kTileEdge)) {
        const int y1 = std::min(m.height, y0 + static_cast<int>(kTileEdge));
        for (int x0 = 0; x0 < m.width; x0 += static_cast<int>(kTileEdge)) {
            const int x1 = std::min(m.width, x0 + static_cast<int>(kTileEdge));
            TileRect tile{};
            tile.x0 = x0; tile.y0 = y0; tile.x1 = x1; tile.y1 = y1;
            tile.hx0 = std::max(0, x0 - halo);
            tile.hy0 = std::max(0, y0 - halo);
            tile.hx1 = std::min(m.width, x1 + halo);
            tile.hy1 = std::min(m.height, y1 + halo);

            const auto fill = fill_stage2(source, tile, workspace);
            if (!fill) {
                return Status::error(StatusCode::SourceFailed,
                                     "Stage-2 source read failed: " + fill.message);
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
                m.cfa, workspace.cam.data());
            if (!reconstructed) {
                return Status::error(StatusCode::ReconstructionFailed,
                                     "camera-native reconstruction failed: " + reconstructed.message);
            }

            scientific_master_digest::v0_1::TileView digestTile{};
            digestTile.x = static_cast<std::uint32_t>(tile.x0);
            digestTile.y = static_cast<std::uint32_t>(tile.y0);
            digestTile.width = static_cast<std::uint32_t>(coreWidth);
            digestTile.height = static_cast<std::uint32_t>(coreHeight);
            digestTile.rgb = workspace.cam.data();
            digestTile.rowStrideSamples = static_cast<std::size_t>(coreWidth) * 3u;
            if (!digest.add_tile(digestTile)) {
                return Status::error(StatusCode::DigestFailed,
                                     "Scientific Master digest rejected export tile: " + digest.error());
            }

            std::fill(payload.begin(), payload.end(), 0u);
            for (int yy = 0; yy < coreHeight; ++yy) {
                for (int xx = 0; xx < coreWidth; ++xx) {
                    const std::size_t coreIndex =
                        static_cast<std::size_t>(yy) * static_cast<std::size_t>(coreWidth) +
                        static_cast<std::size_t>(xx);
                    if (role == ProjectionRole::LinearScientificMaster) {
                        for (std::uint32_t c = 0; c < 3u; ++c) {
                            const float value = workspace.cam[3u * coreIndex + c];
                            std::uint32_t bits = 0u;
                            std::memcpy(&bits, &value, sizeof(bits));
                            const std::size_t tileSample =
                                (static_cast<std::size_t>(yy) * kTileEdge +
                                 static_cast<std::size_t>(xx)) * 3u + c;
                            write_u32_at(payload, tileSample * 4u, bits);
                        }
                    } else {
                        const int globalX = tile.x0 + xx;
                        const int globalY = tile.y0 + yy;
                        const int channel = measured_channel(m.cfa, globalX, globalY);
                        const float value = workspace.cam[3u * coreIndex +
                            static_cast<std::size_t>(channel)];
                        std::uint32_t bits = 0u;
                        std::memcpy(&bits, &value, sizeof(bits));
                        const std::size_t tileSample =
                            static_cast<std::size_t>(yy) * kTileEdge +
                            static_cast<std::size_t>(xx);
                        write_u32_at(payload, tileSample * 4u, bits);
                    }
                }
            }

            if (!write_sink(sink, payload.data(), payload.size(), out.bytesWritten)) {
                return Status::error(StatusCode::SinkFailed, "failed writing DNG tile payload");
            }
            ++out.tilesWritten;
            out.workspacePeakBytes = std::max(
                out.workspacePeakBytes,
                vector_bytes(workspace) + payload.capacity() + header.capacity());
        }
    }

    if (!digest.finalize(out.observedScientificMasterHash)) {
        return Status::error(StatusCode::DigestFailed,
                             "Scientific Master digest finalization failed: " + digest.error());
    }
    out.scientificMasterMatched =
        out.observedScientificMasterHash == metadata.expectedScientificMasterHash;
    if (!out.scientificMasterMatched) {
        return Status::error(StatusCode::ScientificMasterMismatch,
                             "generated DNG pixels did not bind to expected finalized Scientific Master");
    }

    const std::size_t sourceResident = source.residentBytesUpperBound();
    const std::size_t sinkResident = sink.residentBytesUpperBound();
    if (sourceResident > std::numeric_limits<std::size_t>::max() - sinkResident ||
        sourceResident + sinkResident > std::numeric_limits<std::size_t>::max() -
                                      out.workspacePeakBytes) {
        return Status::error(StatusCode::NumericOverflow, "resident accounting overflow");
    }
    out.logicalResidentUpperBound = sourceResident + sinkResident + out.workspacePeakBytes;
    return Status::ok();
}

} // namespace

Status write_linear_scientific_master_dng(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const ProjectionMetadata& metadata,
    ISequentialByteSink& sink,
    Result& out) noexcept {
    return write_projection(ProjectionRole::LinearScientificMaster,
                            source, reconstruction, metadata, sink, out);
}

Status write_measured_preserving_cfa_dng(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const ProjectionMetadata& metadata,
    ISequentialByteSink& sink,
    Result& out) noexcept {
    return write_projection(ProjectionRole::MeasuredPreservingCfa,
                            source, reconstruction, metadata, sink, out);
}

const char* role_name(ProjectionRole role) noexcept {
    switch (role) {
        case ProjectionRole::LinearScientificMaster:
            return "LINEAR_SCIENTIFIC_MASTER_PROJECTION";
        case ProjectionRole::MeasuredPreservingCfa:
            return "MEASURED_PRESERVING_CFA_PROJECTION";
    }
    return "UNKNOWN_PROJECTION";
}

const char* status_name(StatusCode code) noexcept {
    switch (code) {
        case StatusCode::Ok: return "OK";
        case StatusCode::InvalidArgument: return "INVALID_ARGUMENT";
        case StatusCode::InvalidAuthority: return "INVALID_AUTHORITY";
        case StatusCode::InvalidColorMatrix: return "INVALID_COLOR_MATRIX";
        case StatusCode::InvalidWhitePoint: return "INVALID_WHITE_POINT";
        case StatusCode::NumericOverflow: return "NUMERIC_OVERFLOW";
        case StatusCode::SourceFailed: return "SOURCE_FAILED";
        case StatusCode::ReconstructionFailed: return "RECONSTRUCTION_FAILED";
        case StatusCode::DigestFailed: return "DIGEST_FAILED";
        case StatusCode::SinkFailed: return "SINK_FAILED";
        case StatusCode::ScientificMasterMismatch: return "SCIENTIFIC_MASTER_MISMATCH";
    }
    return "UNKNOWN";
}

} // namespace truthraw::dng_compatibility_projection::v0_1
