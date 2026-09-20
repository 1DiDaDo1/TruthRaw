#include "output_acutance_v0_81.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace bridge = truthraw::output_acutance_v0_81;

namespace {
#define REQUIRE(x) do { if(!(x)){ std::cerr<<"FAIL line "<<__LINE__<<": "<<#x<<"\n"; std::exit(2);} } while(0)

std::vector<float> make_base(int w, int h) {
    std::vector<float> rgb(3u * std::size_t(w) * std::size_t(h));
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const std::size_t i = std::size_t(y) * w + x;
            const float ramp = 0.025f + 0.55f * float(x + 2 * y + 1) / float(w + 2 * h);
            const float texture = 0.012f * std::sin(0.77f * float(x)) * std::cos(0.61f * float(y));
            const float edge = x >= w / 2 ? 0.09f : 0.0f;
            rgb[3u*i] = ramp + edge + texture;
            rgb[3u*i+1u] = 0.91f * ramp + 0.55f * edge - 0.3f * texture;
            rgb[3u*i+2u] = 0.72f * ramp + 0.25f * edge + 0.6f * texture;
        }
    }
    return rgb;
}

bool same_bits(float a, float b) {
    return std::bit_cast<std::uint32_t>(a) == std::bit_cast<std::uint32_t>(b);
}

void canonical_output_and_plan_parity(
    float sigma,
    float ratio,
    truthraw_v47k::OutputProfile profile) {
    constexpr int w = 31;
    constexpr int h = 23;
    const auto input = make_base(w, h);
    const std::size_t pixels = std::size_t(w) * h;
    std::vector<float> half(pixels, 0.0f);
    std::vector<std::uint8_t> censor(pixels, 0u);

    std::vector<float> actual;
    std::vector<float> gains;
    bridge::Result result{};
    REQUIRE(bridge::apply_final_resize_acutance_and_rebase_hdr(
        input, w, h, sigma, ratio, profile, false,
        half, censor, actual, gains, result));

    const auto expectedPlan =
        truthraw_v47k::choose_output_acutance_plan(sigma, ratio, profile);
    std::vector<float> expected(input.size());
    REQUIRE(truthraw_v47k::apply_output_acutance(
        input.data(), w, h, expectedPlan, expected.data()));

    REQUIRE(same_bits(result.plan.noiseSigmaAt2Pct, expectedPlan.noiseSigmaAt2Pct));
    REQUIRE(same_bits(result.plan.noiseConfidence, expectedPlan.noiseConfidence));
    REQUIRE(same_bits(result.plan.resizeRatio, expectedPlan.resizeRatio));
    REQUIRE(same_bits(result.plan.resizeNeed, expectedPlan.resizeNeed));
    REQUIRE(same_bits(result.plan.strength, expectedPlan.strength));
    REQUIRE(same_bits(result.plan.deltaCap, expectedPlan.deltaCap));
    REQUIRE(result.profile == profile);
    REQUIRE(result.applied);
    REQUIRE(!result.hdrRebased);
    REQUIRE(result.changedPixels > 0u);
    REQUIRE(gains.size() == pixels);
    REQUIRE(std::all_of(gains.begin(), gains.end(), [](float g){ return same_bits(g,1.0f); }));

    REQUIRE(actual.size() == expected.size());
    for (std::size_t i=0; i<actual.size(); ++i) {
        if (!same_bits(actual[i], expected[i])) {
            std::cerr << "canonical output mismatch i=" << i << "\n";
            std::exit(2);
        }
    }
    for (std::size_t i=0; i<input.size(); ++i) {
        const auto regenerated = make_base(w,h);
        REQUIRE(same_bits(input[i], regenerated[i]));
    }
}

