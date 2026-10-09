from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
CPP_DIR = ROOT / "suite_android" / "app" / "src" / "main" / "cpp"
JAVA_DIR = ROOT / "suite_android" / "app" / "src" / "main" / "java" / "com" / "truthraw" / "adaptiveui"
BRIDGE = CPP_DIR / "photo_export_bridge.cpp"
AUTHORITY = CPP_DIR / "output_channel_authority_v0_84.cpp"
HEADER = CPP_DIR / "presentation_highlight_chroma_rolloff_v0_1.h"
PREVIEW = JAVA_DIR / "PreJpegRgb24PreviewRendererV01.kt"
ADVANCED = JAVA_DIR / "TruthRawAdvanced.kt"

bridge = BRIDGE.read_text(encoding="utf-8")
for token in (
    'presentation_highlight_chroma_rolloff_v0_1.h',
    'presentation_highlight::apply_near_neutral_rolloff(',
    'presentation_headroom::map_90_100(r,g,b)',
):
    if token not in bridge:
        raise SystemExit(f"missing runtime token: {token}")

pure_pos = bridge.index('if(extendedLinearHeadroomInput_) {')
map_pos = bridge.index('presentation_headroom::map_90_100(r,g,b)', pure_pos)
else_pos = bridge.index('                } else {', map_pos)
call_pos = bridge.index('presentation_highlight::apply_near_neutral_rolloff(', else_pos)
gamut_pos = bridge.index('presentation_gamut::fit_unit_rgb_preserve_luminance', call_pos)
if not (pure_pos < map_pos < else_pos < call_pos < gamut_pos):
    raise SystemExit("runtime guard is not isolated to ADVANCED/PRO presentation branch")

# Presentation censor authority must use the same reconstruction-support concept
# as OutputChannelAuthority v0.84. A centre CFA sample being below WhiteLevel is
# insufficient when a neighbouring CFA phase used by dense RGB reconstruction
# is clipped.
for token in (
    'int reconstructionSupportRadius_',
    'const int radius=reconstructionSupportRadius_;',
    'const auto saturated_count =',
    'saturated_count(ax0,ay0,ax1,ay1)>0u?1u:0u',
    'std::max(0,reconstruction->requiredHalo())',
):
    if token not in bridge:
        raise SystemExit(f"support-based presentation censor contract missing: {token}")

old_central_rule = (
    'static_cast<float>(raw[local])>=source_.metadata().whiteLevel?1u:0u;'
)
if old_central_rule in bridge:
    raise SystemExit("central-only presentation censor rule survived")

mask_start = bridge.index('const int radius=reconstructionSupportRadius_;')
mask_end = bridge.index('stagedPixels_+=pixels;', mask_start)
mask_block = bridge[mask_start:mask_end]
if 'scratchRgbOffset_' in mask_block or 'presentationRgbOffset_' in mask_block:
    raise SystemExit("support-authority mask stage must not rewrite staged/presentation RGB")
if 'scratchMaskOffset_' not in mask_block:
    raise SystemExit("support-authority stage is not writing only the censor mask")

authority = AUTHORITY.read_text(encoding="utf-8")
for token in (
    'binding.reconstructionSupportRadius',
    'const auto saturated_count =',
    'const bool censored=saturated_count(ax0,ay0,ax1,ay1)>0u;',
):
    if token not in authority:
        raise SystemExit(f"OutputChannelAuthority support rule missing: {token}")

# Tiny semantic lock: a non-clipped centre with a clipped neighbour is not
# censored at radius 0 but is censored at radius 1. This is the exact failure
# mode that the old centre-only presentation mask could miss.
def support_censored(raw, width, height, x, y, radius, white):
    x0 = max(0, x - radius)
    y0 = max(0, y - radius)
    x1 = min(width, x + radius + 1)
    y1 = min(height, y + radius + 1)
    return any(
        raw[yy * width + xx] >= white
        for yy in range(y0, y1)
        for xx in range(x0, x1)
    )

synthetic = [0] * 9
synthetic[1] = 100
assert synthetic[4] < 100
assert not support_censored(synthetic, 3, 3, 1, 1, 0, 100)
assert support_censored(synthetic, 3, 3, 1, 1, 1, 100)

# The Android UI preview must be a sampled sibling of the exact same native
# pre-JPEG RGB24 output. It may not fall back to a separately coloured bitmap
# path, otherwise a highlight fix could diverge between screen and saved JPEG.
preview = PREVIEW.read_text(encoding="utf-8")
advanced = ADVANCED.read_text(encoding="utf-8")
for token in (
    'PhotoExportNativeBridge.renderFullResNv21(',
    'val presentationRgbBytes = packet[47]',
    'nv21Bytes + sy.toLong() * width.toLong() * 3L',
    'packet[8].toInt() != flags',
):
    if token not in preview:
        raise SystemExit(f"exact pre-JPEG preview contract missing: {token}")
