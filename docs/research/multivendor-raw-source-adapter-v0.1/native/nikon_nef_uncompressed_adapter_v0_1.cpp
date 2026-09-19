#include "multivendor_raw_source_adapter_v0_1.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace truthraw::multivendor_raw_source_adapter::v0_1 {
namespace {

using streaming_v0_1::IRawTileSource;
using streaming_v0_1::StreamStatus;
using streaming_v0_1::StreamStatusCode;

constexpr std::uint16_t kTypeByte = 1;
constexpr std::uint16_t kTypeAscii = 2;
constexpr std::uint16_t kTypeShort = 3;
constexpr std::uint16_t kTypeLong = 4;
constexpr std::uint16_t kTypeUndefined = 7;

constexpr std::uint16_t kTagImageWidth = 256;
constexpr std::uint16_t kTagImageLength = 257;
constexpr std::uint16_t kTagBitsPerSample = 258;
constexpr std::uint16_t kTagCompression = 259;
constexpr std::uint16_t kTagPhotometric = 262;
constexpr std::uint16_t kTagMake = 271;
constexpr std::uint16_t kTagStripOffsets = 273;
constexpr std::uint16_t kTagSamplesPerPixel = 277;
constexpr std::uint16_t kTagRowsPerStrip = 278;
constexpr std::uint16_t kTagStripByteCounts = 279;
constexpr std::uint16_t kTagSubIfds = 330;
constexpr std::uint16_t kTagCfaRepeatPatternDim = 33421;
constexpr std::uint16_t kTagCfaPattern = 33422;
constexpr std::uint16_t kPhotometricCfa = 32803;

struct Entry {
    std::uint16_t tag = 0;
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::uint32_t valueOrOffset = 0;
};

struct ParsedIfd {
    std::uint32_t offset = 0;
    std::vector<Entry> entries;
    std::uint32_t next = 0;
};

std::uint16_t le16(const std::uint8_t* p) noexcept {
    return static_cast<std::uint16_t>(p[0]) |
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(p[1]) << 8u);
}

std::uint32_t le32(const std::uint8_t* p) noexcept {
    return static_cast<std::uint32_t>(p[0]) |
        (static_cast<std::uint32_t>(p[1]) << 8u) |
        (static_cast<std::uint32_t>(p[2]) << 16u) |
        (static_cast<std::uint32_t>(p[3]) << 24u);
}

std::size_t typeSize(std::uint16_t type) noexcept {
    switch (type) {
        case kTypeByte:
        case kTypeAscii:
        case kTypeUndefined:
            return 1u;
        case kTypeShort:
            return 2u;
        case kTypeLong:
            return 4u;
        default:
            return 0u;
    }
}

const Entry* findEntry(const ParsedIfd& ifd, std::uint16_t tag) noexcept {
    const auto it = std::find_if(ifd.entries.begin(), ifd.entries.end(),
        [&](const Entry& e) { return e.tag == tag; });
    return it == ifd.entries.end() ? nullptr : &*it;
}

class NefParser {
public:
    explicit NefParser(std::shared_ptr<IRawByteSource> bytes) : bytes_(std::move(bytes)) {}

