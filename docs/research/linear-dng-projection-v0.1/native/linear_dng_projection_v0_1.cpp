#include "linear_dng_projection_v0_1.h"

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

namespace truthraw::linear_dng_projection::v0_1 {
namespace {

using finalized_scientific_preview_release::v0_2::PreviewAuthority;
using streaming_v0_1::TileRect;
using streaming_v0_1::detail::Workspace;
using streaming_v0_1::detail::fill_stage2;
using streaming_v0_1::detail::vector_bytes;

constexpr int kCanonicalCore = 64;
constexpr std::uint16_t kClassicTiffMagic = 42;
constexpr std::uint16_t kTiffByte = 1;
constexpr std::uint16_t kTiffAscii = 2;
constexpr std::uint16_t kTiffShort = 3;
constexpr std::uint16_t kTiffLong = 4;
constexpr std::uint16_t kTiffRational = 5;
constexpr std::uint16_t kTiffSRational = 10;
constexpr std::uint16_t kLinearRawPhotometric = 34892;
constexpr std::uint32_t kMaxIfdEntries = 512;

struct Entry final {
    std::uint16_t tag = 0;
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::vector<std::uint8_t> payload;
    std::uint32_t externalOffset = 0;
};

bool add_ok(std::uint64_t a, std::uint64_t b, std::uint64_t& out) noexcept {
    if (a > std::numeric_limits<std::uint64_t>::max() - b) return false;
    out = a + b;
    return true;
}

bool mul_ok(std::uint64_t a, std::uint64_t b, std::uint64_t& out) noexcept {
    if (a != 0u && b > std::numeric_limits<std::uint64_t>::max() / a) return false;
    out = a * b;
    return true;
}

std::uint64_t type_size(std::uint16_t type) noexcept {
    switch (type) {
        case 1: case 2: case 6: case 7: return 1;
        case 3: case 8: return 2;
        case 4: case 9: case 11: return 4;
        case 5: case 10: case 12: return 8;
        default: return 0;
    }
}

std::uint16_t dec16(const std::uint8_t* p, bool little) noexcept {
    if (little) {
        return static_cast<std::uint16_t>(p[0]) |
               static_cast<std::uint16_t>(static_cast<std::uint16_t>(p[1]) << 8u);
    }
    return static_cast<std::uint16_t>(static_cast<std::uint16_t>(p[0]) << 8u) |
           static_cast<std::uint16_t>(p[1]);
}

std::uint32_t dec32(const std::uint8_t* p, bool little) noexcept {
    if (little) {
        return static_cast<std::uint32_t>(p[0]) |
               (static_cast<std::uint32_t>(p[1]) << 8u) |
               (static_cast<std::uint32_t>(p[2]) << 16u) |
               (static_cast<std::uint32_t>(p[3]) << 24u);
    }
    return (static_cast<std::uint32_t>(p[0]) << 24u) |
           (static_cast<std::uint32_t>(p[1]) << 16u) |
           (static_cast<std::uint32_t>(p[2]) << 8u) |
           static_cast<std::uint32_t>(p[3]);
}

void put16(std::uint8_t* p, std::uint16_t value, bool little) noexcept {
    if (little) {
        p[0] = static_cast<std::uint8_t>(value & 0xffu);
        p[1] = static_cast<std::uint8_t>((value >> 8u) & 0xffu);
    } else {
        p[0] = static_cast<std::uint8_t>((value >> 8u) & 0xffu);
        p[1] = static_cast<std::uint8_t>(value & 0xffu);
    }
}

void put32(std::uint8_t* p, std::uint32_t value, bool little) noexcept {
    if (little) {
        p[0] = static_cast<std::uint8_t>(value & 0xffu);
        p[1] = static_cast<std::uint8_t>((value >> 8u) & 0xffu);
        p[2] = static_cast<std::uint8_t>((value >> 16u) & 0xffu);
        p[3] = static_cast<std::uint8_t>((value >> 24u) & 0xffu);
    } else {
        p[0] = static_cast<std::uint8_t>((value >> 24u) & 0xffu);
        p[1] = static_cast<std::uint8_t>((value >> 16u) & 0xffu);
        p[2] = static_cast<std::uint8_t>((value >> 8u) & 0xffu);
        p[3] = static_cast<std::uint8_t>(value & 0xffu);
    }
}

bool read_exact(tile_dng_v0_1::IRandomAccessByteSource& source,
                std::uint64_t offset,
                void* dst,
                std::size_t bytes) noexcept {
    if (offset > source.sizeBytes() || bytes > source.sizeBytes() - offset) return false;
    return source.readExact(offset, dst, bytes);
}

bool is_color_tag(std::uint16_t tag) noexcept {
    switch (tag) {
        case 50721: // ColorMatrix1
        case 50722: // ColorMatrix2
        case 50723: // CameraCalibration1
        case 50724: // CameraCalibration2
        case 50727: // AnalogBalance
        case 50728: // AsShotNeutral
        case 50778: // CalibrationIlluminant1
        case 50779: // CalibrationIlluminant2
        case 50931: // CameraCalibrationSignature
        case 50932: // ProfileCalibrationSignature
        case 50964: // ForwardMatrix1
        case 50965: // ForwardMatrix2
            return true;
        default:
            return false;
    }
}

Status copy_source_color_tags(tile_dng_v0_1::IRandomAccessByteSource& source,
                              bool& little,
                              std::vector<Entry>& out) noexcept {
    std::array<std::uint8_t, 8> header{};
    if (!read_exact(source, 0u, header.data(), header.size())) {
        return Status::error(StatusCode::SourceReadFailed, "cannot read source TIFF header");
    }
    if (header[0] == 'I' && header[1] == 'I') little = true;
    else if (header[0] == 'M' && header[1] == 'M') little = false;
    else return Status::error(StatusCode::SourceMetadataFailed, "source has invalid TIFF byte order");

    const std::uint16_t magic = dec16(header.data() + 2, little);
    if (magic == 43u) {
        return Status::error(StatusCode::UnsupportedContainer, "BigTIFF export source is not supported in v0.1");
    }
    if (magic != kClassicTiffMagic) {
        return Status::error(StatusCode::SourceMetadataFailed, "classic TIFF magic 42 required");
    }
    const std::uint32_t root = dec32(header.data() + 4, little);
    if (root == 0u || root > source.sizeBytes() || source.sizeBytes() - root < 2u) {
        return Status::error(StatusCode::SourceMetadataFailed, "source IFD0 offset is invalid");
    }

    std::array<std::uint8_t, 2> countBytes{};
    if (!read_exact(source, root, countBytes.data(), countBytes.size())) {
        return Status::error(StatusCode::SourceReadFailed, "cannot read source IFD0 count");
    }
    const std::uint16_t count = dec16(countBytes.data(), little);
    if (count > kMaxIfdEntries) {
        return Status::error(StatusCode::SourceMetadataFailed, "source IFD0 entry cap exceeded");
    }

    bool colorMatrix1Seen = false;
    bool asShotNeutralSeen = false;
    std::array<std::uint8_t, 12> rawEntry{};
    for (std::uint16_t index = 0; index < count; ++index) {
        const std::uint64_t entryOffset = static_cast<std::uint64_t>(root) + 2u + 12ull * index;
        if (!read_exact(source, entryOffset, rawEntry.data(), rawEntry.size())) {
            return Status::error(StatusCode::SourceReadFailed, "cannot read source IFD0 entry");
        }
        const std::uint16_t tag = dec16(rawEntry.data(), little);
        if (!is_color_tag(tag)) continue;
        const std::uint16_t type = dec16(rawEntry.data() + 2, little);
        const std::uint32_t itemCount = dec32(rawEntry.data() + 4, little);
        const std::uint64_t itemSize = type_size(type);
        if (itemSize == 0u || itemCount == 0u) {
            return Status::error(StatusCode::SourceMetadataFailed, "source color tag has unsupported TIFF type/count");
        }
        std::uint64_t payloadBytes64 = 0u;
        if (!mul_ok(itemSize, itemCount, payloadBytes64) ||
            payloadBytes64 > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            return Status::error(StatusCode::SourceMetadataFailed, "source color tag payload size overflow");
        }
        const auto payloadBytes = static_cast<std::size_t>(payloadBytes64);
        Entry entry;
        entry.tag = tag;
        entry.type = type;
        entry.count = itemCount;
        entry.payload.resize(payloadBytes);
        if (payloadBytes <= 4u) {
            std::memcpy(entry.payload.data(), rawEntry.data() + 8, payloadBytes);
        } else {
            const std::uint32_t payloadOffset = dec32(rawEntry.data() + 8, little);
            if (!read_exact(source, payloadOffset, entry.payload.data(), payloadBytes)) {
                return Status::error(StatusCode::SourceReadFailed, "cannot read source color tag payload");
            }
        }
        if (tag == 50721u) colorMatrix1Seen = true;
        if (tag == 50728u) asShotNeutralSeen = true;
        out.push_back(std::move(entry));
    }

    if (!colorMatrix1Seen || !asShotNeutralSeen) {
        return Status::error(StatusCode::SourceMetadataFailed,
                             "source ColorMatrix1/AsShotNeutral required for LinearRaw compatibility projection");
    }
    return Status::ok();
}

std::vector<std::uint8_t> short_payload(bool little, std::initializer_list<std::uint16_t> values) {
    std::vector<std::uint8_t> out(values.size() * 2u);
    std::size_t offset = 0u;
    for (const auto value : values) {
        put16(out.data() + offset, value, little);
        offset += 2u;
    }
    return out;
}

std::vector<std::uint8_t> long_payload(bool little, std::initializer_list<std::uint32_t> values) {
    std::vector<std::uint8_t> out(values.size() * 4u);
    std::size_t offset = 0u;
    for (const auto value : values) {
        put32(out.data() + offset, value, little);
        offset += 4u;
    }
    return out;
}

std::vector<std::uint8_t> rational_payload(bool little,
                                           std::initializer_list<std::pair<std::uint32_t,std::uint32_t>> values) {
    std::vector<std::uint8_t> out(values.size() * 8u);
    std::size_t offset = 0u;
    for (const auto& value : values) {
        put32(out.data() + offset, value.first, little);
        put32(out.data() + offset + 4u, value.second, little);
        offset += 8u;
    }
    return out;
}

std::vector<std::uint8_t> srational_payload(bool little, std::int32_t numerator, std::int32_t denominator) {
    std::vector<std::uint8_t> out(8u);
    put32(out.data(), static_cast<std::uint32_t>(numerator), little);
    put32(out.data() + 4u, static_cast<std::uint32_t>(denominator), little);
    return out;
}

std::vector<std::uint8_t> ascii_payload(const std::string& text) {
    std::vector<std::uint8_t> out(text.begin(), text.end());
    out.push_back(0u);
    return out;
}

void add_entry(std::vector<Entry>& entries,
               std::uint16_t tag,
               std::uint16_t type,
               std::uint32_t count,
               std::vector<std::uint8_t> payload) {
    entries.push_back(Entry{tag, type, count, std::move(payload), 0u});
}

Status write_container_header(IRandomAccessByteSink& sink,
                              bool little,
                              const DngMetadata& metadata,
                              std::vector<Entry>& entries,
                              std::uint32_t& pixelOffsetOut,
                              std::uint64_t& totalBytesOut,
                              std::uint64_t& pixelBytesOut) noexcept {
    if (metadata.width <= 0 || metadata.height <= 0) {
        return Status::error(StatusCode::InvalidArgument, "invalid output dimensions");
    }
    const auto width = static_cast<std::uint32_t>(metadata.width);
    const auto height = static_cast<std::uint32_t>(metadata.height);

    std::uint64_t pixels = 0u;
    std::uint64_t samples = 0u;
    std::uint64_t pixelBytes = 0u;
    if (!mul_ok(width, height, pixels) || !mul_ok(pixels, 3u, samples) || !mul_ok(samples, 2u, pixelBytes)) {
        return Status::error(StatusCode::OutputTooLarge, "LinearRaw pixel payload size overflow");
    }
    if (pixelBytes > std::numeric_limits<std::uint32_t>::max()) {
        return Status::error(StatusCode::OutputTooLarge, "classic TIFF StripByteCounts exceeds uint32");
    }

    add_entry(entries, 254, kTiffLong, 1, long_payload(little, {0u}));
    add_entry(entries, 256, kTiffLong, 1, long_payload(little, {width}));
    add_entry(entries, 257, kTiffLong, 1, long_payload(little, {height}));
    add_entry(entries, 258, kTiffShort, 3, short_payload(little, {16u,16u,16u}));
    add_entry(entries, 259, kTiffShort, 1, short_payload(little, {1u}));
    add_entry(entries, 262, kTiffShort, 1, short_payload(little, {kLinearRawPhotometric}));
    const std::string description =
        "TruthRaw bounded LinearRaw compatibility projection; source=" + metadata.sourceId;
    add_entry(entries, 270, kTiffAscii, static_cast<std::uint32_t>(description.size() + 1u), ascii_payload(description));
    add_entry(entries, 271, kTiffAscii, 9u, ascii_payload("TruthRaw"));
    const std::string model = "Scientific Master Linear Projection";
    add_entry(entries, 272, kTiffAscii, static_cast<std::uint32_t>(model.size() + 1u), ascii_payload(model));
    add_entry(entries, 273, kTiffLong, 1, long_payload(little, {0u})); // patched after aux layout
    add_entry(entries, 274, kTiffShort, 1,
              short_payload(little, {static_cast<std::uint16_t>(metadata.orientation)}));
    add_entry(entries, 277, kTiffShort, 1, short_payload(little, {3u}));
    add_entry(entries, 278, kTiffLong, 1, long_payload(little, {height}));
    add_entry(entries, 279, kTiffLong, 1,
              long_payload(little, {static_cast<std::uint32_t>(pixelBytes)}));
    add_entry(entries, 284, kTiffShort, 1, short_payload(little, {1u}));
    const std::string software = "TruthRaw Linear DNG Projection v0.1";
    add_entry(entries, 305, kTiffAscii, static_cast<std::uint32_t>(software.size() + 1u), ascii_payload(software));
    add_entry(entries, 339, kTiffShort, 3, short_payload(little, {1u,1u,1u}));
    add_entry(entries, 50706, kTiffByte, 4, std::vector<std::uint8_t>{1u,4u,0u,0u});
    add_entry(entries, 50707, kTiffByte, 4, std::vector<std::uint8_t>{1u,1u,0u,0u});
    const std::string uniqueModel = "TruthRaw LinearRaw Projection v0.1";
    add_entry(entries, 50708, kTiffAscii, static_cast<std::uint32_t>(uniqueModel.size() + 1u), ascii_payload(uniqueModel));
    add_entry(entries, 50713, kTiffShort, 2, short_payload(little, {1u,1u}));
    add_entry(entries, 50714, kTiffRational, 3,
              rational_payload(little, {{0u,1u},{0u,1u},{0u,1u}}));
    add_entry(entries, 50717, kTiffLong, 3, long_payload(little, {65535u,65535u,65535u}));
    add_entry(entries, 50719, kTiffRational, 2,
              rational_payload(little, {{0u,1u},{0u,1u}}));
    add_entry(entries, 50720, kTiffRational, 2,
              rational_payload(little, {{width,1u},{height,1u}}));
    add_entry(entries, 50730, kTiffSRational, 1, srational_payload(little, 0, 1));
    add_entry(entries, 50734, kTiffRational, 1, rational_payload(little, {{1u,1u}}));
    add_entry(entries, 50829, kTiffLong, 4, long_payload(little, {0u,0u,height,width}));

    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
        return a.tag < b.tag;
    });
    for (std::size_t i = 1; i < entries.size(); ++i) {
        if (entries[i-1].tag == entries[i].tag) {
            return Status::error(StatusCode::SourceMetadataFailed, "duplicate output DNG tag");
        }
    }
    if (entries.size() > std::numeric_limits<std::uint16_t>::max()) {
        return Status::error(StatusCode::OutputTooLarge, "output IFD entry count overflow");
    }

