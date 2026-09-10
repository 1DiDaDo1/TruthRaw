#include "uncertainty_aware_scurve_v1.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace truthraw_scurve_v1 {
namespace {

constexpr double kEps = 1e-12;

bool finite_rgb(const std::array<double,3>& x) {
    return std::isfinite(x[0]) && std::isfinite(x[1]) && std::isfinite(x[2]);
}

double clamp01(double x) { return std::clamp(x, 0.0, 1.0); }

double confidence_from_snr(double y, const AppearanceEvidenceV1& e, const AppearanceParamsV1& p) {
    if (!e.luminanceSigmaKnown || !std::isfinite(e.luminanceSigmaUpper) || e.luminanceSigmaUpper < 0.0) {
        return 0.0;
    }
    if (e.luminanceSigmaUpper <= kEps) return 1.0;
    const double snr = std::max(0.0, y) / e.luminanceSigmaUpper;
    const double d = std::max(p.snrHigh - p.snrLow, kEps);
    return smoothstep01_v1((snr - p.snrLow) / d);
}

std::array<double,3> scalar_scale(const std::array<double,3>& rgb, double s) {
    return {rgb[0]*s, rgb[1]*s, rgb[2]*s};
}

bool in_gamut(const std::array<double,3>& rgb) {
    constexpr double tol = 1e-12;
    for (double v : rgb) if (v < -tol || v > 1.0 + tol || !std::isfinite(v)) return false;
    return true;
}

std::array<double,3> gamut_limit_scalar(const std::array<double,3>& rgb, bool& limited) {
    double mx = std::max({rgb[0], rgb[1], rgb[2]});
    const double mn = std::min({rgb[0], rgb[1], rgb[2]});
    limited = false;
    if (mn < 0.0) limited = true;
    std::array<double,3> out {std::max(0.0,rgb[0]), std::max(0.0,rgb[1]), std::max(0.0,rgb[2])};
    mx = std::max({out[0],out[1],out[2]});
    if (mx > 1.0) {
        const double s = 1.0 / mx;
        out = scalar_scale(out, s);
        limited = true;
    }
    return out;
}

std::array<double,3> rgb_to_oklab(const std::array<double,3>& c) {
    const double l = 0.4122214708*c[0] + 0.5363325363*c[1] + 0.0514459929*c[2];
    const double m = 0.2119034982*c[0] + 0.6806995451*c[1] + 0.1073969566*c[2];
    const double s = 0.0883024619*c[0] + 0.2817188376*c[1] + 0.6299787005*c[2];
    const double l_ = std::cbrt(std::max(0.0,l));
    const double m_ = std::cbrt(std::max(0.0,m));
    const double s_ = std::cbrt(std::max(0.0,s));
    return {
        0.2104542553*l_ + 0.7936177850*m_ - 0.0040720468*s_,
        1.9779984951*l_ - 2.4285922050*m_ + 0.4505937099*s_,
        0.0259040371*l_ + 0.7827717662*m_ - 0.8086757660*s_
    };
}

std::array<double,3> oklab_to_rgb(const std::array<double,3>& lab) {
    const double l_ = lab[0] + 0.3963377774*lab[1] + 0.2158037573*lab[2];
    const double m_ = lab[0] - 0.1055613458*lab[1] - 0.0638541728*lab[2];
    const double s_ = lab[0] - 0.0894841775*lab[1] - 1.2914855480*lab[2];
    const double l = l_*l_*l_;
    const double m = m_*m_*m_;
    const double s = s_*s_*s_;
    return {
         4.0767416621*l - 3.3077115913*m + 0.2309699292*s,
        -1.2684380046*l + 2.6097574011*m - 0.3413193965*s,
        -0.0041960863*l - 0.7034186147*m + 1.7076147010*s
    };
}

std::array<double,3> chroma_scaled_preserve_y(
    const std::array<double,3>& rgb, double gain, double targetY) {
    auto lab = rgb_to_oklab(rgb);
    lab[1] *= gain;
    lab[2] *= gain;
    auto out = oklab_to_rgb(lab);
    const double y = linear_srgb_luminance_v1(out);
    if (y > kEps && targetY >= 0.0) out = scalar_scale(out, targetY / y);
    return out;
}

double tone_value(double y, double q, const AppearanceParamsV1& p) {
    const double safe = uncertain_shadow_curve_v1(y, p.uncertainShadowFloor, p.uncertainShadowEnd);
    const double contrast = power_s_curve_v1(y, p.reliableSCurvePower);
    return safe*(1.0-q) + contrast*q;
}

double tone_slope_numeric(double y, double q, const AppearanceParamsV1& p) {
    const double h = 1e-5;
    const double lo = std::max(0.0, y-h);
    const double hi = std::min(1.0, y+h);
    if (hi <= lo) return 0.0;
    return (tone_value(hi,q,p)-tone_value(lo,q,p))/(hi-lo);
}

} // namespace

