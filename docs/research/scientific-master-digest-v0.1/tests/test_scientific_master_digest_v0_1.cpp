#include "scientific_master_digest_v0_1.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

using truthraw::scientific_master_digest::v0_1::ScientificMasterDigestAccumulator;
using truthraw::scientific_master_digest::v0_1::Sha256;
using truthraw::scientific_master_digest::v0_1::TileView;
using truthraw::scientific_master_digest::v0_1::to_hex;

namespace {

struct TestContext {
    int failures = 0;
    void expect(bool condition, const std::string& message) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << '\n';
        }
    }
};

std::vector<float> make_master(std::uint32_t width, std::uint32_t height) {
    std::vector<float> rgb(static_cast<std::size_t>(width) * height * 3u);
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            const std::size_t i = (static_cast<std::size_t>(y) * width + x) * 3u;
            rgb[i + 0] = static_cast<float>(x) * 0.001f - static_cast<float>(y) * 0.0001f;
            rgb[i + 1] = static_cast<float>(x + y) * 0.0007f;
            rgb[i + 2] = static_cast<float>(y) * 0.0013f - 0.125f;
        }
    }
    return rgb;
}

bool add_partition(ScientificMasterDigestAccumulator& acc,
                   const std::vector<float>& rgb,
                   std::uint32_t width,
                   std::uint32_t height,
                   std::uint32_t tileEdge,
                   bool reverseOrder = false) {
    struct Rect { std::uint32_t x, y, w, h; };
    std::vector<Rect> rects;
    for (std::uint32_t y = 0; y < height; y += tileEdge) {
        for (std::uint32_t x = 0; x < width; x += tileEdge) {
            rects.push_back({x, y, std::min(tileEdge, width - x), std::min(tileEdge, height - y)});
        }
    }
    if (reverseOrder) std::reverse(rects.begin(), rects.end());
    for (const auto& r : rects) {
        const float* p = rgb.data() + (static_cast<std::size_t>(r.y) * width + r.x) * 3u;
        TileView tile{r.x, r.y, r.w, r.h, p, static_cast<std::size_t>(width) * 3u};
        if (!acc.add_tile(tile)) return false;
    }
    return true;
}

Sha256 digest_for_partition(const std::vector<float>& rgb,
                            std::uint32_t width,
                            std::uint32_t height,
                            std::uint32_t tileEdge,
                            bool reverseOrder,
                            TestContext& t) {
    ScientificMasterDigestAccumulator acc(width, height);
    t.expect(acc.valid(), "accumulator must accept dimensions");
    t.expect(add_partition(acc, rgb, width, height, tileEdge, reverseOrder),
             "canonical partition must be accepted: " + acc.error());
    Sha256 digest{};
    t.expect(acc.finalize(digest), "complete master must finalize: " + acc.error());
    return digest;
}

void test_known_vector(TestContext& t) {
    const std::vector<float> rgb = {1.0f, -0.0f, 2.0f};
    ScientificMasterDigestAccumulator acc(1, 1);
    TileView tile{0, 0, 1, 1, rgb.data(), 3};
    t.expect(acc.add_tile(tile), "known-vector tile accepted");
    Sha256 digest{};
    t.expect(acc.finalize(digest), "known-vector finalization succeeds");
    t.expect(to_hex(digest) == "39f2b2bb86579267fd9ad903ae9f231aa93605517395900b723ccfc2419d6190",
             "known-vector SHA-256 must match independent reference implementation");
}

void test_partition_and_order_invariance(TestContext& t) {
    constexpr std::uint32_t width = 258;
    constexpr std::uint32_t height = 194;
    const auto rgb = make_master(width, height);
    const auto d64 = digest_for_partition(rgb, width, height, 64, false, t);
    const auto d128 = digest_for_partition(rgb, width, height, 128, false, t);
    const auto d256 = digest_for_partition(rgb, width, height, 256, false, t);
    const auto d128Reverse = digest_for_partition(rgb, width, height, 128, true, t);
    t.expect(d64 == d128, "64 and 128 runtime tiles must have identical master identity");
    t.expect(d64 == d256, "64 and 256 runtime tiles must have identical master identity");
    t.expect(d64 == d128Reverse, "runtime tile traversal order must not change master identity");
}