    AdapterStatus parse(
        std::string& make,
        DngMetadata& metadata,
        std::uint32_t& rowsPerStrip,
        std::vector<std::uint32_t>& stripOffsets,
        std::vector<std::uint32_t>& stripByteCounts) noexcept {
        try {
            std::array<std::uint8_t, 8> header{};
            if (!bytes_->readExact(0u, header.data(), header.size())) {
                return fail(AdapterStatusCode::DecodeFailed, "NEF TIFF header read failed");
            }
            if (header[0] != 'I' || header[1] != 'I' || le16(header.data() + 2u) != 42u) {
                return fail(AdapterStatusCode::InvalidContainer, "NEF adapter currently requires little-endian classic TIFF");
            }
            const std::uint32_t ifd0 = le32(header.data() + 4u);
            if (ifd0 < 8u || ifd0 >= bytes_->sizeBytes()) {
                return fail(AdapterStatusCode::InvalidContainer, "invalid NEF IFD0 offset");
            }

            std::vector<ParsedIfd> ifds;
            std::vector<std::uint32_t> queue{ifd0};
            for (std::size_t qi = 0; qi < queue.size() && ifds.size() < 32u; ++qi) {
                const auto offset = queue[qi];
                if (std::any_of(ifds.begin(), ifds.end(), [&](const ParsedIfd& x) { return x.offset == offset; })) {
                    continue;
                }
                ParsedIfd ifd;
                auto st = readIfd(offset, ifd);
                if (!st) return st;
                ifds.push_back(ifd);
                if (ifd.next != 0u) queue.push_back(ifd.next);

                if (const Entry* sub = findEntry(ifd, kTagSubIfds)) {
                    std::vector<std::uint32_t> subs;
                    st = readUnsignedArray(*sub, subs);
                    if (!st) return st;
                    queue.insert(queue.end(), subs.begin(), subs.end());
                }
            }

            if (ifds.empty()) return fail(AdapterStatusCode::InvalidContainer, "NEF contained no readable IFD");

            if (const Entry* makeEntry = findEntry(ifds.front(), kTagMake)) {
                auto st = readAscii(*makeEntry, make);
                if (!st) return st;
            }
            if (make.find("NIKON") == std::string::npos) {
                return fail(AdapterStatusCode::InvalidContainer, "TIFF Make is not Nikon");
            }

            const ParsedIfd* raw = nullptr;
            for (const auto& ifd : ifds) {
                if (isStrictRawIfd(ifd)) {
                    if (raw != nullptr) {
                        return fail(AdapterStatusCode::UnsupportedContainerFeature,
                            "multiple strict uncompressed CFA IFD candidates");
                    }
                    raw = &ifd;
                }
            }
            if (raw == nullptr) {
                return fail(AdapterStatusCode::UnsupportedContainerFeature,
                    "no supported uncompressed 16-bit Nikon CFA IFD found");
            }

            std::uint32_t width = 0, height = 0, bits = 0, compression = 0;
            std::uint32_t samples = 0, photometric = 0, rows = 0;
            auto st = readScalar(*findEntry(*raw, kTagImageWidth), width); if (!st) return st;
            st = readScalar(*findEntry(*raw, kTagImageLength), height); if (!st) return st;
            st = readScalar(*findEntry(*raw, kTagBitsPerSample), bits); if (!st) return st;
            st = readScalar(*findEntry(*raw, kTagCompression), compression); if (!st) return st;
            st = readScalar(*findEntry(*raw, kTagSamplesPerPixel), samples); if (!st) return st;
            st = readScalar(*findEntry(*raw, kTagPhotometric), photometric); if (!st) return st;
            st = readScalar(*findEntry(*raw, kTagRowsPerStrip), rows); if (!st) return st;

            if (width < 2u || height < 2u ||
                width > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
                height > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
                bits != 16u || compression != 1u || samples != 1u ||
                photometric != kPhotometricCfa || rows == 0u) {
                return fail(AdapterStatusCode::UnsupportedContainerFeature,
                    "NEF raw IFD violates strict uncompressed CFA subset");
            }

            std::vector<std::uint32_t> repeat;
            st = readUnsignedArray(*findEntry(*raw, kTagCfaRepeatPatternDim), repeat);
            if (!st) return st;
            if (repeat.size() != 2u || repeat[0] != 2u || repeat[1] != 2u) {
                return fail(AdapterStatusCode::UnsupportedContainerFeature, "NEF CFA repeat pattern is not 2x2");
            }

            std::vector<std::uint8_t> pattern;
            st = readByteArray(*findEntry(*raw, kTagCfaPattern), pattern);
            if (!st) return st;
            if (pattern.size() != 4u) {
                return fail(AdapterStatusCode::InvalidContainer, "NEF CFA pattern cardinality is not 4");
            }

            CfaPattern cfa;
            if (pattern == std::vector<std::uint8_t>{0,1,1,2}) cfa = CfaPattern::RGGB;
            else if (pattern == std::vector<std::uint8_t>{2,1,1,0}) cfa = CfaPattern::BGGR;
            else if (pattern == std::vector<std::uint8_t>{1,0,2,1}) cfa = CfaPattern::GRBG;
            else if (pattern == std::vector<std::uint8_t>{1,2,0,1}) cfa = CfaPattern::GBRG;
            else return fail(AdapterStatusCode::UnsupportedContainerFeature, "unsupported NEF CFA pattern");

            st = readUnsignedArray(*findEntry(*raw, kTagStripOffsets), stripOffsets); if (!st) return st;
            st = readUnsignedArray(*findEntry(*raw, kTagStripByteCounts), stripByteCounts); if (!st) return st;
            if (stripOffsets.empty() || stripOffsets.size() != stripByteCounts.size()) {
                return fail(AdapterStatusCode::InvalidContainer, "NEF strip arrays invalid");
            }

            const std::uint64_t expectedStrips =
                (static_cast<std::uint64_t>(height) + rows - 1u) / rows;
            if (stripOffsets.size() != expectedStrips) {
                return fail(AdapterStatusCode::UnsupportedContainerFeature, "NEF strip count does not match rows-per-strip");
            }

            for (std::size_t i = 0; i < stripOffsets.size(); ++i) {
                const std::uint32_t y0 = static_cast<std::uint32_t>(i) * rows;
                const std::uint32_t rowCount = std::min(rows, height - y0);
                const std::uint64_t required =
                    static_cast<std::uint64_t>(rowCount) * width * 2u;
                const std::uint64_t end =
                    static_cast<std::uint64_t>(stripOffsets[i]) + stripByteCounts[i];
                if (stripByteCounts[i] < required || end > bytes_->sizeBytes()) {
                    return fail(AdapterStatusCode::InvalidContainer, "NEF strip bounds/length invalid");
                }
            }

            metadata.width = static_cast<int>(width);
            metadata.height = static_cast<int>(height);
            metadata.cfa = cfa;
            metadata.orientation = Orientation::Normal;
            metadata.whiteLevel = 65535.0f; // storage ceiling only; scientific admission remains blocked.
            metadata.blackPhase = {0.f,0.f,0.f,0.f}; // unknown, not promoted as calibration.
            metadata.hasNoiseProfile = false;
            metadata.hasGainField = false;
            metadata.hasResidualBlack = false;
            rowsPerStrip = rows;
            return AdapterStatus::ok();
        } catch (const std::bad_alloc&) {
            return fail(AdapterStatusCode::BudgetExceeded, "NEF parser allocation failed");
        } catch (...) {
            return fail(AdapterStatusCode::DecodeFailed, "unexpected NEF parser failure");
        }
    }

private:
    AdapterStatus fail(AdapterStatusCode code, std::string message) noexcept {
        return AdapterStatus::error(code, std::move(message));
    }

