#include "truthraw_dng_projection_v0_1.h"

#include "full_frame_streaming_v0_1_internal.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace truthraw::dng_projection::v0_1 {
namespace {

using finalized_scientific_preview_release::v0_2::PreviewAuthority;
using scientific_master_digest::v0_1::ScientificMasterDigestAccumulator;
using scientific_master_digest::v0_1::TileView;
using scientific_preview_binding_v0_1::BindingStatusCode;
using streaming_v0_1::TileRect;
using streaming_v0_1::detail::Workspace;
using streaming_v0_1::detail::fill_stage2;
using streaming_v0_1::detail::vector_bytes;

constexpr int kCanonicalCore = scientific_master_digest::v0_1::kCanonicalCellEdge;
constexpr std::uint32_t kRowsPerStrip = kCanonicalCore;
constexpr std::uint32_t kPhotometricCfa = 32803u;
constexpr std::uint32_t kPhotometricLinearRaw = 34892u;
constexpr std::uint16_t kSampleFormatIeeeFloat = 3u;
constexpr std::uint16_t kCompressionNone = 1u;
constexpr std::uint16_t kPlanarChunky = 1u;
constexpr std::size_t kMaxRootEntries = 512u;
constexpr std::size_t kMaxCopiedTagBytes = 1024u * 1024u;

constexpr std::uint16_t kTypeByte = 1u;
constexpr std::uint16_t kTypeAscii = 2u;
constexpr std::uint16_t kTypeShort = 3u;
constexpr std::uint16_t kTypeLong = 4u;

constexpr std::uint16_t kTagNewSubfileType = 254u;
constexpr std::uint16_t kTagImageWidth = 256u;
constexpr std::uint16_t kTagImageLength = 257u;
constexpr std::uint16_t kTagBitsPerSample = 258u;
constexpr std::uint16_t kTagCompression = 259u;
constexpr std::uint16_t kTagPhotometric = 262u;
constexpr std::uint16_t kTagImageDescription = 270u;
constexpr std::uint16_t kTagStripOffsets = 273u;
constexpr std::uint16_t kTagOrientation = 274u;
constexpr std::uint16_t kTagSamplesPerPixel = 277u;
constexpr std::uint16_t kTagRowsPerStrip = 278u;
constexpr std::uint16_t kTagStripByteCounts = 279u;
constexpr std::uint16_t kTagPlanarConfiguration = 284u;
constexpr std::uint16_t kTagSoftware = 305u;
constexpr std::uint16_t kTagSampleFormat = 339u;
constexpr std::uint16_t kTagCfaRepeatPatternDim = 33421u;
constexpr std::uint16_t kTagCfaPattern = 33422u;
constexpr std::uint16_t kTagDngVersion = 50706u;
constexpr std::uint16_t kTagDngBackwardVersion = 50707u;
constexpr std::uint16_t kTagUniqueCameraModel = 50708u;
constexpr std::uint16_t kTagCfaPlaneColor = 50710u;
constexpr std::uint16_t kTagCfaLayout = 50711u;

constexpr std::array<std::uint16_t, 13> kCopiedColorTags{{
    50721u, // ColorMatrix1
    50722u, // ColorMatrix2
    50723u, // CameraCalibration1
    50724u, // CameraCalibration2
    50727u, // AnalogBalance
    50728u, // AsShotNeutral
    50729u, // AsShotWhiteXY
    50778u, // CalibrationIlluminant1
    50779u, // CalibrationIlluminant2
    50931u, // CameraCalibrationSignature
    50932u, // ProfileCalibrationSignature
    50964u, // ForwardMatrix1
    50965u, // ForwardMatrix2
}};

struct Entry final {
    std::uint16_t tag = 0u;
    std::uint16_t type = 0u;
    std::uint32_t count = 0u;
    std::vector<std::uint8_t> data;
};

std::size_t type_size(std::uint16_t type) noexcept {
    switch (type) {
        case 1: case 2: case 6: case 7: return 1u;
        case 3: case 8: return 2u;
        case 4: case 9: case 11: return 4u;
        case 5: case 10: case 12: return 8u;
        default: return 0u;
    }
}

bool checked_add(std::uint64_t a, std::uint64_t b, std::uint64_t& out) noexcept {
    if (a > std::numeric_limits<std::uint64_t>::max() - b) return false;
    out = a + b;
    return true;
}

bool checked_mul(std::uint64_t a, std::uint64_t b, std::uint64_t& out) noexcept {
    if (a != 0u && b > std::numeric_limits<std::uint64_t>::max() / a) return false;
    out = a * b;
    return true;
}

std::size_t align4(std::size_t value) noexcept {
    return (value + 3u) & ~std::size_t(3u);
}

void put16(std::vector<std::uint8_t>& out, std::size_t offset, std::uint16_t value) {
    if (out.size() < offset + 2u) out.resize(offset + 2u, 0u);
    out[offset] = static_cast<std::uint8_t>(value & 0xffu);
    out[offset + 1u] = static_cast<std::uint8_t>((value >> 8u) & 0xffu);
}

void put32(std::vector<std::uint8_t>& out, std::size_t offset, std::uint32_t value) {
    if (out.size() < offset + 4u) out.resize(offset + 4u, 0u);
    out[offset] = static_cast<std::uint8_t>(value & 0xffu);
    out[offset + 1u] = static_cast<std::uint8_t>((value >> 8u) & 0xffu);
    out[offset + 2u] = static_cast<std::uint8_t>((value >> 16u) & 0xffu);
    out[offset + 3u] = static_cast<std::uint8_t>((value >> 24u) & 0xffu);
}

std::uint16_t dec16le(const std::uint8_t* p) noexcept {
    return static_cast<std::uint16_t>(p[0]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(p[1]) << 8u);
}

std::uint32_t dec32le(const std::uint8_t* p) noexcept {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8u) |
           (static_cast<std::uint32_t>(p[2]) << 16u) |
           (static_cast<std::uint32_t>(p[3]) << 24u);
}

