#!/usr/bin/env python3

from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "suite_android" / "app" / "src" / "main" / "cpp" / "photo_export_bridge.cpp"

text = TARGET.read_text(encoding="utf-8")

already = (
    '#include "presentation_gamut_fit_v0_1.h"' in text
    and "presentation_gamut::fit_nonnegative_preserve_luminance" in text
    and "presentation_gamut::fit_unit_rgb_preserve_luminance" in text
)

if already:
    print("presentation gamut fit v0.1 already applied")
else:
    replacements = [
        (
            '#include "output_acutance_v0_81.h"\n',
            '#include "output_acutance_v0_81.h"\n#include "presentation_gamut_fit_v0_1.h"\n',
            "presentation gamut header include",
        ),
        (
            'namespace hdr_authority = truthraw::hdr_authority::v0_83;\n',
            'namespace hdr_authority = truthraw::hdr_authority::v0_83;\n'
            'namespace presentation_gamut = truthraw::presentation_gamut_fit::v0_1;\n',
            "presentation gamut namespace alias",
        ),
        (
            '                float r=std::max(supportRgb[3u*si],0.0f);\n'
            '                float g=std::max(supportRgb[3u*si+1u],0.0f);\n'
            '                float b=std::max(supportRgb[3u*si+2u],0.0f);\n'
            '                if(!std::isfinite(r)||!std::isfinite(g)||!std::isfinite(b)) r=g=b=0.0f;\n',
            '                float r=supportRgb[3u*si];\n'
            '                float g=supportRgb[3u*si+1u];\n'
            '                float b=supportRgb[3u*si+2u];\n'
            '                if(!presentation_gamut::fit_nonnegative_preserve_luminance(r,g,b)) {\n'
            '                    r=g=b=0.0f;\n'
            '                }\n',
            "replace channel-wise negative clipping",
        ),
        (
            '                const std::size_t ci=\n'
            '                    static_cast<std::size_t>(y-y0)*cw+static_cast<std::size_t>(x-x0);\n'
            '                coreRgb[3u*ci]=linear_to_srgb(r);\n',
            '                if(!presentation_gamut::fit_unit_rgb_preserve_luminance(r,g,b)) {\n'
            '                    return StreamStatus::error(\n'
            '                        StreamStatusCode::SinkFailed,\n'
            '                        "full-res presentation gamut fit failed");\n'
            '                }\n\n'
            '                const std::size_t ci=\n'
            '                    static_cast<std::size_t>(y-y0)*cw+static_cast<std::size_t>(x-x0);\n'
            '                coreRgb[3u*ci]=linear_to_srgb(r);\n',
            "insert final display-gamut fit",
        ),
    ]

    for old, new, label in replacements:
        count = text.count(old)
        if count != 1:
            print(f"FAIL: {label}: expected exactly one anchor, found {count}", file=sys.stderr)
            raise SystemExit(2)
        text = text.replace(old, new, 1)

    TARGET.write_text(text, encoding="utf-8")
    print("PRESENTATION_GAMUT_FIT_V0_1_PATCH_APPLIED")

# The full-resolution workflow historically invokes this patch entry point.
# Keep that stable outer cable and chain the new PURE Float32 headroom experiment
# from here, rather than requiring a change to any sealed scientific workflow.
subprocess.run(
    [sys.executable, str(ROOT / "tools" / "apply_pure_float_headroom_output_v0_1.py")],
    check=True,
)