    AdapterStatus readIfd(std::uint32_t offset, ParsedIfd& out) noexcept {
        std::array<std::uint8_t, 2> countBytes{};
        if (!bytes_->readExact(offset, countBytes.data(), countBytes.size())) {
            return fail(AdapterStatusCode::DecodeFailed, "NEF IFD count read failed");
        }
        const std::uint16_t count = le16(countBytes.data());
        if (count == 0u || count > 512u) {
            return fail(AdapterStatusCode::InvalidContainer, "NEF IFD entry count invalid");
        }
        const std::uint64_t tableBytes = static_cast<std::uint64_t>(count) * 12u;
        const std::uint64_t end = static_cast<std::uint64_t>(offset) + 2u + tableBytes + 4u;
        if (end > bytes_->sizeBytes()) {
            return fail(AdapterStatusCode::InvalidContainer, "NEF IFD table out of bounds");
        }

        std::vector<std::uint8_t> buffer(static_cast<std::size_t>(tableBytes + 4u));
        if (!bytes_->readExact(static_cast<std::uint64_t>(offset) + 2u, buffer.data(), buffer.size())) {
            return fail(AdapterStatusCode::DecodeFailed, "NEF IFD table read failed");
        }

        out.offset = offset;
        out.entries.clear();
        out.entries.reserve(count);
        for (std::uint16_t i = 0; i < count; ++i) {
            const std::uint8_t* p = buffer.data() + static_cast<std::size_t>(i) * 12u;
            Entry e;
            e.tag = le16(p);
            e.type = le16(p + 2u);
            e.count = le32(p + 4u);
            e.valueOrOffset = le32(p + 8u);
            if (typeSize(e.type) == 0u || e.count == 0u) continue;
            out.entries.push_back(e);
        }
        out.next = le32(buffer.data() + tableBytes);
        return AdapterStatus::ok();
    }

