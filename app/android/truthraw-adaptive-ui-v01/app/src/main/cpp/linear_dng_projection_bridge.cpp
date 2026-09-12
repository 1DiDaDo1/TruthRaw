#include <jni.h>

#include "bounded_srgb_preview_sink_v0_1.h"
#include "dng_color_binding_producer_v0_2.h"
#include "finalized_scientific_preview_release_v0_2.h"
#include "full_frame_streaming_v0_1.h"
#include "full_frame_streaming_v0_1_internal.h"
#include "scientific_master_digest_v0_1.h"
#include "scientific_master_streaming_binding_v0_2.h"
#include "scientific_preview_source_binding_v0_1.h"
#include "scientific_preview_source_binding_v0_2.h"
#include "technical_backplane_v0_1.h"
#include "tile_native_dng_source_v0_1.h"
#include "truthraw/core.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <unistd.h>

// This product includes DNG technology under license by Adobe.
//
// TruthRaw role boundary:
// - input Direct-CFA remains measured evidence;
// - the Scientific Master remains camera-native reconstructed float RGB;
// - this file is a bounded 16-bit LinearRaw compatibility projection only;
// - it never becomes a new evidence root and never upgrades color authority.

namespace {

using truthraw::NeutralReferenceAppearance;
using truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction;
using truthraw::dng_color_binding_producer_v0_2::ProducerResult;
using truthraw::finalized_scientific_preview_release::v0_2::PreviewAuthority;
using truthraw::finalized_scientific_preview_release::v0_2::ReleaseResult;
using truthraw::preview_surface_v0_1::BoundedSrgbPreviewSink;
using truthraw::scientific_preview_binding_v0_1::SourceSeal;
using truthraw::scientific_preview_binding_v0_2::PreparedScientificPreviewSource;
using truthraw::streaming_v0_1::TileRect;
using truthraw::streaming_v0_1::detail::Workspace;
using truthraw::streaming_v0_1::detail::fill_stage2;
using truthraw::tile_dng_v0_1::IRandomAccessByteSource;
using truthraw::tile_dng_v0_1::PosixFdByteSource;
using truthraw::tile_dng_v0_1::TileNativeDngSource;

constexpr jlong kPacketMagic = 0x54524431LL; // TRD1
constexpr int kCanonicalCore = truthraw::scientific_master_digest::v0_1::kCanonicalCellEdge;
constexpr int kGatePreviewEdge = 64;
constexpr std::uint16_t kPhotometricLinearRaw = 34892;

constexpr std::uint16_t kTypeByte = 1;
constexpr std::uint16_t kTypeAscii = 2;
constexpr std::uint16_t kTypeShort = 3;
constexpr std::uint16_t kTypeLong = 4;
constexpr std::uint16_t kTypeRational = 5;
constexpr std::uint16_t kTypeSByte = 6;
constexpr std::uint16_t kTypeUndefined = 7;
constexpr std::uint16_t kTypeSShort = 8;
constexpr std::uint16_t kTypeSLong = 9;
constexpr std::uint16_t kTypeSRational = 10;
constexpr std::uint16_t kTypeFloat = 11;
constexpr std::uint16_t kTypeDouble = 12;

constexpr std::uint16_t kTagImageWidth = 256;
constexpr std::uint16_t kTagImageLength = 257;
constexpr std::uint16_t kTagBitsPerSample = 258;
constexpr std::uint16_t kTagCompression = 259;
constexpr std::uint16_t kTagPhotometric = 262;
constexpr std::uint16_t kTagImageDescription = 270;
constexpr std::uint16_t kTagMake = 271;
constexpr std::uint16_t kTagModel = 272;
constexpr std::uint16_t kTagStripOffsets = 273;
constexpr std::uint16_t kTagOrientation = 274;
constexpr std::uint16_t kTagSamplesPerPixel = 277;
constexpr std::uint16_t kTagRowsPerStrip = 278;
constexpr std::uint16_t kTagStripByteCounts = 279;
constexpr std::uint16_t kTagPlanarConfiguration = 284;
constexpr std::uint16_t kTagSoftware = 305;
constexpr std::uint16_t kTagSampleFormat = 339;
constexpr std::uint16_t kTagDngVersion = 50706;
constexpr std::uint16_t kTagDngBackwardVersion = 50707;
constexpr std::uint16_t kTagUniqueCameraModel = 50708;
constexpr std::uint16_t kTagBlackLevelRepeatDim = 50713;
constexpr std::uint16_t kTagBlackLevel = 50714;
constexpr std::uint16_t kTagWhiteLevel = 50717;
constexpr std::uint16_t kTagDefaultCropOrigin = 50719;
constexpr std::uint16_t kTagDefaultCropSize = 50720;
constexpr std::uint16_t kTagColorMatrix1 = 50721;
constexpr std::uint16_t kTagColorMatrix2 = 50722;
constexpr std::uint16_t kTagCameraCalibration1 = 50723;
constexpr std::uint16_t kTagCameraCalibration2 = 50724;
constexpr std::uint16_t kTagAnalogBalance = 50727;
constexpr std::uint16_t kTagAsShotNeutral = 50728;
constexpr std::uint16_t kTagBaselineExposure = 50730;
constexpr std::uint16_t kTagCalibrationIlluminant1 = 50778;
constexpr std::uint16_t kTagCalibrationIlluminant2 = 50779;
constexpr std::uint16_t kTagColorimetricReference = 50879;
constexpr std::uint16_t kTagCameraCalibrationSignature = 50931;
constexpr std::uint16_t kTagProfileCalibrationSignature = 50932;
constexpr std::uint16_t kTagForwardMatrix1 = 50964;
constexpr std::uint16_t kTagForwardMatrix2 = 50965;

constexpr std::array<std::uint16_t, 12> kCopiedColorTags{{
    kTagColorMatrix1,
    kTagColorMatrix2,
    kTagCameraCalibration1,
    kTagCameraCalibration2,
    kTagAnalogBalance,
    kTagAsShotNeutral,
    kTagCalibrationIlluminant1,
    kTagCalibrationIlluminant2,
    kTagCameraCalibrationSignature,
    kTagProfileCalibrationSignature,
    kTagForwardMatrix1,
    kTagForwardMatrix2,
}};

enum class ExportCode : std::int32_t {
    Ok = 0,
    InvalidArgument = 1,
    SourceBindingFailed = 2,
    ColorBindingFailed = 3,
    PrepareFailed = 4,
    SourceOpenFailed = 5,
    FinalizedGateFailed = 6,
    SourceReverificationFailed = 7,
    UnsupportedSourceTiff = 8,
    ColorMetadataCopyFailed = 9,
    ScientificMasterReplayFailed = 10,
    ScientificMasterDigestMismatch = 11,
    InvalidProjectionRange = 12,
    DestinationIoFailed = 13,
    DngLayoutOverflow = 14,
};

struct CopiedTag final {
    std::uint16_t tag = 0;
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::vector<std::uint8_t> payload;
};

struct IfdEntry final {
    std::uint16_t tag = 0;
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::vector<std::uint8_t> payload;
    std::uint32_t externalOffset = 0;
};

struct ReplayMetrics final {
    float maximumPositive = 0.0f;
    std::uint64_t sampleCount = 0;
    std::uint64_t clampedLow = 0;
    std::uint64_t clampedHigh = 0;
};

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

void enc16le(std::vector<std::uint8_t>& out, std::size_t at, std::uint16_t v) {
    out.at(at) = static_cast<std::uint8_t>(v & 0xffu);
    out.at(at + 1u) = static_cast<std::uint8_t>((v >> 8u) & 0xffu);
}

void enc32le(std::vector<std::uint8_t>& out, std::size_t at, std::uint32_t v) {
    out.at(at) = static_cast<std::uint8_t>(v & 0xffu);
    out.at(at + 1u) = static_cast<std::uint8_t>((v >> 8u) & 0xffu);
    out.at(at + 2u) = static_cast<std::uint8_t>((v >> 16u) & 0xffu);
    out.at(at + 3u) = static_cast<std::uint8_t>((v >> 24u) & 0xffu);
}

std::vector<std::uint8_t> short_payload(std::uint16_t value) {
    return {static_cast<std::uint8_t>(value & 0xffu),
            static_cast<std::uint8_t>((value >> 8u) & 0xffu)};
}

std::vector<std::uint8_t> shorts_payload(std::initializer_list<std::uint16_t> values) {
    std::vector<std::uint8_t> out(values.size() * 2u, 0u);
    std::size_t at = 0u;
    for (const auto v : values) {
        out[at++] = static_cast<std::uint8_t>(v & 0xffu);
        out[at++] = static_cast<std::uint8_t>((v >> 8u) & 0xffu);
    }
    return out;
}

std::vector<std::uint8_t> long_payload(std::uint32_t value) {
    return {static_cast<std::uint8_t>(value & 0xffu),
            static_cast<std::uint8_t>((value >> 8u) & 0xffu),
            static_cast<std::uint8_t>((value >> 16u) & 0xffu),
            static_cast<std::uint8_t>((value >> 24u) & 0xffu)};
}

std::vector<std::uint8_t> longs_payload(std::initializer_list<std::uint32_t> values) {
    std::vector<std::uint8_t> out(values.size() * 4u, 0u);
    std::size_t at = 0u;
    for (const auto v : values) {
        out[at++] = static_cast<std::uint8_t>(v & 0xffu);
        out[at++] = static_cast<std::uint8_t>((v >> 8u) & 0xffu);
        out[at++] = static_cast<std::uint8_t>((v >> 16u) & 0xffu);
        out[at++] = static_cast<std::uint8_t>((v >> 24u) & 0xffu);
    }
    return out;
}

std::vector<std::uint8_t> ascii_payload(const std::string& text) {
    std::vector<std::uint8_t> out(text.begin(), text.end());
    out.push_back(0u);
    return out;
}

std::uint64_t type_size(std::uint16_t type) noexcept {
    switch (type) {
        case kTypeByte:
        case kTypeAscii:
        case kTypeSByte:
        case kTypeUndefined:
            return 1u;
        case kTypeShort:
        case kTypeSShort:
            return 2u;
        case kTypeLong:
        case kTypeSLong:
        case kTypeFloat:
            return 4u;
        case kTypeRational:
        case kTypeSRational:
        case kTypeDouble:
            return 8u;
        default:
            return 0u;
    }
}

bool is_copied_color_tag(std::uint16_t tag) noexcept {
    return std::find(kCopiedColorTags.begin(), kCopiedColorTags.end(), tag) !=
           kCopiedColorTags.end();
}

bool read_exact(IRandomAccessByteSource& source,
                std::uint64_t offset,
                void* dst,
                std::size_t count) {
    if (offset > source.sizeBytes() || count > source.sizeBytes() - offset) return false;
    return source.readExact(offset, dst, count);
}

ExportCode copy_source_color_tags(IRandomAccessByteSource& source,
                                  std::vector<CopiedTag>& out) {
    std::array<std::uint8_t, 8> header{};
    if (!read_exact(source, 0u, header.data(), header.size())) {
        return ExportCode::ColorMetadataCopyFailed;
    }
    if (!(header[0] == 'I' && header[1] == 'I') || dec16le(header.data() + 2u) != 42u) {
        return ExportCode::UnsupportedSourceTiff;
    }
    const std::uint32_t root = dec32le(header.data() + 4u);
    std::array<std::uint8_t, 2> countBytes{};
    if (root == 0u || !read_exact(source, root, countBytes.data(), countBytes.size())) {
        return ExportCode::ColorMetadataCopyFailed;
    }
    const std::uint16_t entryCount = dec16le(countBytes.data());
    if (entryCount > 512u) return ExportCode::ColorMetadataCopyFailed;

    out.clear();
    for (std::uint16_t i = 0u; i < entryCount; ++i) {
        const std::uint64_t entryOffset = static_cast<std::uint64_t>(root) + 2u +
                                          12ull * static_cast<std::uint64_t>(i);
        std::array<std::uint8_t, 12> entry{};
        if (!read_exact(source, entryOffset, entry.data(), entry.size())) {
            return ExportCode::ColorMetadataCopyFailed;
        }
        const std::uint16_t tag = dec16le(entry.data());
        if (!is_copied_color_tag(tag)) continue;
        const std::uint16_t type = dec16le(entry.data() + 2u);
        const std::uint32_t count = dec32le(entry.data() + 4u);
        const std::uint64_t itemSize = type_size(type);
        if (itemSize == 0u) return ExportCode::ColorMetadataCopyFailed;
        if (count != 0u && itemSize > std::numeric_limits<std::uint64_t>::max() / count) {
            return ExportCode::ColorMetadataCopyFailed;
        }
        const std::uint64_t bytes64 = itemSize * count;
        if (bytes64 > 64u * 1024u) return ExportCode::ColorMetadataCopyFailed;
        const std::size_t bytes = static_cast<std::size_t>(bytes64);
        CopiedTag copied;
        copied.tag = tag;
        copied.type = type;
        copied.count = count;
        copied.payload.resize(bytes);
        if (bytes <= 4u) {
            std::copy_n(entry.data() + 8u, bytes, copied.payload.data());
        } else {
            const std::uint32_t payloadOffset = dec32le(entry.data() + 8u);
            if (!read_exact(source, payloadOffset, copied.payload.data(), bytes)) {
                return ExportCode::ColorMetadataCopyFailed;
            }
        }
        out.push_back(std::move(copied));
    }

    const auto has = [&](std::uint16_t tag) {
        return std::any_of(out.begin(), out.end(),
                           [&](const CopiedTag& t) { return t.tag == tag; });
    };
    if (!has(kTagColorMatrix1) || !has(kTagAsShotNeutral) ||
        !has(kTagCalibrationIlluminant1)) {
        return ExportCode::ColorMetadataCopyFailed;
    }
    return ExportCode::Ok;
}

bool write_all(int fd, const std::uint8_t* data, std::size_t count) {
    std::size_t done = 0u;
    while (done < count) {
        const ssize_t n = ::write(fd, data + done, count - done);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0) return false;
        done += static_cast<std::size_t>(n);
    }
    return true;
}