void test_bitwise_sensitivity(TestContext& t) {
    constexpr std::uint32_t width = 130;
    constexpr std::uint32_t height = 70;
    auto rgb = make_master(width, height);
    const auto before = digest_for_partition(rgb, width, height, 64, false, t);
    rgb[(static_cast<std::size_t>(17) * width + 29) * 3u + 1u] =
        std::nextafter(rgb[(static_cast<std::size_t>(17) * width + 29) * 3u + 1u],
                       std::numeric_limits<float>::infinity());
    const auto after = digest_for_partition(rgb, width, height, 128, false, t);
    t.expect(before != after, "one float-bit content change must change Scientific Master digest");
}

void test_signed_zero_is_identity(TestContext& t) {
    const std::vector<float> plus = {1.0f, +0.0f, 2.0f};
    const std::vector<float> minus = {1.0f, -0.0f, 2.0f};
    const auto a = digest_for_partition(plus, 1, 1, 64, false, t);
    const auto b = digest_for_partition(minus, 1, 1, 64, false, t);
    t.expect(a != b, "v0.1 identity is bit-exact and must preserve signed zero distinction");
}

void test_fail_closed_contracts(TestContext& t) {
    {
        ScientificMasterDigestAccumulator acc(128, 64);
        auto rgb = make_master(64, 64);
        TileView tile{0, 0, 64, 64, rgb.data(), 64u * 3u};
        t.expect(acc.add_tile(tile), "first half master accepted");
        Sha256 digest{};
        t.expect(!acc.finalize(digest), "missing canonical cell must block finalization");
    }
    {
        ScientificMasterDigestAccumulator acc(64, 64);
        auto rgb = make_master(64, 64);
        TileView tile{0, 0, 64, 64, rgb.data(), 64u * 3u};
        t.expect(acc.add_tile(tile), "first cell accepted");
        t.expect(!acc.add_tile(tile), "duplicate canonical cell must fail closed");
    }
    {
        ScientificMasterDigestAccumulator acc(64, 64);
        auto rgb = make_master(64, 64);
        rgb[7] = std::numeric_limits<float>::quiet_NaN();
        TileView tile{0, 0, 64, 64, rgb.data(), 64u * 3u};
        t.expect(!acc.add_tile(tile), "NaN must fail closed");
    }
    {
        ScientificMasterDigestAccumulator acc(128, 64);
        auto rgb = make_master(63, 64);
        TileView tile{1, 0, 63, 64, rgb.data(), 63u * 3u};
        t.expect(!acc.add_tile(tile), "non-canonical tile origin must fail closed");
    }
}

void test_200mp_state_bound(TestContext& t) {
    constexpr std::uint32_t width = 16320;
    constexpr std::uint32_t height = 12288;
    ScientificMasterDigestAccumulator acc(width, height);
    t.expect(acc.valid(), "200MP-class dimensions must fit bounded digest state");
    const auto m = acc.metrics();
    t.expect(m.cellColumns == 255, "200MP-class master cell columns = 255");
    t.expect(m.cellRows == 192, "200MP-class master cell rows = 192");
    t.expect(m.cellCount == 48960, "200MP-class master cell count = 48960");
    t.expect(m.residentBytesUpperBound < 2u * 1024u * 1024u,
             "digest bookkeeping for 200MP-class master must remain below 2 MiB");
}

}  // namespace

int main() {
    TestContext t;
    test_known_vector(t);
    test_partition_and_order_invariance(t);
    test_bitwise_sensitivity(t);
    test_signed_zero_is_identity(t);
    test_fail_closed_contracts(t);
    test_200mp_state_bound(t);
    if (t.failures != 0) {
        std::cerr << t.failures << " test assertion(s) failed\n";
        return 1;
    }
    std::cout << "Scientific Master Digest v0.1: PASS\n";
    return 0;
}