    bool isStrictRawIfd(const ParsedIfd& ifd) const noexcept {
        return findEntry(ifd, kTagImageWidth) &&
            findEntry(ifd, kTagImageLength) &&
            findEntry(ifd, kTagBitsPerSample) &&
            findEntry(ifd, kTagCompression) &&
            findEntry(ifd, kTagPhotometric) &&
            findEntry(ifd, kTagStripOffsets) &&
            findEntry(ifd, kTagSamplesPerPixel) &&
            findEntry(ifd, kTagRowsPerStrip) &&
            findEntry(ifd, kTagStripByteCounts) &&
            findEntry(ifd, kTagCfaRepeatPatternDim) &&
            findEntry(ifd, kTagCfaPattern);
    }

    AdapterStatus readEntryBytes(const Entry& e, std::vector<std::uint8_t>& out) noexcept {
        const std::size_t scalar = typeSize(e.type);
        if (scalar == 0u || e.count > std::numeric_limits<std::size_t>::max() / scalar) {
            return fail(AdapterStatusCode::InvalidContainer, "NEF tag byte size overflow");
        }
        const std::size_t bytes = scalar * static_cast<std::size_t>(e.count);
        out.assign(bytes, 0u);
        if (bytes <= 4u) {
            const std::array<std::uint8_t, 4> inlineBytes = {
                static_cast<std::uint8_t>(e.valueOrOffset & 0xffu),
                static_cast<std::uint8_t>((e.valueOrOffset >> 8u) & 0xffu),
                static_cast<std::uint8_t>((e.valueOrOffset >> 16u) & 0xffu),
                static_cast<std::uint8_t>((e.valueOrOffset >> 24u) & 0xffu),
            };
            std::copy_n(inlineBytes.begin(), bytes, out.begin());
            return AdapterStatus::ok();
        }
        const std::uint64_t end = static_cast<std::uint64_t>(e.valueOrOffset) + bytes;
        if (end > bytes_->sizeBytes()) {
            return fail(AdapterStatusCode::InvalidContainer, "NEF tag payload out of bounds");
        }
        if (!bytes_->readExact(e.valueOrOffset, out.data(), out.size())) {
            return fail(AdapterStatusCode::DecodeFailed, "NEF tag payload read failed");
        }
        return AdapterStatus::ok();
    }

    AdapterStatus readScalar(const Entry& e, std::uint32_t& out) noexcept {
        if (e.count != 1u || (e.type != kTypeShort && e.type != kTypeLong)) {
            return fail(AdapterStatusCode::InvalidContainer, "NEF scalar tag type/count invalid");
        }
        if (e.type == kTypeShort) {
            out = e.valueOrOffset & 0xffffu;
        } else {
            out = e.valueOrOffset;
        }
        return AdapterStatus::ok();
    }

    AdapterStatus readUnsignedArray(const Entry& e, std::vector<std::uint32_t>& out) noexcept {
        if (e.type != kTypeShort && e.type != kTypeLong) {
            return fail(AdapterStatusCode::InvalidContainer, "NEF integer array type invalid");
        }
        std::vector<std::uint8_t> bytes;
        auto st = readEntryBytes(e, bytes);
        if (!st) return st;
        out.clear();
        out.reserve(e.count);
        const std::size_t step = e.type == kTypeShort ? 2u : 4u;
        for (std::uint32_t i = 0; i < e.count; ++i) {
            const auto* p = bytes.data() + static_cast<std::size_t>(i) * step;
            out.push_back(e.type == kTypeShort ? le16(p) : le32(p));
        }
        return AdapterStatus::ok();
    }

    AdapterStatus readByteArray(const Entry& e, std::vector<std::uint8_t>& out) noexcept {
        if (e.type != kTypeByte && e.type != kTypeUndefined) {
            return fail(AdapterStatusCode::InvalidContainer, "NEF byte array type invalid");
        }
        return readEntryBytes(e, out);
    }