std::vector<std::uint8_t> short_data(std::initializer_list<std::uint16_t> values) {
    std::vector<std::uint8_t> out(values.size() * 2u, 0u);
    std::size_t index = 0u;
    for (const auto value : values) {
        put16(out, index * 2u, value);
        ++index;
    }
    return out;
}

std::vector<std::uint8_t> long_data(const std::vector<std::uint32_t>& values) {
    std::vector<std::uint8_t> out(values.size() * 4u, 0u);
    for (std::size_t i = 0u; i < values.size(); ++i) put32(out, i * 4u, values[i]);
    return out;
}

std::vector<std::uint8_t> ascii_data(const std::string& text) {
    std::vector<std::uint8_t> out(text.begin(), text.end());
    if (out.empty() || out.back() != 0u) out.push_back(0u);
    return out;
}

Entry make_short(std::uint16_t tag, std::uint16_t value) {
    return Entry{tag, kTypeShort, 1u, short_data({value})};
}

Entry make_long(std::uint16_t tag, std::uint32_t value) {
    std::vector<std::uint8_t> bytes(4u, 0u);
    put32(bytes, 0u, value);
    return Entry{tag, kTypeLong, 1u, std::move(bytes)};
}

Entry make_ascii(std::uint16_t tag, const std::string& value) {
    auto bytes = ascii_data(value);
    return Entry{tag, kTypeAscii, static_cast<std::uint32_t>(bytes.size()), std::move(bytes)};
}

Entry make_byte4(std::uint16_t tag, std::array<std::uint8_t, 4> value) {
    return Entry{tag, kTypeByte, 4u, {value.begin(), value.end()}};
}

bool copied_color_tag(std::uint16_t tag) noexcept {
    return std::find(kCopiedColorTags.begin(), kCopiedColorTags.end(), tag) !=
           kCopiedColorTags.end();
}

