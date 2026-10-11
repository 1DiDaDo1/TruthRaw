#!/usr/bin/env python3
from pathlib import Path

p = Path("suite_android/app/src/main/cpp/photo_export_bridge.cpp")
s = p.read_text(encoding="utf-8")
original = s

inc_anchor = '#include "presentation_illuminant_warmth_retention_v0_1.h"\n'
inc_new = (
    inc_anchor
    + '#include "presentation_censored_illuminant_hue_floor_v0_1.h"\n'
)
if '#include "presentation_censored_illuminant_hue_floor_v0_1.h"' not in s:
    if inc_anchor not in s:
        raise SystemExit("missing Warm Illuminant include anchor")
    s = s.replace(inc_anchor, inc_new, 1)

ns_anchor = (
    "namespace presentation_illuminant_warmth = "
    "truthraw::presentation_illuminant_warmth_retention::v0_1;\n"
)
ns_new = (
    ns_anchor
    + "namespace presentation_censored_illuminant_hue_floor = "
      "truthraw::presentation_censored_illuminant_hue_floor::v0_1;\n"
)
if "namespace presentation_censored_illuminant_hue_floor =" not in s:
    if ns_anchor not in s:
        raise SystemExit("missing Warm Illuminant namespace anchor")
    s = s.replace(ns_anchor, ns_new, 1)

warm_tail = (
    '                            "full-res source-white warmth retention failed");\n'
    "                    }\n"
    "                    const float mx=std::max(r,std::max(g,b));\n"
)
floor_tail = (
    '                            "full-res source-white warmth retention failed");\n'
    "                    }\n"
    "                    // Deeply censored, nearly neutral bright highlights may retain\n"
    "                    // a small source-white-consistent warm hue floor. This remains\n"
    "                    // downstream Appearance only; luminance and scientific state\n"
    "                    // are unchanged and PURE never enters this branch.\n"
    "                    if(!presentation_censored_illuminant_hue_floor::apply(\n"
    "                            r,g,b,presentationSourceWhite_,\n"
    "                            highlightCensorFraction,naturalLightEnabled)) {\n"
    "                        return StreamStatus::error(\n"
    "                            StreamStatusCode::SinkFailed,\n"
    '                            "full-res censored illuminant hue floor failed");\n'
    "                    }\n"
    "                    const float mx=std::max(r,std::max(g,b));\n"
)
call = "presentation_censored_illuminant_hue_floor::apply("
if call not in s:
    if warm_tail not in s:
        raise SystemExit("missing post-Warm Illuminant runtime anchor")
    s = s.replace(warm_tail, floor_tail, 1)

required = [
    '#include "presentation_censored_illuminant_hue_floor_v0_1.h"',
    "namespace presentation_censored_illuminant_hue_floor =",
    call,
    "r,g,b,presentationSourceWhite_,",
    "highlightCensorFraction,naturalLightEnabled",
    "full-res censored illuminant hue floor failed",
]
for token in required:
    if token not in s:
        raise SystemExit(f"post-patch contract missing: {token}")

if s.count(call) != 1:
    raise SystemExit("censored illuminant hue floor runtime call must be unique")

deep = s.find("presentation_deep_censor_chroma::apply(")
warm = s.find("presentation_illuminant_warmth::apply(")
floor = s.find(call)
highlight = s.find("presentation_highlight::apply_near_neutral_rolloff(")
gamut = s.find("presentation_gamut::fit_unit_rgb_preserve_luminance(")
if min(deep, warm, floor, highlight, gamut) < 0:
    raise SystemExit("missing Appearance ordering anchor")
if not (deep < warm < floor < highlight < gamut):
    raise SystemExit(
        "invalid Appearance order: deep -> warm -> hue-floor -> highlight -> gamut required"
    )

p.write_text(s, encoding="utf-8")
print(
    "CENSORED_ILLUMINANT_HUE_FLOOR_V01_"
    + ("APPLIED" if s != original else "ALREADY_APPLIED")
)
