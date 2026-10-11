from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BRIDGE = ROOT / "suite_android" / "app" / "src" / "main" / "cpp" / "photo_export_bridge.cpp"

text = BRIDGE.read_text(encoding="utf-8")
original = text

include_token = '#include "presentation_censored_chroma_fallback_v0_1.h"\n'
if include_token not in text:
    anchor = '#include "presentation_gamut_fit_v0_1.h"\n'
    if anchor not in text:
        raise SystemExit("missing presentation gamut include anchor")
    text = text.replace(anchor, anchor + include_token, 1)

namespace_token = (
    'namespace presentation_censored_chroma = '
    'truthraw::presentation_censored_chroma_fallback::v0_1;\n'
)
if namespace_token not in text:
    anchor = (
        'namespace presentation_gamut = '
        'truthraw::presentation_gamut_fit::v0_1;\n'
    )
    if anchor not in text:
        raise SystemExit("missing presentation gamut namespace anchor")
    text = text.replace(anchor, anchor + namespace_token, 1)

fraction_helper = r'''        const auto highlightCensorFractionAt=[&](int px,int py) noexcept -> float {
            const int radius=reconstructionSupportRadius_;
            const int ax0=std::max(sx0,px-radius);
            const int ay0=std::max(sy0,py-radius);
            const int ax1=std::min(sx1,px+radius+1);
            const int ay1=std::min(sy1,py+radius+1);
            const int lx0=ax0-sx0;
            const int ly0=ay0-sy0;
            const int lx1=ax1-sx0;
            const int ly1=ay1-sy0;
            const std::uint32_t count=
                highlightCensorIntegral[
                    static_cast<std::size_t>(ly1)*highlightIntegralWidth+lx1] -
                highlightCensorIntegral[
                    static_cast<std::size_t>(ly0)*highlightIntegralWidth+lx1] -
                highlightCensorIntegral[
                    static_cast<std::size_t>(ly1)*highlightIntegralWidth+lx0] +
                highlightCensorIntegral[
                    static_cast<std::size_t>(ly0)*highlightIntegralWidth+lx0];
            const std::uint32_t area=static_cast<std::uint32_t>(
                std::max(0,ax1-ax0)*std::max(0,ay1-ay0));
            if(area==0u) return 0.0f;
            return std::clamp(
                static_cast<float>(count)/static_cast<float>(area),0.0f,1.0f);
        };

'''
if 'const auto highlightCensorFractionAt=' not in text:
    anchor = '        // Build a deterministic integral image from already-rendered RGB.\n'
    if anchor not in text:
        raise SystemExit("missing highlight support helper insertion anchor")
    text = text.replace(anchor, fraction_helper + anchor, 1)

fraction_value = (
    '                const float highlightCensorFraction='
    'highlightCensorFractionAt(x,y);\n'
)
if fraction_value not in text:
    anchor = '                const bool highlightCensored=highlightCensoredAt(x,y);\n'
    if anchor not in text:
        raise SystemExit("missing final highlight authority value anchor")
    text = text.replace(anchor, anchor + fraction_value, 1)

apply_block = r'''                    // CENSORED output chromaticity is not scene colour truth. Contract
                    // only the unsupported chroma component in proportion to the
                    // reconstruction-support CENSOR fraction. Luminance is retained;
                    // accepted source-white warmth is deliberately applied afterwards.
                    if(!presentation_censored_chroma::apply(
                            r,g,b,highlightCensorFraction)) {
                        return StreamStatus::error(
                            StreamStatusCode::SinkFailed,
                            "full-res censored chroma fallback failed");
                    }
'''
if 'presentation_censored_chroma::apply(' not in text:
    anchor = (
        '                    // Natural Light may retain a bounded fraction of a warm\n'
        '                    // source-white appearance. This is presentation-only and\n'
    )
    if anchor not in text:
        raise SystemExit("missing Warm Illuminant ordering anchor")
    text = text.replace(anchor, apply_block + anchor, 1)

required = (
    include_token.strip(),
    namespace_token.strip(),
    'const auto highlightCensorFractionAt=',
    'const float highlightCensorFraction=highlightCensorFractionAt(x,y);',
    'presentation_censored_chroma::apply(',
    'presentation_illuminant_warmth::apply(',
    'presentation_highlight::apply_near_neutral_rolloff(',
)
for token in required:
    if token not in text:
        raise SystemExit(f"candidate wiring missing token: {token}")

pure_pos = text.index('if(extendedLinearHeadroomInput_) {')
else_pos = text.index('                } else {', pure_pos)
candidate_pos = text.index('presentation_censored_chroma::apply(', else_pos)
warm_pos = text.index('presentation_illuminant_warmth::apply(', candidate_pos)
highlight_pos = text.index('presentation_highlight::apply_near_neutral_rolloff(', warm_pos)
gamut_pos = text.index('presentation_gamut::fit_unit_rgb_preserve_luminance', highlight_pos)
if not (pure_pos < else_pos < candidate_pos < warm_pos < highlight_pos < gamut_pos):
    raise SystemExit("censored chroma fallback ordering/isolation contract failed")

preacutance_pos = text.index('std::vector<float> preAcutance(', text.index('StreamStatus finalizeCoreTile('))
core_rgb_pos = text.index('std::vector<std::uint8_t> coreRgb(', preacutance_pos)
if 'presentation_censored_chroma::apply(' in text[preacutance_pos:core_rgb_pos]:
    raise SystemExit("censored chroma fallback leaked into pre-acutance/detail stages")

if text != original:
    BRIDGE.write_text(text, encoding="utf-8")

print("PRESENTATION_CENSORED_CHROMA_FALLBACK_V0_1_APPLIED")
