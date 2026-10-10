#!/usr/bin/env python3

from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
CPP_DIR = ROOT / "suite_android" / "app" / "src" / "main" / "cpp"
BRIDGE = CPP_DIR / "photo_export_bridge.cpp"

bridge = BRIDGE.read_text(encoding="utf-8")
for token in (
    'presentation_headroom_map_v0_1.h',
    'process_pure_extended_linear_headroom_v0_1',
    'PURE_EXTENDED_LINEAR_FLOAT32_HEADROOM_90_100_APPEARANCE_ONLY_V0_1',
    'presentationHeadroomMode==kPresentationHeadroomPureMap90To100',
):
    if token not in bridge:
        raise SystemExit(f"FAIL: photo output headroom route missing {token!r}")

if 'pureExtendedLinearHeadroomCandidate=(flags==0)' in bridge:
    raise SystemExit("FAIL: obsolete flags==0 headroom selector is still present")

CPP = r'''
#include <cassert>
#include <cmath>
#include <iostream>
#include "presentation_headroom_map_v0_1.h"

namespace h = truthraw::presentation_headroom_map::v0_1;

static bool close(float a,float b,float eps=2e-6f){
    return std::abs(a-b)<=eps;
}

int main(){
    // Below the measured 90/100 knee: exact identity.
    {
        float r=.20f,g=.40f,b=.60f;
        const float r0=r,g0=g,b0=b;
        assert(h::luminance709(r,g,b)<h::kReferenceWhite);
        assert(h::map_90_100(r,g,b));
        assert(r==r0 && g==g0 && b==b0);
    }

    // Neutral values above the knee map monotonically into reserved headroom.
    float y095=0.95f, y100=1.0f, y200=2.0f;
    {
        float r=y095,g=y095,b=y095;
        assert(h::map_90_100(r,g,b));
        y095=h::luminance709(r,g,b);
    }
    {
        float r=y100,g=y100,b=y100;
        assert(h::map_90_100(r,g,b));
        y100=h::luminance709(r,g,b);
    }
    {
        float r=y200,g=y200,b=y200;
        assert(h::map_90_100(r,g,b));
        y200=h::luminance709(r,g,b);
    }
    assert(y095>0.90f && y095<1.0f);
    assert(y100>y095 && y100<1.0f);
    assert(y200>y100 && y200<1.0f);

    // RGB chromaticity ratios are preserved by the luminance-only scale.
    {
        float r=2.0f,g=1.0f,b=0.5f;
        const float rg=r/g, gb=g/b;
        assert(h::luminance709(r,g,b)>h::kReferenceWhite);
        assert(h::map_90_100(r,g,b));
        assert(close(r/g,rg));
        assert(close(g/b,gb));
        assert(h::luminance709(r,g,b)<=1.0f);
    }

    // Invalid input fails closed.
    {
        float r=NAN,g=.5f,b=.5f;
        assert(!h::map_90_100(r,g,b));
    }

    std::cout << "PRESENTATION_HEADROOM_MAP_V0_1_PASS\n";
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="draw_headroom_map_") as tmp:
    tmp=Path(tmp)
    src=tmp/"test.cpp"
    exe=tmp/"test"
    src.write_text(CPP,encoding="utf-8")
    subprocess.run([
        "c++","-std=c++17","-O2","-Wall","-Wextra","-pedantic",
        f"-I{CPP_DIR}",str(src),"-o",str(exe)
    ],check=True)
    subprocess.run([str(exe)],check=True)
