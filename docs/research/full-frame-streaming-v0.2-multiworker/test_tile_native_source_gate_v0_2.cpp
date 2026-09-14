#include "tile_native_dng_source_v0_1.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

using namespace truthraw;
using namespace truthraw::streaming_v0_1;
using namespace truthraw::tile_dng_v0_1;

namespace {

void require_active(bool ok, const char* expr, int line) {
    if (!ok) {
        std::cerr << "REQUIRE_FAIL line=" << line << " expr=" << expr << "\n";
        std::exit(2);
    }
}
#define REQUIRE(expr) require_active(bool(expr), #expr, __LINE__)

class DngMemSource final : public IRandomAccessByteSource {
public:
    explicit DngMemSource(std::vector<std::uint8_t> bytes) : bytes_(std::move(bytes)) {}
    std::uint64_t sizeBytes() const override { return bytes_.size(); }
    std::size_t residentBytesUpperBound() const override { return sizeof(*this) + bytes_.capacity(); }
    bool readExact(std::uint64_t offset, void* dst, std::size_t count) override {
        if (offset > bytes_.size() || count > bytes_.size() - std::size_t(offset)) return false;
        std::memcpy(dst, bytes_.data() + std::size_t(offset), count);
        return true;
    }
private:
    std::vector<std::uint8_t> bytes_;
};

void put16(std::vector<std::uint8_t>& b, std::size_t o, std::uint16_t v) {
    if (b.size() < o + 2) b.resize(o + 2);
    b[o] = std::uint8_t(v);
    b[o + 1] = std::uint8_t(v >> 8);
}
void put32(std::vector<std::uint8_t>& b, std::size_t o, std::uint32_t v) {
    if (b.size() < o + 4) b.resize(o + 4);
    for (int i = 0; i < 4; ++i) b[o + std::size_t(i)] = std::uint8_t(v >> (8 * i));
}
void put64(std::vector<std::uint8_t>& b, std::size_t o, std::uint64_t v) {
    if (b.size() < o + 8) b.resize(o + 8);
    for (int i = 0; i < 8; ++i) b[o + std::size_t(i)] = std::uint8_t(v >> (8 * i));
}
std::vector<std::uint8_t> shorts(std::initializer_list<std::uint16_t> xs) {
    std::vector<std::uint8_t> b(xs.size() * 2);
    std::size_t p = 0;
    for (auto v : xs) { put16(b, p, v); p += 2; }
    return b;
}
std::vector<std::uint8_t> longs(const std::vector<std::uint32_t>& xs) {
    std::vector<std::uint8_t> b(xs.size() * 4);
    for (std::size_t i = 0; i < xs.size(); ++i) put32(b, 4 * i, xs[i]);
    return b;
}
std::vector<std::uint8_t> rationals(const std::array<std::uint32_t, 4>& xs) {
    std::vector<std::uint8_t> b(32);
    for (int i = 0; i < 4; ++i) {
        put32(b, std::size_t(8 * i), xs[std::size_t(i)]);
        put32(b, std::size_t(8 * i + 4), 1);
    }
    return b;
}
std::vector<std::uint8_t> doubles(const std::array<double, 6>& xs) {
    std::vector<std::uint8_t> b(48);
    for (int i = 0; i < 6; ++i) {
        std::uint64_t u = 0;
        std::memcpy(&u, &xs[std::size_t(i)], sizeof(u));
        put64(b, std::size_t(8 * i), u);
    }
    return b;
}

struct Entry {
    std::uint16_t tag = 0;
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::vector<std::uint8_t> data;
    std::size_t entryOffset = 0;
    std::size_t payloadOffset = 0;
};

std::vector<std::uint8_t> make_dng(int w, int h, const std::vector<std::uint16_t>& raw) {
    std::vector<Entry> entries;
    auto add = [&](std::uint16_t tag, std::uint16_t type, std::uint32_t count,
                   std::vector<std::uint8_t> data) {
        entries.push_back({tag, type, count, std::move(data), 0, 0});
    };

    add(256, 4, 1, longs({std::uint32_t(w)}));
    add(257, 4, 1, longs({std::uint32_t(h)}));
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

    const std::uint32_t rowsPerStrip = 2;
    const std::uint32_t stripCount = std::uint32_t((h + 1) / 2);
    add(273, 4, stripCount, std::vector<std::uint8_t>(std::size_t(stripCount) * 4));
    add(278, 4, 1, longs({rowsPerStrip}));
    add(279, 4, stripCount, std::vector<std::uint8_t>(std::size_t(stripCount) * 4));

    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return a.tag < b.tag; });
    std::vector<std::uint8_t> bytes(8 + 2 + entries.size() * 12 + 4, 0);
    bytes[0] = 'I'; bytes[1] = 'I';
    put16(bytes, 2, 42); put32(bytes, 4, 8); put16(bytes, 8, std::uint16_t(entries.size()));
    std::size_t ext = bytes.size();

    for (std::size_t i = 0; i < entries.size(); ++i) {
        auto& e = entries[i];
        e.entryOffset = 10 + 12 * i;
        put16(bytes, e.entryOffset, e.tag);
        put16(bytes, e.entryOffset + 2, e.type);
        put32(bytes, e.entryOffset + 4, e.count);
        if (e.data.size() <= 4) {
            std::copy(e.data.begin(), e.data.end(), bytes.begin() + std::ptrdiff_t(e.entryOffset + 8));
        } else {
            e.payloadOffset = ext;
            put32(bytes, e.entryOffset + 8, std::uint32_t(ext));
            bytes.insert(bytes.end(), e.data.begin(), e.data.end());
            ext = bytes.size();
        }
    }

    std::vector<std::uint32_t> offsets;
    std::vector<std::uint32_t> counts;
    for (std::uint32_t s = 0; s < stripCount; ++s) {
        const int y0 = int(s * rowsPerStrip);
        const int rows = std::min<int>(int(rowsPerStrip), h - y0);
        offsets.push_back(std::uint32_t(bytes.size()));
        counts.push_back(std::uint32_t(rows * w * 2));
        for (int y = y0; y < y0 + rows; ++y) {
            for (int x = 0; x < w; ++x) {
                const std::size_t o = bytes.size();
                bytes.resize(o + 2);
                put16(bytes, o, raw[std::size_t(y) * std::size_t(w) + std::size_t(x)]);
            }
        }
    }

    for (const auto& e : entries) {
        if (e.tag == 273) {
            const auto d = longs(offsets);
            std::copy(d.begin(), d.end(), bytes.begin() + std::ptrdiff_t(e.payloadOffset));
        }
        if (e.tag == 279) {
            const auto d = longs(counts);
            std::copy(d.begin(), d.end(), bytes.begin() + std::ptrdiff_t(e.payloadOffset));
        }
    }
    return bytes;
}

