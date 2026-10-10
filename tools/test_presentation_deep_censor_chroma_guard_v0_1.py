from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
CPP_DIR = ROOT / "suite_android" / "app" / "src" / "main" / "cpp"
BRIDGE = CPP_DIR / "photo_export_bridge.cpp"
DEEP = CPP_DIR / "presentation_deep_censor_chroma_guard_v0_1.h"
SHOULDER = CPP_DIR / "presentation_near_censor_chroma_shoulder_v0_1.h"
FALLBACK = CPP_DIR / "presentation_censored_chroma_fallback_v0_1.h"

bridge = BRIDGE.read_text(encoding="utf-8")
deep = DEEP.read_text(encoding="utf-8")
shoulder = SHOULDER.read_text(encoding="utf-8")
fallback = FALLBACK.read_text(encoding="utf-8")

for token in (
    '#include "presentation_deep_censor_chroma_guard_v0_1.h"',
    'namespace presentation_deep_censor_chroma = truthraw::presentation_deep_censor_chroma_guard::v0_1;',
    'presentation_near_censor_chroma::apply(',
    'presentation_censored_chroma::apply(',
    'presentation_deep_censor_chroma::apply(',
    'presentation_illuminant_warmth::apply(',
):
    if token not in bridge:
        raise SystemExit(f"missing deep-censor runtime token: {token}")

for token in (
    'kCensorFractionStart = 0.50f',
    'kCensorFractionFull = 0.80f',
    'kMaxRemainingChromaContraction = 0.78f',
):
    if token not in deep:
        raise SystemExit(f"missing deep-censor constant: {token}")

# Accepted fallback and low-fraction shoulder remain frozen.
for token in (
    'kCensorFractionStart = 0.15f',
    'kCensorFractionFull = 0.85f',
    'kMaxChromaContraction = 0.84f',
):
    if token not in fallback:
        raise SystemExit(f"accepted fallback changed unexpectedly: {token}")
for token in (
    'kCensorFractionStart = 0.02f',
    'kCensorFractionFullShoulder = 0.15f',
    'kCensorFractionFadeStart = 0.35f',
    'kCensorFractionFadeEnd = 0.55f',
    'kMaxChromaContraction = 0.12f',
):
    if token not in shoulder:
        raise SystemExit(f"near-censor shoulder changed unexpectedly: {token}")

pure_pos = bridge.index('if(extendedLinearHeadroomInput_) {')
else_pos = bridge.index('                } else {', pure_pos)
shoulder_pos = bridge.index('presentation_near_censor_chroma::apply(', else_pos)
fallback_pos = bridge.index('presentation_censored_chroma::apply(', shoulder_pos)
deep_pos = bridge.index('presentation_deep_censor_chroma::apply(', fallback_pos)
warm_pos = bridge.index('presentation_illuminant_warmth::apply(', deep_pos)
highlight_pos = bridge.index('presentation_highlight::apply_near_neutral_rolloff(', warm_pos)
gamut_pos = bridge.index('presentation_gamut::fit_unit_rgb_preserve_luminance', highlight_pos)
if not (pure_pos < else_pos < shoulder_pos < fallback_pos < deep_pos < warm_pos < highlight_pos < gamut_pos):
    raise SystemExit("deep-censor candidate is not isolated to downstream ADVANCED/PRO Appearance")

source = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include "presentation_near_censor_chroma_shoulder_v0_1.h"
#include "presentation_censored_chroma_fallback_v0_1.h"
#include "presentation_deep_censor_chroma_guard_v0_1.h"
namespace s = truthraw::presentation_near_censor_chroma_shoulder::v0_1;
namespace f = truthraw::presentation_censored_chroma_fallback::v0_1;
namespace d = truthraw::presentation_deep_censor_chroma_guard::v0_1;

static double chroma(float r,float g,float b) {
    return static_cast<double>(std::max({r,g,b})-std::min({r,g,b}));
}
static void chain(float authority,float& r,float& g,float& b) {
    assert(s::apply(r,g,b,authority));
    assert(f::apply(r,g,b,authority));
    assert(d::apply(r,g,b,authority));
}
int main() {
    {
        float r=0.30f,g=0.20f,b=0.80f;
        const float r0=r,g0=g,b0=b;
        assert(d::apply(r,g,b,0.50f));
        assert(r==r0 && g==g0 && b==b0);
    }
    {
        float r=0.337f,g=0.216f,b=0.855f;
        const double y0=d::luminance709(r,g,b);
        const double c0=chroma(r,g,b);
        assert(d::apply(r,g,b,0.80f));
        assert(std::abs(d::luminance709(r,g,b)-y0)<2e-7);
        const double ratio=chroma(r,g,b)/c0;
        assert(ratio>0.219 && ratio<0.221);
    }
    {
        // Direct-sky regression authority observed on device: 26591/36864.
        constexpr float authority = 26591.0f/36864.0f;
        float r=0.337f,g=0.216f,b=0.855f;
        const double c0=chroma(r,g,b);
        chain(authority,r,g,b);
        const double ratio=chroma(r,g,b)/c0;
        // v0.1 shoulder+fallback left about 23.5% at this authority; the deep
        // guard must reduce the total residual to < 9% while preserving Y.
        assert(ratio < 0.09);
        assert(ratio > 0.07);
    }
    {
        // Combined authority response must never re-introduce chroma as censor
        // authority increases.
        const float r0=0.72f,g0=0.31f,b0=0.90f;
        double previous=chroma(r0,g0,b0)+1.0;
        for(int i=0;i<=1000;++i) {
            const float authority=static_cast<float>(i)/1000.0f;
            float r=r0,g=g0,b=b0;
            const double y0=d::luminance709(r,g,b);
            chain(authority,r,g,b);
            assert(std::abs(d::luminance709(r,g,b)-y0)<3e-7);
            const double cc=chroma(r,g,b);
            assert(cc<=previous+2e-7);
            previous=cc;
        }
        float r=r0,g=g0,b=b0;
        chain(1.0f,r,g,b);
        const double ratio=chroma(r,g,b)/chroma(r0,g0,b0);
        assert(ratio>0.034 && ratio<0.036);
    }
    {
        // Hue-independent: yellow receives the same authority-driven contraction.
        float r=0.90f,g=0.82f,b=0.15f;
        const double c0=chroma(r,g,b);
        chain(1.0f,r,g,b);
        const double ratio=chroma(r,g,b)/c0;
        assert(ratio>0.034 && ratio<0.036);
    }
    {
        float r=0.3f,g=0.2f,b=0.8f;
        const float nan=std::numeric_limits<float>::quiet_NaN();
        assert(!d::apply(r,g,b,nan));
    }
}
'''

with tempfile.TemporaryDirectory() as td:
    td = Path(td)
    cpp = td / "test.cpp"
    exe = td / "test"
    cpp.write_text(source, encoding="utf-8")
    subprocess.run(
        ["g++", "-std=c++20", "-O2", "-Wall", "-Wextra", "-Werror",
         "-I", str(CPP_DIR), str(cpp), "-o", str(exe)],
        check=True,
    )
    subprocess.run([str(exe)], check=True)

print("PRESENTATION_DEEP_CENSOR_CHROMA_GUARD_V0_1_REGRESSION_PASS")
