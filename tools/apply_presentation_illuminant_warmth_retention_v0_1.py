#!/usr/bin/env python3
from pathlib import Path

TARGET = Path("suite_android/app/src/main/cpp/photo_export_bridge.cpp")


def require_replace(text: str, old: str, new: str, label: str) -> str:
    if new in text:
        return text
    if old not in text:
        raise SystemExit(f"missing contract anchor: {label}")
    return text.replace(old, new, 1)


def main() -> None:
    text = TARGET.read_text(encoding="utf-8")
    original = text

    include_old = '#include "presentation_highlight_chroma_rolloff_v0_1.h"\n'
    include_new = (
        '#include "presentation_highlight_chroma_rolloff_v0_1.h"\n'
        '#include "presentation_illuminant_warmth_retention_v0_1.h"\n'
    )
    text = require_replace(text, include_old, include_new, "warmth include")

    ns_old = (
        "namespace presentation_highlight = "
        "truthraw::presentation_highlight_chroma_rolloff::v0_1;\n"
    )
    ns_new = (
        ns_old
        + "namespace presentation_illuminant_warmth = "
          "truthraw::presentation_illuminant_warmth_retention::v0_1;\n"
    )
    text = require_replace(text, ns_old, ns_new, "warmth namespace")

    ctor_sig_old = (
        "        float noiseSigmaAt2Pct,\n"
        "        bool extendedLinearHeadroomInput)\n"
    )
    ctor_sig_new = (
        "        float noiseSigmaAt2Pct,\n"
        "        presentation_illuminant_warmth::SourceWhitePoint presentationSourceWhite,\n"
        "        bool extendedLinearHeadroomInput)\n"
    )
    text = require_replace(text, ctor_sig_old, ctor_sig_new, "sink constructor signature")

    ctor_init_old = (
        "          noiseSigmaAt2Pct_(noiseSigmaAt2Pct),\n"
        "          extendedLinearHeadroomInput_(extendedLinearHeadroomInput),\n"
    )
    ctor_init_new = (
        "          noiseSigmaAt2Pct_(noiseSigmaAt2Pct),\n"
        "          presentationSourceWhite_(presentationSourceWhite),\n"
        "          extendedLinearHeadroomInput_(extendedLinearHeadroomInput),\n"
    )
    text = require_replace(text, ctor_init_old, ctor_init_new, "sink constructor initializer")

    member_old = (
        "    float noiseSigmaAt2Pct_=0.0f;\n"
        "    bool extendedLinearHeadroomInput_=false;\n"
    )
    member_new = (
        "    float noiseSigmaAt2Pct_=0.0f;\n"
        "    presentation_illuminant_warmth::SourceWhitePoint presentationSourceWhite_{};\n"
        "    bool extendedLinearHeadroomInput_=false;\n"
    )
    text = require_replace(text, member_old, member_new, "sink source-white member")

    source_anchor = (
        "    // Selection is an explicit downstream output contract. flags==0 alone is\n"
        "    // not sufficient: scientific/helper renders also legitimately carry zero\n"
        "    // appearance flags and must not silently enter the PURE headroom path.\n"
    )
    source_block = (
        "    presentation_illuminant_warmth::SourceWhitePoint presentationSourceWhite{};\n"
        "    presentationSourceWhite.known=\n"
        "        illuminationState.whitePointKnown &&\n"
        "        illuminationState.whitePointAuthority!=illumination_state::EstimateAuthority::Unknown;\n"
        "    presentationSourceWhite.x=illuminationState.whiteX;\n"
        "    presentationSourceWhite.y=illuminationState.whiteY;\n"
        "    presentationSourceWhite.correlatedColorTemperatureK=\n"
        "        illuminationState.correlatedColorTemperatureK;\n\n"
        + source_anchor
    )
    text = require_replace(text, source_anchor, source_block, "source-white binding")

    sink_call_old = (
        "        *source,\n"
        "        noiseSigma,\n"
        "        pureExtendedLinearHeadroomCandidate);\n"
    )
    sink_call_new = (
        "        *source,\n"
        "        noiseSigma,\n"
        "        presentationSourceWhite,\n"
        "        pureExtendedLinearHeadroomCandidate);\n"
    )
    text = require_replace(text, sink_call_old, sink_call_new, "sink source-white argument")

    warm_anchor = (
        "                } else {\n"
        "                    const float mx=std::max(r,std::max(g,b));\n"
    )
    warm_block = (
        "                } else {\n"
        "                    // Natural Light may retain a bounded fraction of a warm\n"
        "                    // source-white appearance. This is presentation-only and\n"
        "                    // deliberately excluded from the PURE headroom branch.\n"
        "                    if(!presentation_illuminant_warmth::apply(\n"
        "                            r,g,b,presentationSourceWhite_,naturalLightEnabled)) {\n"
        "                        return StreamStatus::error(\n"
        "                            StreamStatusCode::SinkFailed,\n"
        "                            \"full-res source-white warmth retention failed\");\n"
        "                    }\n"
        "                    const float mx=std::max(r,std::max(g,b));\n"
    )
    text = require_replace(text, warm_anchor, warm_block, "ADVANCED/PRO warmth call")

    required = [
        '#include "presentation_illuminant_warmth_retention_v0_1.h"',
        "namespace presentation_illuminant_warmth =",
        "presentationSourceWhite_(presentationSourceWhite)",
        "presentationSourceWhite.known=",
        "presentation_illuminant_warmth::apply(",
        "r,g,b,presentationSourceWhite_,naturalLightEnabled",
    ]
    for token in required:
        if token not in text:
            raise SystemExit(f"post-patch contract missing: {token}")

    pure_pos = text.index("if(extendedLinearHeadroomInput_)")
    else_pos = text.index("                } else {", pure_pos)
    warm_pos = text.index("presentation_illuminant_warmth::apply(", else_pos)
    highlight_pos = text.index("presentation_highlight::apply_near_neutral_rolloff(", warm_pos)
    gamut_pos = text.index("presentation_gamut::fit_unit_rgb_preserve_luminance(", highlight_pos)
    if not (pure_pos < else_pos < warm_pos < highlight_pos < gamut_pos):
        raise SystemExit("warmth stage ordering contract failed")

    if text.count("presentation_illuminant_warmth::apply(") != 1:
        raise SystemExit("warmth apply must have exactly one runtime call")

    if text == original:
        print("APPEARANCE_WARM_ILLUMINANT_RETENTION_V0_1_ALREADY_APPLIED")
        return

    TARGET.write_text(text, encoding="utf-8")
    print("APPEARANCE_WARM_ILLUMINANT_RETENTION_V0_1_APPLIED")


if __name__ == "__main__":
    main()
