from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
CPP_DIR = ROOT / "suite_android" / "app" / "src" / "main" / "cpp"
BRIDGE = CPP_DIR / "photo_export_bridge.cpp"
HEADER = CPP_DIR / "presentation_near_censor_chroma_shoulder_v0_1.h"
FALLBACK = CPP_DIR / "presentation_censored_chroma_fallback_v0_1.h"

bridge = BRIDGE.read_text(encoding="utf-8")
header = HEADER.read_text(encoding="utf-8")
fallback = FALLBACK.read_text(encoding="utf-8")

for token in (
    '#include "presentation_near_censor_chroma_shoulder_v0_1.h"',
    'namespace presentation_near_censor_chroma = truthraw::presentation_near_censor_chroma_shoulder::v0_1;',
    'presentation_near_censor_chroma::apply(',
    'presentation_censored_chroma::apply(',
):
    if token not in bridge:
        raise SystemExit(f"missing near-censor runtime token: {token}")

for token in (
    'kCensorFractionStart = 0.02f',
    'kCensorFractionFullShoulder = 0.15f',
    'kCensorFractionFadeStart = 0.35f',
    'kCensorFractionFadeEnd = 0.55f',
    'kMaxChromaContraction = 0.12f',
):
    if token not in header:
        raise SystemExit(f"missing near-censor candidate constant: {token}")

# Freeze the already accepted fallback constants: this candidate must not alter them.
for token in (
    'kCensorFractionStart = 0.15f',
    'kCensorFractionFull = 0.85f',
    'kMaxChromaContraction = 0.84f',
):
    if token not in fallback:
        raise SystemExit(f"accepted fallback constant changed: {token}")

pure_pos = bridge.index('if(extendedLinearHeadroomInput_) {')
else_pos = bridge.index('                } else {', pure_pos)
natural_pos = bridge.index('presentation_natural_light_field::apply(', else_pos)
shoulder_pos = bridge.index('presentation_near_censor_chroma::apply(', natural_pos)
fallback_pos = bridge.index('presentation_censored_chroma::apply(', shoulder_pos)
warm_pos = bridge.index('presentation_illuminant_warmth::apply(', fallback_pos)
highlight_pos = bridge.index('presentation_highlight::apply_near_neutral_rolloff(', warm_pos)
gamut_pos = bridge.index('presentation_gamut::fit_unit_rgb_preserve_luminance', highlight_pos)
if not (
    pure_pos < else_pos < natural_pos < shoulder_pos < fallback_pos <
    warm_pos < highlight_pos < gamut_pos
):
    raise SystemExit("near-censor candidate is not isolated/order-safe in ADVANCED/PRO")

if bridge.count('presentation_near_censor_chroma::apply(') != 1:
    raise SystemExit("near-censor shoulder runtime call count changed")

finalize_pos = bridge.index('StreamStatus finalizeCoreTile(')
preacutance_pos = bridge.index('std::vector<float> preAcutance(', finalize_pos)
core_rgb_pos = bridge.index('std::vector<std::uint8_t> coreRgb(', preacutance_pos)
if 'presentation_near_censor_chroma::apply(' in bridge[preacutance_pos:core_rgb_pos]:
    raise SystemExit("near-censor candidate leaked into detail/acutance preparation")

source = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include "presentation_near_censor_chroma_shoulder_v0_1.h"
#include "presentation_censored_chroma_fallback_v0_1.h"
namespace s = truthraw::presentation_near_censor_chroma_shoulder::v0_1;
namespace f = truthraw::presentation_censored_chroma_fallback::v0_1;

static double chroma(float r,float g,float b) {
    return static_cast<double>(std::max({r,g,b})-std::min({r,g,b}));
}

static double combined_ratio(float authority,float r0,float g0,float b0) {
    float r=r0,g=g0,b=b0;
    assert(s::apply(r,g,b,authority));
    assert(f::apply(r,g,b,authority));
    return chroma(r,g,b)/chroma(r0,g0,b0);
}

int main() {
    {
        float r=0.45f,g=0.31f,b=0.91f;
        const float r0=r,g0=g,b0=b;
        assert(s::apply(r,g,b,0.0f));
        assert(r==r0 && g==g0 && b==b0);
        assert(s::apply(r,g,b,s::kCensorFractionStart));
        assert(r==r0 && g==g0 && b==b0);
    }
    {
        float r=0.45f,g=0.31f,b=0.91f;
        const double y0=s::luminance709(r,g,b);
        const double c0=chroma(r,g,b);
        assert(s::apply(r,g,b,s::kCensorFractionFullShoulder));
        assert(std::abs(s::luminance709(r,g,b)-y0)<2e-7);
        const double ratio=chroma(r,g,b)/c0;
        assert(ratio>0.879 && ratio<0.881);
    }
    {
        // The shoulder fades out once the accepted fallback carries enough authority.
        float r=0.45f,g=0.31f,b=0.91f;
        const float r0=r,g0=g,b0=b;
        assert(s::apply(r,g,b,s::kCensorFractionFadeEnd));
        assert(std::abs(r-r0)<1e-7f && std::abs(g-g0)<1e-7f && std::abs(b-b0)<1e-7f);
        assert(s::apply(r,g,b,1.0f));
        assert(std::abs(r-r0)<1e-7f && std::abs(g-g0)<1e-7f && std::abs(b-b0)<1e-7f);
    }
    {
        // The combined shoulder + accepted fallback response must never regain chroma
        // as reconstruction-support censor authority increases.
        const float r0=0.45f,g0=0.31f,b0=0.91f;
        double previous=1.000001;
        for(int i=0;i<=100;++i) {
            const float authority=static_cast<float>(i)/100.0f;
            const double ratio=combined_ratio(authority,r0,g0,b0);
            assert(ratio<=previous+2e-7);
            previous=ratio;
        }
    }
    {
        // Hue-independent authority rule: same censor fraction gives the same
        // chroma-scale ratio to a yellow triplet and a purple triplet.
        float pr=0.45f,pg=0.31f,pb=0.91f;
        float yr=0.90f,yg=0.82f,yb=0.15f;
        const double pc0=chroma(pr,pg,pb), yc0=chroma(yr,yg,yb);
        assert(s::apply(pr,pg,pb,0.15f));
        assert(s::apply(yr,yg,yb,0.15f));
        const double pRatio=chroma(pr,pg,pb)/pc0;
        const double yRatio=chroma(yr,yg,yb)/yc0;
        assert(std::abs(pRatio-yRatio)<2e-7);
    }
    {
        float r=0.3f,g=0.2f,b=0.8f;
        const float nan=std::numeric_limits<float>::quiet_NaN();
        assert(!s::apply(r,g,b,nan));
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

print("PRESENTATION_NEAR_CENSOR_CHROMA_SHOULDER_V0_1_REGRESSION_PASS")
