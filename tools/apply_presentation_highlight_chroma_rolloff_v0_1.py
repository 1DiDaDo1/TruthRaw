from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "suite_android" / "app" / "src" / "main" / "cpp" / "photo_export_bridge.cpp"

text = TARGET.read_text(encoding="utf-8")

include_marker = '#include "presentation_highlight_chroma_rolloff_v0_1.h"'
if include_marker not in text:
    anchor = '#include "presentation_gamut_fit_v0_1.h"\n'
    if anchor not in text:
        raise SystemExit("presentation gamut include anchor missing")
    text = text.replace(anchor, anchor + include_marker + "\n", 1)

alias_marker = (
    'namespace presentation_highlight = '
    'truthraw::presentation_highlight_chroma_rolloff::v0_1;'
)
if alias_marker not in text:
    anchor = (
        'namespace presentation_gamut = '
        'truthraw::presentation_gamut_fit::v0_1;\n'
    )
    if anchor not in text:
        raise SystemExit("presentation gamut namespace anchor missing")
    text = text.replace(anchor, anchor + alias_marker + "\n", 1)

call_marker = 'presentation_highlight::apply_near_neutral_rolloff('
if call_marker not in text:
    old = '''                } else {
                    const float mx=std::max(r,std::max(g,b));
                    if(mx>0.92f) {
                        const float shoulder=
                            0.92f+0.08f*(1.0f-std::exp(-3.0f*(mx-0.92f)));
                        const float sc=shoulder/std::max(mx,1e-8f);
                        r*=sc; g*=sc; b*=sc;
                    }
                }

                if(!presentation_gamut::fit_unit_rgb_preserve_luminance(r,g,b)) {
'''
    new = '''                } else {
                    const float mx=std::max(r,std::max(g,b));
                    if(mx>0.92f) {
                        const float shoulder=
                            0.92f+0.08f*(1.0f-std::exp(-3.0f*(mx-0.92f)));
                        const float sc=shoulder/std::max(mx,1e-8f);
                        r*=sc; g*=sc; b*=sc;
                    }
                    if(!presentation_highlight::apply_near_neutral_rolloff(
                            r,g,b,censored)) {
                        return StreamStatus::error(
                            StreamStatusCode::SinkFailed,
                            "full-res presentation highlight chroma roll-off failed");
                    }
                }

                if(!presentation_gamut::fit_unit_rgb_preserve_luminance(r,g,b)) {
'''
    if old not in text:
        raise SystemExit(
            "ADVANCED/PRO presentation shoulder target not found; refusing non-deterministic patch"
        )
    text = text.replace(old, new, 1)

required = (
    include_marker,
    alias_marker,
    call_marker,
    'if(extendedLinearHeadroomInput_) {',
    'presentation_headroom::map_90_100(r,g,b)',
)
missing = [token for token in required if token not in text]
if missing:
    raise SystemExit("highlight chroma roll-off contract missing: " + ", ".join(missing))

# The new guard must remain in the ADVANCED/PRO `else` branch only. PURE's
# proven 90/100 mapping must not be changed by this patch.
pure_pos = text.index('if(extendedLinearHeadroomInput_) {')
map_pos = text.index('presentation_headroom::map_90_100(r,g,b)', pure_pos)
else_pos = text.index('                } else {', map_pos)
call_pos = text.index(call_marker, else_pos)
gamut_pos = text.index('presentation_gamut::fit_unit_rgb_preserve_luminance', call_pos)
if not (pure_pos < map_pos < else_pos < call_pos < gamut_pos):
    raise SystemExit("highlight guard escaped ADVANCED/PRO presentation branch")

TARGET.write_text(text, encoding="utf-8")
print("PRESENTATION_HIGHLIGHT_CHROMA_ROLLOFF_V01_APPLIED")
