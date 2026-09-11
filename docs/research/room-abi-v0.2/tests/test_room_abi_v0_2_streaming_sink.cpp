#include "bounded_record_file_sink_v0_1.h"
#include "room_abi_v0_2.h"
#include "tile_native_dng_source_v0_1.h"

#include "truthraw/core.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>
#include <unistd.h>

using namespace truthraw;
using namespace truthraw::room_abi::v0_2;
using namespace truthraw::streaming_v0_1;
using namespace truthraw::tile_dng_v0_1;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(2);
    }
}

class DngMemSource final : public IRandomAccessByteSource {
public:
    explicit DngMemSource(std::vector<std::uint8_t> bytes) : bytes_(std::move(bytes)) {}
    std::uint64_t sizeBytes() const override { return bytes_.size(); }
    std::size_t residentBytesUpperBound() const override { return sizeof(*this) + bytes_.capacity(); }
    bool readExact(std::uint64_t offset, void* dst, std::size_t count) override {
        if (offset > bytes_.size() || count > bytes_.size() - offset) return false;
        std::memcpy(dst, bytes_.data() + offset, count);
        return true;
    }
private:
    std::vector<std::uint8_t> bytes_;
};

void put16(std::vector<std::uint8_t>& b, std::size_t o, std::uint16_t v) {
    if (b.size() < o + 2U) b.resize(o + 2U);
    b[o] = static_cast<std::uint8_t>(v);
    b[o + 1U] = static_cast<std::uint8_t>(v >> 8U);
}

void put32(std::vector<std::uint8_t>& b, std::size_t o, std::uint32_t v) {
    if (b.size() < o + 4U) b.resize(o + 4U);
    for (int i = 0; i < 4; ++i) b[o + static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(v >> (8 * i));
}

void put64(std::vector<std::uint8_t>& b, std::size_t o, std::uint64_t v) {
    if (b.size() < o + 8U) b.resize(o + 8U);
    for (int i = 0; i < 8; ++i) b[o + static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(v >> (8 * i));
}

std::uint32_t get32(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8U) |
           (static_cast<std::uint32_t>(p[2]) << 16U) |
           (static_cast<std::uint32_t>(p[3]) << 24U);
}

std::uint64_t get64(const std::uint8_t* p) {
    std::uint64_t v = 0U;
    for (int i = 0; i < 8; ++i) v |= static_cast<std::uint64_t>(p[i]) << (8U * i);
    return v;
}

std::vector<std::uint8_t> shorts(std::initializer_list<std::uint16_t> values) {
    std::vector<std::uint8_t> b(values.size() * 2U);
    std::size_t p = 0U;
    for (auto v : values) { put16(b, p, v); p += 2U; }
    return b;
}

std::vector<std::uint8_t> longs(const std::vector<std::uint32_t>& values) {
    std::vector<std::uint8_t> b(values.size() * 4U);
    for (std::size_t i = 0; i < values.size(); ++i) put32(b, 4U * i, values[i]);
    return b;
}

std::vector<std::uint8_t> rationals(const std::array<std::uint32_t, 4>& values) {
    std::vector<std::uint8_t> b(32U);
    for (std::size_t i = 0; i < values.size(); ++i) {
        put32(b, 8U * i, values[i]);
        put32(b, 8U * i + 4U, 1U);
    }
    return b;
}

std::vector<std::uint8_t> doubles(const std::array<double, 6>& values) {
    std::vector<std::uint8_t> b(48U);
    for (std::size_t i = 0; i < values.size(); ++i) {
        std::uint64_t bits = 0U;
        std::memcpy(&bits, &values[i], sizeof(bits));
        put64(b, 8U * i, bits);
    }
    return b;
}

struct Entry {
    std::uint16_t tag = 0;
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::vector<std::uint8_t> data;
    std::size_t recordOffset = 0U;
    std::size_t payloadOffset = 0U;
};

std::vector<std::uint8_t> make_dng(int width, int height, const std::vector<std::uint16_t>& raw) {
    std::vector<Entry> entries;
    auto add = [&](std::uint16_t tag, std::uint16_t type, std::uint32_t count, std::vector<std::uint8_t> data) {
        entries.push_back({tag, type, count, std::move(data), 0U, 0U});
    };

    add(256, 4, 1, longs({static_cast<std::uint32_t>(width)}));
    add(257, 4, 1, longs({static_cast<std::uint32_t>(height)}));
    add(258, 3, 1, shorts({16}));
    add(259, 3, 1, shorts({1}));
    add(262, 3, 1, shorts({32803}));
    add(274, 3, 1, shorts({1}));
    add(277, 3, 1, shorts({1}));
    add(284, 3, 1, shorts({1}));
    add(339, 3, 1, shorts({1}));
    add(33421, 3, 2, shorts({2, 2}));
    add(33422, 1, 4, {2, 1, 1, 0});
    add(50710, 1, 3, {0, 1, 2});
    add(50713, 3, 2, shorts({2, 2}));
    add(50714, 5, 4, rationals({64, 65, 66, 67}));
    add(50717, 4, 1, longs({1023}));
    add(51041, 12, 6, doubles({0.0009, 1e-6, 0.0010, 1.2e-6, 0.0011, 1.4e-6}));

    constexpr std::uint32_t rowsPerStrip = 2U;
    const std::uint32_t stripCount = static_cast<std::uint32_t>((height + 1) / 2);
    add(273, 4, stripCount, std::vector<std::uint8_t>(static_cast<std::size_t>(stripCount) * 4U));
    add(278, 4, 1, longs({rowsPerStrip}));
    add(279, 4, stripCount, std::vector<std::uint8_t>(static_cast<std::size_t>(stripCount) * 4U));

    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return a.tag < b.tag; });
    std::vector<std::uint8_t> bytes(8U + 2U + entries.size() * 12U + 4U, 0U);
    bytes[0] = 'I'; bytes[1] = 'I'; put16(bytes, 2U, 42U); put32(bytes, 4U, 8U);
    put16(bytes, 8U, static_cast<std::uint16_t>(entries.size()));
    std::size_t extension = bytes.size();

    for (std::size_t i = 0; i < entries.size(); ++i) {
        auto& e = entries[i];
        e.recordOffset = 10U + 12U * i;
        put16(bytes, e.recordOffset, e.tag);
        put16(bytes, e.recordOffset + 2U, e.type);
        put32(bytes, e.recordOffset + 4U, e.count);
        if (e.data.size() <= 4U) {
            std::copy(e.data.begin(), e.data.end(), bytes.begin() + static_cast<std::ptrdiff_t>(e.recordOffset + 8U));
        } else {
            e.payloadOffset = extension;
            put32(bytes, e.recordOffset + 8U, static_cast<std::uint32_t>(extension));
            bytes.insert(bytes.end(), e.data.begin(), e.data.end());
            extension = bytes.size();
        }
    }

    std::vector<std::uint32_t> offsets;
    std::vector<std::uint32_t> counts;
    for (std::uint32_t strip = 0; strip < stripCount; ++strip) {
        const int y0 = static_cast<int>(strip * rowsPerStrip);
        const int rows = std::min<int>(static_cast<int>(rowsPerStrip), height - y0);
        offsets.push_back(static_cast<std::uint32_t>(bytes.size()));
        counts.push_back(static_cast<std::uint32_t>(rows * width * 2));
        for (int y = y0; y < y0 + rows; ++y) {
            for (int x = 0; x < width; ++x) {
                const std::size_t o = bytes.size();
                bytes.resize(o + 2U);
                put16(bytes, o, raw[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)]);
            }
        }
    }

    for (const auto& e : entries) {
        if (e.tag == 273U) {
            const auto data = longs(offsets);
            std::copy(data.begin(), data.end(), bytes.begin() + static_cast<std::ptrdiff_t>(e.payloadOffset));
        }
        if (e.tag == 279U) {
            const auto data = longs(counts);
            std::copy(data.begin(), data.end(), bytes.begin() + static_cast<std::ptrdiff_t>(e.payloadOffset));
        }
    }
    return bytes;
}