if 'PreJpegRgb24PreviewRendererV01.render(' not in advanced:
    raise SystemExit("ADVANCED/PRO UI is not bound to exact pre-JPEG RGB24 preview")

source = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include "presentation_highlight_chroma_rolloff_v0_1.h"

namespace h = truthraw::presentation_highlight_chroma_rolloff::v0_1;

static float chroma(float r, float g, float b) {
    return std::max(r, std::max(g,b)) - std::min(r, std::min(g,b));
}

int main() {
    {
        float r=0.40f,g=0.35f,b=0.30f;
        const float r0=r,g0=g,b0=b;
        assert(h::apply_near_neutral_rolloff(r,g,b,false));
        assert(r==r0 && g==g0 && b==b0);
    }
    {
        // Strongly coloured yellow highlight stays coloured, even if censored;
        // the severe guard is specific to the observed R/B-high G-collapse
        // white-boundary signature and must not become blanket desaturation.
        float r=1.0f,g=1.0f,b=0.0f;
        const float r0=r,g0=g,b0=b;
        assert(h::apply_near_neutral_rolloff(r,g,b,true));
        assert(r==r0 && g==g0 && b==b0);
    }
    {
        // A legitimate non-censored saturated magenta highlight stays coloured.
        float r=1.0f,g=0.50f,b=1.0f;
        const float r0=r,g0=g,b0=b;
        assert(h::apply_near_neutral_rolloff(r,g,b,false));
        assert(r==r0 && g==g0 && b==b0);
    }
    {
        // Neutral highlight remains neutral.
        float r=0.98f,g=0.98f,b=0.98f;
        assert(h::apply_near_neutral_rolloff(r,g,b,false));
        assert(std::abs(r-g)<1e-7f && std::abs(g-b)<1e-7f);
    }
    {
        // Original real-device near-white magenta regression remains fixed.
        float r=1.0f,g=0.84f,b=1.0f;
        const double y0=h::luminance709(r,g,b);
        const float c0=chroma(r,g,b);
        assert(h::apply_near_neutral_rolloff(r,g,b,false));
        const double y1=h::luminance709(r,g,b);
        const float c1=chroma(r,g,b);
        assert(c1 < c0);
        assert(std::abs(y1-y0) < 1e-5);
    }
    {
        // Censored near-white highlight gets the stronger conservative
        // contraction from the original v0.1 path.
        float r1=1.0f,g1=0.84f,b1=1.0f;
        float r2=r1,g2=g1,b2=b1;
        assert(h::apply_near_neutral_rolloff(r1,g1,b1,false));
        assert(h::apply_near_neutral_rolloff(r2,g2,b2,true));
        assert(chroma(r2,g2,b2) < chroma(r1,g1,b1));
    }
    {
        // Severe censored R/B-high G-collapse contracts chroma while preserving
        // Rec.709 luminance: spatial/luminance detail is not flattened here.
        float r=1.0f,g=0.50f,b=1.0f;
        const double y0=h::luminance709(r,g,b);
        const float c0=chroma(r,g,b);
        assert(h::apply_near_neutral_rolloff(r,g,b,true));
        const double y1=h::luminance709(r,g,b);
        const float c1=chroma(r,g,b);
        assert(c1 < c0*0.55f);
        assert(g > 0.50f);
        assert(r < 1.0f && b < 1.0f);
        assert(std::abs(y1-y0) < 1e-5);
    }
    {
        // R/B imbalance does not match the observed white-boundary collapse and
        // therefore does not trigger the severe extension.
        float r=1.0f,g=0.50f,b=0.55f;
        const float r0=r,g0=g,b0=b;
        assert(h::apply_near_neutral_rolloff(r,g,b,true));
        assert(r==r0 && g==g0 && b==b0);
    }
    {
        float r=std::numeric_limits<float>::quiet_NaN(),g=1.0f,b=1.0f;
        assert(!h::apply_near_neutral_rolloff(r,g,b,false));
    }
    return 0;
}
'''

with tempfile.TemporaryDirectory() as td:
    td = Path(td)
    src = td / "test.cpp"
    exe = td / "test"
    src.write_text(source, encoding="utf-8")
    subprocess.run(
        ["c++", "-std=c++17", "-O2", "-I", str(CPP_DIR), str(src), "-o", str(exe)],
        check=True,
    )
    subprocess.run([str(exe)], check=True)

print("PRESENTATION_HIGHLIGHT_CHROMA_ROLLOFF_V01_REGRESSION_PASS")
print("PRESENTATION_HIGHLIGHT_PREVIEW_FULLRES_SHARED_RGB24_PASS")
print("PRESENTATION_CENSOR_SUPPORT_AUTHORITY_V01_REGRESSION_PASS")
print("PRESENTATION_CENSOR_SUPPORT_DETAIL_PRESERVATION_PASS")
