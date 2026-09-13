#include "linear_dng_preview_container_v0_3.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace truthraw::linear_dng_preview_container::v0_3 {
namespace {

constexpr std::uint16_t kClassicTiffMagic = 42u;
constexpr std::uint16_t kTiffAscii = 2u;
constexpr std::uint16_t kTiffShort = 3u;
constexpr std::uint16_t kTiffLong = 4u;
constexpr std::uint16_t kTagNewSubFileType = 254u;
constexpr std::uint16_t kTagImageWidth = 256u;
constexpr std::uint16_t kTagImageLength = 257u;
constexpr std::uint16_t kTagBitsPerSample = 258u;
constexpr std::uint16_t kTagCompression = 259u;
constexpr std::uint16_t kTagPhotometricInterpretation = 262u;
constexpr std::uint16_t kTagStripOffsets = 273u;
constexpr std::uint16_t kTagOrientation = 274u;
constexpr std::uint16_t kTagSamplesPerPixel = 277u;
constexpr std::uint16_t kTagRowsPerStrip = 278u;
constexpr std::uint16_t kTagStripByteCounts = 279u;
constexpr std::uint16_t kTagPlanarConfiguration = 284u;
constexpr std::uint16_t kTagSoftware = 305u;
constexpr std::uint16_t kTagSubIfds = 330u;
constexpr std::uint16_t kTagYCbCrSubSampling = 530u;
constexpr std::uint16_t kTagYCbCrPositioning = 531u;
constexpr std::uint16_t kTagPreviewApplicationName = 50966u;
constexpr std::uint16_t kTagPreviewApplicationVersion = 50967u;
constexpr std::uint16_t kTagPreviewSettingsName = 50968u;
constexpr std::uint16_t kTagPreviewColorSpace = 50970u;
constexpr std::uint32_t kPreviewColorSpaceSrgb = 2u;
constexpr std::uint32_t kMaxIfdEntries = 512u;
constexpr std::uint64_t kIfdInsertionBytes = 12u;
constexpr std::size_t kPreviewEntryCount = 19u;
constexpr std::size_t kCopyChunk = 64u * 1024u;
constexpr char kSoftware[] = "TruthRaw Linear DNG Preview v0.3";
constexpr char kPreviewApplication[] = "TruthRaw";
constexpr char kPreviewVersion[] = "0.3";
constexpr char kPreviewSettings[] = "FINALIZED_SCIENTIFIC_PREVIEW";

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

std::uint16_t dec16be(const std::uint8_t* p) noexcept {
    return static_cast<std::uint16_t>(static_cast<std::uint16_t>(p[0]) << 8u) |
           static_cast<std::uint16_t>(p[1]);
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

std::uint64_t type_size(std::uint16_t type) noexcept {
    switch (type) {
        case 1u: case 2u: case 6u: case 7u: return 1u;
        case 3u: case 8u: return 2u;
        case 4u: case 9u: case 11u: return 4u;
        case 5u: case 10u: case 12u: return 8u;
        default: return 0u;
    }
}

bool add_ok(std::uint64_t a, std::uint64_t b, std::uint64_t& out) noexcept {
    if (a > std::numeric_limits<std::uint64_t>::max() - b) return false;
    out = a + b;
    return true;
}

std::uint64_t align4(std::uint64_t value) noexcept {
    return (value + 3u) & ~std::uint64_t{3u};
}

bool read_exact(tile_dng_v0_1::IRandomAccessByteSource& source,
                std::uint64_t offset,
                void* dst,
                std::size_t bytes) noexcept {
    if (offset > source.sizeBytes() || bytes > source.sizeBytes() - offset) return false;
    return source.readExact(offset, dst, bytes);
}

struct JpegInfo final {
    int width = 0;
    int height = 0;
    std::uint16_t ySubsamplingH = 0u;
    std::uint16_t ySubsamplingV = 0u;
};

Status validate_baseline_jpeg(const PreviewJpeg& preview,
                              const Options& options,
                              JpegInfo& out) noexcept {
    out = {};
    if (preview.bytes == nullptr || preview.width <= 0 || preview.height <= 0) {
        return Status::error(StatusCode::InvalidArgument, "preview source/dimensions are invalid");
    }
    if (options.maxPreviewLongEdge <= 0 ||
        std::max(preview.width, preview.height) > options.maxPreviewLongEdge) {
        return Status::error(StatusCode::PreviewTooLarge, "preview exceeds configured portable-preview edge");
    }
    const std::uint64_t byteCount = preview.bytes->sizeBytes();
    if (byteCount < 16u || options.maxPreviewJpegBytes == 0u ||
        byteCount > options.maxPreviewJpegBytes ||
        byteCount > std::numeric_limits<std::uint32_t>::max()) {
        return Status::error(StatusCode::PreviewTooLarge, "preview JPEG byte length is outside contract");
    }

    std::array<std::uint8_t,2> pair{};
    if (!read_exact(*preview.bytes, 0u, pair.data(), pair.size())) {
        return Status::error(StatusCode::PreviewReadFailed, "cannot read preview JPEG SOI");
    }
    if (pair[0] != 0xffu || pair[1] != 0xd8u) {
        return Status::error(StatusCode::PreviewJpegInvalid, "preview is not a JPEG SOI stream");
    }
    if (!read_exact(*preview.bytes, byteCount - 2u, pair.data(), pair.size())) {
        return Status::error(StatusCode::PreviewReadFailed, "cannot read preview JPEG EOI");
    }
    if (pair[0] != 0xffu || pair[1] != 0xd9u) {
        return Status::error(StatusCode::PreviewJpegInvalid, "preview JPEG is truncated or lacks EOI");
    }

    bool sof0Seen = false;
    std::uint64_t cursor = 2u;
    while (cursor + 4u <= byteCount) {
        std::uint8_t markerByte = 0u;
        if (!read_exact(*preview.bytes, cursor, &markerByte, 1u)) {
            return Status::error(StatusCode::PreviewReadFailed, "cannot read JPEG marker prefix");
        }
        if (markerByte != 0xffu) {
            return Status::error(StatusCode::PreviewJpegInvalid, "unexpected byte before JPEG scan data");
        }
        do {
            ++cursor;
            if (cursor >= byteCount) {
                return Status::error(StatusCode::PreviewJpegInvalid, "unterminated JPEG marker");
            }
            if (!read_exact(*preview.bytes, cursor, &markerByte, 1u)) {
                return Status::error(StatusCode::PreviewReadFailed, "cannot read JPEG marker");
            }
        } while (markerByte == 0xffu);
        const std::uint8_t marker = markerByte;
        ++cursor;

        if (marker == 0xd9u) break;
        if (marker == 0xdau) {
            if (!sof0Seen) {
                return Status::error(StatusCode::PreviewJpegInvalid, "JPEG scan begins before baseline SOF0");
            }
            break;
        }
        if (marker == 0x01u || (marker >= 0xd0u && marker <= 0xd7u)) continue;

        std::array<std::uint8_t,2> lenBytes{};
        if (!read_exact(*preview.bytes, cursor, lenBytes.data(), lenBytes.size())) {
            return Status::error(StatusCode::PreviewReadFailed, "cannot read JPEG segment length");
        }
        const std::uint16_t segmentLength = dec16be(lenBytes.data());
        if (segmentLength < 2u || cursor + segmentLength > byteCount) {
            return Status::error(StatusCode::PreviewJpegInvalid, "JPEG segment length is invalid");
        }

        const bool isSof = (marker >= 0xc0u && marker <= 0xcfu) &&
                           marker != 0xc4u && marker != 0xc8u && marker != 0xccu;
        if (isSof && marker != 0xc0u) {
            return Status::error(StatusCode::PreviewJpegInvalid, "preview JPEG is not baseline SOF0 DCT");
        }
        if (marker == 0xc0u) {
            if (segmentLength < 17u) {
                return Status::error(StatusCode::PreviewJpegInvalid,
                                     "baseline JPEG SOF0 is too short for three components");
            }
            std::array<std::uint8_t,15> sof{};
            if (!read_exact(*preview.bytes, cursor + 2u, sof.data(), sof.size())) {
                return Status::error(StatusCode::PreviewReadFailed, "cannot read baseline JPEG SOF0");
            }
            const int precision = sof[0];
            const int height = dec16be(sof.data() + 1u);
            const int width = dec16be(sof.data() + 3u);
            const int components = sof[5];
            if (precision != 8 || width != preview.width || height != preview.height || components != 3) {
                return Status::error(StatusCode::PreviewJpegInvalid,
                                     "baseline JPEG dimensions/components do not match preview contract");
            }
            const std::uint8_t ySampling = sof[7];
            const std::uint8_t cbSampling = sof[10];
            const std::uint8_t crSampling = sof[13];
            const std::uint16_t yh = static_cast<std::uint16_t>((ySampling >> 4u) & 0x0fu);
            const std::uint16_t yv = static_cast<std::uint16_t>(ySampling & 0x0fu);
            const std::uint16_t cbh = static_cast<std::uint16_t>((cbSampling >> 4u) & 0x0fu);
            const std::uint16_t cbv = static_cast<std::uint16_t>(cbSampling & 0x0fu);
            const std::uint16_t crh = static_cast<std::uint16_t>((crSampling >> 4u) & 0x0fu);
            const std::uint16_t crv = static_cast<std::uint16_t>(crSampling & 0x0fu);
            if (yh == 0u || yv == 0u || yh > 4u || yv > 4u ||
                cbh != 1u || cbv != 1u || crh != 1u || crv != 1u) {
                return Status::error(StatusCode::PreviewJpegInvalid,
                                     "unsupported baseline JPEG chroma subsampling layout");
            }
            out.width = width;
            out.height = height;
            out.ySubsamplingH = yh;
            out.ySubsamplingV = yv;
            sof0Seen = true;
        }
        cursor += segmentLength;
    }
    if (!sof0Seen) {
        return Status::error(StatusCode::PreviewJpegInvalid, "baseline JPEG SOF0 was not found");
    }
    return Status::ok();
}

struct PreviewEntry final {
    std::uint16_t tag = 0u;
    std::uint16_t type = 0u;
    std::uint32_t count = 0u;
    std::vector<std::uint8_t> payload;
    std::uint32_t externalOffset = 0u;
};

std::vector<std::uint8_t> ascii_payload(const char* text) {
    const std::size_t n = std::strlen(text);
    std::vector<std::uint8_t> out(text, text + n);
    out.push_back(0u);
    return out;
}

std::vector<std::uint8_t> short_payload(bool little,
                                        std::initializer_list<std::uint16_t> values) {
    std::vector<std::uint8_t> out(values.size() * 2u, 0u);
    std::size_t offset = 0u;
    for (const auto value : values) {
        put16(out.data() + offset, value, little);
        offset += 2u;
    }
    return out;
}

std::vector<std::uint8_t> long_payload(bool little,
                                       std::initializer_list<std::uint32_t> values) {
    std::vector<std::uint8_t> out(values.size() * 4u, 0u);
    std::size_t offset = 0u;
    for (const auto value : values) {
        put32(out.data() + offset, value, little);
        offset += 4u;
    }
    return out;
}

void add_preview_entry(std::vector<PreviewEntry>& entries,
                       std::uint16_t tag,
                       std::uint16_t type,
                       std::uint32_t count,
                       std::vector<std::uint8_t> payload) {
    entries.push_back(PreviewEntry{tag, type, count, std::move(payload), 0u});
}

class PreviewInjectingSink final : public linear_dng_projection::v0_1::IRandomAccessByteSink {
public:
    PreviewInjectingSink(linear_dng_projection::v0_1::IRandomAccessByteSink& sink,
                         tile_dng_v0_1::IRandomAccessByteSource& jpeg,
                         int previewWidth,
                         int previewHeight,
                         std::uint16_t subsamplingH,
                         std::uint16_t subsamplingV) noexcept
        : sink_(sink), jpeg_(jpeg), previewWidth_(previewWidth), previewHeight_(previewHeight),
          subsamplingH_(subsamplingH), subsamplingV_(subsamplingV) {}

    bool resize(std::uint64_t bytes) override {
        if (bytes == 0u) {
            reset();
            return sink_.resize(0u);
        }
        baseBytes_ = bytes;
        std::uint64_t expandedRawEnd = 0u;
        if (!add_ok(baseBytes_, kIfdInsertionBytes, expandedRawEnd)) return fail();
        expandedRawEnd_ = expandedRawEnd;
        const std::uint64_t previewIfd = align4(expandedRawEnd_);
        if (previewIfd > std::numeric_limits<std::uint32_t>::max()) return fail();
        previewIfdOffset_ = static_cast<std::uint32_t>(previewIfd);

        previewIfdBytes_ = 2u + 12u * kPreviewEntryCount + 4u;
        std::uint64_t cursor = previewIfd + previewIfdBytes_;
        const auto accountPayload = [&cursor](std::size_t size) -> bool {
            if (size <= 4u) return true;
            if ((cursor & 1u) != 0u) ++cursor;
            return add_ok(cursor, size, cursor);
        };
        if (!accountPayload(6u) ||
            !accountPayload(sizeof(kSoftware)) ||
            !accountPayload(sizeof(kPreviewApplication)) ||
            !accountPayload(sizeof(kPreviewVersion)) ||
            !accountPayload(sizeof(kPreviewSettings))) {
            return fail();
        }
        cursor = align4(cursor);
        if (cursor > std::numeric_limits<std::uint32_t>::max()) return fail();
        previewJpegOffset_ = static_cast<std::uint32_t>(cursor);
        std::uint64_t finalBytes = 0u;
        if (!add_ok(cursor, jpeg_.sizeBytes(), finalBytes) ||
            finalBytes > std::numeric_limits<std::uint32_t>::max()) {
            return fail();
        }
        finalBytes_ = finalBytes;
        resized_ = sink_.resize(finalBytes_);
        if (!resized_) failed_ = true;
        return resized_;
    }

    bool writeExact(std::uint64_t offset, const void* src, std::size_t bytes) override {
        if (failed_ || !resized_ || src == nullptr) return false;
        if (offset == 0u) {
            if (bytes != 8u) return fail();
            const auto* p = static_cast<const std::uint8_t*>(src);
            if (p[0] == 'I' && p[1] == 'I') little_ = true;
            else if (p[0] == 'M' && p[1] == 'M') little_ = false;
            else return fail();
            if (dec16(p + 2u, little_) != kClassicTiffMagic ||
                dec32(p + 4u, little_) != 8u) return fail();
            headerSeen_ = true;
            return sink_.writeExact(0u, src, bytes);
        }
        if (offset == 8u) {
            if (!headerSeen_ || ifdSeen_) return fail();
            return write_expanded_raw_ifd(src, bytes);
        }
        if (!ifdSeen_ || offset < 8u + oldIfdBytes_) return fail();
        return sink_.writeExact(offset + kIfdInsertionBytes, src, bytes);
    }

    std::size_t residentBytesUpperBound() const override {
        const std::size_t local = sizeof(*this) + kCopyChunk;
        const std::size_t base = sink_.residentBytesUpperBound();
        if (base > std::numeric_limits<std::size_t>::max() - local) {
            return std::numeric_limits<std::size_t>::max();
        }
        return base + local;
    }

    bool finish_preview() noexcept {
        if (failed_ || !resized_ || !headerSeen_ || !ifdSeen_) return false;
        if (!zero_range(expandedRawEnd_, previewJpegOffset_)) return fail();
        if (!write_preview_ifd()) return fail();

        std::vector<std::uint8_t> buffer(kCopyChunk, 0u);
        std::uint64_t copied = 0u;
        while (copied < jpeg_.sizeBytes()) {
            const std::uint64_t remaining = jpeg_.sizeBytes() - copied;
            const std::size_t chunk = static_cast<std::size_t>(
                std::min<std::uint64_t>(remaining, buffer.size()));
            if (!jpeg_.readExact(copied, buffer.data(), chunk)) return fail();
            if (!sink_.writeExact(static_cast<std::uint64_t>(previewJpegOffset_) + copied,
                                  buffer.data(), chunk)) return fail();
            copied += chunk;
        }
        previewFinished_ = copied == jpeg_.sizeBytes();
        return previewFinished_;
    }

    bool complete() const noexcept { return !failed_ && previewFinished_; }
    std::uint64_t finalBytes() const noexcept { return finalBytes_; }
    std::uint32_t previewIfdOffset() const noexcept { return previewIfdOffset_; }
    std::uint32_t previewJpegOffset() const noexcept { return previewJpegOffset_; }

private:
    bool fail() noexcept {
        failed_ = true;
        return false;
    }

    void reset() noexcept {
        resized_ = false;
        headerSeen_ = false;
        ifdSeen_ = false;
        previewFinished_ = false;
        failed_ = false;
        baseBytes_ = 0u;
        expandedRawEnd_ = 0u;
        finalBytes_ = 0u;
        oldIfdBytes_ = 0u;
        previewIfdBytes_ = 0u;
        previewIfdOffset_ = 0u;
        previewJpegOffset_ = 0u;
    }

    bool zero_range(std::uint64_t begin, std::uint64_t end) noexcept {
        if (end < begin) return false;
        std::array<std::uint8_t,4096> zeroes{};
        std::uint64_t cursor = begin;
        while (cursor < end) {
            const std::uint64_t remaining = end - cursor;
            const std::size_t chunk = static_cast<std::size_t>(
                std::min<std::uint64_t>(remaining, zeroes.size()));
            if (!sink_.writeExact(cursor, zeroes.data(), chunk)) return false;
            cursor += chunk;
        }
        return true;
    }

    bool write_expanded_raw_ifd(const void* src, std::size_t bytes) noexcept {
        if (bytes < 6u) return fail();
        const auto* p = static_cast<const std::uint8_t*>(src);
        const std::uint16_t count = dec16(p, little_);
        if (count == 0u || count >= kMaxIfdEntries) return fail();
        const std::size_t expected = 2u + 12u * static_cast<std::size_t>(count) + 4u;
        if (bytes != expected) return fail();
        oldIfdBytes_ = bytes;

        struct RawEntry final {
            std::array<std::uint8_t,12> bytes{};
            std::uint16_t tag = 0u;
        };
        std::vector<RawEntry> entries;
        entries.reserve(static_cast<std::size_t>(count) + 1u);
        bool subIfdAlreadyPresent = false;

        for (std::uint16_t i = 0; i < count; ++i) {
            RawEntry e;
            std::memcpy(e.bytes.data(), p + 2u + 12u * i, 12u);
            e.tag = dec16(e.bytes.data(), little_);
            if (e.tag == kTagSubIfds) subIfdAlreadyPresent = true;

            const std::uint16_t type = dec16(e.bytes.data() + 2u, little_);
            const std::uint32_t itemCount = dec32(e.bytes.data() + 4u, little_);
            const std::uint64_t itemSize = type_size(type);
            if (itemSize == 0u || itemCount == 0u ||
                itemSize > std::numeric_limits<std::uint64_t>::max() / itemCount) return fail();
            const std::uint64_t payloadBytes = itemSize * static_cast<std::uint64_t>(itemCount);
            if (payloadBytes > 4u) {
                const std::uint32_t oldOffset = dec32(e.bytes.data() + 8u, little_);
                if (oldOffset == 0u ||
                    oldOffset > std::numeric_limits<std::uint32_t>::max() - kIfdInsertionBytes) return fail();
                put32(e.bytes.data() + 8u,
                      static_cast<std::uint32_t>(oldOffset + kIfdInsertionBytes), little_);
            } else if (e.tag == kTagStripOffsets) {
                if (type != kTiffLong || itemCount != 1u) return fail();
                const std::uint32_t oldOffset = dec32(e.bytes.data() + 8u, little_);
                if (oldOffset > std::numeric_limits<std::uint32_t>::max() - kIfdInsertionBytes) return fail();
                put32(e.bytes.data() + 8u,
                      static_cast<std::uint32_t>(oldOffset + kIfdInsertionBytes), little_);
            }
            entries.push_back(e);
        }
        if (subIfdAlreadyPresent) return fail();

        RawEntry sub;
        sub.tag = kTagSubIfds;
        put16(sub.bytes.data(), kTagSubIfds, little_);
        put16(sub.bytes.data() + 2u, kTiffLong, little_);
        put32(sub.bytes.data() + 4u, 1u, little_);
        put32(sub.bytes.data() + 8u, previewIfdOffset_, little_);
        entries.push_back(sub);
        std::sort(entries.begin(), entries.end(),
                  [](const RawEntry& a, const RawEntry& b) { return a.tag < b.tag; });
        for (std::size_t i = 1; i < entries.size(); ++i) {
            if (entries[i-1].tag == entries[i].tag) return fail();
        }

        std::vector<std::uint8_t> expanded(bytes + kIfdInsertionBytes, 0u);
        put16(expanded.data(), static_cast<std::uint16_t>(entries.size()), little_);
        for (std::size_t i = 0; i < entries.size(); ++i) {
            std::memcpy(expanded.data() + 2u + 12u * i, entries[i].bytes.data(), 12u);
        }
        if (dec32(p + bytes - 4u, little_) != 0u) return fail();
        put32(expanded.data() + expanded.size() - 4u, 0u, little_);
        ifdSeen_ = sink_.writeExact(8u, expanded.data(), expanded.size());
        if (!ifdSeen_) failed_ = true;
        return ifdSeen_;
    }

    bool write_preview_ifd() noexcept {
        std::vector<PreviewEntry> entries;
        add_preview_entry(entries, kTagNewSubFileType, kTiffLong, 1u,
                          long_payload(little_, {1u}));
        add_preview_entry(entries, kTagImageWidth, kTiffLong, 1u,
                          long_payload(little_, {static_cast<std::uint32_t>(previewWidth_)}));
        add_preview_entry(entries, kTagImageLength, kTiffLong, 1u,
                          long_payload(little_, {static_cast<std::uint32_t>(previewHeight_)}));
        add_preview_entry(entries, kTagBitsPerSample, kTiffShort, 3u,
                          short_payload(little_, {8u,8u,8u}));
        add_preview_entry(entries, kTagCompression, kTiffShort, 1u,
                          short_payload(little_, {7u}));
        add_preview_entry(entries, kTagPhotometricInterpretation, kTiffShort, 1u,
                          short_payload(little_, {6u}));
        add_preview_entry(entries, kTagStripOffsets, kTiffLong, 1u,
                          long_payload(little_, {previewJpegOffset_}));
        add_preview_entry(entries, kTagOrientation, kTiffShort, 1u,
                          short_payload(little_, {1u}));
        add_preview_entry(entries, kTagSamplesPerPixel, kTiffShort, 1u,
                          short_payload(little_, {3u}));
        add_preview_entry(entries, kTagRowsPerStrip, kTiffLong, 1u,
                          long_payload(little_, {static_cast<std::uint32_t>(previewHeight_)}));
        add_preview_entry(entries, kTagStripByteCounts, kTiffLong, 1u,
                          long_payload(little_, {static_cast<std::uint32_t>(jpeg_.sizeBytes())}));
        add_preview_entry(entries, kTagPlanarConfiguration, kTiffShort, 1u,
                          short_payload(little_, {1u}));

        auto software = ascii_payload(kSoftware);
        add_preview_entry(entries, kTagSoftware, kTiffAscii,
                          static_cast<std::uint32_t>(software.size()), std::move(software));
        add_preview_entry(entries, kTagYCbCrSubSampling, kTiffShort, 2u,
                          short_payload(little_, {subsamplingH_, subsamplingV_}));
        add_preview_entry(entries, kTagYCbCrPositioning, kTiffShort, 1u,
                          short_payload(little_, {1u}));

        auto application = ascii_payload(kPreviewApplication);
        add_preview_entry(entries, kTagPreviewApplicationName, kTiffAscii,
                          static_cast<std::uint32_t>(application.size()), std::move(application));
        auto version = ascii_payload(kPreviewVersion);
        add_preview_entry(entries, kTagPreviewApplicationVersion, kTiffAscii,
                          static_cast<std::uint32_t>(version.size()), std::move(version));
        auto settings = ascii_payload(kPreviewSettings);
        add_preview_entry(entries, kTagPreviewSettingsName, kTiffAscii,
                          static_cast<std::uint32_t>(settings.size()), std::move(settings));
        add_preview_entry(entries, kTagPreviewColorSpace, kTiffLong, 1u,
                          long_payload(little_, {kPreviewColorSpaceSrgb}));

        if (entries.size() != kPreviewEntryCount) return false;
        std::sort(entries.begin(), entries.end(),
                  [](const PreviewEntry& a, const PreviewEntry& b) { return a.tag < b.tag; });

        const std::uint64_t ifdBytes = 2u + 12u * entries.size() + 4u;
        if (ifdBytes != previewIfdBytes_) return false;
        std::uint64_t cursor = static_cast<std::uint64_t>(previewIfdOffset_) + ifdBytes;
        for (auto& entry : entries) {
            if (entry.payload.size() <= 4u) continue;
            if ((cursor & 1u) != 0u) ++cursor;
            if (cursor > std::numeric_limits<std::uint32_t>::max()) return false;
            entry.externalOffset = static_cast<std::uint32_t>(cursor);
            if (!add_ok(cursor, entry.payload.size(), cursor)) return false;
        }
        cursor = align4(cursor);
        if (cursor != previewJpegOffset_) return false;

        std::vector<std::uint8_t> ifd(static_cast<std::size_t>(ifdBytes), 0u);
        put16(ifd.data(), static_cast<std::uint16_t>(entries.size()), little_);
        for (std::size_t i = 0; i < entries.size(); ++i) {
            const auto& entry = entries[i];
            auto* dst = ifd.data() + 2u + 12u * i;
            put16(dst, entry.tag, little_);
            put16(dst + 2u, entry.type, little_);
            put32(dst + 4u, entry.count, little_);
            if (entry.payload.size() <= 4u) {
                std::copy(entry.payload.begin(), entry.payload.end(), dst + 8u);
            } else {
                put32(dst + 8u, entry.externalOffset, little_);
            }
        }
        put32(ifd.data() + ifd.size() - 4u, 0u, little_);
        if (!sink_.writeExact(previewIfdOffset_, ifd.data(), ifd.size())) return false;
        for (const auto& entry : entries) {
            if (entry.payload.size() <= 4u) continue;
            if (!sink_.writeExact(entry.externalOffset,
                                  entry.payload.data(), entry.payload.size())) return false;
        }
        return true;
    }

    linear_dng_projection::v0_1::IRandomAccessByteSink& sink_;
    tile_dng_v0_1::IRandomAccessByteSource& jpeg_;
    int previewWidth_ = 0;
    int previewHeight_ = 0;
    std::uint16_t subsamplingH_ = 0u;
    std::uint16_t subsamplingV_ = 0u;
    bool little_ = true;
    bool resized_ = false;
    bool headerSeen_ = false;
    bool ifdSeen_ = false;
    bool previewFinished_ = false;
    bool failed_ = false;
    std::uint64_t baseBytes_ = 0u;
    std::uint64_t expandedRawEnd_ = 0u;
    std::uint64_t finalBytes_ = 0u;
    std::size_t oldIfdBytes_ = 0u;
    std::size_t previewIfdBytes_ = 0u;
    std::uint32_t previewIfdOffset_ = 0u;
    std::uint32_t previewJpegOffset_ = 0u;
};

} // namespace

Status write_finalized_linear_dng_with_preview(
    const finalized_scientific_preview_release::v0_2::ReleaseResult& finalized,
    tile_dng_v0_1::IRandomAccessByteSource& sealedSourceBytes,
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const PreviewJpeg& preview,
    linear_dng_projection::v0_1::IRandomAccessByteSink& sink,
    const Options& options,
    Result& out) noexcept {
    out = {};
    if (options.memoryBudgetBytes == 0u) {
        return Status::error(StatusCode::InvalidArgument, "v0.3 memory budget must be non-zero");
    }

    JpegInfo jpegInfo;
    const auto jpegStatus = validate_baseline_jpeg(preview, options, jpegInfo);
    if (!jpegStatus) return jpegStatus;

    PreviewInjectingSink injectingSink(
        sink, *preview.bytes, preview.width, preview.height,
        jpegInfo.ySubsamplingH, jpegInfo.ySubsamplingV);

    linear_dng_projection::v0_2::Options v02Options;
    v02Options.memoryBudgetBytes = options.memoryBudgetBytes;
    linear_dng_projection::v0_2::Result v02Result;
    const auto v02Status = linear_dng_projection::v0_2::write_finalized_linear_dng(
        finalized, sealedSourceBytes, source, reconstruction,
        injectingSink, v02Options, v02Result);
    if (!v02Status) {
        (void)sink.resize(0u);
        return Status::error(StatusCode::UnderlyingLinearDngFailed,
                             std::string("v0.2 LinearRaw projection failed: ") + v02Status.message);
    }
    if (!injectingSink.finish_preview() || !injectingSink.complete()) {
        (void)sink.resize(0u);
        return Status::error(StatusCode::SinkFailed,
                             "failed to finalize embedded JPEG preview SubIFD");
    }

    out.linearRaw = v02Result;
    out.outputBytes = injectingSink.finalBytes();
    out.previewJpegBytes = preview.bytes->sizeBytes();
    out.previewIfdOffset = injectingSink.previewIfdOffset();
    out.previewJpegOffset = injectingSink.previewJpegOffset();
    out.previewWidth = preview.width;
    out.previewHeight = preview.height;
    out.rawIfdRemainsPrimary = true;
    out.previewIsReducedSubIfd = true;
    out.previewColorSpaceSrgb = true;
    out.previewOrientationNormalized = true;
    return Status::ok();
}

const char* status_name(StatusCode code) noexcept {
    switch (code) {
        case StatusCode::Ok: return "OK";
        case StatusCode::InvalidArgument: return "INVALID_ARGUMENT";
        case StatusCode::PreviewTooLarge: return "PREVIEW_TOO_LARGE";
        case StatusCode::PreviewReadFailed: return "PREVIEW_READ_FAILED";
        case StatusCode::PreviewJpegInvalid: return "PREVIEW_JPEG_INVALID";
        case StatusCode::UnderlyingLinearDngFailed: return "UNDERLYING_LINEAR_DNG_FAILED";
        case StatusCode::ContainerPatchFailed: return "CONTAINER_PATCH_FAILED";
        case StatusCode::SinkFailed: return "SINK_FAILED";
    }
    return "UNKNOWN";
}

} // namespace truthraw::linear_dng_preview_container::v0_3