Status read_source_color_tags(
    tile_dng_v0_1::IRandomAccessByteSource& source,
    std::vector<Entry>& out) {
    out.clear();
    std::array<std::uint8_t, 8> header{};
    if (!source.readExact(0u, header.data(), header.size())) {
        return Status::error(StatusCode::SourceReadFailed, "cannot read source TIFF header");
    }
    if (!(header[0] == 'I' && header[1] == 'I')) {
        return Status::error(StatusCode::UnsupportedSourceTiff,
                             "DNG projection v0.1 requires little-endian classic TIFF source metadata");
    }
    const std::uint16_t magic = dec16le(header.data() + 2u);
    if (magic == 43u) {
        return Status::error(StatusCode::UnsupportedSourceTiff,
                             "BigTIFF source metadata is not supported by DNG projection v0.1");
    }
    if (magic != 42u) {
        return Status::error(StatusCode::UnsupportedSourceTiff,
                             "classic TIFF magic 42 required");
    }
    const std::uint32_t root = dec32le(header.data() + 4u);
    if (root == 0u || root > source.sizeBytes() || source.sizeBytes() - root < 2u) {
        return Status::error(StatusCode::UnsupportedSourceTiff, "source IFD0 offset is invalid");
    }

    std::array<std::uint8_t, 2> countBytes{};
    if (!source.readExact(root, countBytes.data(), countBytes.size())) {
        return Status::error(StatusCode::SourceReadFailed, "cannot read source IFD0 count");
    }
    const std::uint16_t count = dec16le(countBytes.data());
    if (count > kMaxRootEntries) {
        return Status::error(StatusCode::UnsupportedSourceTiff, "source IFD0 entry cap exceeded");
    }

    std::uint64_t ifdBytes = 0u;
    if (!checked_mul(count, 12u, ifdBytes)) {
        return Status::error(StatusCode::UnsupportedSourceTiff, "source IFD0 size overflow");
    }
    std::uint64_t ifdEnd = 0u;
    if (!checked_add(static_cast<std::uint64_t>(root) + 2u, ifdBytes + 4u, ifdEnd) ||
        ifdEnd > source.sizeBytes()) {
        return Status::error(StatusCode::UnsupportedSourceTiff, "source IFD0 is truncated");
    }

    bool hasColorMatrix1 = false;
    std::size_t copiedBytes = 0u;
    std::array<std::uint8_t, 12> rawEntry{};
    for (std::uint16_t i = 0u; i < count; ++i) {
        const std::uint64_t entryOffset = static_cast<std::uint64_t>(root) + 2u + 12ull * i;
        if (!source.readExact(entryOffset, rawEntry.data(), rawEntry.size())) {
            return Status::error(StatusCode::SourceReadFailed, "cannot read source IFD0 entry");
        }
        const std::uint16_t tag = dec16le(rawEntry.data());
        if (!copied_color_tag(tag)) continue;
        const std::uint16_t type = dec16le(rawEntry.data() + 2u);
        const std::uint32_t itemCount = dec32le(rawEntry.data() + 4u);
        const std::size_t itemSize = type_size(type);
        if (itemSize == 0u || itemCount == 0u) {
            return Status::error(StatusCode::UnsupportedSourceTiff,
                                 "required source color tag has unsupported TIFF type/cardinality");
        }
        std::uint64_t dataBytes64 = 0u;
        if (!checked_mul(itemCount, itemSize, dataBytes64) ||
            dataBytes64 > kMaxCopiedTagBytes ||
            copiedBytes > kMaxCopiedTagBytes - static_cast<std::size_t>(dataBytes64)) {
            return Status::error(StatusCode::UnsupportedSourceTiff,
                                 "source color metadata exceeds bounded copy limit");
        }
        const std::size_t dataBytes = static_cast<std::size_t>(dataBytes64);
        std::vector<std::uint8_t> data(dataBytes, 0u);
        if (dataBytes <= 4u) {
            std::copy_n(rawEntry.data() + 8u, dataBytes, data.data());
        } else {
            const std::uint32_t dataOffset = dec32le(rawEntry.data() + 8u);
            if (dataOffset > source.sizeBytes() || dataBytes > source.sizeBytes() - dataOffset) {
                return Status::error(StatusCode::UnsupportedSourceTiff,
                                     "source color tag payload is out of range");
            }
            if (!source.readExact(dataOffset, data.data(), data.size())) {
                return Status::error(StatusCode::SourceReadFailed,
                                     "cannot read source color tag payload");
            }
        }
        copiedBytes += dataBytes;
        if (tag == 50721u) hasColorMatrix1 = true;
        out.push_back(Entry{tag, type, itemCount, std::move(data)});
    }
    if (!hasColorMatrix1) {
        return Status::error(StatusCode::MissingRequiredColorMetadata,
                             "ColorMatrix1 is required for non-monochrome DNG projection");
    }
    return Status::ok();
}

std::array<std::uint8_t, 4> cfa_pattern(CfaPattern cfa) noexcept {
    switch (cfa) {
        case CfaPattern::BGGR: return {2u, 1u, 1u, 0u};
        case CfaPattern::RGGB: return {0u, 1u, 1u, 2u};
        case CfaPattern::GRBG: return {1u, 0u, 2u, 1u};
        case CfaPattern::GBRG: return {1u, 2u, 0u, 1u};
    }
    return {2u, 1u, 1u, 0u};
}

std::size_t header_size_for(const std::vector<Entry>& entries) {
    std::size_t cursor = 8u + 2u + entries.size() * 12u + 4u;
    for (const auto& entry : entries) {
        if (entry.data.size() <= 4u) continue;
        cursor = align4(cursor);
        cursor += entry.data.size();
    }
    return align4(cursor);
}

