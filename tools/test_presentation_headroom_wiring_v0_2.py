#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
JAVA = ROOT / "suite_android" / "app" / "src" / "main" / "java" / "com" / "truthraw" / "adaptiveui"
CPP = ROOT / "suite_android" / "app" / "src" / "main" / "cpp" / "photo_export_bridge.cpp"

checks = []

def require(path: Path, needle: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    if needle not in text:
        print(f"FAIL: {label}: missing {needle!r} in {path}", file=sys.stderr)
        raise SystemExit(2)
    checks.append(label)


def forbid(path: Path, needle: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    if needle in text:
        print(f"FAIL: {label}: forbidden {needle!r} remains in {path}", file=sys.stderr)
        raise SystemExit(2)
    checks.append(label)

mode = JAVA / "PresentationHeadroomModeV01.kt"
binding = JAVA / "DrawPhotoOutputCableV01.kt"
exporter = JAVA / "FullResJpegExport.kt"
main = JAVA / "MainActivity.kt"

require(mode, "const val OFF = 0", "mode-off")
require(mode, "const val PURE_MAP_90_TO_100 = 1", "mode-pure-90-100")
require(mode, "OUTPUT_PURE -> PURE_MAP_90_TO_100", "mode-route-pure")
require(mode, "OUTPUT_ADVANCED,", "mode-route-advanced")
require(mode, "OUTPUT_PRO -> OFF", "mode-route-pro")

require(binding, "val presentationHeadroomMode: Int,", "binding-field")
require(binding, "PresentationHeadroomModeV01.forPhotoOutputRoute(route)", "binding-freeze")
require(binding, "binding.presentationHeadroomMode != expectedHeadroomMode", "binding-safety")
require(binding, "currentHeadroomMode != binding.presentationHeadroomMode", "binding-picker-freshness")

require(exporter, "presentationHeadroomMode: Int,", "jni-argument")
require(exporter, "presentationHeadroomMode: Int = PresentationHeadroomModeV01.OFF", "helper-default-off")
require(exporter, "PresentationHeadroomModeV01.isKnown(presentationHeadroomMode)", "exporter-fail-closed")
require(exporter, "userQuarterTurns,\n                        presentationHeadroomMode,", "jni-forward")

require(main, "presentationHeadroomMode = binding.presentationHeadroomMode", "product-output-forward")

require(CPP, "constexpr jint kPresentationHeadroomOff = 0;", "native-mode-off")
require(CPP, "constexpr jint kPresentationHeadroomPureMap90To100 = 1;", "native-mode-pure")
require(CPP, "jint userQuarterTurns, jint presentationHeadroomMode,", "native-jni-argument")
require(CPP, "presentationHeadroomMode==kPresentationHeadroomPureMap90To100", "native-explicit-selector")
require(CPP, "process_pure_extended_linear_headroom_v0_1", "existing-linear-headroom-route-retained")
require(CPP, "presentation_headroom::map_90_100", "existing-90-100-map-retained")
forbid(CPP, "pureExtendedLinearHeadroomCandidate=(flags==0)", "no-flags-zero-selector")

# Scientific/sealed streaming is intentionally not patched by v0.2. This test is
# source-contract-only; the workflow separately runs the sealed integrity verifier.
print(f"PRESENTATION_HEADROOM_WIRING_V0_2_PASS checks={len(checks)}")