    AdapterStatus readAscii(const Entry& e, std::string& out) noexcept {
        if (e.type != kTypeAscii) {
            return fail(AdapterStatusCode::InvalidContainer, "NEF ASCII tag type invalid");
        }
        std::vector<std::uint8_t> bytes;
        auto st = readEntryBytes(e, bytes);
        if (!st) return st;
        out.assign(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        while (!out.empty() && out.back() == '\0') out.pop_back();
        return AdapterStatus::ok();
    }

    std::shared_ptr<IRawByteSource> bytes_;
};

class NikonNefUncompressedSource final : public IRawTileSource {
public:
    NikonNefUncompressedSource(
        std::shared_ptr<IRawByteSource> bytes,
        DngMetadata metadata,
        std::uint32_t rowsPerStrip,
        std::vector<std::uint32_t> stripOffsets,
        std::vector<std::uint32_t> stripByteCounts) noexcept
        : bytes_(std::move(bytes)),
          metadata_(std::move(metadata)),
          rowsPerStrip_(rowsPerStrip),
          stripOffsets_(std::move(stripOffsets)),
          stripByteCounts_(std::move(stripByteCounts)) {}

    const DngMetadata& metadata() const override { return metadata_; }

    std::size_t residentBytesUpperBound() const override {
        return sizeof(*this) +
            bytes_->residentBytesUpperBound() +
            stripOffsets_.capacity() * sizeof(std::uint32_t) +
            stripByteCounts_.capacity() * sizeof(std::uint32_t);
    }

    StreamStatus readRawTile(
        const TileRect& rect,
        std::uint16_t* rawOut,
        std::size_t rawCount,
        float* gainOut,
        std::size_t gainCount) override {
        if (rawOut == nullptr || gainOut != nullptr || gainCount != 0u ||
            rect.hx0 < 0 || rect.hy0 < 0 || rect.hx1 > metadata_.width || rect.hy1 > metadata_.height ||
            rect.hx0 >= rect.hx1 || rect.hy0 >= rect.hy1) {
            return StreamStatus::error(StreamStatusCode::InvalidArgument, "invalid NEF tile request");
        }
        const std::size_t tileW = static_cast<std::size_t>(rect.hx1 - rect.hx0);
        const std::size_t tileH = static_cast<std::size_t>(rect.hy1 - rect.hy0);
        if (tileW > std::numeric_limits<std::size_t>::max() / tileH || rawCount != tileW * tileH) {
            return StreamStatus::error(StreamStatusCode::InvalidArgument, "NEF tile count mismatch");
        }

        std::vector<std::uint8_t> row(tileW * 2u);
        std::size_t out = 0u;
        for (int y = rect.hy0; y < rect.hy1; ++y) {
            const std::uint32_t stripIndex = static_cast<std::uint32_t>(y) / rowsPerStrip_;
            if (stripIndex >= stripOffsets_.size()) {
                return StreamStatus::error(StreamStatusCode::SourceFailed, "NEF strip index out of range");
            }
            const std::uint32_t rowInStrip = static_cast<std::uint32_t>(y) % rowsPerStrip_;
            const std::uint64_t offset =
                static_cast<std::uint64_t>(stripOffsets_[stripIndex]) +
                (static_cast<std::uint64_t>(rowInStrip) * static_cast<std::uint64_t>(metadata_.width) +
                 static_cast<std::uint64_t>(rect.hx0)) * 2u;
            if (!bytes_->readExact(offset, row.data(), row.size())) {
                return StreamStatus::error(StreamStatusCode::SourceFailed, "NEF row sample read failed");
            }
            for (std::size_t x = 0; x < tileW; ++x) {
                rawOut[out++] = le16(row.data() + x * 2u);
            }
        }
        return StreamStatus::ok();
    }

    StreamStatus readRowBias(int y0, int y1, float* out, std::size_t count) override {
        if (out == nullptr || y0 < 0 || y1 < y0 || y1 > metadata_.height ||
            count != static_cast<std::size_t>(y1 - y0)) {
            return StreamStatus::error(StreamStatusCode::InvalidArgument, "invalid NEF row-bias request");
        }
        std::fill(out, out + count, 0.0f);
        return StreamStatus::ok();
    }