Status build_header(const std::vector<Entry>& inputEntries,
                    std::vector<std::uint8_t>& headerOut) {
    auto entries = inputEntries;
    std::sort(entries.begin(), entries.end(),
              [](const Entry& a, const Entry& b) { return a.tag < b.tag; });
    for (std::size_t i = 1u; i < entries.size(); ++i) {
        if (entries[i - 1u].tag == entries[i].tag) {
            return Status::error(StatusCode::InvalidArgument, "duplicate output TIFF tag");
        }
    }
    if (entries.size() > std::numeric_limits<std::uint16_t>::max()) {
        return Status::error(StatusCode::TiffOverflow, "too many output IFD entries");
    }

    const std::size_t totalHeader = header_size_for(entries);
    if (totalHeader > std::numeric_limits<std::uint32_t>::max()) {
        return Status::error(StatusCode::TiffOverflow, "output TIFF header exceeds classic offset range");
    }
    headerOut.assign(totalHeader, 0u);
    headerOut[0] = 'I';
    headerOut[1] = 'I';
    put16(headerOut, 2u, 42u);
    put32(headerOut, 4u, 8u);
    put16(headerOut, 8u, static_cast<std::uint16_t>(entries.size()));

    std::size_t external = 8u + 2u + entries.size() * 12u + 4u;
    for (std::size_t i = 0u; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        const std::size_t pos = 10u + i * 12u;
        put16(headerOut, pos, entry.tag);
        put16(headerOut, pos + 2u, entry.type);
        put32(headerOut, pos + 4u, entry.count);
        if (entry.data.size() <= 4u) {
            std::copy(entry.data.begin(), entry.data.end(),
                      headerOut.begin() + static_cast<std::ptrdiff_t>(pos + 8u));
        } else {
            external = align4(external);
            if (external > std::numeric_limits<std::uint32_t>::max()) {
                return Status::error(StatusCode::TiffOverflow, "output tag offset exceeds classic TIFF range");
            }
            put32(headerOut, pos + 8u, static_cast<std::uint32_t>(external));
            if (external + entry.data.size() > headerOut.size()) {
                return Status::error(StatusCode::TiffOverflow, "internal TIFF header layout overflow");
            }
            std::copy(entry.data.begin(), entry.data.end(),
                      headerOut.begin() + static_cast<std::ptrdiff_t>(external));
            external += entry.data.size();
        }
    }
    // next IFD offset remains zero in the pre-zeroed buffer.
    return Status::ok();
}