    const std::uint64_t ifdBytes64 = 2u + 12u * entries.size() + 4u;
    std::uint64_t auxOffset64 = 0u;
    if (!add_ok(8u, ifdBytes64, auxOffset64) || auxOffset64 > std::numeric_limits<std::uint32_t>::max()) {
        return Status::error(StatusCode::OutputTooLarge, "output IFD offset overflow");
    }
    std::uint64_t cursor = auxOffset64;
    for (auto& entry : entries) {
        if (entry.payload.size() <= 4u) continue;
        if ((cursor & 1u) != 0u) ++cursor;
        if (cursor > std::numeric_limits<std::uint32_t>::max()) {
            return Status::error(StatusCode::OutputTooLarge, "output tag payload offset overflow");
        }
        entry.externalOffset = static_cast<std::uint32_t>(cursor);
        if (!add_ok(cursor, entry.payload.size(), cursor)) {
            return Status::error(StatusCode::OutputTooLarge, "output tag payload size overflow");
        }
    }
    cursor = (cursor + 3u) & ~std::uint64_t{3u};
    if (cursor > std::numeric_limits<std::uint32_t>::max()) {
        return Status::error(StatusCode::OutputTooLarge, "output pixel offset exceeds classic TIFF range");
    }
    const std::uint32_t pixelOffset = static_cast<std::uint32_t>(cursor);
    for (auto& entry : entries) {
        if (entry.tag == 273u) {
            entry.payload = long_payload(little, {pixelOffset});
            break;
        }
    }

