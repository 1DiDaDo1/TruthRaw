#include "room_abi_v0_2.h"
#include "tile_native_dng_source_v0_1.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

using namespace truthraw::room_abi::v0_2;
using namespace truthraw::tile_dng_v0_1;

namespace {

constexpr std::uint64_t MiB = 1024ULL * 1024ULL;

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

class BoundedSink final : public truthraw::streaming_v0_1::IStreamingSink {
public:
    explicit BoundedSink(std::size_t resident) : resident_(resident) {}
    std::size_t residentBytesUpperBound() const override { return resident_; }
    truthraw::streaming_v0_1::StreamStatus beginFrame(
        int, int, truthraw::Orientation, const truthraw::ExposurePlan&, bool, bool) override {
        return truthraw::streaming_v0_1::StreamStatus::ok();
    }
    truthraw::streaming_v0_1::StreamStatus writeSdrTile(
        const truthraw::TileRect&, const float*, std::size_t) override {
        return truthraw::streaming_v0_1::StreamStatus::ok();
    }
    truthraw::streaming_v0_1::StreamStatus writeHalfLogGainBlock(
        const truthraw::streaming_v0_1::HalfStateRect&, const float*, std::size_t) override {
        return truthraw::streaming_v0_1::StreamStatus::ok();
    }
    truthraw::streaming_v0_1::StreamStatus writeStage2DiagnosticTile(
        const truthraw::TileRect&, const float*, std::size_t) override {
        return truthraw::streaming_v0_1::StreamStatus::ok();
    }
    truthraw::streaming_v0_1::StreamStatus finishFrame() override {
        return truthraw::streaming_v0_1::StreamStatus::ok();
    }
private:
    std::size_t resident_ = 0;
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

std::vector<std::uint8_t> shorts(std::initializer_list<std::uint16_t> values) {
    std::vector<std::uint8_t> b(values.size() * 2U);
    std::size_t p = 0;
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
        std::uint64_t bits = 0;
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
    std::size_t recordOffset = 0;
    std::size_t payloadOffset = 0;
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

truthraw::technical_backplane::v0_1::State valid_backplane() {
    truthraw::technical_backplane::v0_1::State state{};
    state.sourceEvidenceHash.fill(0x11U);
    state.scientificMasterHash.fill(0x22U);
    state.zeroLineHash.fill(0x33U);
    state.sceneScaleHash.fill(0x44U);
    state.physicalFrameCount = 1U;
    state.independentEvidenceCount = 1U;
    state.claimStatus = truthraw::technical_backplane::v0_1::ClaimStatus::Candidate;
    return state;
}

} // namespace

int main() {
    constexpr int width = 16;
    constexpr int height = 12;
    std::vector<std::uint16_t> raw(static_cast<std::size_t>(width) * height);
    for (std::size_t i = 0; i < raw.size(); ++i) raw[i] = static_cast<std::uint16_t>(70U + (i * 17U) % 900U);

    auto bytes = std::make_shared<DngMemSource>(make_dng(width, height, raw));
    OpenOptions options{};
    options.sourceEvidenceId = "room_abi_v02_concrete_tile_source";
    options.color.valid = true;
    options.color.bindingId = "room_abi_v02_fixture_color";
    options.color.cameraToXyzD50 = {0.62F, 0.21F, 0.08F, 0.18F, 0.71F, 0.07F, 0.03F, 0.12F, 0.79F};

    std::unique_ptr<TileNativeDngSource> source;
    const auto openStatus = TileNativeDngSource::open(bytes, options, source);
    require(static_cast<bool>(openStatus), "real TileNativeDngSource opens synthetic valid DNG");
    require(source != nullptr, "real TileNativeDngSource object created");
    require(source->metadata().width == width && source->metadata().height == height,
            "real source metadata geometry bound");
    require(!source->audit().fullFileMaterialized && !source->audit().fullRawMaterialized,
            "opening concrete source does not materialize full file or raw");

    truthraw::TileRect tile{};
    tile.x0 = 0; tile.y0 = 0; tile.x1 = 4; tile.y1 = 4;
    tile.hx0 = 0; tile.hy0 = 0; tile.hx1 = 4; tile.hy1 = 4;
    std::array<std::uint16_t, 16> tileRaw{};
    const auto tileStatus = source->readRawTile(tile, tileRaw.data(), tileRaw.size(), nullptr, 0U);
    require(static_cast<bool>(tileStatus), "real TileNativeDngSource reads bounded tile");
    require(tileRaw[0] == raw[0] && tileRaw[3] == raw[3] && tileRaw[4] == raw[width],
            "tile data maps to source RAW exactly");
    require(source->audit().rawPayloadBytesRead > 0U && !source->audit().fullRawMaterialized,
            "tile read records payload IO without full RAW materialization");

    BoundedSink sink(512U * 1024U);
    StreamingEndpointBinding endpoints{};
    require(bind_streaming_endpoints(*source, sink, endpoints) == Status::Ok,
            "Room ABI v0.2 binds concrete TileNativeDngSource");
    require(endpoints.source == source.get() && endpoints.sink == &sink,
            "source/sink are borrowed exact objects");
    require(endpoints.sourceResidentUpperBound == source->residentBytesUpperBound(),
            "ABI uses concrete source resident upper bound exactly");
    require(endpoints.sinkResidentUpperBound == sink.residentBytesUpperBound(),
            "ABI uses bounded sink resident upper bound exactly");

    auto backplane = valid_backplane();
    SharedLineageBinding lineage{};
    require(bind_shared_lineage(backplane, lineage) == Status::Ok, "shared lineage valid");

    AdaptiveAllRoomRequest request{};
    request.execution.status = truthraw::building_runtime::v0_1::Status::Ok;
    request.execution.resources.valid = true;
    request.execution.resources.totalWorkingSetBudgetBytes = 32U * MiB;
    request.execution.resources.perHeavyRoomBudgetBytes = 8U * MiB;
    request.execution.resources.maxConcurrentHeavyRooms = 1U;
    request.execution.resources.tileSize = 128U;
    request.execution.resources.cpuThreadsPerHeavyRoom = 1U;
    request.execution.placementCount = 1U;
    request.execution.waveCount = 1U;
    request.execution.placements[0].room = truthraw::building_runtime::v0_1::RoomId::Architect;
    request.execution.placements[0].wave = 0U;
    request.execution.placements[0].lane = 0U;
    request.execution.placements[0].heavyLease = true;
    request.execution.placements[0].memoryLeaseBytes = 8U * MiB;
    request.lineage = lineage;
    request.endpoints = endpoints;
    request.profiles = default_room_profiles();
    for (std::size_t i = 0; i < request.demands.size(); ++i) {
        request.demands[i].room = static_cast<truthraw::building_runtime::v0_1::RoomId>(i);
        request.demands[i].valid = true;
    }
    request.demands[static_cast<std::size_t>(truthraw::building_runtime::v0_1::RoomId::Architect)].transientPeakBytes = 1U * MiB;

    AdaptiveAllRoomPlan plan{};
    require(plan_adaptive_all_room_binding(request, plan) == Status::Ok,
            "whole-house admission accepts concrete tile-native source binding");
    require(plan.valid && plan.sourceSinkCountedOnce && plan.oneSharedBackplaneForAllRooms,
            "concrete source participates in shared whole-house contract");
    require(plan.peakResidentUpperBound ==
                endpoints.sourceResidentUpperBound + endpoints.sinkResidentUpperBound + 1U * MiB,
            "concrete source resident memory counted exactly once with active room demand");
    require(!plan.sceneIsoAxisPresent, "concrete source binding cannot reintroduce scene ISO");

    std::cout << "tile_source_resident_bytes=" << endpoints.sourceResidentUpperBound << '\n';
    std::cout << "sink_resident_bytes=" << endpoints.sinkResidentUpperBound << '\n';
    std::cout << "raw_payload_bytes_read=" << source->audit().rawPayloadBytesRead << '\n';
    std::cout << "full_raw_materialized=" << source->audit().fullRawMaterialized << '\n';
    std::cout << "scene_iso_axis=ABSENT\n";
    std::cout << "ROOM_ABI_V0_2_CONCRETE_TILE_SOURCE_PASS\n";
    return 0;
}