bool write_all(int fd, const std::vector<std::uint8_t>& data) {
    return write_all(fd, data.data(), data.size());
}

std::uint32_t align4(std::uint32_t value) noexcept {
    return (value + 3u) & ~3u;
}

void add_entry(std::vector<IfdEntry>& entries,
               std::uint16_t tag,
               std::uint16_t type,
               std::uint32_t count,
               std::vector<std::uint8_t> payload) {
    entries.push_back(IfdEntry{tag, type, count, std::move(payload), 0u});
}

ExportCode build_dng_prefix(const truthraw::DngMetadata& metadata,
                            const SourceSeal& sourceSeal,
                            const std::vector<CopiedTag>& copied,
                            float projectionScale,
                            std::uint32_t pixelByteCount,
                            std::vector<std::uint8_t>& prefix,
                            std::uint32_t& pixelOffsetOut) {
    if (metadata.width <= 0 || metadata.height <= 0 || !(projectionScale >= 1.0f) ||
        !std::isfinite(projectionScale)) {
        return ExportCode::InvalidProjectionRange;
    }

    std::vector<IfdEntry> entries;
    add_entry(entries, kTagImageWidth, kTypeLong, 1u,
              long_payload(static_cast<std::uint32_t>(metadata.width)));
    add_entry(entries, kTagImageLength, kTypeLong, 1u,
              long_payload(static_cast<std::uint32_t>(metadata.height)));
    add_entry(entries, kTagBitsPerSample, kTypeShort, 3u,
              shorts_payload({16u, 16u, 16u}));
    add_entry(entries, kTagCompression, kTypeShort, 1u, short_payload(1u));
    add_entry(entries, kTagPhotometric, kTypeShort, 1u,
              short_payload(kPhotometricLinearRaw));

    const std::string description =
        "TruthRaw reconstructed LinearRaw compatibility projection; NOT direct sensor evidence; source=" +
        sourceSeal.sourceEvidenceId;
    add_entry(entries, kTagImageDescription, kTypeAscii,
              static_cast<std::uint32_t>(description.size() + 1u), ascii_payload(description));
    add_entry(entries, kTagMake, kTypeAscii, 9u, ascii_payload("TruthRaw"));
    const std::string model = "Scientific Master LinearRaw Projection v0.1";
    add_entry(entries, kTagModel, kTypeAscii,
              static_cast<std::uint32_t>(model.size() + 1u), ascii_payload(model));
    add_entry(entries, kTagStripOffsets, kTypeLong, 1u, long_payload(0u));
    add_entry(entries, kTagOrientation, kTypeShort, 1u,
              short_payload(static_cast<std::uint16_t>(metadata.orientation)));
    add_entry(entries, kTagSamplesPerPixel, kTypeShort, 1u, short_payload(3u));
    add_entry(entries, kTagRowsPerStrip, kTypeLong, 1u,
              long_payload(static_cast<std::uint32_t>(metadata.height)));
    add_entry(entries, kTagStripByteCounts, kTypeLong, 1u, long_payload(pixelByteCount));
    add_entry(entries, kTagPlanarConfiguration, kTypeShort, 1u, short_payload(1u));

    const std::string software = "TruthRaw linear-dng-projection-v0.1";
    add_entry(entries, kTagSoftware, kTypeAscii,
              static_cast<std::uint32_t>(software.size() + 1u), ascii_payload(software));
    add_entry(entries, kTagSampleFormat, kTypeShort, 3u,
              shorts_payload({1u, 1u, 1u}));
    add_entry(entries, kTagDngVersion, kTypeByte, 4u, {1u, 4u, 0u, 0u});
    add_entry(entries, kTagDngBackwardVersion, kTypeByte, 4u, {1u, 1u, 0u, 0u});

    const std::string unique = "TruthRaw Scientific Master LinearRaw Projection v0.1";
    add_entry(entries, kTagUniqueCameraModel, kTypeAscii,
              static_cast<std::uint32_t>(unique.size() + 1u), ascii_payload(unique));
    add_entry(entries, kTagBlackLevelRepeatDim, kTypeShort, 2u,
              shorts_payload({1u, 1u}));
    add_entry(entries, kTagBlackLevel, kTypeShort, 1u, short_payload(0u));
    add_entry(entries, kTagWhiteLevel, kTypeLong, 1u, long_payload(65535u));
    add_entry(entries, kTagDefaultCropOrigin, kTypeLong, 2u, longs_payload({0u, 0u}));
    add_entry(entries, kTagDefaultCropSize, kTypeLong, 2u,
              longs_payload({static_cast<std::uint32_t>(metadata.width),
                             static_cast<std::uint32_t>(metadata.height)}));

    const double baseline = std::log2(static_cast<double>(projectionScale));
    if (!std::isfinite(baseline) || std::abs(baseline) > 2147.0) {
        return ExportCode::InvalidProjectionRange;
    }
    constexpr std::int32_t denominator = 1000000;
    const auto numerator64 = std::llround(baseline * static_cast<double>(denominator));
    if (numerator64 < std::numeric_limits<std::int32_t>::min() ||
        numerator64 > std::numeric_limits<std::int32_t>::max()) {
        return ExportCode::InvalidProjectionRange;
    }
    const auto numerator = static_cast<std::int32_t>(numerator64);
    std::vector<std::uint8_t> baselinePayload(8u, 0u);
    const auto putS32 = [&](std::size_t at, std::int32_t value) {
        const std::uint32_t u = static_cast<std::uint32_t>(value);
        baselinePayload[at] = static_cast<std::uint8_t>(u & 0xffu);
        baselinePayload[at + 1u] = static_cast<std::uint8_t>((u >> 8u) & 0xffu);
        baselinePayload[at + 2u] = static_cast<std::uint8_t>((u >> 16u) & 0xffu);
        baselinePayload[at + 3u] = static_cast<std::uint8_t>((u >> 24u) & 0xffu);
    };
    putS32(0u, numerator);
    putS32(4u, denominator);
    add_entry(entries, kTagBaselineExposure, kTypeSRational, 1u, std::move(baselinePayload));
    add_entry(entries, kTagColorimetricReference, kTypeShort, 1u, short_payload(0u));

    for (const auto& tag : copied) {
        add_entry(entries, tag.tag, tag.type, tag.count, tag.payload);
    }

    std::sort(entries.begin(), entries.end(),
              [](const IfdEntry& a, const IfdEntry& b) { return a.tag < b.tag; });
    if (entries.size() > std::numeric_limits<std::uint16_t>::max()) {
        return ExportCode::DngLayoutOverflow;
    }

    const std::uint64_t ifdSize64 = 2ull + 12ull * entries.size() + 4ull;
    const std::uint64_t fixedEnd64 = 8ull + ifdSize64;
    if (fixedEnd64 > std::numeric_limits<std::uint32_t>::max()) {
        return ExportCode::DngLayoutOverflow;
    }
    std::uint32_t cursor = align4(static_cast<std::uint32_t>(fixedEnd64));
    for (auto& entry : entries) {
        if (entry.payload.size() > 4u) {
            entry.externalOffset = cursor;
            if (entry.payload.size() > std::numeric_limits<std::uint32_t>::max() - cursor) {
                return ExportCode::DngLayoutOverflow;
            }
            cursor = align4(cursor + static_cast<std::uint32_t>(entry.payload.size()));
        }
    }
    const std::uint32_t pixelOffset = align4(cursor);
    if (pixelByteCount > std::numeric_limits<std::uint32_t>::max() - pixelOffset) {
        return ExportCode::DngLayoutOverflow;
    }

    for (auto& entry : entries) {
        if (entry.tag == kTagStripOffsets) entry.payload = long_payload(pixelOffset);
    }

    prefix.assign(pixelOffset, 0u);
    prefix[0] = 'I';
    prefix[1] = 'I';
    enc16le(prefix, 2u, 42u);
    enc32le(prefix, 4u, 8u);
    enc16le(prefix, 8u, static_cast<std::uint16_t>(entries.size()));

    std::size_t entryAt = 10u;
    for (const auto& entry : entries) {
        enc16le(prefix, entryAt, entry.tag);
        enc16le(prefix, entryAt + 2u, entry.type);
        enc32le(prefix, entryAt + 4u, entry.count);
        if (entry.payload.size() <= 4u) {
            for (std::size_t i = 0u; i < entry.payload.size(); ++i) {
                prefix[entryAt + 8u + i] = entry.payload[i];
            }
        } else {
            enc32le(prefix, entryAt + 8u, entry.externalOffset);
            std::copy(entry.payload.begin(), entry.payload.end(),
                      prefix.begin() + entry.externalOffset);
        }
        entryAt += 12u;
    }
    enc32le(prefix, entryAt, 0u);
    pixelOffsetOut = pixelOffset;
    return ExportCode::Ok;
}