    std::uint64_t totalBytes = 0u;
    if (!add_ok(pixelOffset, pixelBytes, totalBytes) ||
        totalBytes > std::numeric_limits<std::uint32_t>::max()) {
        return Status::error(StatusCode::OutputTooLarge, "output LinearRaw DNG exceeds classic TIFF 4 GiB range");
    }
    if (!sink.resize(totalBytes)) {
        return Status::error(StatusCode::SinkFailed, "cannot resize output DNG");
    }

    std::array<std::uint8_t, 8> header{};
    header[0] = little ? 'I' : 'M';
    header[1] = little ? 'I' : 'M';
    put16(header.data() + 2, kClassicTiffMagic, little);
    put32(header.data() + 4, 8u, little);
    if (!sink.writeExact(0u, header.data(), header.size())) {
        return Status::error(StatusCode::SinkFailed, "cannot write TIFF header");
    }

    std::vector<std::uint8_t> ifd(static_cast<std::size_t>(ifdBytes64), 0u);
    put16(ifd.data(), static_cast<std::uint16_t>(entries.size()), little);
    for (std::size_t index = 0; index < entries.size(); ++index) {
        const auto& entry = entries[index];
        auto* dst = ifd.data() + 2u + 12u * index;
        put16(dst, entry.tag, little);
        put16(dst + 2, entry.type, little);
        put32(dst + 4, entry.count, little);
        if (entry.payload.size() <= 4u) {
            std::copy(entry.payload.begin(), entry.payload.end(), dst + 8);
        } else {
            put32(dst + 8, entry.externalOffset, little);
        }
    }
    put32(ifd.data() + ifd.size() - 4u, 0u, little);
    if (!sink.writeExact(8u, ifd.data(), ifd.size())) {
        return Status::error(StatusCode::SinkFailed, "cannot write output IFD");
    }

