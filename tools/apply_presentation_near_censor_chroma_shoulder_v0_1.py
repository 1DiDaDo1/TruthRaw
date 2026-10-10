from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BRIDGE = ROOT / "suite_android" / "app" / "src" / "main" / "cpp" / "photo_export_bridge.cpp"

text = BRIDGE.read_text(encoding="utf-8")
original = text

include_token = '#include "presentation_near_censor_chroma_shoulder_v0_1.h"\n'
if include_token not in text:
    anchor = '#include "presentation_censored_chroma_fallback_v0_1.h"\n'
    if anchor not in text:
        raise SystemExit("missing censored chroma include anchor")
    text = text.replace(anchor, anchor + include_token, 1)

namespace_token = (
    'namespace presentation_near_censor_chroma = '
    'truthraw::presentation_near_censor_chroma_shoulder::v0_1;\n'
)
if namespace_token not in text:
    anchor = (
        'namespace presentation_censored_chroma = '
        'truthraw::presentation_censored_chroma_fallback::v0_1;\n'
    )
    if anchor not in text:
        raise SystemExit("missing censored chroma namespace anchor")
    text = text.replace(anchor, anchor + namespace_token, 1)

apply_block = r'''                    // Residual low-fraction near-censor shoulder. This is a bounded
                    // APPEARANCE-only chroma contraction that preserves Rec.709 luminance
                    // and spatial detail. The accepted full censored fallback remains the
                    // downstream authority for stronger reconstruction-support censoring.
                    if(!presentation_near_censor_chroma::apply(
                            r,g,b,highlightCensorFraction)) {
                        return StreamStatus::error(
                            StreamStatusCode::SinkFailed,
                            "full-res near-censor chroma shoulder failed");
                    }
'''
if 'presentation_near_censor_chroma::apply(' not in text:
    anchor = (
        '                    // CENSORED output chromaticity is not scene colour truth. Contract\n'
        '                    // only the unsupported chroma component in proportion to the\n'
    )
    if anchor not in text:
        raise SystemExit("missing censored chroma fallback ordering anchor")
    text = text.replace(anchor, apply_block + anchor, 1)

required = (
    include_token.strip(),
    namespace_token.strip(),
    'const float highlightCensorFraction=highlightCensorFractionAt(x,y);',
    'presentation_near_censor_chroma::apply(',
    'presentation_censored_chroma::apply(',
    'presentation_illuminant_warmth::apply(',
    'presentation_highlight::apply_near_neutral_rolloff(',
)
for token in required:
    if token not in text:
        raise SystemExit(f"near-censor candidate wiring missing token: {token}")

if text.count('presentation_near_censor_chroma::apply(') != 1:
    raise SystemExit("near-censor shoulder must have exactly one runtime call")

pure_pos = text.index('if(extendedLinearHeadroomInput_) {')
else_pos = text.index('                } else {', pure_pos)
natural_pos = text.index('presentation_natural_light_field::apply(', else_pos)
shoulder_pos = text.index('presentation_near_censor_chroma::apply(', natural_pos)
fallback_pos = text.index('presentation_censored_chroma::apply(', shoulder_pos)
warm_pos = text.index('presentation_illuminant_warmth::apply(', fallback_pos)
highlight_pos = text.index('presentation_highlight::apply_near_neutral_rolloff(', warm_pos)
gamut_pos = text.index('presentation_gamut::fit_unit_rgb_preserve_luminance', highlight_pos)
if not (
    pure_pos < else_pos < natural_pos < shoulder_pos < fallback_pos <
    warm_pos < highlight_pos < gamut_pos
):
    raise SystemExit("near-censor shoulder ordering/isolation contract failed")

preacutance_pos = text.index('std::vector<float> preAcutance(', text.index('StreamStatus finalizeCoreTile('))
core_rgb_pos = text.index('std::vector<std::uint8_t> coreRgb(', preacutance_pos)
if 'presentation_near_censor_chroma::apply(' in text[preacutance_pos:core_rgb_pos]:
    raise SystemExit("near-censor shoulder leaked into restoration/detail/acutance stages")

if text != original:
    BRIDGE.write_text(text, encoding="utf-8")

print("PRESENTATION_NEAR_CENSOR_CHROMA_SHOULDER_V0_1_APPLIED")