TileRect canonical_tile(const truthraw::DngMetadata& metadata,
                        int x0,
                        int y0,
                        int halo) {
    TileRect tile{};
    tile.x0 = x0;
    tile.y0 = y0;
    tile.x1 = std::min(metadata.width, x0 + kCanonicalCore);
    tile.y1 = std::min(metadata.height, y0 + kCanonicalCore);
    tile.hx0 = std::max(0, tile.x0 - halo);
    tile.hy0 = std::max(0, tile.y0 - halo);
    tile.hx1 = std::min(metadata.width, tile.x1 + halo);
    tile.hy1 = std::min(metadata.height, tile.y1 + halo);
    return tile;
}

ExportCode replay_master_and_measure(TileNativeDngSource& source,
                                     ResearchEdgeAwareMeasuredPreservingReconstruction& reconstruction,
                                     const ReleaseResult& release,
                                     ReplayMetrics& metrics) {
    const auto& metadata = source.metadata();
    truthraw::scientific_master_digest::v0_1::ScientificMasterDigestAccumulator digest(
        static_cast<std::uint32_t>(metadata.width),
        static_cast<std::uint32_t>(metadata.height));
    if (!digest.valid()) return ExportCode::ScientificMasterReplayFailed;

    Workspace workspace{};
    const int halo = reconstruction.requiredHalo();
    if (halo < 0) return ExportCode::ScientificMasterReplayFailed;

    for (int y0 = 0; y0 < metadata.height; y0 += kCanonicalCore) {
        for (int x0 = 0; x0 < metadata.width; x0 += kCanonicalCore) {
            const TileRect tile = canonical_tile(metadata, x0, y0, halo);
            const auto stage2 = fill_stage2(source, tile, workspace);
            if (!stage2) return ExportCode::ScientificMasterReplayFailed;

            const int tileWidth = tile.hx1 - tile.hx0;
            const int tileHeight = tile.hy1 - tile.hy0;
            const int coreWidth = tile.x1 - tile.x0;
            const int coreHeight = tile.y1 - tile.y0;
            const std::size_t corePixels = static_cast<std::size_t>(coreWidth) *
                                           static_cast<std::size_t>(coreHeight);
            workspace.cam.resize(corePixels * 3u);
            const auto reconstructed = reconstruction.reconstructTile(
                workspace.stage2.data(), tileWidth, tileHeight,
                tile.hx0, tile.hy0,
                tile.x0, tile.y0, coreWidth, coreHeight,
                metadata.cfa, workspace.cam.data());
            if (!reconstructed) return ExportCode::ScientificMasterReplayFailed;

            truthraw::scientific_master_digest::v0_1::TileView view{};
            view.x = static_cast<std::uint32_t>(tile.x0);
            view.y = static_cast<std::uint32_t>(tile.y0);
            view.width = static_cast<std::uint32_t>(coreWidth);
            view.height = static_cast<std::uint32_t>(coreHeight);
            view.rgb = workspace.cam.data();
            view.rowStrideSamples = static_cast<std::size_t>(coreWidth) * 3u;
            if (!digest.add_tile(view)) return ExportCode::ScientificMasterReplayFailed;

            for (const float value : workspace.cam) {
                if (!std::isfinite(value)) return ExportCode::ScientificMasterReplayFailed;
                if (value > metrics.maximumPositive) metrics.maximumPositive = value;
                ++metrics.sampleCount;
            }
        }
    }

    truthraw::scientific_master_digest::v0_1::Sha256 replayHash{};
    if (!digest.finalize(replayHash)) return ExportCode::ScientificMasterReplayFailed;
    if (replayHash != release.scientificIdentity.scientificMasterHash) {
        return ExportCode::ScientificMasterDigestMismatch;
    }
    if (!(metrics.maximumPositive > 0.0f) || !std::isfinite(metrics.maximumPositive)) {
        return ExportCode::InvalidProjectionRange;
    }
    return ExportCode::Ok;
}

