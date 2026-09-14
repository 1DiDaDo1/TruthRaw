#include "multiworker_streaming_v0_2.h"
#include "tile_native_dng_source_v0_1.h"
#include "streaming_test_support_v0_1.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

using namespace truthraw::tile_dng_v0_1;
using truthraw::streaming_v0_2::MultiWorkerTelemetry;
using truthraw::streaming_v0_2::process_multiworker_streaming;

namespace {

class DngMemSource final : public IRandomAccessByteSource {
public:
    explicit DngMemSource(std::vector<std::uint8_t> b) : bytes_(std::move(b)) {}
    std::uint64_t sizeBytes() const override { return bytes_.size(); }
    std::size_t residentBytesUpperBound() const override { return sizeof(*this) + bytes_.capacity(); }
    bool readExact(std::uint64_t o, void* d, std::size_t n) override {
        if (o > bytes_.size() || n > bytes_.size() - std::size_t(o)) return false;
        std::memcpy(d, bytes_.data() + std::size_t(o), n);
        return true;
    }
private:
    std::vector<std::uint8_t> bytes_;
};

void p16(std::vector<std::uint8_t>& b, std::size_t o, std::uint16_t v) {
    if (b.size() < o + 2) b.resize(o + 2);
    b[o] = std::uint8_t(v);
    b[o + 1] = std::uint8_t(v >> 8);
}
void p32(std::vector<std::uint8_t>& b, std::size_t o, std::uint32_t v) {
    if (b.size() < o + 4) b.resize(o + 4);
    for (int i = 0; i < 4; ++i) b[o + std::size_t(i)] = std::uint8_t(v >> (8 * i));
}
void p64(std::vector<std::uint8_t>& b, std::size_t o, std::uint64_t v) {
    if (b.size() < o + 8) b.resize(o + 8);
    for (int i = 0; i < 8; ++i) b[o + std::size_t(i)] = std::uint8_t(v >> (8 * i));
}
std::vector<std::uint8_t> shorts(std::initializer_list<std::uint16_t> xs) {
    std::vector<std::uint8_t> b(xs.size() * 2);
    std::size_t p = 0;
    for (auto v : xs) { p16(b, p, v); p += 2; }
    return b;
}
std::vector<std::uint8_t> longs(const std::vector<std::uint32_t>& xs) {
    std::vector<std::uint8_t> b(xs.size() * 4);
    for (std::size_t i = 0; i < xs.size(); ++i) p32(b, 4 * i, xs[i]);
    return b;
}
std::vector<std::uint8_t> rationals(const std::array<std::uint32_t, 4>& xs) {
    std::vector<std::uint8_t> b(32);
    for (int i = 0; i < 4; ++i) {
        p32(b, std::size_t(8 * i), xs[std::size_t(i)]);
        p32(b, std::size_t(8 * i + 4), 1);
    }
    return b;
}
std::vector<std::uint8_t> doubles(const std::array<double, 6>& xs) {
    std::vector<std::uint8_t> b(48);
    for (int i = 0; i < 6; ++i) {
        std::uint64_t u = 0;
        std::memcpy(&u, &xs[std::size_t(i)], sizeof(u));
        p64(b, std::size_t(8 * i), u);
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
    bytes[0] = 'I';
    bytes[1] = 'I';
    p16(bytes, 2, 42);
    p32(bytes, 4, 8);
    p16(bytes, 8, std::uint16_t(entries.size()));
    std::size_t ext = bytes.size();

    for (std::size_t i = 0; i < entries.size(); ++i) {
        auto& e = entries[i];
        e.entryOffset = 10 + 12 * i;
        p16(bytes, e.entryOffset, e.tag);
        p16(bytes, e.entryOffset + 2, e.type);
        p32(bytes, e.entryOffset + 4, e.count);
        if (e.data.size() <= 4) {
            std::copy(e.data.begin(), e.data.end(), bytes.begin() + std::ptrdiff_t(e.entryOffset + 8));
        } else {
            e.payloadOffset = ext;
            p32(bytes, e.entryOffset + 8, std::uint32_t(ext));
            bytes.insert(bytes.end(), e.data.begin(), e.data.end());
            ext = bytes.size();
        }
    }

    std::vector<std::uint32_t> offsets;
    std::vector<std::uint32_t> counts;
    for (std::uint32_t strip = 0; strip < stripCount; ++strip) {
        const int y0 = int(strip * rowsPerStrip);
        const int rows = std::min<int>(int(rowsPerStrip), h - y0);
        offsets.push_back(std::uint32_t(bytes.size()));
        counts.push_back(std::uint32_t(rows * w * 2));
        for (int y = y0; y < y0 + rows; ++y) {
            for (int x = 0; x < w; ++x) {
                const std::size_t o = bytes.size();
                bytes.resize(o + 2);
                p16(bytes, o, raw[std::size_t(y) * std::size_t(w) + std::size_t(x)]);
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

std::unique_ptr<TileNativeDngSource> open_source(const std::vector<std::uint8_t>& bytes,
                                                  const std::array<float, 9>& matrix) {
    auto mem = std::make_shared<DngMemSource>(bytes);
    OpenOptions options;
    options.sourceEvidenceId = "tile_dng_multiworker_streaming_fixture";
    options.color.valid = true;
    options.color.bindingId = "fixture_color_binding";
    options.color.cameraToXyzD50 = matrix;
    std::unique_ptr<TileNativeDngSource> source;
    const auto opened = TileNativeDngSource::open(mem, options, source);
    REQUIRE(opened);
    return source;
}

class ProbeSource final : public IRawTileSource {
public:
    explicit ProbeSource(IRawTileSource& inner) : inner_(inner) {}
    const DngMetadata& metadata() const override { return inner_.metadata(); }
    std::size_t residentBytesUpperBound() const override { return inner_.residentBytesUpperBound(); }
    StreamStatus readRawTile(const TileRect& r, std::uint16_t* raw, std::size_t rawN,
                             float* gain, std::size_t gainN) override {
        enter();
        auto status = inner_.readRawTile(r, raw, rawN, gain, gainN);
        leave();
        return status;
    }
    StreamStatus readRowBias(int y0, int y1, float* out, std::size_t n) override {
        enter();
        auto status = inner_.readRowBias(y0, y1, out, n);
        leave();
        return status;
    }
    StreamStatus readColBias(int x0, int x1, float* out, std::size_t n) override {
        enter();
        auto status = inner_.readColBias(x0, x1, out, n);
        leave();
        return status;
    }
    int peak() const { return peak_.load(); }
private:
    void enter() {
        const int now = active_.fetch_add(1) + 1;
        int old = peak_.load();
        while (now > old && !peak_.compare_exchange_weak(old, now)) {}
    }
    void leave() { active_.fetch_sub(1); }
    IRawTileSource& inner_;
    std::atomic<int> active_{0};
    std::atomic<int> peak_{0};
};

void exact_float_bytes(const std::vector<float>& a, const std::vector<float>& b, const char* label) {
    REQUIRE(a.size() == b.size());
    if (!a.empty() && std::memcmp(a.data(), b.data(), a.size() * sizeof(float)) != 0) {
        std::cerr << "EXACT_FLOAT_MISMATCH " << label
                  << " max_abs_diff=" << max_abs_diff(a, b) << "\n";
        std::exit(3);
    }
}

struct Output {
    StreamingResult result;
    std::vector<float> sdr;
    std::vector<float> gain;
    std::vector<float> diagnostic;
    SourceAudit audit;
    int sourcePeak = 0;
    MultiWorkerTelemetry telemetry;
};

Output reference_run(const std::vector<std::uint8_t>& bytes,
                     const std::array<float, 9>& matrix, int w, int h) {
    auto source = open_source(bytes, matrix);
    auto reconstruction = std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto appearance = std::make_shared<SkinSafeDetailedCrispAppearance>();
    CollectSink sink(w, h, true);
    StreamingTruthRawProcessor processor(reconstruction, appearance);
    StreamingOptions options;
    options.tile = {32, 7};
    options.workers = 1;
    options.hdrEnabled = true;
    options.streamScientificDiagnostics = true;
    StreamingResult result;
    const auto status = processor.process(*source, sink, options, result);
    REQUIRE(status);
    REQUIRE(sink.finished());
    return {result, sink.sdr(), sink.gain(), sink.diagnostic(), source->audit(), 1, {}};
}

Output candidate_run(const std::vector<std::uint8_t>& bytes,
                     const std::array<float, 9>& matrix, int w, int h, int workers) {
    auto source = open_source(bytes, matrix);
    ProbeSource probe(*source);
    ResearchEdgeAwareMeasuredPreservingReconstruction reconstruction;
    SkinSafeDetailedCrispAppearance appearance;
    CollectSink sink(w, h, true);
    StreamingOptions options;
    options.tile = {32, 7};
    options.workers = workers;
    options.hdrEnabled = true;
    options.streamScientificDiagnostics = true;
    StreamingResult result;
    MultiWorkerTelemetry telemetry;
    const auto status = process_multiworker_streaming(
        probe, sink, reconstruction, appearance, options, result, telemetry);
    REQUIRE(status);
    REQUIRE(sink.finished());
    return {result, sink.sdr(), sink.gain(), sink.diagnostic(), source->audit(), probe.peak(), telemetry};
}

void compare_outputs(const Output& reference, const Output& candidate, int workers) {
    compare_exposure(reference.result.exposure, candidate.result.exposure);
    REQUIRE(reference.result.stage2Over1Count == candidate.result.stage2Over1Count);
    REQUIRE(reference.result.clippedCount == candidate.result.clippedCount);
    REQUIRE(reference.result.tilesProcessedPass1 == candidate.result.tilesProcessedPass1);
    REQUIRE(reference.result.tilesProcessedPass2 == candidate.result.tilesProcessedPass2);
    exact_float_bytes(reference.sdr, candidate.sdr, "sdr");
    exact_float_bytes(reference.gain, candidate.gain, "halfLogGain");
    exact_float_bytes(reference.diagnostic, candidate.diagnostic, "stage2Diagnostic");
    REQUIRE(candidate.sourcePeak == 1);
    REQUIRE(candidate.telemetry.effectiveWorkers == workers);
    REQUIRE(candidate.telemetry.orderedCommit);
    REQUIRE(candidate.telemetry.maxReadyPackets <= candidate.telemetry.queueDepth);
    REQUIRE(candidate.audit.tileReadCalls == candidate.result.tilesProcessedPass1 + candidate.result.tilesProcessedPass2);
    REQUIRE(!candidate.audit.fullRawMaterialized);
    REQUIRE(!candidate.audit.fullFileMaterialized);
    REQUIRE(candidate.result.provenance.physicalFrameCount == 1);
    REQUIRE(candidate.result.provenance.independentEvidenceCount == 1);
}

} // namespace

int main() {
    // The support header exposes TU-local fixture helpers; exercise them under -Werror.
    { const auto fixture = make_frame(2, 2); REQUIRE(fixture.meta.width == 2); }

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
    const auto reference = reference_run(bytes, matrix, w, h);
    const auto two = candidate_run(bytes, matrix, w, h, 2);
    compare_outputs(reference, two, 2);
    const auto four = candidate_run(bytes, matrix, w, h, 4);
    compare_outputs(reference, four, 4);

    std::cout << "TILE_NATIVE_DNG_MULTIWORKER_STREAMING_V0_2_PASS\n"
              << "source_concurrency=1\n"
              << "workers_tested=2,4\n"
              << "authoritative_output_equivalence=EXACT_FLOAT_BYTES\n";
    return 0;
}
