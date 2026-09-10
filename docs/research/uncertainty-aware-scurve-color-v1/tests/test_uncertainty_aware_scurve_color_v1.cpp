#include "uncertainty_aware_scurve_color_v1.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace truthraw::appearance::v1;

namespace {
void require(bool ok, const char* msg) {
    if (!ok) {
        std::cerr << "FAIL: " << msg << '\n';
        std::exit(1);
    }
}

bool near(float a, float b, float eps = 2.0e-6f) {
    return std::fabs(a - b) <= eps;
}

float chroma_radius(const RgbLinearV1& x) {
    const float y = luminance709(x);
    const float dr = x.r-y, dg = x.g-y, db = x.b-y;
    return std::sqrt(dr*dr + dg*dg + db*db);
}
}

int main() {
    AppearanceConfigV1 c{};
    require(validate_config(c), "default config valid");

    // Fixed-strength S-curve is monotonic over its full domain.
    float prev = -1.0f;
    for (int i = 0; i <= 10000; ++i) {
        const float x = static_cast<float>(i) / 10000.0f;
        const float y = s_curve01(x, 0.54f);
        require(y + 1.0e-7f >= prev, "S-curve monotonic");
        prev = y;
    }

    const RgbLinearV1 input{0.12f, 0.08f, 0.05f};
    const RgbLinearV1 saved = input;

    auto low = apply_pixel(input, {0.1f, 0.0f, false}, c);
    auto high = apply_pixel(input, {0.9f, 1.0f, false}, c);
    require(low.status == AppearanceStatusV1::Applied, "low-confidence apply");
    require(high.status == AppearanceStatusV1::Applied, "high-confidence apply");
    require(input.r == saved.r && input.g == saved.g && input.b == saved.b,
            "input/scientific data not mutated");
    require(!low.scientificMasterModified && !high.scientificMasterModified,
            "master modification flag false");

    require(low.effectiveChromaGain < 1.0f, "low-confidence chroma restrained");
    require(high.effectiveChromaGain > 1.0f, "high-confidence chroma may be fuller");
    require(chroma_radius(high.rgb) > chroma_radius(low.rgb),
            "supported chroma gets more appearance freedom");

    // Low-confidence dark values receive stronger shadow compression, but
    // high-confidence midtones do not receive uncertainty-driven extra slope.
    require(low.effectiveCurveStrength > high.effectiveCurveStrength,
            "dark low-confidence extra compression active");
    auto midLow = apply_pixel({0.55f,0.50f,0.45f}, {0.0f,0.0f,false}, c);
    auto midHigh = apply_pixel({0.55f,0.50f,0.45f}, {1.0f,1.0f,false}, c);
    require(near(midLow.effectiveCurveStrength, c.globalCurveStrength),
            "low confidence does not alter midtone curve");
    require(near(midHigh.effectiveCurveStrength, c.globalCurveStrength),
            "high confidence uses same global midtone curve");

    // Chroma adjustment around luminance must preserve luminance before final
    // gamut clamp to numerical precision.
    require(near(low.outputLuminanceBeforeClamp,
                 s_curve01(low.inputLuminance, low.effectiveCurveStrength), 3.0e-6f),
            "low-confidence luminance preserved through chroma gate");
    require(near(high.outputLuminanceBeforeClamp,
                 s_curve01(high.inputLuminance, high.effectiveCurveStrength), 3.0e-6f),
            "high-confidence luminance preserved through chroma gate");

    auto censored = apply_pixel(input, {1.0f,1.0f,true}, c);
    require(censored.effectiveChromaGain <= 1.0f,
            "censored source cannot receive positive chroma enhancement");

    // Fail closed: invalid confidence or out-of-domain display RGB returns the
    // original pixel exactly.
    auto badQ = apply_pixel(input, {1.2f,0.5f,false}, c);
    require(badQ.status == AppearanceStatusV1::InvalidInput, "bad confidence rejected");
    require(badQ.rgb.r == input.r && badQ.rgb.g == input.g && badQ.rgb.b == input.b,
            "bad confidence fail-closed identity");
    auto badRgb = apply_pixel({-0.1f,0.2f,0.3f}, {0.5f,0.5f,false}, c);
    require(badRgb.status == AppearanceStatusV1::InvalidInput, "negative display RGB rejected");

    AppearanceConfigV1 badC = c;
    badC.highConfidenceChromaGain = 2.0f;
    auto badCfg = apply_pixel(input, {0.5f,0.5f,false}, badC);
    require(badCfg.status == AppearanceStatusV1::InvalidConfig, "unsafe config rejected");

    std::cout << "TRUTHRAW_UNCERTAINTY_AWARE_SCURVE_COLOR_V1_TESTS_PASS\n";
    return 0;
}