    static constexpr std::array<std::uint8_t,4> zeroes{{0u,0u,0u,0u}};
    for (const auto& entry : entries) {
        if (entry.payload.size() <= 4u) continue;
        if (!sink.writeExact(entry.externalOffset, entry.payload.data(), entry.payload.size())) {
            return Status::error(StatusCode::SinkFailed, "cannot write output tag payload");
        }
    }
    const std::uint64_t auxEnd = [&]() {
        std::uint64_t end = auxOffset64;
        for (const auto& entry : entries) {
            if (entry.payload.size() > 4u) {
                end = std::max<std::uint64_t>(end,
                    static_cast<std::uint64_t>(entry.externalOffset) + entry.payload.size());
            }
        }
        return end;
    }();
    if (auxEnd < pixelOffset) {
        const std::size_t padding = static_cast<std::size_t>(pixelOffset - auxEnd);
        if (padding > zeroes.size() || !sink.writeExact(auxEnd, zeroes.data(), padding)) {
            return Status::error(StatusCode::SinkFailed, "cannot write DNG alignment padding");
        }
    }

    pixelOffsetOut = pixelOffset;
    totalBytesOut = totalBytes;
    pixelBytesOut = pixelBytes;
    return Status::ok();
}

bool nonzero_hash(const scientific_master_streaming_binding::v0_2::Hash256& hash) noexcept {
    for (const auto value : hash) if (value != 0u) return true;
    return false;
}