bool same_matrix(const std::array<float, 9>& a, const std::array<float, 9>& b) noexcept {
    for (std::size_t i = 0u; i < a.size(); ++i) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

bool same_hash(const scientific_master_digest::v0_1::Sha256& a,
               const scientific_master_digest::v0_1::Sha256& b) noexcept {
    return a == b;
}

Status map_reverify(const scientific_preview_binding_v0_1::BindingStatus& status) {
    if (status.code == BindingStatusCode::SourceReadFailed) {
        return Status::error(StatusCode::SourceReadFailed, status.message);
    }
    return Status::error(StatusCode::SourceSealMismatch, status.message);
}

Status validate_admission(
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    const finalized_scientific_preview_release::v0_2::ReleaseResult& release,
    const streaming_v0_1::IRawTileSource& source) {
    if (!prepared.mainHouseComputeAllowed || !prepared.sourceBoundAppearanceReleaseAllowed ||
        prepared.physicalFrameCount != 1u || prepared.independentEvidenceCount != 1u) {
        return Status::error(StatusCode::AdmissionRejected,
                             "prepared source is not a single-frame admitted TruthRaw source");
    }
    if (release.authority == PreviewAuthority::None ||
        release.streaming.provenance.physicalFrameCount != 1u ||
        release.streaming.provenance.independentEvidenceCount != 1u ||
        release.streaming.provenance.scientificMasterModifiedByAppearance ||
        release.streaming.provenance.counterfactualObservationCreated ||
        release.scientificIdentity.physicalFrameCount != 1u ||
        release.scientificIdentity.independentEvidenceCount != 1u) {
        return Status::error(StatusCode::AdmissionRejected,
                             "finalized release/provenance does not authorize downstream projection");
    }
    if (release.canonicalPhase2.backplane.physicalFrameCount != 1u ||
        release.canonicalPhase2.backplane.independentEvidenceCount != 1u ||
        release.canonicalPhase2.backplane.sourceEvidenceHash != prepared.source.sha256 ||
        release.canonicalPhase2.backplane.scientificMasterHash !=
            release.scientificIdentity.scientificMasterHash) {
        return Status::error(StatusCode::AdmissionRejected,
                             "Technical Backplane identity does not match finalized projection source/master");
    }
    if (source.metadata().sourceId != prepared.source.sourceEvidenceId ||
        prepared.color.sourceEvidenceId != prepared.source.sourceEvidenceId ||
        !same_matrix(source.metadata().cameraToXyzD50, prepared.color.cameraToXyzD50)) {
        return Status::error(StatusCode::SourceIdentityMismatch,
                             "streaming source/color identity differs from prepared finalized source");
    }
    return Status::ok();
}

Status make_output_entries(
    ProjectionKind kind,
    const DngMetadata& metadata,
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    const finalized_scientific_preview_release::v0_2::ReleaseResult& release,
    std::vector<Entry> colorEntries,
    std::vector<Entry>& entriesOut,
    std::vector<std::uint32_t>& stripByteCountsOut) {
    if (metadata.width <= 0 || metadata.height <= 0) {
        return Status::error(StatusCode::InvalidArgument, "invalid source dimensions for DNG projection");
    }
    const std::uint32_t samplesPerPixel =
        kind == ProjectionKind::LinearRawCameraRgbFloat32 ? 3u : 1u;
    const std::uint32_t stripCount =
        (static_cast<std::uint32_t>(metadata.height) + kRowsPerStrip - 1u) / kRowsPerStrip;
    stripByteCountsOut.assign(stripCount, 0u);

    std::uint64_t totalPixelBytes = 0u;
    for (std::uint32_t strip = 0u; strip < stripCount; ++strip) {
        const std::uint32_t y0 = strip * kRowsPerStrip;
        const std::uint32_t rows = std::min<std::uint32_t>(
            kRowsPerStrip, static_cast<std::uint32_t>(metadata.height) - y0);
        std::uint64_t bytes = 0u;
        if (!checked_mul(rows, static_cast<std::uint32_t>(metadata.width), bytes) ||
            !checked_mul(bytes, samplesPerPixel, bytes) ||
            !checked_mul(bytes, sizeof(float), bytes) ||
            bytes > std::numeric_limits<std::uint32_t>::max()) {
            return Status::error(StatusCode::TiffOverflow, "output strip byte count overflow");
        }
        stripByteCountsOut[strip] = static_cast<std::uint32_t>(bytes);
        if (!checked_add(totalPixelBytes, bytes, totalPixelBytes)) {
            return Status::error(StatusCode::TiffOverflow, "output pixel byte count overflow");
        }
    }

    entriesOut.clear();
    entriesOut.reserve(24u + colorEntries.size());
    entriesOut.push_back(make_long(kTagNewSubfileType, 0u));
    entriesOut.push_back(make_long(kTagImageWidth, static_cast<std::uint32_t>(metadata.width)));
    entriesOut.push_back(make_long(kTagImageLength, static_cast<std::uint32_t>(metadata.height)));
    if (samplesPerPixel == 1u) {
        entriesOut.push_back(make_short(kTagBitsPerSample, 32u));
    } else {
        entriesOut.push_back(Entry{kTagBitsPerSample, kTypeShort, 3u, short_data({32u,32u,32u})});
    }
    entriesOut.push_back(make_short(kTagCompression, kCompressionNone));
    entriesOut.push_back(make_short(
        kTagPhotometric,
        static_cast<std::uint16_t>(kind == ProjectionKind::Stage2CfaFloat32
            ? kPhotometricCfa : kPhotometricLinearRaw)));

    const std::string masterHex = scientific_master_digest::v0_1::to_hex(
        release.scientificIdentity.scientificMasterHash);
    const std::string role = kind == ProjectionKind::Stage2CfaFloat32
        ? "TRUTHRAW_STAGE2_CFA_PROJECTION;DERIVED_MEASUREMENT;NOT_DIRECT_SENSOR_EVIDENCE"
        : "TRUTHRAW_LINEAR_RAW_PROJECTION;RECONSTRUCTED_COMPATIBILITY_PROJECTION;NOT_SENSOR_EVIDENCE";
    entriesOut.push_back(make_ascii(
        kTagImageDescription,
        role + ";SOURCE=" + prepared.source.sourceEvidenceId + ";SCIENTIFIC_MASTER_SHA256=" + masterHex));

    entriesOut.push_back(Entry{kTagStripOffsets, kTypeLong, stripCount,
                               std::vector<std::uint8_t>(static_cast<std::size_t>(stripCount) * 4u, 0u)});
    entriesOut.push_back(make_short(kTagOrientation, static_cast<std::uint16_t>(metadata.orientation)));
    entriesOut.push_back(make_short(kTagSamplesPerPixel, static_cast<std::uint16_t>(samplesPerPixel)));
    entriesOut.push_back(make_long(kTagRowsPerStrip, kRowsPerStrip));
    entriesOut.push_back(Entry{kTagStripByteCounts, kTypeLong, stripCount,
                               long_data(stripByteCountsOut)});
    entriesOut.push_back(make_short(kTagPlanarConfiguration, kPlanarChunky));
    entriesOut.push_back(make_ascii(kTagSoftware, "TruthRaw dng-projection-v0.1"));
    if (samplesPerPixel == 1u) {
        entriesOut.push_back(make_short(kTagSampleFormat, kSampleFormatIeeeFloat));
    } else {
        entriesOut.push_back(Entry{kTagSampleFormat, kTypeShort, 3u,
                                   short_data({kSampleFormatIeeeFloat,
                                               kSampleFormatIeeeFloat,
                                               kSampleFormatIeeeFloat})});
    }
    entriesOut.push_back(make_byte4(kTagDngVersion, {1u,4u,0u,0u}));
    entriesOut.push_back(make_byte4(kTagDngBackwardVersion, {1u,4u,0u,0u}));
    entriesOut.push_back(make_ascii(
        kTagUniqueCameraModel,
        kind == ProjectionKind::Stage2CfaFloat32
            ? "TruthRaw Stage2 CFA Projection v0.1"
            : "TruthRaw LinearRaw Projection v0.1"));

    if (kind == ProjectionKind::Stage2CfaFloat32) {
        entriesOut.push_back(Entry{kTagCfaRepeatPatternDim, kTypeShort, 2u, short_data({2u,2u})});
        entriesOut.push_back(make_byte4(kTagCfaPattern, cfa_pattern(metadata.cfa)));
        entriesOut.push_back(Entry{kTagCfaPlaneColor, kTypeByte, 3u, {0u,1u,2u}});
        entriesOut.push_back(make_short(kTagCfaLayout, 1u));
    }

    for (auto& entry : colorEntries) entriesOut.push_back(std::move(entry));

    const std::size_t provisionalHeader = header_size_for(entriesOut);
    if (provisionalHeader > std::numeric_limits<std::uint32_t>::max()) {
        return Status::error(StatusCode::TiffOverflow, "output header exceeds classic TIFF offset range");
    }
    std::vector<std::uint32_t> stripOffsets(stripCount, 0u);
    std::uint64_t offset = provisionalHeader;
    for (std::uint32_t strip = 0u; strip < stripCount; ++strip) {
        if (offset > std::numeric_limits<std::uint32_t>::max()) {
            return Status::error(StatusCode::TiffOverflow, "strip offset exceeds classic TIFF range");
        }
        stripOffsets[strip] = static_cast<std::uint32_t>(offset);
        if (!checked_add(offset, stripByteCountsOut[strip], offset) ||
            offset > std::numeric_limits<std::uint32_t>::max()) {
            return Status::error(StatusCode::TiffOverflow,
                                 "DNG projection exceeds classic TIFF 4 GiB offset range");
        }
    }
    (void)totalPixelBytes;
    for (auto& entry : entriesOut) {
        if (entry.tag == kTagStripOffsets) {
            entry.data = long_data(stripOffsets);
            break;
        }
    }
    if (header_size_for(entriesOut) != provisionalHeader) {
        return Status::error(StatusCode::TiffOverflow, "strip offset layout changed unexpectedly");
    }
    return Status::ok();
}

Status write_sink(ISequentialByteSink& sink,
                  const void* data,
                  std::size_t bytes,
                  Result& result) {
    if (bytes == 0u) return Status::ok();
    if (!sink.write(data, bytes)) {
        return Status::error(StatusCode::OutputFailed, "output sink write failed");
    }
    if (result.bytesWritten > std::numeric_limits<std::uint64_t>::max() - bytes) {
        return Status::error(StatusCode::TiffOverflow, "output byte counter overflow");
    }
    result.bytesWritten += bytes;
    return Status::ok();
}

Status account_budget(const Options& options,
                      streaming_v0_1::IRawTileSource& source,
                      const Workspace& workspace,
                      const ScientificMasterDigestAccumulator& digest,
                      const std::vector<float>& strip,
                      const std::vector<std::uint8_t>& rowBytes,
                      const std::vector<std::uint8_t>& header,
                      Result& result) {
    const std::size_t workspaceBytes = vector_bytes(workspace);
    result.logicalWorkspacePeakBytes = std::max(
        result.logicalWorkspacePeakBytes,
        workspaceBytes + strip.capacity() * sizeof(float) + rowBytes.capacity() + header.capacity());
    const auto digestMetrics = digest.metrics();
    std::uint64_t resident = source.residentBytesUpperBound();
    if (!checked_add(resident, result.logicalWorkspacePeakBytes, resident) ||
        !checked_add(resident, digestMetrics.residentBytesUpperBound, resident) ||
        resident > std::numeric_limits<std::size_t>::max()) {
        return Status::error(StatusCode::BudgetExceeded, "projection resident accounting overflow");
    }
    result.logicalResidentUpperBound = std::max(
        result.logicalResidentUpperBound, static_cast<std::size_t>(resident));
    if (options.memoryBudgetBytes != 0u &&
        result.logicalResidentUpperBound > options.memoryBudgetBytes) {
        return Status::error(StatusCode::BudgetExceeded,
                             "DNG projection exceeds caller logical resident budget");
    }
    return Status::ok();
}

void encode_float_row_le(const float* samples,
                         std::size_t count,
                         std::vector<std::uint8_t>& out) {
    out.resize(count * sizeof(float));
    for (std::size_t i = 0u; i < count; ++i) {
        std::uint32_t bits = 0u;
        static_assert(sizeof(bits) == sizeof(samples[i]), "float32 projection requires 32-bit float");
        std::memcpy(&bits, samples + i, sizeof(bits));
        put32(out, i * 4u, bits);
    }
}

} // namespace

