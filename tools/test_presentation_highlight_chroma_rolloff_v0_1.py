from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
CPP_DIR = ROOT / "suite_android" / "app" / "src" / "main" / "cpp"
JAVA_DIR = ROOT / "suite_android" / "app" / "src" / "main" / "java" / "com" / "truthraw" / "adaptiveui"
BRIDGE = CPP_DIR / "photo_export_bridge.cpp"
AUTHORITY = CPP_DIR / "output_channel_authority_v0_84.cpp"
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

# PURE stays isolated in its own extended-linear/headroom branch. The
# support-aware highlight authority boundary remains ADVANCED/PRO-only.
pure_pos = bridge.index('if(extendedLinearHeadroomInput_) {')
map_pos = bridge.index('presentation_headroom::map_90_100(r,g,b)', pure_pos)
else_pos = bridge.index('                } else {', map_pos)
call_pos = bridge.index('presentation_highlight::apply_near_neutral_rolloff(', else_pos)
gamut_pos = bridge.index('presentation_gamut::fit_unit_rgb_preserve_luminance', call_pos)
if not (pure_pos < map_pos < else_pos < call_pos < gamut_pos):
    raise SystemExit("highlight authority boundary is not isolated to ADVANCED/PRO")

# Historical shared staged censor mask remains centre-only. Do not broaden it:
# restoration/HDR/Local Field must keep their established evidence semantics.
central_rule = 'static_cast<float>(raw[local])>=source_.metadata().whiteLevel?1u:0u;'
if central_rule not in bridge:
    raise SystemExit("historical centre-only staged censor rule was not preserved")

write_start = bridge.index('    StreamStatus writeSdrTile(')
write_end = bridge.index('        stagedPixels_+=pixels;', write_start)
write_block = bridge[write_start:write_end]
for forbidden in ('supportPixels', 'saturatedPrefix', 'reconstructionSupportRadius_'):
    if forbidden in write_block:
        raise SystemExit(f"shared staged censor mask was broadened: {forbidden}")

# Keep the good part of the earlier work: a stricter reconstruction-support
# authority is computed separately and consumed only at the final highlight
# boundary. It must not leak into detail, HDR, restoration or local-field tone.
for token in (
    'int reconstructionSupportRadius_',
    'std::max({3,localFieldRadius,reconstructionSupportRadius_})',
    'const int highlightIntegralWidth=sw+1;',
    'std::vector<std::uint32_t> highlightCensorIntegral(',
    'const auto highlightCensoredAt=',
    'const bool censored=supportMask[si]!=0u;',
    'const bool highlightCensored=highlightCensoredAt(x,y);',
    'r,g,b,highlightCensored)',
    'std::max(0,reconstruction->requiredHalo())',
):
    if token not in bridge:
        raise SystemExit(f"support-aware highlight authority contract missing: {token}")

if 'if((flags_&kFlagHdr)!=0 && hdrPipelineEnabled_ && !censored)' not in bridge:
    raise SystemExit("HDR no longer uses centre-only censor state")
if 'r,g,b,localFieldY,censored,naturalLightEnabled' not in bridge:
    raise SystemExit("Natural Light local field no longer uses centre-only censor state")
if 'presentation_highlight::apply_near_neutral_rolloff(\n                            r,g,b,censored)' in bridge:
    raise SystemExit("highlight boundary still consumes shared detail/HDR censor state")

finalize_start = bridge.index('    StreamStatus finalizeCoreTile(')
preacutance_start = bridge.index('std::vector<float> preAcutance(', finalize_start)
core_rgb_start = bridge.index('std::vector<std::uint8_t> coreRgb(', preacutance_start)
final_loop_start = bridge.index('        for(int y=y0;y<y1;++y) {', core_rgb_start)
pre_final_block = bridge[preacutance_start:final_loop_start]
for forbidden in (
    'const bool highlightCensored=',
    'r,g,b,highlightCensored)',
    'presentation_highlight::apply_near_neutral_rolloff(',
):
    if forbidden in pre_final_block:
        raise SystemExit(f"highlight-only authority leaked upstream: {forbidden}")

authority = AUTHORITY.read_text(encoding="utf-8")
for token in (
    'binding.reconstructionSupportRadius',
    'const auto saturated_count =',
    'const bool censored=saturated_count(ax0,ay0,ax1,ay1)>0u;',
):
    if token not in authority:
        raise SystemExit(f"OutputChannelAuthority support rule missing: {token}")

# A clipped neighbour may make highlight authority censored while the centre
# remains uncensored for HDR/detail/local-field. This split is intentional.
def support_censored(raw, width, height, x, y, radius, white):
    x0 = max(0, x - radius)
    y0 = max(0, y - radius)
    x1 = min(width, x + radius + 1)
    y1 = min(height, y + radius + 1)
    return any(raw[yy * width + xx] >= white for yy in range(y0, y1) for xx in range(x0, x1))

synthetic = [0] * 9
synthetic[1] = 100
assert synthetic[4] < 100
assert not (synthetic[4] >= 100)
assert support_censored(synthetic, 3, 3, 1, 1, 1, 100)

# Preview remains a sampled sibling of the same pre-JPEG RGB24 output.
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

# Semantic lock: the support-aware highlight stage is now observation-only.
# Every finite RGB triplet must be returned bit-for-bit unchanged regardless of
# authority or whether it matches the former near-neutral/severe signatures.
source = r'''
#include <cassert>
#include <cmath>
#include <limits>
#include "presentation_highlight_chroma_rolloff_v0_1.h"
namespace h = truthraw::presentation_highlight_chroma_rolloff::v0_1;

static void unchanged(float r, float g, float b, bool censored) {
    const float r0=r, g0=g, b0=b;
    const double y0=h::luminance709(r,g,b);
    assert(h::apply_near_neutral_rolloff(r,g,b,censored));
    assert(r==r0 && g==g0 && b==b0);
    assert(std::abs(h::luminance709(r,g,b)-y0) < 1e-12);
}

int main() {
    unchanged(0.40f,0.35f,0.30f,false);
    unchanged(1.00f,1.00f,0.00f,true);
    unchanged(1.00f,0.50f,1.00f,false);
    unchanged(0.98f,0.98f,0.98f,false);
    unchanged(1.00f,0.84f,1.00f,false);
    unchanged(1.00f,0.84f,1.00f,true);
    unchanged(1.00f,0.50f,1.00f,true);
    unchanged(1.00f,0.50f,0.55f,true);
    float r=std::numeric_limits<float>::quiet_NaN(), g=1.0f, b=1.0f;
    assert(!h::apply_near_neutral_rolloff(r,g,b,false));
    return 0;
}
'''

with tempfile.TemporaryDirectory() as td:
    td = Path(td)
    src = td / "test.cpp"
    exe = td / "test"
    src.write_text(source, encoding="utf-8")
    subprocess.run(["c++", "-std=c++17", "-O2", "-I", str(CPP_DIR), str(src), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)

print("PRESENTATION_HIGHLIGHT_PIXEL_PASSTHROUGH_V02_REGRESSION_PASS")
print("PRESENTATION_HIGHLIGHT_PREVIEW_FULLRES_SHARED_RGB24_PASS")
print("PRESENTATION_HIGHLIGHT_CENSOR_AUTHORITY_SPLIT_V01_REGRESSION_PASS")
print("PRESENTATION_HIGHLIGHT_DETAIL_HDR_LOCAL_FIELD_ISOLATION_PASS")