std::vector<TileRect> make_tiles(int w, int h, int core, int halo) {
    std::vector<TileRect> tiles;
    for (int y0 = 0; y0 < h; y0 += core) {
        for (int x0 = 0; x0 < w; x0 += core) {
            TileRect t;
            t.x0 = x0; t.y0 = y0;
            t.x1 = std::min(w, x0 + core); t.y1 = std::min(h, y0 + core);
            t.hx0 = std::max(0, t.x0 - halo); t.hy0 = std::max(0, t.y0 - halo);
            t.hx1 = std::min(w, t.x1 + halo); t.hy1 = std::min(h, t.y1 + halo);
            tiles.push_back(t);
        }
    }
    return tiles;
}

std::unique_ptr<TileNativeDngSource> open_source(const std::vector<std::uint8_t>& bytes,
                                                 const std::array<float, 9>& matrix) {
    auto mem = std::make_shared<DngMemSource>(bytes);
    OpenOptions options;
    options.sourceEvidenceId = "tile_native_multiworker_gate_fixture";
    options.color.valid = true;
    options.color.bindingId = "tile_native_multiworker_gate_color";
    options.color.cameraToXyzD50 = matrix;
    std::unique_ptr<TileNativeDngSource> source;
    const auto status = TileNativeDngSource::open(mem, options, source);
    REQUIRE(status);
    return source;
}

std::vector<std::uint16_t> read_tile(IRawTileSource& source, const TileRect& tile) {
    const std::size_t n = std::size_t(tile.hx1 - tile.hx0) * std::size_t(tile.hy1 - tile.hy0);
    std::vector<std::uint16_t> raw(n);
    const auto status = source.readRawTile(tile, raw.data(), n, nullptr, 0);
    REQUIRE(status);
    return raw;
}

struct ProbeSource final : IRawTileSource {
    explicit ProbeSource(IRawTileSource& inner) : inner_(inner) {}
    const DngMetadata& metadata() const override { return inner_.metadata(); }
    std::size_t residentBytesUpperBound() const override { return inner_.residentBytesUpperBound(); }
    StreamStatus readRawTile(const TileRect& rect, std::uint16_t* rawOut, std::size_t rawCount,
                             float* gainOut, std::size_t gainCount) override {
        const int now = active_.fetch_add(1) + 1;
        int old = peak_.load();
        while (now > old && !peak_.compare_exchange_weak(old, now)) {}
        auto status = inner_.readRawTile(rect, rawOut, rawCount, gainOut, gainCount);
        active_.fetch_sub(1);
        return status;
    }
    StreamStatus readRowBias(int y0, int y1, float* out, std::size_t count) override {
        return inner_.readRowBias(y0, y1, out, count);
    }
    StreamStatus readColBias(int x0, int x1, float* out, std::size_t count) override {
        return inner_.readColBias(x0, x1, out, count);
    }
    int peak() const { return peak_.load(); }
private:
    IRawTileSource& inner_;
    std::atomic<int> active_{0};
    std::atomic<int> peak_{0};
};

