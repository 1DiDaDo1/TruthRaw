from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
CPP_DIR = ROOT / "suite_android" / "app" / "src" / "main" / "cpp"
BRIDGE = CPP_DIR / "photo_export_bridge.cpp"
HEADER = CPP_DIR / "presentation_censored_chroma_fallback_v0_1.h"

bridge = BRIDGE.read_text(encoding="utf-8")
header = HEADER.read_text(encoding="utf-8")

for token in (
    '#include "presentation_censored_chroma_fallback_v0_1.h"',
    'namespace presentation_censored_chroma = truthraw::presentation_censored_chroma_fallback::v0_1;',
    'const auto highlightCensorFractionAt=',
    'const float highlightCensorFraction=highlightCensorFractionAt(x,y);',
    'presentation_censored_chroma::apply(',
):
    if token not in bridge:
        raise SystemExit(f"missing censored chroma runtime token: {token}")

for token in (
    'kCensorFractionStart = 0.15f',
    'kCensorFractionFull = 0.85f',
    'kMaxChromaContraction = 0.84f',
):
    if token not in header:
        raise SystemExit(f"missing frozen candidate constant: {token}")

pure_pos = bridge.index('if(extendedLinearHeadroomInput_) {')
else_pos = bridge.index('                } else {', pure_pos)
candidate_pos = bridge.index('presentation_censored_chroma::apply(', else_pos)
warm_pos = bridge.index('presentation_illuminant_warmth::apply(', candidate_pos)
highlight_pos = bridge.index('presentation_highlight::apply_near_neutral_rolloff(', warm_pos)
gamut_pos = bridge.index('presentation_gamut::fit_unit_rgb_preserve_luminance', highlight_pos)
if not (pure_pos < else_pos < candidate_pos < warm_pos < highlight_pos < gamut_pos):
    raise SystemExit("candidate is not isolated to ADVANCED/PRO before Warm Illuminant")

finalize_pos = bridge.index('StreamStatus finalizeCoreTile(')
preacutance_pos = bridge.index('std::vector<float> preAcutance(', finalize_pos)
core_rgb_pos = bridge.index('std::vector<std::uint8_t> coreRgb(', preacutance_pos)
if 'presentation_censored_chroma::apply(' in bridge[preacutance_pos:core_rgb_pos]:
    raise SystemExit("candidate leaked into restoration/detail/acutance preparation")

# Existing centre-only consumers must remain untouched.
for token in (
    'if((flags_&kFlagHdr)!=0 && hdrPipelineEnabled_ && !censored)',
    'r,g,b,localFieldY,censored,naturalLightEnabled',
    'const bool highlightCensored=highlightCensoredAt(x,y);',
):
    if token not in bridge:
        raise SystemExit(f"historical authority split changed: {token}")

source = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include "presentation_censored_chroma_fallback_v0_1.h"
namespace c = truthraw::presentation_censored_chroma_fallback::v0_1;

static double chroma(float r,float g,float b) {
    return static_cast<double>(std::max({r,g,b})-std::min({r,g,b}));
}

static void apply_at(float f,float& r,float& g,float& b) {
    assert(c::apply(r,g,b,f));
    assert(std::isfinite(r)&&std::isfinite(g)&&std::isfinite(b));
}

int main() {
    {
        float r=0.337f,g=0.216f,b=0.855f;
        const float r0=r,g0=g,b0=b;
        apply_at(0.0f,r,g,b);
        assert(r==r0 && g==g0 && b==b0);
    }
    {
        float r=0.337f,g=0.216f,b=0.855f;
        const float r0=r,g0=g,b0=b;
        apply_at(c::kCensorFractionStart,r,g,b);
        assert(r==r0 && g==g0 && b==b0);
    }
    {
        const float r0=0.337f,g0=0.216f,b0=0.855f;
        const double y0=c::luminance709(r0,g0,b0);
        double previous=chroma(r0,g0,b0)+1.0;
        for(float f: {0.0f,0.20f,0.35f,0.50f,0.70f,0.85f,1.0f}) {
            float r=r0,g=g0,b=b0;
            apply_at(f,r,g,b);
            const double y=c::luminance709(r,g,b);
            assert(std::abs(y-y0)<2e-7);
            const double cc=chroma(r,g,b);
            assert(cc<=previous+1e-7);
            previous=cc;
        }
        float r=r0,g=g0,b=b0;
        apply_at(1.0f,r,g,b);
        assert(chroma(r,g,b) < 0.20 * chroma(r0,g0,b0));
        assert(chroma(r,g,b) > 0.10 * chroma(r0,g0,b0));
    }
    {
        // Contraction follows authority, not hue. A yellow triplet with the same
        // fully-censored authority contracts by the same amount while preserving Y.
        float r=0.90f,g=0.82f,b=0.15f;
        const double y0=c::luminance709(r,g,b);
        const double c0=chroma(r,g,b);
        apply_at(1.0f,r,g,b);
        assert(std::abs(c::luminance709(r,g,b)-y0)<2e-7);
        const double ratio=chroma(r,g,b)/c0;
        assert(ratio>0.15 && ratio<0.17);
    }
    {
        float r=0.3f,g=0.2f,b=0.8f;
        const float nan=std::numeric_limits<float>::quiet_NaN();
        assert(!c::apply(r,g,b,nan));
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

print("PRESENTATION_CENSORED_CHROMA_FALLBACK_V0_1_REGRESSION_PASS")