    StreamStatus readColBias(int x0, int x1, float* out, std::size_t count) override {
        if (out == nullptr || x0 < 0 || x1 < x0 || x1 > metadata_.width ||
            count != static_cast<std::size_t>(x1 - x0)) {
            return StreamStatus::error(StreamStatusCode::InvalidArgument, "invalid NEF col-bias request");
        }
        std::fill(out, out + count, 0.0f);
        return StreamStatus::ok();
    }

private:
    std::shared_ptr<IRawByteSource> bytes_;
    DngMetadata metadata_{};
    std::uint32_t rowsPerStrip_ = 0u;
    std::vector<std::uint32_t> stripOffsets_;
    std::vector<std::uint32_t> stripByteCounts_;
};

class NikonNefUncompressedAdapter final : public IRawSourceAdapter {
public:
    RawFormatFamily formatFamily() const noexcept override { return RawFormatFamily::NikonNef; }

    AdapterStatus open(
        std::shared_ptr<IRawByteSource> bytes,
        const RawSourceOpenRequest& request,
        std::unique_ptr<IRawTileSource>& outSource,
        RawSourceDescriptor& outDescriptor) noexcept override {
        outSource.reset();
        outDescriptor = {};
        if (!bytes || request.declaredFormat != RawFormatFamily::NikonNef ||
            !request.sourceSeal.valid || request.sourceSeal.sourceEvidenceId.empty()) {
            return AdapterStatus::error(AdapterStatusCode::InvalidArgument, "invalid Nikon NEF adapter request");
        }
        if (request.sourceSeal.byteLength != bytes->sizeBytes()) {
            return AdapterStatus::error(AdapterStatusCode::SourceSealMismatch, "NEF sealed byte length mismatch");
        }

        DngMetadata metadata;
        std::string make;
        std::uint32_t rowsPerStrip = 0u;
        std::vector<std::uint32_t> offsets;
        std::vector<std::uint32_t> counts;
        NefParser parser(bytes);
        const auto parsed = parser.parse(make, metadata, rowsPerStrip, offsets, counts);
        if (!parsed) return parsed;

        metadata.sourceId = request.sourceSeal.sourceEvidenceId;
        if (request.color.valid) {
            metadata.cameraToXyzD50 = request.color.cameraToXyzD50;
        }

        auto source = std::unique_ptr<NikonNefUncompressedSource>(
            new (std::nothrow) NikonNefUncompressedSource(
                bytes, metadata, rowsPerStrip, std::move(offsets), std::move(counts)));
        if (!source) {
            return AdapterStatus::error(AdapterStatusCode::BudgetExceeded, "NEF source allocation failed");
        }
        if (source->residentBytesUpperBound() > request.maxResidentBytes) {
            return AdapterStatus::error(AdapterStatusCode::BudgetExceeded, "NEF source exceeds memory budget");
        }

        outDescriptor.format = RawFormatFamily::NikonNef;
        outDescriptor.decoderId = "truthraw.nikon-nef-uncompressed16-cfa.v0.1";
        outDescriptor.sourceEvidenceId = request.sourceSeal.sourceEvidenceId;
        outDescriptor.sourceSealAcceptedAtBoundary = true;
        outDescriptor.exactCfaSamplesAvailable = true;
        outDescriptor.scientificColorBindingProvided = request.color.valid;
        outDescriptor.measurementAdmissionReady = true;
        outDescriptor.scientificAdmissionReady = false;
        outDescriptor.syntheticConformanceOnly = false;
        outDescriptor.directSensorAdcClaimAllowed = false;
        outDescriptor.fullRawFrameMaterialized = false;

        // Deliberately blocked: black level/saturation/noise/color authority are
        // not established from this strict container subset alone.
        outSource = std::move(source);
        return AdapterStatus::ok();
    }
};

} // namespace

std::shared_ptr<IRawSourceAdapter> makeNikonNefUncompressedAdapter() {
    return std::make_shared<NikonNefUncompressedAdapter>();
}

} // namespace truthraw::multivendor_raw_source_adapter::v0_1