void verify_workers(const std::vector<std::uint8_t>& bytes,
                    const std::array<float, 9>& matrix,
                    const std::vector<TileRect>& tiles,
                    const std::vector<std::vector<std::uint16_t>>& reference,
                    std::uint64_t referenceRawPayloadBytes,
                    int workerCount) {
    auto source = open_source(bytes, matrix);
    ProbeSource probe(*source);
    std::mutex sourceMutex;
    const std::size_t totalJobs = tiles.size() * 2;
    std::vector<std::vector<std::uint16_t>> outputs(totalJobs);
    std::atomic<std::size_t> next{0};
    std::atomic<bool> ok{true};
    std::vector<std::thread> workers;
    workers.reserve(std::size_t(workerCount));

    for (int wi = 0; wi < workerCount; ++wi) {
        workers.emplace_back([&] {
            while (true) {
                const std::size_t job = next.fetch_add(1);
                if (job >= totalJobs) break;
                const std::size_t tileIndex = job % tiles.size();
                const auto& t = tiles[tileIndex];
                const std::size_t n = std::size_t(t.hx1 - t.hx0) * std::size_t(t.hy1 - t.hy0);
                std::vector<std::uint16_t> raw(n);
                StreamStatus status;
                {
                    std::lock_guard<std::mutex> lock(sourceMutex);
                    status = probe.readRawTile(t, raw.data(), n, nullptr, 0);
                }
                if (!status) { ok.store(false); break; }
                outputs[job] = std::move(raw);
            }
        });
    }
    for (auto& thread : workers) thread.join();

    REQUIRE(ok.load());
    REQUIRE(probe.peak() == 1);
    for (std::size_t job = 0; job < totalJobs; ++job) {
        REQUIRE(outputs[job] == reference[job % tiles.size()]);
    }
    REQUIRE(source->audit().tileReadCalls == totalJobs);
    REQUIRE(source->audit().rawPayloadBytesRead == 2 * referenceRawPayloadBytes);
    REQUIRE(!source->audit().fullRawMaterialized);
    REQUIRE(!source->audit().fullFileMaterialized);
    REQUIRE(source->audit().strileLocatorsValidatedLazily);

    std::cout << "workers=" << workerCount
              << " tiles=" << tiles.size()
              << " source_peak=" << probe.peak()
              << " raw_reads=" << source->audit().tileReadCalls
              << " raw_payload_bytes=" << source->audit().rawPayloadBytesRead << "\n";
}

} // namespace

int main() {
    const int w = 258;
    const int h = 194;
    std::vector<std::uint16_t> raw(std::size_t(w) * std::size_t(h));
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int v = 70 + ((x * 37 + y * 53 + x * y * 3) % 900);
            if ((x + y) % 97 == 0) v = 1023;
            raw[std::size_t(y) * std::size_t(w) + std::size_t(x)] = std::uint16_t(v);
        }
    }

    const std::array<float, 9> matrix = {
        0.62f, 0.21f, 0.08f,
        0.18f, 0.71f, 0.07f,
        0.03f, 0.12f, 0.79f
    };
    const auto bytes = make_dng(w, h, raw);
    const auto tiles = make_tiles(w, h, 32, 7);

    auto serial = open_source(bytes, matrix);
    std::vector<std::vector<std::uint16_t>> reference;
    reference.reserve(tiles.size());
    for (const auto& tile : tiles) reference.push_back(read_tile(*serial, tile));
    REQUIRE(serial->audit().tileReadCalls == tiles.size());
    const std::uint64_t referenceRawPayloadBytes = serial->audit().rawPayloadBytesRead;
    REQUIRE(referenceRawPayloadBytes > 0);
    REQUIRE(!serial->audit().fullRawMaterialized);
    REQUIRE(!serial->audit().fullFileMaterialized);

    verify_workers(bytes, matrix, tiles, reference, referenceRawPayloadBytes, 2);
    verify_workers(bytes, matrix, tiles, reference, referenceRawPayloadBytes, 4);

    std::cout << "TILE_NATIVE_DNG_MULTIWORKER_SOURCE_GATE_V0_2_PASS\n";
    return 0;
}