Status export_projection(
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    const finalized_scientific_preview_release::v0_2::ReleaseResult& release,
    tile_dng_v0_1::IRandomAccessByteSource& sourceBytes,
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    ISequentialByteSink& output,
    const Options& options,
    Result& out) noexcept {
    out = {};
    out.kind = options.kind;
    out.physicalFrameCount = 1u;
    out.independentEvidenceCount = 1u;
    out.createsEvidence = false;
    out.fullFrameMaterialized = false;

    if (options.kind != ProjectionKind::Stage2CfaFloat32 &&
        options.kind != ProjectionKind::LinearRawCameraRgbFloat32) {
        return Status::error(StatusCode::InvalidArgument, "unknown DNG projection kind");
    }
    const auto admission = validate_admission(prepared, release, source);
    if (!admission) return admission;

    const auto preVerified = scientific_preview_binding_v0_1::reverify_source_sha256(
        sourceBytes, prepared.source);
    if (!preVerified) return map_reverify(preVerified);
    out.sourceVerifiedBefore = true;

    std::vector<Entry> colorEntries;
    const auto colorStatus = read_source_color_tags(sourceBytes, colorEntries);
    if (!colorStatus) return colorStatus;

    std::vector<Entry> entries;
    std::vector<std::uint32_t> stripByteCounts;
    const auto entryStatus = make_output_entries(
        options.kind, source.metadata(), prepared, release,
        std::move(colorEntries), entries, stripByteCounts);
    if (!entryStatus) return entryStatus;

    std::vector<std::uint8_t> header;
    const auto headerStatus = build_header(entries, header);
    if (!headerStatus) return headerStatus;
    auto status = write_sink(output, header.data(), header.size(), out);
    if (!status) return status;

    const auto& metadata = source.metadata();
    const int halo = reconstruction.requiredHalo();
    if (halo < 0) {
        return Status::error(StatusCode::InvalidArgument,
                             "reconstruction backend returned negative halo");
    }
    ScientificMasterDigestAccumulator digest(
        static_cast<std::uint32_t>(metadata.width),
        static_cast<std::uint32_t>(metadata.height));
    if (!digest.valid()) {
        return Status::error(StatusCode::DigestFailed,
                             "projection Scientific Master digest initialization failed: " + digest.error());
    }

    Workspace workspace{};
    std::vector<float> strip;
    std::vector<std::uint8_t> rowBytes;
    const int samplesPerPixel =
        options.kind == ProjectionKind::LinearRawCameraRgbFloat32 ? 3 : 1;

    for (int y0 = 0; y0 < metadata.height; y0 += static_cast<int>(kRowsPerStrip)) {
        const int y1 = std::min(metadata.height, y0 + static_cast<int>(kRowsPerStrip));
        const int rows = y1 - y0;
        const std::size_t stripSamples =
            static_cast<std::size_t>(rows) * static_cast<std::size_t>(metadata.width) *
            static_cast<std::size_t>(samplesPerPixel);
        strip.assign(stripSamples, 0.0f);

        for (int x0 = 0; x0 < metadata.width; x0 += kCanonicalCore) {
            const int x1 = std::min(metadata.width, x0 + kCanonicalCore);
            TileRect tile{};
            tile.x0 = x0;
            tile.y0 = y0;
            tile.x1 = x1;
            tile.y1 = y1;
            tile.hx0 = std::max(0, x0 - halo);
            tile.hy0 = std::max(0, y0 - halo);
            tile.hx1 = std::min(metadata.width, x1 + halo);
            tile.hy1 = std::min(metadata.height, y1 + halo);

            const auto filled = fill_stage2(source, tile, workspace);
            if (!filled) {
                return Status::error(StatusCode::SourceReadFailed,
                                     "projection Stage-2 source read failed: " + filled.message);
            }
            const int tileWidth = tile.hx1 - tile.hx0;
            const int tileHeight = tile.hy1 - tile.hy0;
            const int coreWidth = tile.x1 - tile.x0;
            const int coreHeight = tile.y1 - tile.y0;
            const std::size_t corePixels =
                static_cast<std::size_t>(coreWidth) * static_cast<std::size_t>(coreHeight);
            workspace.cam.resize(corePixels * 3u);
            const auto reconstructed = reconstruction.reconstructTile(
                workspace.stage2.data(), tileWidth, tileHeight,
                tile.hx0, tile.hy0,
                tile.x0, tile.y0, coreWidth, coreHeight,
                metadata.cfa, workspace.cam.data());
            if (!reconstructed) {
                return Status::error(StatusCode::ReconstructionFailed,
                                     "camera-native projection reconstruction failed: " +
                                         reconstructed.message);
            }

            TileView digestTile{};
            digestTile.x = static_cast<std::uint32_t>(tile.x0);
            digestTile.y = static_cast<std::uint32_t>(tile.y0);
            digestTile.width = static_cast<std::uint32_t>(coreWidth);
            digestTile.height = static_cast<std::uint32_t>(coreHeight);
            digestTile.rgb = workspace.cam.data();
            digestTile.rowStrideSamples = static_cast<std::size_t>(coreWidth) * 3u;
            if (!digest.add_tile(digestTile)) {
                return Status::error(StatusCode::DigestFailed,
                                     "projection Scientific Master digest rejected tile: " + digest.error());
            }

            for (int cy = 0; cy < coreHeight; ++cy) {
                const int gy = tile.y0 + cy;
                const std::size_t stripRow = static_cast<std::size_t>(gy - y0);
                for (int cx = 0; cx < coreWidth; ++cx) {
                    const int gx = tile.x0 + cx;
                    const std::size_t dstPixel =
                        stripRow * static_cast<std::size_t>(metadata.width) +
                        static_cast<std::size_t>(gx);
                    if (options.kind == ProjectionKind::Stage2CfaFloat32) {
                        const int lx = gx - tile.hx0;
                        const int ly = gy - tile.hy0;
                        const std::size_t src =
                            static_cast<std::size_t>(ly) * static_cast<std::size_t>(tileWidth) +
                            static_cast<std::size_t>(lx);
                        strip[dstPixel] = workspace.stage2[src];
                    } else {
                        const std::size_t srcPixel =
                            static_cast<std::size_t>(cy) * static_cast<std::size_t>(coreWidth) +
                            static_cast<std::size_t>(cx);
                        strip[3u * dstPixel] = workspace.cam[3u * srcPixel];
                        strip[3u * dstPixel + 1u] = workspace.cam[3u * srcPixel + 1u];
                        strip[3u * dstPixel + 2u] = workspace.cam[3u * srcPixel + 2u];
                    }
                }
            }
            ++out.canonicalTilesProcessed;
            status = account_budget(options, source, workspace, digest, strip, rowBytes, header, out);
            if (!status) return status;
        }

        const std::size_t rowSamples =
            static_cast<std::size_t>(metadata.width) * static_cast<std::size_t>(samplesPerPixel);
        for (int row = 0; row < rows; ++row) {
            encode_float_row_le(strip.data() + static_cast<std::size_t>(row) * rowSamples,
                                rowSamples, rowBytes);
            status = write_sink(output, rowBytes.data(), rowBytes.size(), out);
            if (!status) return status;
        }
        ++out.stripsWritten;
    }

    if (!digest.finalize(out.recomputedScientificMasterHash)) {
        return Status::error(StatusCode::DigestFailed,
                             "projection Scientific Master digest finalization failed: " + digest.error());
    }

    const auto postVerified = scientific_preview_binding_v0_1::reverify_source_sha256(
        sourceBytes, prepared.source);
    if (!postVerified) return map_reverify(postVerified);
    out.sourceVerifiedAfter = true;

    if (!same_hash(out.recomputedScientificMasterHash,
                   release.scientificIdentity.scientificMasterHash)) {
        return Status::error(StatusCode::DigestMismatch,
                             "exported projection reconstruction does not match finalized Scientific Master digest");
    }
    out.scientificMasterHashMatched = true;
    return Status::ok();
}