void hdr_rebase_preserves_effective_target() {
    constexpr int w = 27;
    constexpr int h = 19;
    const auto input = make_base(w,h);
    const std::size_t pixels = std::size_t(w)*h;
    std::vector<float> half(pixels, 0.35f);
    std::vector<std::uint8_t> censor(pixels, 0u);

    // One censored point and one zero-HDR point.
    censor[7] = 1u;
    half[11] = 0.0f;

    std::vector<float> out;
    std::vector<float> gain;
    bridge::Result result{};
    REQUIRE(bridge::apply_final_resize_acutance_and_rebase_hdr(
        input, w, h, 0.00135f, 4.0f,
        truthraw_v47k::OutputProfile::AdaptiveDetail,
        true, half, censor, out, gain, result));

    REQUIRE(result.hdrRebased);
    REQUIRE(result.hdrRebasedPixels > 0u);
    REQUIRE(same_bits(gain[7],1.0f));
    REQUIRE(same_bits(gain[11],1.0f));

    float maxUnclampedError = 0.0f;
    for (std::size_t i=0; i<pixels; ++i) {
        if (censor[i] != 0u) continue;
        if (half[i] <= 1e-5f) {
            REQUIRE(same_bits(gain[i],1.0f));
            continue;
        }
        const float beforeY = truthraw_v47k::luminance709(
            input[3u*i],input[3u*i+1u],input[3u*i+2u]);
        const float afterY = truthraw_v47k::luminance709(
            out[3u*i],out[3u*i+1u],out[3u*i+2u]);
        const float target =
            beforeY * bridge::legacy_display_gain_from_half_log(half[i]);
        const float realized = afterY * gain[i];

        // For the normal validated range used here no 3.04x cap should bind.
        if (afterY > 1e-8f && target > afterY) {
            const float rawNeeded = target / afterY;
            if (rawNeeded < 3.039f) {
                maxUnclampedError = std::max(maxUnclampedError, std::abs(realized-target));
            }
        } else {
            REQUIRE(gain[i] >= 1.0f);
        }
    }
    REQUIRE(maxUnclampedError <= 2.0e-6f);
    REQUIRE(result.maxEffectiveHdrTargetAbsError < 0.01f);
}

void high_gain_is_bounded_and_censor_stays_unity() {
    constexpr int w=13,h=11;
    const auto input=make_base(w,h);
    const std::size_t pixels=std::size_t(w)*h;
    std::vector<float> half(pixels,10.0f);
    std::vector<std::uint8_t> censor(pixels,0u);
    censor[pixels/2u]=1u;
    std::vector<float> out,gain;
    bridge::Result result{};
    REQUIRE(bridge::apply_final_resize_acutance_and_rebase_hdr(
        input,w,h,0.0007f,8.0f,
        truthraw_v47k::OutputProfile::AdaptiveDetail,
        true,half,censor,out,gain,result));
    for(std::size_t i=0;i<pixels;++i){
        REQUIRE(gain[i] >= 1.0f);
        REQUIRE(gain[i] <= 3.040001f);
    }
    REQUIRE(same_bits(gain[pixels/2u],1.0f));
}

void invalid_contracts_fail_closed() {
    std::vector<float> base(3u*4u*4u,0.2f);
    std::vector<float> half(16u,0.1f);
    std::vector<std::uint8_t> censor(16u,0u);
    std::vector<float> out,gain;
    bridge::Result result{};

    REQUIRE(!bridge::apply_final_resize_acutance_and_rebase_hdr(
        base,4,4,-0.1f,2.0f,truthraw_v47k::OutputProfile::Neutral,
        true,half,censor,out,gain,result));
    REQUIRE(!bridge::apply_final_resize_acutance_and_rebase_hdr(
        base,4,4,0.0f,0.5f,truthraw_v47k::OutputProfile::Neutral,
        true,half,censor,out,gain,result));
    half.pop_back();
    REQUIRE(!bridge::apply_final_resize_acutance_and_rebase_hdr(
        base,4,4,0.0f,2.0f,truthraw_v47k::OutputProfile::Neutral,
        true,half,censor,out,gain,result));
}

} // namespace

int main(){
    canonical_output_and_plan_parity(0.0f,1.0f,truthraw_v47k::OutputProfile::Neutral);
    canonical_output_and_plan_parity(0.0013f,2.0f,truthraw_v47k::OutputProfile::AdaptiveDetail);
    canonical_output_and_plan_parity(0.0022f,8.0f,truthraw_v47k::OutputProfile::AdaptiveDetail);
    hdr_rebase_preserves_effective_target();
    high_gain_is_bounded_and_censor_stays_unity();
    invalid_contracts_fail_closed();
    std::cout<<"OUTPUT_ACUTANCE_V0_81_PASS\n";
    return 0;
}