double linear_srgb_luminance_v1(const std::array<double,3>& rgb) {
    return 0.2126*rgb[0] + 0.7152*rgb[1] + 0.0722*rgb[2];
}

double smoothstep01_v1(double x) {
    x = clamp01(x);
    return x*x*(3.0-2.0*x);
}

double power_s_curve_v1(double x, double power) {
    x = clamp01(x);
    if (!(std::isfinite(power) && power >= 1.0)) return x;
    if (x <= 0.0 || x >= 1.0) return x;
    const double a = std::pow(x,power);
    const double b = std::pow(1.0-x,power);
    return a/(a+b);
}

double uncertain_shadow_curve_v1(double x, double floorSlope, double shadowEnd) {
    x = clamp01(x);
    floorSlope = clamp01(floorSlope);
    if (!(std::isfinite(shadowEnd) && shadowEnd > 0.0)) return x;
    const double t = smoothstep01_v1(x/shadowEnd);
    const double scale = floorSlope + (1.0-floorSlope)*t;
    return x*scale;
}

StatusV1 apply_uncertainty_aware_scurve_v1(
    const std::array<double,3>& displayLinearRgb,
    const AppearanceEvidenceV1& evidence,
    const AppearanceParamsV1& p,
    AppearanceResultV1& out) {
    out = {};
    if (!finite_rgb(displayLinearRgb)) return StatusV1::error("RGB must be finite");
    for (double v : displayLinearRgb) if (v < 0.0) return StatusV1::error("display-linear RGB must be non-negative");
    if (!(p.snrHigh > p.snrLow && p.snrLow >= 0.0)) return StatusV1::error("invalid SNR gates");
    if (!(p.reliableSCurvePower >= 1.0 && std::isfinite(p.reliableSCurvePower))) return StatusV1::error("invalid S-curve power");
    if (!(p.chromaGainLowConfidence > 0.0 && p.chromaGainHighConfidence >= p.chromaGainLowConfidence)) return StatusV1::error("invalid chroma gains");
    if (!(p.chromaConfidenceHigh > p.chromaConfidenceLow)) return StatusV1::error("invalid chroma confidence gates");

    const double y0 = linear_srgb_luminance_v1(displayLinearRgb);
    out.luminanceIn = y0;
    const double y = clamp01(y0);
    const double qY = confidence_from_snr(y,evidence,p);
    out.luminanceConfidence = qY;
    const double yTone = tone_value(y,qY,p);
    out.localToneSlope = tone_slope_numeric(y,qY,p);
    if (evidence.luminanceSigmaKnown && std::isfinite(evidence.luminanceSigmaUpper) && evidence.luminanceSigmaUpper >= 0.0) {
        out.sigmaOutUpper = std::abs(out.localToneSlope)*evidence.luminanceSigmaUpper;
    }

    const double toneScale = y0 > kEps ? yTone/y0 : 1.0;
    auto toned = scalar_scale(displayLinearRgb,toneScale);
    toned = gamut_limit_scalar(toned,out.toneGamutLimited);
    out.luminanceAfterTone = linear_srgb_luminance_v1(toned);

    double qC = 0.0;
    if (evidence.chromaConfidenceKnown && std::isfinite(evidence.chromaConfidence)) qC = clamp01(evidence.chromaConfidence);
    if (evidence.sourceHighCensored) qC = 0.0;
    out.chromaConfidenceUsed = qC;
    const double qScaled = smoothstep01_v1((qC-p.chromaConfidenceLow) /
        std::max(p.chromaConfidenceHigh-p.chromaConfidenceLow,kEps));
    const double desiredGain = p.chromaGainLowConfidence +
        (p.chromaGainHighConfidence-p.chromaGainLowConfidence)*qScaled;
    out.requestedChromaGain = desiredGain;

    const double targetY = out.luminanceAfterTone;
    auto colored = chroma_scaled_preserve_y(toned,desiredGain,targetY);
    double applied = desiredGain;
    if (!in_gamut(colored)) {
        out.chromaGamutLimited = true;
        if (desiredGain > 1.0) {
            double lo = 1.0, hi = desiredGain;
            for (int i=0;i<std::max(1,p.gamutSearchIterations);++i) {
                const double mid = 0.5*(lo+hi);
                auto trial = chroma_scaled_preserve_y(toned,mid,targetY);
                if (in_gamut(trial)) lo=mid; else hi=mid;
            }
            applied = lo;
            colored = chroma_scaled_preserve_y(toned,applied,targetY);
        } else {
            bool lim=false;
            colored = gamut_limit_scalar(colored,lim);
        }
    }
    out.appliedChromaGain = applied;
    out.rgb = {clamp01(colored[0]),clamp01(colored[1]),clamp01(colored[2])};
    return StatusV1::success();
}

} // namespace truthraw_scurve_v1