std::vector<std::uint8_t> read_all_fd(int fd) {
    const off_t end = ::lseek(fd, 0, SEEK_END);
    require(end > 0, "sink output has bytes");
    require(::lseek(fd, 0, SEEK_SET) == 0, "rewind sink file");
    std::vector<std::uint8_t> out(static_cast<std::size_t>(end));
    std::size_t done = 0U;
    while (done < out.size()) {
        const ssize_t n = ::read(fd, out.data() + done, out.size() - done);
        require(n > 0, "read sink output");
        done += static_cast<std::size_t>(n);
    }
    return out;
}

void validate_record_stream(const std::vector<std::uint8_t>& bytes,
                            std::uint64_t expectedSdr,
                            std::uint64_t expectedGain,
                            std::uint64_t expectedDiag) {
    require(bytes.size() >= 96U + 32U, "record stream contains header and end record");
    require(std::memcmp(bytes.data(), "TRSINK01", 8U) == 0, "record stream magic");
    require(get32(bytes.data() + 8U) == 1U, "record stream version");

    std::size_t p = 96U;
    std::uint64_t sdr = 0U, gain = 0U, diag = 0U, end = 0U;
    while (p + 32U <= bytes.size()) {
        const std::uint32_t type = get32(bytes.data() + p);
        const std::uint64_t count = get64(bytes.data() + p + 24U);
        p += 32U;
        require(count <= (bytes.size() - p) / sizeof(float), "record payload bounded by file");
        const std::size_t payload = static_cast<std::size_t>(count) * sizeof(float);
        if (type == 2U) ++sdr;
        else if (type == 3U) ++gain;
        else if (type == 4U) ++diag;
        else if (type == 5U) { ++end; require(count == 0U, "end record has no payload"); }
        else require(false, "unexpected record type");
        p += payload;
    }
    require(p == bytes.size(), "record stream parsed exactly");
    require(sdr == expectedSdr, "SDR record count preserved");
    require(gain == expectedGain, "gain record count preserved");
    require(diag == expectedDiag, "diagnostic record count preserved");
    require(end == 1U, "exactly one end record");
}

} // namespace

