#!/usr/bin/env python3

from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
HEADER_DIR = ROOT / "suite_android" / "app" / "src" / "main" / "cpp"

CPP = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <iostream>

#include "presentation_gamut_fit_v0_1.h"

namespace gamut = truthraw::presentation_gamut_fit::v0_1;

static bool close(double a, double b, double eps = 2e-6) {
    return std::abs(a - b) <= eps;
}

static double chroma_span(float r, float g, float b) {
    return static_cast<double>(std::max(r, std::max(g, b)) -
                               std::min(r, std::min(g, b)));
}

int main() {
    // 1. In-gamut presentation RGB is an exact identity operation.
    {
        float r = 0.125f, g = 0.5f, b = 0.875f;
        const float r0 = r, g0 = g, b0 = b;
        assert(gamut::fit_nonnegative_preserve_luminance(r, g, b));
        assert(r == r0 && g == g0 && b == b0);
        assert(gamut::fit_unit_rgb_preserve_luminance(r, g, b));
        assert(r == r0 && g == g0 && b == b0);
    }

    // 2. Negative camera/display RGB is healed toward the neutral axis rather
    // than hard-clipping one channel. Representable luminance is preserved.
    {
        float r = 1.20f, g = 0.70f, b = -0.15f;
        const double y0 = gamut::luminance709(r, g, b);
        const double hard_clip_span = chroma_span(1.20f, 0.70f, 0.0f);
        assert(gamut::fit_nonnegative_preserve_luminance(r, g, b));
        assert(r >= 0.0f && g >= 0.0f && b >= 0.0f);
        assert(close(gamut::luminance709(r, g, b), y0));
        assert(chroma_span(r, g, b) < hard_clip_span);
        assert(b <= 2e-6f);
    }

    // 3. A value outside both sides of the display cube is compressed into
    // [0,1] while preserving representable Rec.709 luminance.
    {
        float r = -0.10f, g = 0.52f, b = 1.20f;
        const double y0 = gamut::luminance709(r, g, b);
        assert(y0 > 0.0 && y0 < 1.0);
        assert(gamut::fit_unit_rgb_preserve_luminance(r, g, b));
        assert(r >= 0.0f && r <= 1.0f);
        assert(g >= 0.0f && g <= 1.0f);
        assert(b >= 0.0f && b <= 1.0f);
        assert(close(gamut::luminance709(r, g, b), y0));
    }

    // 4. Non-positive display luminance maps to display black, rather than a
    // channel-selective saturated residue.
    {
        float r = -0.20f, g = -0.10f, b = 0.05f;
        assert(gamut::luminance709(r, g, b) <= 0.0);
        assert(gamut::fit_nonnegative_preserve_luminance(r, g, b));
        assert(r == 0.0f && g == 0.0f && b == 0.0f);
    }

    // 5. Above-display luminance maps to neutral display white; this is the
    // only representable neutral endpoint once luminance itself exceeds 1.
    {
        float r = 1.20f, g = 1.10f, b = 1.30f;
        assert(gamut::fit_unit_rgb_preserve_luminance(r, g, b));
        assert(r == 1.0f && g == 1.0f && b == 1.0f);
    }

    // 6. NaN/Inf are rejected fail-closed.
    {
        float r = std::numeric_limits<float>::quiet_NaN();
        float g = 0.5f, b = 0.5f;
        assert(!gamut::fit_nonnegative_preserve_luminance(r, g, b));
    }
    {
        float r = 0.5f;
        float g = std::numeric_limits<float>::infinity();
        float b = 0.5f;
        assert(!gamut::fit_unit_rgb_preserve_luminance(r, g, b));
    }

    std::cout << "PRESENTATION_GAMUT_FIT_V0_1_PASS\n";
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="draw_gamut_fit_") as tmp:
    tmp_path = Path(tmp)
    cpp = tmp_path / "test.cpp"
    exe = tmp_path / "test"
    cpp.write_text(CPP, encoding="utf-8")
    subprocess.run(
        [
            "c++",
            "-std=c++17",
            "-O2",
            "-Wall",
            "-Wextra",
            "-pedantic",
            f"-I{HEADER_DIR}",
            str(cpp),
            "-o",
            str(exe),
        ],
        check=True,
    )
    subprocess.run([str(exe)], check=True)

# Keep the historical workflow entry point while making the new output-only
# headroom candidate part of the same regression gate.
subprocess.run(
    [sys.executable, str(ROOT / "tools" / "test_presentation_headroom_map_v0_1.py")],
    check=True,
)