Status check_release(const finalized_scientific_preview_release::v0_2::ReleaseResult& finalized) noexcept {
    if (finalized.authority == PreviewAuthority::None) {
        return Status::error(StatusCode::FinalizedReleaseRequired,
                             "LinearRaw projection requires a finalized Scientific Preview admission");
    }
    if (!nonzero_hash(finalized.scientificIdentity.scientificMasterHash)) {
        return Status::error(StatusCode::FinalizedReleaseRequired,
                             "LinearRaw projection requires a real Scientific Master digest");
    }
    if (finalized.scientificIdentity.physicalFrameCount != 1u ||
        finalized.scientificIdentity.independentEvidenceCount != 1u ||
        finalized.streaming.provenance.physicalFrameCount != 1u ||
        finalized.streaming.provenance.independentEvidenceCount != 1u ||
        finalized.streaming.provenance.scientificMasterModifiedByAppearance ||
        finalized.streaming.provenance.counterfactualObservationCreated) {
        return Status::error(StatusCode::EvidenceInvariantViolation,
                             "finalized frame/evidence/provenance invariant rejected for DNG projection");
    }
    return Status::ok();
}

std::uint16_t quantize_linear(float value,
                              std::uint64_t& clippedLow,
                              std::uint64_t& clippedHigh,
                              bool& finite) noexcept {
    if (!std::isfinite(value)) {
        finite = false;
        return 0u;
    }
    if (value < 0.0f) {
        ++clippedLow;
        return 0u;
    }
    if (value > 1.0f) {
        ++clippedHigh;
        return 65535u;
    }
    const double scaled = static_cast<double>(value) * 65535.0;
    const auto rounded = static_cast<std::uint32_t>(std::floor(scaled + 0.5));
    return static_cast<std::uint16_t>(std::min<std::uint32_t>(rounded, 65535u));
}

} // namespace