int main() {
    constexpr int width = 32;
    constexpr int height = 24;
    std::vector<std::uint16_t> raw(static_cast<std::size_t>(width) * height);
    for (std::size_t i = 0; i < raw.size(); ++i) raw[i] = static_cast<std::uint16_t>(70U + (i * 17U) % 900U);

    auto bytes = std::make_shared<DngMemSource>(make_dng(width, height, raw));
    OpenOptions open{};
    open.sourceEvidenceId = "room_abi_v02_streaming_sink_fixture";
    open.color.valid = true;
    open.color.bindingId = "room_abi_v02_streaming_sink_color";
    open.color.cameraToXyzD50 = {0.62F, 0.21F, 0.08F, 0.18F, 0.71F, 0.07F, 0.03F, 0.12F, 0.79F};

    std::unique_ptr<TileNativeDngSource> source;
    const auto os = TileNativeDngSource::open(bytes, open, source);
    require(static_cast<bool>(os) && source != nullptr, "TileNativeDngSource opens fixture");
    require(!source->audit().fullFileMaterialized && !source->audit().fullRawMaterialized,
            "source opens without full-frame materialization");

    char path[] = "/tmp/truthraw-room-v02-sink-XXXXXX";
    const int fd = ::mkstemp(path);
    require(fd >= 0, "temporary output file created");
    ::unlink(path);

    BoundedRecordFileSink sink(fd);
    require(sink.residentBytesUpperBound() == BoundedRecordFileSink::kResidentAllowanceBytes,
            "sink resident bound fixed and explicit");

    StreamingEndpointBinding endpoints{};
    require(bind_streaming_endpoints(*source, sink, endpoints) == Status::Ok,
            "Room ABI binds concrete source and bounded file sink");
    require(endpoints.sourceResidentUpperBound == source->residentBytesUpperBound(),
            "source resident bound propagated exactly");
    require(endpoints.sinkResidentUpperBound == BoundedRecordFileSink::kResidentAllowanceBytes,
            "sink resident bound propagated exactly");

    auto reconstruction = std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto appearance = std::make_shared<SkinSafeDetailedCrispAppearance>();
    StreamingTruthRawProcessor processor(reconstruction, appearance);

    StreamingOptions options{};
    options.tile = {8, 7};
    options.workers = 1;
    options.hdrEnabled = true;
    options.streamScientificDiagnostics = true;
    options.sdrLutSize = 4096;
    options.memoryBudgetBytes = 4U * 1024U * 1024U;

    StreamingResult result{};
    const auto ps = processor.process(*source, sink, options, result);
    require(static_cast<bool>(ps), "TileNativeDngSource -> processor -> file sink succeeds");
    require(sink.begun() && sink.finished(), "file sink completes frame lifecycle");
    require(sink.sdrRecordCount() > 1U && sink.gainRecordCount() > 0U && sink.diagnosticRecordCount() > 1U,
            "processor emits bounded multi-record output");
    require(!result.memory.adapterOwnsFullRawFrame && !result.memory.adapterOwnsFullSdrFrame &&
            !result.memory.adapterOwnsFullHalfGainFrame && !result.memory.adapterOwnsFullDiagnosticFrame,
            "streaming processor owns no full-frame buffers");
    require(result.memory.sourceResidentUpperBound == source->residentBytesUpperBound(),
            "processor accounts concrete source bound");
    require(result.memory.sinkResidentUpperBound == sink.residentBytesUpperBound(),
            "processor accounts concrete sink bound");
    require(result.provenance.physicalFrameCount == 1U && result.provenance.independentEvidenceCount == 1U,
            "single-frame evidence counts preserved");
    require(!source->audit().fullRawMaterialized,
            "source remains tile-native after complete two-pass processing");

    const auto fileBytes = read_all_fd(fd);
    require(fileBytes.size() == sink.bytesWritten(), "sink byte accounting exact");
    validate_record_stream(fileBytes, sink.sdrRecordCount(), sink.gainRecordCount(), sink.diagnosticRecordCount());

    ::close(fd);

    std::cout << "ROOM_ABI_V0_2_STREAMING_SINK_PASS\n";
    std::cout << "source_resident_bound=" << source->residentBytesUpperBound() << '\n';
    std::cout << "sink_resident_bound=" << sink.residentBytesUpperBound() << '\n';
    std::cout << "output_bytes=" << sink.bytesWritten() << '\n';
    std::cout << "sdr_records=" << sink.sdrRecordCount() << '\n';
    std::cout << "gain_records=" << sink.gainRecordCount() << '\n';
    std::cout << "diagnostic_records=" << sink.diagnosticRecordCount() << '\n';
    std::cout << "adapter_full_frame_buffers=0\n";
    std::cout << "scene_iso_axis=0\n";
    return 0;
}
