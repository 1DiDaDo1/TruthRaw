#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
h = root / "suite_android/app/src/main/cpp/presentation_censored_illuminant_hue_floor_v0_1.h"
bridge = root / "suite_android/app/src/main/cpp/photo_export_bridge.cpp"
assert h.exists() and bridge.exists()

txt = h.read_text().lower()
for token in [
    "sobel(", "laplacian(", "sharpen(", "blur(",
    "object_detector(", "hue_detector(", "camera_model"
]:
    assert token not in txt, token
assert "kmaxrelativechromafloor = 0.060f" in txt
assert "kcensorfractionstart = 0.50f" in txt
assert "kcensorfractionfull = 0.80f" in txt
assert "kneutralrelativechromaend = 0.090f" in txt

b = bridge.read_text()
assert b.count("presentation_censored_illuminant_hue_floor_v0_1.h") == 1
assert b.count("presentation_censored_illuminant_hue_floor::apply(") == 1
deep = b.find("presentation_deep_censor_chroma::apply(")
warm = b.find("presentation_illuminant_warmth::apply(")
floor = b.find("presentation_censored_illuminant_hue_floor::apply(")
highlight = b.find("presentation_highlight::apply_near_neutral_rolloff(")
gamut = b.find("presentation_gamut::fit_unit_rgb_preserve_luminance(")
assert -1 not in (deep, warm, floor, highlight, gamut)
assert deep < warm < floor < highlight < gamut

cpp = r'''
#include <cassert>
#include <cmath>
#include <cstdio>
#include "presentation_censored_illuminant_hue_floor_v0_1.h"
namespace f = truthraw::presentation_censored_illuminant_hue_floor::v0_1;

static double y(float r,float g,float b){
    return .2126*r+.7152*g+.0722*b;
}
static bool same(float a,float b){return std::fabs(a-b)<1e-7f;}

int main(){
    f::SourceWhitePoint warm{};
    warm.known=true;
    warm.x=0.4476;
    warm.y=0.4074;
    warm.correlatedColorTemperatureK=2850.0;

    {
        float r=.95f,g=.95f,b=.95f;
        assert(f::apply(r,g,b,warm,.90f,false));
        assert(same(r,.95f)&&same(g,.95f)&&same(b,.95f));
    }

    {
        auto cool=warm;
        cool.correlatedColorTemperatureK=6500.0;
        float r=.95f,g=.95f,b=.95f;
        assert(f::apply(r,g,b,cool,.90f,true));
        assert(same(r,.95f)&&same(g,.95f)&&same(b,.95f));
    }

    {
        float r=.95f,g=.95f,b=.95f;
        assert(f::apply(r,g,b,warm,.40f,true));
        assert(same(r,.95f)&&same(g,.95f)&&same(b,.95f));
    }
    {
        float r=.60f,g=.60f,b=.60f;
        assert(f::apply(r,g,b,warm,.90f,true));
        assert(same(r,.60f)&&same(g,.60f)&&same(b,.60f));
    }

    {
        float r=.98f,g=.90f,b=.82f;
        const float R=r,G=g,B=b;
        assert(f::apply(r,g,b,warm,.95f,true));
        assert(same(r,R)&&same(g,G)&&same(b,B));
    }

    {
        float r=.337f,g=.216f,b=.855f;
        const float R=r,G=g,B=b;
        assert(f::apply(r,g,b,warm,.95f,true));
        assert(same(r,R)&&same(g,G)&&same(b,B));
    }

    {
        float r=.95f,g=.95f,b=.95f;
        const double yin=y(r,g,b);
        assert(f::apply(r,g,b,warm,.95f,true));
        const double yout=y(r,g,b);
        assert(std::fabs(yin-yout)<3e-7);
        assert(r>.95f);
        assert(b<.95f);
        assert(r>b);
        const float rel=f::relative_chroma(r,g,b,static_cast<float>(yout));
        assert(rel>0.0f && rel<=f::kMaxRelativeChromaFloor+1e-5f);
    }

    {
        float r=NAN,g=.95f,b=.95f;
        assert(!f::apply(r,g,b,warm,.95f,true));
    }

    std::puts("CENSORED_ILLUMINANT_HUE_FLOOR_V01_REGRESSION_PASS");
}
'''
with tempfile.TemporaryDirectory() as td:
    src = Path(td) / "t.cpp"
    exe = Path(td) / "t"
    src.write_text(cpp)
    inc = root / "suite_android/app/src/main/cpp"
    subprocess.run(
        ["g++", "-std=c++17", "-O2", "-I", str(inc), str(src), "-o", str(exe)],
        check=True,
    )
    out = subprocess.check_output([str(exe)], text=True).strip()
    assert out == "CENSORED_ILLUMINANT_HUE_FLOOR_V01_REGRESSION_PASS", out

print("CENSORED_ILLUMINANT_HUE_FLOOR_V01_REGRESSION_PASS")