bool PosixFdByteSink::resize(std::uint64_t bytes) {
    if (fd_ < 0 || bytes > static_cast<std::uint64_t>(std::numeric_limits<off_t>::max())) return false;
    return ::ftruncate(fd_, static_cast<off_t>(bytes)) == 0;
}

bool PosixFdByteSink::writeExact(std::uint64_t offset, const void* src, std::size_t bytes) {
    if (fd_ < 0 || src == nullptr || offset > static_cast<std::uint64_t>(std::numeric_limits<off_t>::max())) return false;
    const auto* p = static_cast<const std::uint8_t*>(src);
    std::size_t written = 0u;
    while (written < bytes) {
        const std::uint64_t current = offset + written;
        if (current > static_cast<std::uint64_t>(std::numeric_limits<off_t>::max())) return false;
        const ssize_t n = ::pwrite(fd_, p + written, bytes - written, static_cast<off_t>(current));
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0) return false;
        written += static_cast<std::size_t>(n);
    }
    return true;
}

Status write_finalized_linear_dng(
    const finalized_scientific_preview_release::v0_2::ReleaseResult& finalized,
    tile_dng_v0_1::IRandomAccessByteSource& sealedSourceBytes,
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    IRandomAccessByteSink& sink,
    const Options& options,
    Result& out) noexcept {
    out = {};
    auto releaseStatus = check_release(finalized);
    if (!releaseStatus) return releaseStatus;

    const auto& metadata = source.metadata();
    if (metadata.width <= 1 || metadata.height <= 1 || reconstruction.requiredHalo() < 0) {
        return Status::error(StatusCode::InvalidArgument, "invalid source/reconstruction for LinearRaw projection");
    }

    bool little = true;
    std::vector<Entry> entries;
    auto metadataStatus = copy_source_color_tags(sealedSourceBytes, little, entries);
    if (!metadataStatus) return metadataStatus;

    std::uint32_t pixelOffset = 0u;
    std::uint64_t totalBytes = 0u;
    std::uint64_t pixelBytes = 0u;
    auto headerStatus = write_container_header(
        sink, little, metadata, entries, pixelOffset, totalBytes, pixelBytes);
    if (!headerStatus) return headerStatus;

    const int halo = reconstruction.requiredHalo();
    Workspace workspace{};
    std::vector<std::uint8_t> rowBytes;
    std::size_t workspacePeak = 0u;
    std::size_t residentPeak = 0u;
    std::uint64_t tilesWritten = 0u;
    std::uint64_t clippedLow = 0u;
    std::uint64_t clippedHigh = 0u;

    for (int y0 = 0; y0 < metadata.height; y0 += kCanonicalCore) {
        const int y1 = std::min(metadata.height, y0 + kCanonicalCore);
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
                                     "Stage-2 source read failed during LinearRaw projection: " + filled.message);
            }
            const int tileWidth = tile.hx1 - tile.hx0;
            const int tileHeight = tile.hy1 - tile.hy0;
            const int coreWidth = tile.x1 - tile.x0;
            const int coreHeight = tile.y1 - tile.y0;
            const std::size_t corePixels = static_cast<std::size_t>(coreWidth) * static_cast<std::size_t>(coreHeight);
            workspace.cam.resize(corePixels * 3u);

            const auto reconstructed = reconstruction.reconstructTile(
                workspace.stage2.data(), tileWidth, tileHeight,
                tile.hx0, tile.hy0,
                tile.x0, tile.y0, coreWidth, coreHeight,
                metadata.cfa, workspace.cam.data());
            if (!reconstructed) {
                return Status::error(StatusCode::ReconstructionFailed,
                                     "camera-native reconstruction failed during LinearRaw projection: " +
                                         reconstructed.message);
            }

            rowBytes.resize(static_cast<std::size_t>(coreWidth) * 3u * 2u);
            for (int row = 0; row < coreHeight; ++row) {
                bool finite = true;
                for (int x = 0; x < coreWidth; ++x) {
                    const std::size_t pixel = static_cast<std::size_t>(row) * static_cast<std::size_t>(coreWidth) +
                                              static_cast<std::size_t>(x);
                    for (int channel = 0; channel < 3; ++channel) {
                        const auto value = workspace.cam[3u * pixel + static_cast<std::size_t>(channel)];
                        const std::uint16_t q = quantize_linear(value, clippedLow, clippedHigh, finite);
                        put16(rowBytes.data() + (3u * static_cast<std::size_t>(x) +
                                                static_cast<std::size_t>(channel)) * 2u,
                              q, little);
                    }
                }
                if (!finite) {
                    return Status::error(StatusCode::NonFiniteScientificSample,
                                         "non-finite reconstructed camera-native RGB sample cannot be projected");
                }
                const std::uint64_t globalY = static_cast<std::uint64_t>(y0 + row);
                const std::uint64_t globalX = static_cast<std::uint64_t>(x0);
                const std::uint64_t pixelIndex = globalY * static_cast<std::uint64_t>(metadata.width) + globalX;
                const std::uint64_t destination = static_cast<std::uint64_t>(pixelOffset) + pixelIndex * 6u;
                if (!sink.writeExact(destination, rowBytes.data(), rowBytes.size())) {
                    return Status::error(StatusCode::SinkFailed, "cannot write LinearRaw pixel row");
                }
            }

            ++tilesWritten;
            workspacePeak = std::max(workspacePeak, vector_bytes(workspace) + rowBytes.capacity());
            std::uint64_t resident64 = static_cast<std::uint64_t>(source.residentBytesUpperBound()) +
                                       static_cast<std::uint64_t>(sink.residentBytesUpperBound()) +
                                       static_cast<std::uint64_t>(workspacePeak);
            if (resident64 > std::numeric_limits<std::size_t>::max()) {
                return Status::error(StatusCode::BudgetExceeded, "LinearRaw resident accounting overflow");
            }
            residentPeak = std::max(residentPeak, static_cast<std::size_t>(resident64));
            if (options.memoryBudgetBytes != 0u && residentPeak > options.memoryBudgetBytes) {
                return Status::error(StatusCode::BudgetExceeded, "LinearRaw projection exceeds caller memory budget");
            }
        }
    }

    Result result;
    result.width = static_cast<std::uint32_t>(metadata.width);
    result.height = static_cast<std::uint32_t>(metadata.height);
    result.outputBytes = totalBytes;
    result.pixelPayloadBytes = pixelBytes;
    result.tilesWritten = tilesWritten;
    result.samplesClippedLow = clippedLow;
    result.samplesClippedHigh = clippedHigh;
    result.logicalWorkspacePeakBytes = workspacePeak;
    result.logicalResidentUpperBound = residentPeak;
    result.linearRawPhotometric = true;
    result.boundedUnsigned16Projection = true;
    result.sourceColorMetadataCopied = true;
    result.fullScientificMasterMaterialized = false;
    result.physicalFrameCount = finalized.scientificIdentity.physicalFrameCount;
    result.independentEvidenceCount = finalized.scientificIdentity.independentEvidenceCount;
    out = result;
    return Status::ok();
}

const char* status_name(StatusCode code) noexcept {
    switch (code) {
        case StatusCode::Ok: return "OK";
        case StatusCode::InvalidArgument: return "INVALID_ARGUMENT";
        case StatusCode::FinalizedReleaseRequired: return "FINALIZED_RELEASE_REQUIRED";
        case StatusCode::EvidenceInvariantViolation: return "EVIDENCE_INVARIANT_VIOLATION";
        case StatusCode::SourceMetadataFailed: return "SOURCE_METADATA_FAILED";
        case StatusCode::SourceReadFailed: return "SOURCE_READ_FAILED";
        case StatusCode::UnsupportedContainer: return "UNSUPPORTED_CONTAINER";
        case StatusCode::ReconstructionFailed: return "RECONSTRUCTION_FAILED";
        case StatusCode::NonFiniteScientificSample: return "NONFINITE_SCIENTIFIC_SAMPLE";
        case StatusCode::BudgetExceeded: return "BUDGET_EXCEEDED";
        case StatusCode::OutputTooLarge: return "OUTPUT_TOO_LARGE";
        case StatusCode::SinkFailed: return "SINK_FAILED";
    }
    return "UNKNOWN";
}

} // namespace truthraw::linear_dng_projection::v0_1
