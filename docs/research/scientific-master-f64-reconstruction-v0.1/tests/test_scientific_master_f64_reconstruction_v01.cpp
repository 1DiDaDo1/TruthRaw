#include "truthraw/core.h"
#include "scientific_master_f64_reconstruction_v0_1.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "REQUIRE FAILED: " << message << "\n";
        std::exit(2);
    }
}

int phase_index(int y, int x) {
    return (y & 1) * 2 + (x & 1);
}

int color_for_phase(truthraw::CfaPattern cfa, int phase) {
    static const int BGGR[4] = {2, 1, 1, 0};
    static const int RGGB[4] = {0, 1, 1, 2};
    static const int GRBG[4] = {1, 0, 2, 1};
    static const int GBRG[4] = {1, 2, 0, 1};
    const int* p = BGGR;
    if (cfa == truthraw::CfaPattern::RGGB) p = RGGB;
    else if (cfa == truthraw::CfaPattern::GRBG) p = GRBG;
    else if (cfa == truthraw::CfaPattern::GBRG) p = GBRG;
    return p[phase];
}

bool float_bits_equal(float a, float b) {
    return std::bit_cast<std::uint32_t>(a) == std::bit_cast<std::uint32_t>(b);
}

float grad_f32(float p, float q, float c0, float c1, float measured) {
    const float curvature = 2.0f * measured - c0 - c1;
    return std::abs(p - q) + 0.5f * std::abs(curvature);
}

double grad_f64(float p, float q, float c0, float c1, float measured) {
    const double pd = static_cast<double>(p);
    const double qd = static_cast<double>(q);
    const double c0d = static_cast<double>(c0);
    const double c1d = static_cast<double>(c1);
    const double md = static_cast<double>(measured);
    const double curvature = 2.0 * md - c0d - c1d;
    return std::abs(pd - qd) + 0.5 * std::abs(curvature);
}

int directional_choice_f32(float ga, float gb) {
    if (ga < 0.72f * gb) return 0;
    if (gb < 0.72f * ga) return 1;
    return 2;
}

int directional_choice_f64(double ga, double gb) {
    if (ga < 0.72 * gb) return 0;
    if (gb < 0.72 * ga) return 1;
    return 2;
}

void verify_known_branch_flip() {
    // Deterministic Float32 neighbourhood found by a bounded numerical search.
    // It is deliberately close to the existing 0.72 directional threshold.
    const float measured = 0.5847317f;
    const float hp = 0.7186155f;
    const float hq = -0.03919089f;
    const float hc0 = 0.9705281f;
    const float hc1 = 0.4011195f;
    const float vp = 0.1167104f;
    const float vq = 0.02867437f;
    const float vc0 = -0.03383426f;
    const float vc1 = 0.14255595f;

    const float gh32 = grad_f32(hp, hq, hc0, hc1, measured);
    const float gv32 = grad_f32(vp, vq, vc0, vc1, measured);
    const double gh64 = grad_f64(hp, hq, hc0, hc1, measured);
    const double gv64 = grad_f64(vp, vq, vc0, vc1, measured);

    const int c32 = directional_choice_f32(gh32, gv32);
    const int c64 = directional_choice_f64(gh64, gv64);
    require(c32 != c64, "known threshold case must change directional branch");

    std::cout << std::setprecision(17)
              << "known_branch_flip_f32_choice=" << c32 << "\n"
              << "known_branch_flip_f64_choice=" << c64 << "\n"
              << "known_branch_flip_gh32=" << gh32 << "\n"
              << "known_branch_flip_gv32=" << gv32 << "\n"
              << "known_branch_flip_gh64=" << gh64 << "\n"
              << "known_branch_flip_gv64=" << gv64 << "\n";
}