const char* projection_name(ProjectionKind kind) noexcept {
    switch (kind) {
        case ProjectionKind::Stage2CfaFloat32: return "TRUTHRAW_STAGE2_CFA_PROJECTION";
        case ProjectionKind::LinearRawCameraRgbFloat32: return "TRUTHRAW_LINEAR_RAW_PROJECTION";
    }
    return "UNKNOWN";
}

const char* status_name(StatusCode code) noexcept {
    switch (code) {
        case StatusCode::Ok: return "OK";
        case StatusCode::InvalidArgument: return "INVALID_ARGUMENT";
        case StatusCode::AdmissionRejected: return "ADMISSION_REJECTED";
        case StatusCode::SourceIdentityMismatch: return "SOURCE_IDENTITY_MISMATCH";
        case StatusCode::SourceSealMismatch: return "SOURCE_SEAL_MISMATCH";
        case StatusCode::SourceReadFailed: return "SOURCE_READ_FAILED";
        case StatusCode::UnsupportedSourceTiff: return "UNSUPPORTED_SOURCE_TIFF";
        case StatusCode::MissingRequiredColorMetadata: return "MISSING_REQUIRED_COLOR_METADATA";
        case StatusCode::ReconstructionFailed: return "RECONSTRUCTION_FAILED";
        case StatusCode::DigestFailed: return "DIGEST_FAILED";
        case StatusCode::DigestMismatch: return "DIGEST_MISMATCH";
        case StatusCode::BudgetExceeded: return "BUDGET_EXCEEDED";
        case StatusCode::TiffOverflow: return "TIFF_OVERFLOW";
        case StatusCode::OutputFailed: return "OUTPUT_FAILED";
    }
    return "UNKNOWN";
}

}  // namespace truthraw::dng_projection::v0_1
