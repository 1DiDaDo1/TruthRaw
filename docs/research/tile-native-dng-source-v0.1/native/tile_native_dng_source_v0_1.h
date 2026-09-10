#pragma once

#include "full_frame_streaming_v0_1.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace truthraw::tile_dng_v0_1 {

using streaming_v0_1::IRawTileSource;
using streaming_v0_1::StreamStatus;
using streaming_v0_1::StreamStatusCode;

enum class DngSourceCode : int {
    Ok = 0,
    IoError,
    InvalidTiff,
    UnsupportedBigTiff,
    UnsupportedCompression,
    UnsupportedBitsPerSample,
    UnsupportedPhotometric,
    UnsupportedTopology,
    AmbiguousRawIfd,
    MissingRequiredTag,
    InvalidTag,
    InvalidStorage,
    BindingMissing,
    BudgetExceeded,
};

struct DngSourceStatus {
    DngSourceCode code = DngSourceCode::Ok;
    std::string message;
    explicit operator bool() const { return code == DngSourceCode::Ok; }
    static DngSourceStatus ok() { return {}; }
    static DngSourceStatus error(DngSourceCode c, std::string m) {
        DngSourceStatus s; s.code = c; s.message = std::move(m); return s;
    }
};

class IRandomAccessByteSource {
public:
    virtual ~IRandomAccessByteSource() = default;
    virtual std::uint64_t sizeBytes() const = 0;
    virtual std::size_t residentBytesUpperBound() const = 0;
    virtual bool readExact(std::uint64_t offset, void* dst, std::size_t count) = 0;
};

// Borrowed POSIX file descriptor. The caller owns fd lifetime.
class PosixFdByteSource final : public IRandomAccessByteSource {
public:
    explicit PosixFdByteSource(int fd);
    std::uint64_t sizeBytes() const override;
    std::size_t residentBytesUpperBound() const override { return sizeof(*this); }
    bool readExact(std::uint64_t offset, void* dst, std::size_t count) override;
private:
    int fd_ = -1;
    std::uint64_t size_ = 0;
};

struct ColorBinding {
    bool valid = false;
    std::string bindingId;
    std::array<float,9> cameraToXyzD50 = {1,0,0, 0,1,0, 0,0,1};
};

struct OpenOptions {
    std::string sourceEvidenceId;
    ColorBinding color;
    std::uint32_t explicitRawIfdOffset = 0; // 0 = require unique supported CFA IFD.
    std::size_t maxOpcodeListBytes = 4u * 1024u * 1024u;
    std::size_t maxResidentBytes = 8u * 1024u * 1024u;
    std::uint16_t maxIfdEntries = 256;
    std::uint16_t maxIfdCount = 16;
};

struct SourceAudit {
    std::uint64_t fileBytesRead = 0;
    std::uint64_t rawPayloadBytesRead = 0;
    std::uint64_t metadataBytesRead = 0;
    std::uint64_t tileReadCalls = 0;
    std::uint32_t rawIfdOffset = 0;
    std::uint32_t strileCount = 0;
    bool tiledStorage = false;
    bool stripArraysMaterialized = false;
    bool tileArraysMaterialized = false;
    bool fullFileMaterialized = false;
    bool fullRawMaterialized = false;
    bool gainMapPresent = false;
    bool strileLocatorsValidatedLazily = true;
};

class TileNativeDngSource final : public IRawTileSource {
public:
    static DngSourceStatus open(
        std::shared_ptr<IRandomAccessByteSource> bytes,
        const OpenOptions& options,
        std::unique_ptr<TileNativeDngSource>& out);

    const DngMetadata& metadata() const override { return metadata_; }
    std::size_t residentBytesUpperBound() const override { return residentUpperBound_; }

    StreamStatus readRawTile(
        const TileRect& rect,
        std::uint16_t* rawOut,
        std::size_t rawCount,
        float* gainOut,
        std::size_t gainCount) override;
    StreamStatus readRowBias(int y0, int y1, float* out, std::size_t count) override;
    StreamStatus readColBias(int x0, int x1, float* out, std::size_t count) override;

    const SourceAudit& audit() const { return audit_; }
    const std::string& colorBindingId() const { return colorBindingId_; }

private:
    struct TagRef {
        std::uint16_t tag = 0;
        std::uint16_t type = 0;
        std::uint32_t count = 0;
        std::uint64_t dataOffset = 0;
        std::uint64_t dataBytes = 0;
    };
    struct Ifd {
        std::uint32_t offset = 0;
        std::uint32_t next = 0;
        std::vector<TagRef> tags;
    };
    struct GainMap {
        std::array<std::int32_t,4> area{}; // top,left,bottom,right
        std::uint32_t plane = 0, planes = 0, rowPitch = 0, colPitch = 0;
        std::uint32_t pointsV = 0, pointsH = 0, mapPlanes = 0;
        double spacingV = 0, spacingH = 0, originV = 0, originH = 0;
        std::vector<float> values;
        bool applies(int y, int x) const;
        float interpolate(int y, int x, int imageH, int imageW) const;
    };

    TileNativeDngSource() = default;
    DngSourceStatus initialize(const OpenOptions& options);
    DngSourceStatus discoverIfds(const OpenOptions& options, std::vector<Ifd>& out);
    DngSourceStatus parseIfd(std::uint32_t offset, const OpenOptions& options, Ifd& out);
    const TagRef* findTag(const Ifd& ifd, std::uint16_t tag) const;
    DngSourceStatus selectAndBindRawIfd(const OpenOptions& options, const std::vector<Ifd>& ifds);
    DngSourceStatus parseGainMaps(const TagRef& tag, const OpenOptions& options);

    bool readBytes(std::uint64_t offset, void* dst, std::size_t n, bool rawPayload = false);
    bool readTagBytes(const TagRef& t, std::uint64_t byteOffset, void* dst, std::size_t n);
    bool readUnsigned(const TagRef& t, std::uint32_t index, std::uint32_t& out);
    bool readFloatLike(const TagRef& t, std::uint32_t index, double& out);
    bool readStrileScalar(const TagRef& t, std::uint32_t index, std::uint32_t& out);
    StreamStatus readRawRectStripped(const TileRect& rect, std::uint16_t* rawOut);
    StreamStatus readRawRectTiled(const TileRect& rect, std::uint16_t* rawOut);
    float gainAt(int y, int x) const;
    std::size_t computeResidentUpperBound() const;

    std::shared_ptr<IRandomAccessByteSource> bytes_;
    bool littleEndian_ = true;
    DngMetadata metadata_{};
    std::string colorBindingId_;
    Ifd rawIfd_{};
    TagRef offsets_{};
    TagRef byteCounts_{};
    std::uint32_t rowsPerStrip_ = 0;
    std::uint32_t tileWidth_ = 0;
    std::uint32_t tileLength_ = 0;
    std::uint32_t strileCount_ = 0;
    bool tiled_ = false;
    std::vector<GainMap> gainMaps_;
    std::size_t residentUpperBound_ = 0;
    SourceAudit audit_{};
};

} // namespace truthraw::tile_dng_v0_1