ExportCode write_linear_raw_pixels(TileNativeDngSource& source,
                                   ResearchEdgeAwareMeasuredPreservingReconstruction& reconstruction,
                                   int destinationFd,
                                   float projectionScale,
                                   ReplayMetrics& metrics) {
    const auto& metadata = source.metadata();
    const int halo = reconstruction.requiredHalo();
    if (halo < 0 || !(projectionScale >= 1.0f) || !std::isfinite(projectionScale)) {
        return ExportCode::InvalidProjectionRange;
    }

    Workspace workspace{};
    for (int y0 = 0; y0 < metadata.height; y0 += kCanonicalCore) {
        const int bandHeight = std::min(kCanonicalCore, metadata.height - y0);
        const std::size_t bandSamples = static_cast<std::size_t>(metadata.width) *
                                        static_cast<std::size_t>(bandHeight) * 3u;
        if (bandSamples > std::numeric_limits<std::size_t>::max() / 2u) {
            return ExportCode::DngLayoutOverflow;
        }
        std::vector<std::uint8_t> band(bandSamples * 2u, 0u);

        for (int x0 = 0; x0 < metadata.width; x0 += kCanonicalCore) {
            const TileRect tile = canonical_tile(metadata, x0, y0, halo);
            const auto stage2 = fill_stage2(source, tile, workspace);
            if (!stage2) return ExportCode::ScientificMasterReplayFailed;

            const int tileWidth = tile.hx1 - tile.hx0;
            const int tileHeight = tile.hy1 - tile.hy0;
            const int coreWidth = tile.x1 - tile.x0;
            const int coreHeight = tile.y1 - tile.y0;
            const std::size_t corePixels = static_cast<std::size_t>(coreWidth) *
                                           static_cast<std::size_t>(coreHeight);
            workspace.cam.resize(corePixels * 3u);
            const auto reconstructed = reconstruction.reconstructTile(
                workspace.stage2.data(), tileWidth, tileHeight,
                tile.hx0, tile.hy0,
                tile.x0, tile.y0, coreWidth, coreHeight,
                metadata.cfa, workspace.cam.data());
            if (!reconstructed) return ExportCode::ScientificMasterReplayFailed;

            for (int yy = 0; yy < coreHeight; ++yy) {
                for (int xx = 0; xx < coreWidth; ++xx) {
                    const std::size_t srcPixel =
                        static_cast<std::size_t>(yy) * static_cast<std::size_t>(coreWidth) +
                        static_cast<std::size_t>(xx);
                    const std::size_t dstPixel =
                        static_cast<std::size_t>(yy) * static_cast<std::size_t>(metadata.width) +
                        static_cast<std::size_t>(tile.x0 + xx);
                    for (std::size_t channel = 0u; channel < 3u; ++channel) {
                        float value = workspace.cam[3u * srcPixel + channel];
                        if (!std::isfinite(value)) return ExportCode::ScientificMasterReplayFailed;
                        if (value < 0.0f) {
                            value = 0.0f;
                            ++metrics.clampedLow;
                        }
                        float normalized = value / projectionScale;
                        if (normalized > 1.0f) {
                            normalized = 1.0f;
                            ++metrics.clampedHigh;
                        }
                        normalized = std::max(0.0f, normalized);
                        const auto q = static_cast<std::uint16_t>(
                            std::lround(static_cast<double>(normalized) * 65535.0));
                        const std::size_t byteAt = 2u * (3u * dstPixel + channel);
                        band[byteAt] = static_cast<std::uint8_t>(q & 0xffu);
                        band[byteAt + 1u] = static_cast<std::uint8_t>((q >> 8u) & 0xffu);
                    }
                }
            }
        }

        if (!write_all(destinationFd, band)) return ExportCode::DestinationIoFailed;
    }
    return ExportCode::Ok;
}

