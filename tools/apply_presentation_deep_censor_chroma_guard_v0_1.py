from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BRIDGE = ROOT / "suite_android" / "app" / "src" / "main" / "cpp" / "photo_export_bridge.cpp"

text = BRIDGE.read_text(encoding="utf-8")
original = text

include_token = '#include "presentation_deep_censor_chroma_guard_v0_1.h"\n'
if include_token not in text:
    anchor = '#include "presentation_censored_chroma_fallback_v0_1.h"\n'
    if anchor not in text:
        raise SystemExit("missing accepted censored chroma include anchor")
    text = text.replace(anchor, anchor + include_token, 1)

namespace_token = (
    'namespace presentation_deep_censor_chroma = '
    'truthraw::presentation_deep_censor_chroma_guard::v0_1;\n'
)
if namespace_token not in text:
    anchor = (
        'namespace presentation_censored_chroma = '
        'truthraw::presentation_censored_chroma_fallback::v0_1;\n'
    )
    if anchor not in text:
        raise SystemExit("missing accepted censored chroma namespace anchor")
    text = text.replace(anchor, anchor + namespace_token, 1)

apply_block = r'''                    // Deep-CENSOR residual chroma guard. The accepted fallback above
                    // remains unchanged; this stage only contracts the remaining unsupported
                    // chroma once reconstruction-support censor authority is already high.
                    // Rec.709 luminance and spatial detail are preserved; Warm Illuminant
                    // remains downstream.
                    if(!presentation_deep_censor_chroma::apply(
                            r,g,b,highlightCensorFraction)) {
                        return StreamStatus::error(
                            StreamStatusCode::SinkFailed,
                            "full-res deep-censor chroma guard failed");
                    }
'''
if 'presentation_deep_censor_chroma::apply(' not in text:
    anchor = (
        '                    // Natural Light may retain a bounded fraction of a warm\n'
        '                    // source-white appearance. This is presentation-only and\n'
    )
    if anchor not in text:
        raise SystemExit("missing Warm Illuminant insertion anchor")
    text = text.replace(anchor, apply_block + anchor, 1)

required = (
    include_token.strip(),
    namespace_token.strip(),
    'presentation_near_censor_chroma::apply(',
    'presentation_censored_chroma::apply(',
    'presentation_deep_censor_chroma::apply(',
    'presentation_illuminant_warmth::apply(',
    'presentation_highlight::apply_near_neutral_rolloff(',
    'presentation_gamut::fit_unit_rgb_preserve_luminance',
)
for token in required:
    if token not in text:
        raise SystemExit(f"deep-censor wiring missing token: {token}")

pure_pos = text.index('if(extendedLinearHeadroomInput_) {')
else_pos = text.index('                } else {', pure_pos)
shoulder_pos = text.index('presentation_near_censor_chroma::apply(', else_pos)
fallback_pos = text.index('presentation_censored_chroma::apply(', shoulder_pos)
deep_pos = text.index('presentation_deep_censor_chroma::apply(', fallback_pos)
warm_pos = text.index('presentation_illuminant_warmth::apply(', deep_pos)
highlight_pos = text.index('presentation_highlight::apply_near_neutral_rolloff(', warm_pos)
gamut_pos = text.index('presentation_gamut::fit_unit_rgb_preserve_luminance', highlight_pos)
if not (pure_pos < else_pos < shoulder_pos < fallback_pos < deep_pos < warm_pos < highlight_pos < gamut_pos):
    raise SystemExit("deep-censor ordering/isolation contract failed")

finalize_pos = text.index('StreamStatus finalizeCoreTile(')
preacutance_pos = text.index('std::vector<float> preAcutance(', finalize_pos)
core_rgb_pos = text.index('std::vector<std::uint8_t> coreRgb(', preacutance_pos)
if 'presentation_deep_censor_chroma::apply(' in text[preacutance_pos:core_rgb_pos]:
    raise SystemExit("deep-censor guard leaked into restoration/detail/acutance preparation")

if text != original:
    BRIDGE.write_text(text, encoding="utf-8")

print("PRESENTATION_DEEP_CENSOR_CHROMA_GUARD_V0_1_APPLIED")