std::vector<float> make_stress_tile(int w, int h) {
    std::vector<float> a(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
    std::uint32_t state = 0x13579bdfu;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            state = state * 1664525u + 1013904223u;
            const float randomPart =
                static_cast<float>((state >> 8) & 0xffffu) / 65535.0f;
            const float edge =
                ((x / 5 + y / 7) & 1) ? 0.68f : 0.18f;
            const float fine =
                static_cast<float>((x * 37 + y * 61 + x * y * 3) % 113) * 0.00037f;
            float v = edge + 0.22f * (randomPart - 0.5f) + fine;
            if (((x * 13 + y * 17) % 97) == 0) v = -0.025f + 0.001f * float(x & 3);
            if (((x * 19 + y * 23) % 131) == 0) v = 1.035f + 0.002f * float(y & 3);
            a[static_cast<std::size_t>(y) * static_cast<std::size_t>(w) +
              static_cast<std::size_t>(x)] = v;
        }
    }
    return a;
}

struct Metrics {
    std::uint64_t reconstructedComponents = 0;
    std::uint64_t differingStoredComponents = 0;
    double maxAbsStoredDelta = 0.0;
};

Metrics compare_backends(truthraw::CfaPattern cfa) {
    constexpr int w = 73;
    constexpr int h = 61;
    const auto input = make_stress_tile(w, h);

    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction f32;
    truthraw::scientific_master_f64_reconstruction_v0_1::ResearchEdgeAwareMeasuredPreservingReconstructionF64 f64;
    std::vector<float> out32(3u * input.size());
    std::vector<float> out64(3u * input.size());

    const auto s32 = f32.reconstructTile(
        input.data(), w, h, 0, 0, 0, 0, w, h, cfa, out32.data());
    const auto s64 = f64.reconstructTile(
        input.data(), w, h, 0, 0, 0, 0, w, h, cfa, out64.data());
    require(static_cast<bool>(s32), "F32 backend failed");
    require(static_cast<bool>(s64), "F64 backend failed");

    Metrics m;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const std::size_t i =
                static_cast<std::size_t>(y) * static_cast<std::size_t>(w) +
                static_cast<std::size_t>(x);
            const int measuredChannel = color_for_phase(cfa, phase_index(y, x));
            for (int c = 0; c < 3; ++c) {
                const float a = out32[3u * i + static_cast<std::size_t>(c)];
                const float b = out64[3u * i + static_cast<std::size_t>(c)];
                require(std::isfinite(a), "F32 output must be finite");
                require(std::isfinite(b), "F64 output must be finite");

                if (c == measuredChannel) {
                    require(float_bits_equal(a, input[i]),
                            "F32 measured CFA component not bit-exact");
                    require(float_bits_equal(b, input[i]),
                            "F64 measured CFA component not bit-exact");
                    require(float_bits_equal(a, b),
                            "measured CFA component differs between backends");
                } else {
                    ++m.reconstructedComponents;
                    if (!float_bits_equal(a, b)) ++m.differingStoredComponents;
                    m.maxAbsStoredDelta = std::max(
                        m.maxAbsStoredDelta,
                        std::abs(static_cast<double>(a) - static_cast<double>(b)));
                }
            }
        }
    }

    require(m.differingStoredComponents > 0,
            "stress tile must expose at least one F32/F64 reconstructed storage difference");
    return m;
}

}  // namespace

int main() {
    verify_known_branch_flip();

    std::uint64_t reconstructed = 0;
    std::uint64_t differing = 0;
    double maxAbs = 0.0;
    const std::array<truthraw::CfaPattern, 4> cfas = {
        truthraw::CfaPattern::BGGR,
        truthraw::CfaPattern::RGGB,
        truthraw::CfaPattern::GRBG,
        truthraw::CfaPattern::GBRG,
    };

    for (const auto cfa : cfas) {
        const auto m = compare_backends(cfa);
        reconstructed += m.reconstructedComponents;
        differing += m.differingStoredComponents;
        maxAbs = std::max(maxAbs, m.maxAbsStoredDelta);
    }

    std::cout << "SCIENTIFIC_MASTER_F64_RECONSTRUCTION_V01_PASS\n";
    std::cout << "measured_cfa_bit_exact=1\n";
    std::cout << "f64_branch_sensitive_compute=1\n";
    std::cout << "float32_storage_boundary=1\n";
    std::cout << "reconstructed_components=" << reconstructed << "\n";
    std::cout << "differing_stored_components=" << differing << "\n";
    std::cout << std::setprecision(17)
              << "max_abs_f32_f64_stored_delta=" << maxAbs << "\n";
    return 0;
}
