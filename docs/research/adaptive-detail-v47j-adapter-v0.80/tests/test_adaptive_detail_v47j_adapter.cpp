#include "adaptive_detail_v47j_adapter.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

extern "C" int truthraw_v47j_reference_apply(
    const float* input,
    int width,
    int height,
    int coreX0,
    int coreY0,
    int coreW,
    int coreH,
    float noiseSigmaAt2Pct,
    float* output);

namespace adapter = truthraw::adaptive_detail_v47j_adapter;

namespace {
#define REQUIRE(x) do { if(!(x)){ std::cerr<<"FAIL line "<<__LINE__<<": "<<#x<<"\n"; std::exit(2);} } while(0)

std::vector<float> make_input(int w, int h) {
    std::vector<float> rgb(3u * std::size_t(w) * std::size_t(h));
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const std::size_t i = std::size_t(y) * w + x;
            const float base =
                0.008f +
                0.74f * float((x * 17 + y * 11 + x * y * 3) % 257) / 256.0f;
            const float edge = x >= w / 2 ? 0.14f : 0.0f;
            const float texture =
                0.018f * std::sin(float(x) * 0.91f) *
                std::cos(float(y) * 0.67f);
            rgb[3u * i] = base + edge + texture;
            rgb[3u * i + 1u] = 0.87f * base + 0.55f * edge - 0.4f * texture;
            rgb[3u * i + 2u] = 0.66f * base + 0.22f * edge + 0.7f * texture;
            if ((x + y) % 31 == 0) rgb[3u * i] = -0.003f;
        }
    }
    return rgb;
}

bool bits_equal(float a, float b) {
    return std::bit_cast<std::uint32_t>(a) ==
           std::bit_cast<std::uint32_t>(b);
}

void compare_case(
    int w,
    int h,
    int x0,
    int y0,
    int cw,
    int ch,
    float noiseSigma) {
    const auto input = make_input(w, h);
    std::vector<float> expected(3u * std::size_t(cw) * std::size_t(ch));
    std::vector<float> actual(expected.size());

    const int referenceStatus = truthraw_v47j_reference_apply(
        input.data(), w, h, x0, y0, cw, ch, noiseSigma, expected.data());
    REQUIRE(referenceStatus == 0);

    adapter::AdaptiveDetailedCrispAppearanceV47j appearance(noiseSigma);
    const auto status = appearance.applyTile(
        input.data(), w, h, x0, y0, cw, ch, actual.data());
    REQUIRE(status);
    REQUIRE(actual.size() == expected.size());

    for (std::size_t i = 0; i < actual.size(); ++i) {
        if (!bits_equal(actual[i], expected[i])) {
            std::cerr << "bit mismatch i=" << i
                      << " expected=" << expected[i]
                      << " actual=" << actual[i] << "\n";
            std::exit(2);
        }
    }

    REQUIRE(appearance.requiredHalo() == 5);
    REQUIRE(appearance.profile() == truthraw::AppearanceProfile::ExternalProfile);
    REQUIRE(std::string(appearance.name()) ==
            "adaptive_detailed_crisp_multiband_hard_edge_guard_v47j");
    REQUIRE(std::string(appearance.colorFidelityPolicy()) ==
            "luminance_only_rgb_direction_preserved_no_semantic_segmentation");
}

void noise_sigma_parity() {
    truthraw::DngMetadata metadata{};
    metadata.hasNoiseProfile = true;
    metadata.noiseProfile = {
        0.0007621435f, 1.8076375e-6f,
        0.0007947308f, 1.2580162e-6f,
        0.0007853744f, 1.7522720e-6f,
    };
    float v = 0.0f;
    for (int c = 0; c < 3; ++c) {
        v += std::max(
            metadata.noiseProfile[2 * c] * 0.02f +
                metadata.noiseProfile[2 * c + 1],
            0.0f);
    }
    const float expected = std::sqrt(v / 3.0f);
    const float actual = adapter::noise_sigma_2pct_from_metadata(metadata);
    REQUIRE(bits_equal(actual, expected));

    metadata.hasNoiseProfile = false;
    REQUIRE(bits_equal(
        adapter::noise_sigma_2pct_from_metadata(metadata), 0.0f));
}

void invalid_geometry_fails_same_way() {
    auto input = make_input(17, 17);
    std::vector<float> out(3u * 10u * 10u);
    adapter::AdaptiveDetailedCrispAppearanceV47j appearance(0.0014f);
    const auto status = appearance.applyTile(
        input.data(), 17, 17, 12, 12, 10, 10, out.data());
    REQUIRE(!status);
    REQUIRE(status.code == truthraw::StatusCode::InvalidArgument);
}

}  // namespace

int main() {
    compare_case(37, 29, 5, 5, 27, 19, 0.0f);
    compare_case(37, 29, 5, 5, 27, 19, 0.0008f);
    compare_case(37, 29, 5, 5, 27, 19, 0.00145f);
    compare_case(37, 29, 5, 5, 27, 19, 0.0022f);
    compare_case(23, 21, 0, 0, 18, 16, 0.0013f);
    compare_case(23, 21, 5, 5, 18, 16, 0.0013f);
    noise_sigma_parity();
    invalid_geometry_fails_same_way();
    std::cout << "ADAPTIVE_DETAIL_V47J_ADAPTER_PARITY_PASS\n";
    return 0;
}
