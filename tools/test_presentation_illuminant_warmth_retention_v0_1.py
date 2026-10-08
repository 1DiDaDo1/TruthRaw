#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / "suite_android/app/src/main/cpp/photo_export_bridge.cpp"
HEADER_DIR = ROOT / "suite_android/app/src/main/cpp"

text = CPP.read_text(encoding="utf-8")

required = [
    '#include "presentation_illuminant_warmth_retention_v0_1.h"',
    "presentationSourceWhite.known=",
    "illuminationState.whitePointAuthority!=illumination_state::EstimateAuthority::Unknown",
    "presentationSourceWhite.x=illuminationState.whiteX;",
    "presentationSourceWhite.y=illuminationState.whiteY;",
    "presentationSourceWhite.correlatedColorTemperatureK=",
    "presentationSourceWhite_(presentationSourceWhite)",
    "presentation_illuminant_warmth::apply(",
    "r,g,b,presentationSourceWhite_,naturalLightEnabled",
]
for token in required:
    if token not in text:
        raise SystemExit(f"missing runtime warmth contract: {token}")

pure_pos = text.index("if(extendedLinearHeadroomInput_)")
else_pos = text.index("                } else {", pure_pos)
warm_pos = text.index("presentation_illuminant_warmth::apply(", else_pos)
highlight_pos = text.index("presentation_highlight::apply_near_neutral_rolloff(", warm_pos)
gamut_pos = text.index("presentation_gamut::fit_unit_rgb_preserve_luminance(", highlight_pos)
if not (pure_pos < else_pos < warm_pos < highlight_pos < gamut_pos):
    raise SystemExit("warmth runtime ordering failed")
if text.count("presentation_illuminant_warmth::apply(") != 1:
    raise SystemExit("warmth runtime call count must equal one")

source = r"""
#include "presentation_illuminant_warmth_retention_v0_1.h"
#include <cmath>
#include <iostream>

namespace w = truthraw::presentation_illuminant_warmth_retention::v0_1;

static double y709(float r, float g, float b) {
    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

int main() {
    {
        float r=0.5f,g=0.5f,b=0.5f;
        const float r0=r,g0=g,b0=b;
        w::SourceWhitePoint p{true,0.44757,0.40745,2856.0};
        if(!w::apply(r,g,b,p,true)) return 10;
        if(!(r>r0 && b<b0 && r>b)) return 11;
        if(std::abs(y709(r,g,b)-y709(r0,g0,b0))>2.0e-5) return 12;
    }
    {
        float r=0.4f,g=0.5f,b=0.6f;
        const float r0=r,g0=g,b0=b;
        w::SourceWhitePoint p{true,0.44757,0.40745,2856.0};
        if(!w::apply(r,g,b,p,false)) return 20;
        if(r!=r0 || g!=g0 || b!=b0) return 21;
    }
    {
        float r=0.4f,g=0.5f,b=0.6f;
        const float r0=r,g0=g,b0=b;
        w::SourceWhitePoint p{true,0.3127,0.3290,6500.0};
        if(!w::apply(r,g,b,p,true)) return 30;
        if(r!=r0 || g!=g0 || b!=b0) return 31;
    }
    {
        float r=0.4f,g=0.5f,b=0.6f;
        const float r0=r,g0=g,b0=b;
        w::SourceWhitePoint p{};
        if(!w::apply(r,g,b,p,true)) return 40;
        if(r!=r0 || g!=g0 || b!=b0) return 41;
    }
    if(std::abs(w::retention_strength(2856.0)-0.18f)>1.0e-7f) return 50;
    if(w::retention_strength(5000.0)!=0.0f) return 51;
    std::cout << "WARM_ILLUMINANT_RETENTION_CPP_PASS\n";
    return 0;
}
"""

with tempfile.TemporaryDirectory() as td:
    td = Path(td)
    src = td / "warm_test.cpp"
    exe = td / "warm_test"
    src.write_text(source, encoding="utf-8")
    subprocess.run(
        ["g++", "-std=c++17", "-O2", "-I", str(HEADER_DIR), str(src), "-o", str(exe)],
        check=True,
    )
    out = subprocess.check_output([str(exe)], text=True).strip()
    if out != "WARM_ILLUMINANT_RETENTION_CPP_PASS":
        raise SystemExit(f"unexpected C++ regression output: {out}")

print("APPEARANCE_WARM_ILLUMINANT_RETENTION_V0_1_REGRESSION_PASS")
