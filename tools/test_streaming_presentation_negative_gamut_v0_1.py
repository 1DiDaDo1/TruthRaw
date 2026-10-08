#!/usr/bin/env python3

from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
STREAMING_DIR = ROOT / "docs" / "research" / "full-frame-streaming-v0.1" / "native"
PASS2 = STREAMING_DIR / "full_frame_streaming_v0_1_pass2.cpp"
PHOTO = ROOT / "suite_android" / "app" / "src" / "main" / "cpp" / "photo_export_bridge.cpp"

pass2 = PASS2.read_text(encoding="utf-8")
photo = PHOTO.read_text(encoding="utf-8")

required_pass2 = [
    '#include "streaming_presentation_gamut_fit_v0_1.h"',
    "PresentationNegativeGamutPolicy::PreserveLuminance",
    "presentation_gamut::fit_nonnegative_preserve_luminance(rr, gg, bb)",
]
for token in required_pass2:
    if token not in pass2:
        raise SystemExit(f"FAIL: streaming pass2 missing {token!r}")

if "PresentationNegativeGamutPolicy::PreserveLuminance" not in photo:
    raise SystemExit("FAIL: photo output did not opt into luminance-preserving pre-sink gamut fit")

CPP = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

#include "streaming_presentation_gamut_fit_v0_1.h"

namespace gamut = truthraw::streaming_v0_1::presentation_gamut_v0_1;

static bool close(double a, double b, double eps = 2e-6) {
    return std::abs(a - b) <= eps;
}

static double span(float r, float g, float b) {
    return static_cast<double>(std::max(r, std::max(g, b)) -
                               std::min(r, std::min(g, b)));
}

int main() {
    // Already non-negative presentation values are exact identity.
    {
        float r = 0.18f, g = 0.42f, b = 0.91f;
        const float r0 = r, g0 = g, b0 = b;
        assert(gamut::fit_nonnegative_preserve_luminance(r, g, b));
        assert(r == r0 && g == g0 && b == b0);
    }

    // A negative channel is not independently clipped. Chroma is compressed
    // toward the neutral axis while representable Rec.709 luminance is kept.
    {
        float r = 1.20f, g = 0.70f, b = -0.15f;
        const double y0 = gamut::luminance709(r, g, b);
        const double hardClipSpan = span(1.20f, 0.70f, 0.0f);
        assert(gamut::fit_nonnegative_preserve_luminance(r, g, b));
        assert(r >= 0.0f && g >= 0.0f && b >= 0.0f);
        assert(close(gamut::luminance709(r, g, b), y0));
        assert(span(r, g, b) < hardClipSpan);
    }

    // Purple-stress case: clipping green independently must not be the policy.
    {
        float r = 0.62f, g = -0.08f, b = 1.15f;
        const double y0 = gamut::luminance709(r, g, b);
        assert(y0 > 0.0);
        assert(gamut::fit_nonnegative_preserve_luminance(r, g, b));
        assert(g >= 0.0f);
        assert(close(gamut::luminance709(r, g, b), y0));
    }

    // Non-positive luminance has only one non-negative neutral endpoint.
    {
        float r = -0.20f, g = -0.10f, b = 0.05f;
        assert(gamut::luminance709(r, g, b) <= 0.0);
        assert(gamut::fit_nonnegative_preserve_luminance(r, g, b));
        assert(r == 0.0f && g == 0.0f && b == 0.0f);
    }

    // Invalid values fail closed.
    {
        float r = std::numeric_limits<float>::quiet_NaN();
        float g = 0.5f, b = 0.5f;
        assert(!gamut::fit_nonnegative_preserve_luminance(r, g, b));
    }

    std::cout << "STREAMING_PRESENTATION_NEGATIVE_GAMUT_V0_1_PASS\n";
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="draw_streaming_gamut_") as tmp:
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
            f"-I{STREAMING_DIR}",
            str(cpp),
            "-o",
            str(exe),
        ],
        check=True,
    )
    subprocess.run([str(exe)], check=True)
