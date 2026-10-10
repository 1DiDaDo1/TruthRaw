#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "suite_android/app/src/main/cpp/presentation_natural_light_field_tone_v0_1.h"
BRIDGE = ROOT / "suite_android/app/src/main/cpp/photo_export_bridge.cpp"


def main() -> None:
    if not HEADER.is_file() or not BRIDGE.is_file():
        raise SystemExit("Natural Light field-tone sources missing")

    bridge = BRIDGE.read_text(encoding="utf-8")
    tokens = [
        '#include "presentation_natural_light_field_tone_v0_1.h"',
        "const bool naturalLightFieldEnabled=",
        "naturalLightEnabled && !extendedLinearHeadroomInput_;",
        "fieldLumaIntegral",
        "localFieldMeanAt",
        "presentation_natural_light_field::apply(",
        "r,g,b,localFieldY,censored,naturalLightEnabled",
    ]
    for token in tokens:
        if token not in bridge:
            raise SystemExit(f"runtime wiring token missing: {token}")

    gate = "        const bool naturalLightEnabled=(flags_&kFlagLight)!=0;\n"
    if bridge.count(gate) != 1:
        raise SystemExit("Natural Light runtime gate count invalid")

    pure_pos = bridge.index("if(extendedLinearHeadroomInput_)")
    else_pos = bridge.index("                } else {", pure_pos)
    field_pos = bridge.index("presentation_natural_light_field::apply(", else_pos)
    warmth_pos = bridge.index("presentation_illuminant_warmth::apply(", field_pos)
    highlight_pos = bridge.index("presentation_highlight::apply_near_neutral_rolloff(", warmth_pos)
    gamut_pos = bridge.index("presentation_gamut::fit_unit_rgb_preserve_luminance(", highlight_pos)
    if not (pure_pos < else_pos < field_pos < warmth_pos < highlight_pos < gamut_pos):
        raise SystemExit("field-tone runtime order invalid")
    if bridge.count("presentation_natural_light_field::apply(") != 1:
        raise SystemExit("field-tone runtime call count invalid")

    source = r'''
#include <cmath>
#include <limits>
#include "presentation_natural_light_field_tone_v0_1.h"

using namespace truthraw::presentation_natural_light_field_tone::v0_1;

static bool near(float a, float b, float e=1e-6f) { return std::fabs(a-b) <= e; }

int main() {
    {
        float r=.30f,g=.24f,b=.18f;
        const float r0=r,g0=g,b0=b;
        if(!apply(r,g,b,.55f,false,false) || !near(r,r0) || !near(g,g0) || !near(b,b0)) return 1;
    }
    {
        float r=.30f,g=.24f,b=.18f;
        const float r0=r,g0=g,b0=b;
        if(!apply(r,g,b,.55f,true,true) || !near(r,r0) || !near(g,g0) || !near(b,b0)) return 2;
    }
    {
        float r=.34f,g=.28f,b=.20f;
        const float rg=r/g, bg=b/g;
        const float y0=luminance709(r,g,b);
        if(!apply(r,g,b,.55f,false,true)) return 3;
        const float y1=luminance709(r,g,b);
        if(!(y1 > y0*1.04f)) return 4;
        if(!(y1 <= y0*std::exp2(.14001f))) return 5;
        if(!near(r/g,rg,2e-6f) || !near(b/g,bg,2e-6f)) return 6;
    }
    {
        float r1=.34f,g1=.28f,b1=.20f;
        float r2=r1,g2=g1,b2=b1;
        if(!apply(r1,g1,b1,.16f,false,true)) return 7;
        if(!apply(r2,g2,b2,.55f,false,true)) return 8;
        if(!(luminance709(r2,g2,b2) > luminance709(r1,g1,b1))) return 9;
    }
    {
        float r=.025f,g=.022f,b=.018f;
        const float y0=luminance709(r,g,b);
        if(!apply(r,g,b,.70f,false,true)) return 10;
        if(!(luminance709(r,g,b) < y0*1.005f)) return 11;
    }
    {
        float r=.92f,g=.86f,b=.78f;
        const float y0=luminance709(r,g,b);
        if(!apply(r,g,b,.80f,false,true)) return 12;
        if(!(luminance709(r,g,b) < y0*1.02f)) return 13;
    }
    {
        float r=.30f,g=.24f,b=.18f;
        const float r0=r,g0=g,b0=b;
        if(!apply(r,g,b,std::numeric_limits<float>::quiet_NaN(),false,true)) return 14;
        if(!near(r,r0)||!near(g,g0)||!near(b,b0)) return 15;
    }
    {
        float r=std::numeric_limits<float>::quiet_NaN(),g=.2f,b=.1f;
        if(apply(r,g,b,.5f,false,true)) return 16;
    }
    return 0;
}
'''

    with tempfile.TemporaryDirectory() as td:
        td = Path(td)
        cpp = td / "test.cpp"
        exe = td / "test"
        cpp.write_text(source, encoding="utf-8")
        subprocess.run([
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
            f"-I{HEADER.parent}", str(cpp), "-o", str(exe)
        ], check=True)
        subprocess.run([str(exe)], check=True)

    print("NATURAL_LIGHT_FIELD_TONE_V0_1_REGRESSION_PASS")


if __name__ == "__main__":
    main()