jlongArray packet(JNIEnv* env,
                  ExportCode code,
                  std::uint64_t bytesWritten = 0u,
                  int width = 0,
                  int height = 0,
                  const ReplayMetrics* metrics = nullptr,
                  float projectionScale = 1.0f) {
    std::array<jlong, 10> values{};
    values[0] = kPacketMagic;
    values[1] = static_cast<jlong>(code);
    values[2] = static_cast<jlong>(width);
    values[3] = static_cast<jlong>(height);
    values[4] = static_cast<jlong>(bytesWritten);
    if (metrics != nullptr) {
        values[5] = static_cast<jlong>(metrics->sampleCount);
        values[6] = static_cast<jlong>(metrics->clampedLow);
        values[7] = static_cast<jlong>(metrics->clampedHigh);
    }
    std::uint32_t scaleBits = 0u;
    std::memcpy(&scaleBits, &projectionScale, sizeof(scaleBits));
    values[8] = static_cast<jlong>(scaleBits);
    values[9] = 16; // output bit depth

    jlongArray out = env->NewLongArray(static_cast<jsize>(values.size()));
    if (out != nullptr) {
        env->SetLongArrayRegion(out, 0, static_cast<jsize>(values.size()), values.data());
    }
    return out;
}

} // namespace

extern "C" JNIEXPORT jlongArray JNICALL
Java_com_truthraw_adaptiveui_NativeDngProjectionBridge_exportFinalizedLinearDng(
    JNIEnv* env,
    jobject,
    jint sourceFd,
    jint destinationFd,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    if (sourceFd < 0 || destinationFd < 0 || maxSourceResidentBytes <= 0 ||
        maxLogicalResidentBytes <= 0) {
        return packet(env, ExportCode::InvalidArgument);
    }

    auto bytes = std::make_shared<PosixFdByteSource>(static_cast<int>(sourceFd));
    SourceSeal sourceSeal;
    const auto sealed = truthraw::scientific_preview_binding_v0_1::seal_source_sha256(*bytes, sourceSeal);
    if (!sealed) return packet(env, ExportCode::SourceBindingFailed);

    ProducerResult produced;
    const auto colorStatus =
        truthraw::dng_color_binding_producer_v0_2::produce_source_metadata_color_binding(
            *bytes, sourceSeal, produced);
    if (!colorStatus) return packet(env, ExportCode::ColorBindingFailed);

    PreparedScientificPreviewSource prepared;
    const auto preparedStatus =
        truthraw::scientific_preview_binding_v0_2::prepare_scientific_color_source(
            sourceSeal, produced.color, prepared);
    if (!preparedStatus) return packet(env, ExportCode::PrepareFailed);
    if (!prepared.mainHouseComputeAllowed || !prepared.sourceBoundAppearanceReleaseAllowed ||
        prepared.scientificPreviewReleaseAllowed || prepared.scientificClaimAllowed ||
        prepared.physicalFrameCount != 1u || prepared.independentEvidenceCount != 1u) {
        return packet(env, ExportCode::PrepareFailed);
    }

    const auto preVerified =
        truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes, sourceSeal);
    if (!preVerified) return packet(env, ExportCode::SourceReverificationFailed);

    auto openOptions = prepared.tileNativeOptions;
    openOptions.maxResidentBytes = static_cast<std::size_t>(maxSourceResidentBytes);
    std::unique_ptr<TileNativeDngSource> gateSource;
    const auto opened = TileNativeDngSource::open(bytes, openOptions, gateSource);
    if (!opened) return packet(env, ExportCode::SourceOpenFailed);

    auto reconstruction = std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto appearance = std::make_shared<NeutralReferenceAppearance>();
    BoundedSrgbPreviewSink gateSink(kGatePreviewEdge);

    truthraw::scientific_master_streaming_binding::v0_2::Options scientificOptions;
    scientificOptions.memoryBudgetBytes = static_cast<std::size_t>(maxLogicalResidentBytes);
    truthraw::streaming_v0_1::StreamingOptions previewOptions;
    previewOptions.tile = {128, 16};
    previewOptions.workers = 1;
    previewOptions.hdrEnabled = true;
    previewOptions.streamScientificDiagnostics = false;
    previewOptions.sdrLutSize = 4096;
    previewOptions.memoryBudgetBytes = static_cast<std::size_t>(maxLogicalResidentBytes);

    std::array<truthraw::technical_backplane::v0_1::RoomStatus,
               truthraw::technical_backplane::v0_1::kRoomCount> roomStatus{};
    roomStatus.fill(truthraw::technical_backplane::v0_1::RoomStatus::ResearchOnly);

    ReleaseResult release;
    const auto released =
        truthraw::finalized_scientific_preview_release::v0_2::create_and_release_finalized_scientific_preview(
            prepared,
            *gateSource,
            reconstruction,
            appearance,
            scientificOptions,
            previewOptions,
            roomStatus,
            truthraw::technical_backplane::v0_1::ClaimStatus::Candidate,
            gateSink,
            release);
    if (!released || release.authority == PreviewAuthority::None || !gateSink.finished()) {
        return packet(env, ExportCode::FinalizedGateFailed);
    }

    if (release.streaming.provenance.physicalFrameCount != 1u ||
        release.streaming.provenance.independentEvidenceCount != 1u ||
        release.streaming.provenance.scientificMasterModifiedByAppearance ||
        release.streaming.provenance.counterfactualObservationCreated) {
        return packet(env, ExportCode::FinalizedGateFailed);
    }

    const auto postGateVerified =
        truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes, sourceSeal);
    if (!postGateVerified) return packet(env, ExportCode::SourceReverificationFailed);

    std::vector<CopiedTag> copiedColorTags;
    const ExportCode copied = copy_source_color_tags(*bytes, copiedColorTags);
    if (copied != ExportCode::Ok) return packet(env, copied);

    std::unique_ptr<TileNativeDngSource> replaySource;
    const auto replayOpened = TileNativeDngSource::open(bytes, openOptions, replaySource);
    if (!replayOpened) return packet(env, ExportCode::SourceOpenFailed);

    ResearchEdgeAwareMeasuredPreservingReconstruction replayReconstruction;
    ReplayMetrics replayMetrics{};
    const ExportCode replay = replay_master_and_measure(
        *replaySource, replayReconstruction, release, replayMetrics);
    if (replay != ExportCode::Ok) {
        return packet(env, replay, 0u,
                      replaySource->metadata().width,
                      replaySource->metadata().height,
                      &replayMetrics);
    }

    const float projectionScale = std::max(1.0f, replayMetrics.maximumPositive);
    const std::uint64_t pixelBytes64 =
        static_cast<std::uint64_t>(replaySource->metadata().width) *
        static_cast<std::uint64_t>(replaySource->metadata().height) * 3ull * 2ull;
    if (pixelBytes64 == 0u || pixelBytes64 > std::numeric_limits<std::uint32_t>::max()) {
        return packet(env, ExportCode::DngLayoutOverflow);
    }

    std::vector<std::uint8_t> prefix;
    std::uint32_t pixelOffset = 0u;
    const ExportCode prefixStatus = build_dng_prefix(
        replaySource->metadata(),
        sourceSeal,
        copiedColorTags,
        projectionScale,
        static_cast<std::uint32_t>(pixelBytes64),
        prefix,
        pixelOffset);
    if (prefixStatus != ExportCode::Ok) {
        return packet(env, prefixStatus, 0u,
                      replaySource->metadata().width,
                      replaySource->metadata().height,
                      &replayMetrics,
                      projectionScale);
    }

    if (::lseek(destinationFd, 0, SEEK_SET) < 0 || ::ftruncate(destinationFd, 0) != 0) {
        return packet(env, ExportCode::DestinationIoFailed);
    }
    if (!write_all(destinationFd, prefix)) {
        return packet(env, ExportCode::DestinationIoFailed);
    }

    std::unique_ptr<TileNativeDngSource> writeSource;
    const auto writeOpened = TileNativeDngSource::open(bytes, openOptions, writeSource);
    if (!writeOpened) return packet(env, ExportCode::SourceOpenFailed);
    ResearchEdgeAwareMeasuredPreservingReconstruction writeReconstruction;
    const ExportCode pixels = write_linear_raw_pixels(
        *writeSource,
        writeReconstruction,
        destinationFd,
        projectionScale,
        replayMetrics);
    if (pixels != ExportCode::Ok) {
        return packet(env, pixels, 0u,
                      writeSource->metadata().width,
                      writeSource->metadata().height,
                      &replayMetrics,
                      projectionScale);
    }

    const auto postVerified =
        truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes, sourceSeal);
    if (!postVerified) return packet(env, ExportCode::SourceReverificationFailed);

    const std::uint64_t bytesWritten = static_cast<std::uint64_t>(pixelOffset) + pixelBytes64;
    if (::ftruncate(destinationFd, static_cast<off_t>(bytesWritten)) != 0) {
        return packet(env, ExportCode::DestinationIoFailed);
    }

    return packet(env,
                  ExportCode::Ok,
                  bytesWritten,
                  writeSource->metadata().width,
                  writeSource->metadata().height,
                  &replayMetrics,
                  projectionScale);
}
