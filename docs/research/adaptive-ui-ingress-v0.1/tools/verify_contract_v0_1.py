#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[4]
UI = ROOT / "app/android/truthraw-adaptive-ui-v01/app/src/main/java/com/truthraw/adaptiveui/MainActivity.kt"
INGRESS = ROOT / "app/android/truthraw-adaptive-ui-v01/app/src/main/java/com/truthraw/adaptiveui/IngressModels.kt"

required_ui = [
    "Intent.EXTRA_ALLOW_MULTIPLE",
    "windowManager.currentWindowMetrics",
    "LayoutTier.COMPACT",
    "LayoutTier.MEDIUM",
    "LayoutTier.EXPANDED",
    "InputRoute.SINGLE_ONE_OUTPUT",
    "InputRoute.SINGLE_MULTIPLE_OUTPUTS",
    "InputRoute.BATCH_INDEPENDENT",
    "InputRoute.MULTI_CAPTURE_ENHANCED",
    "InputRoute.MULTI_CAPTURE_HDR",
]
required_ingress = [
    "data class RawHandle",
    "data class RawJob",
    "data class BatchSession",
    "uris.distinct()",
    "OpenableColumns.DISPLAY_NAME",
    "OpenableColumns.SIZE",
]
forbidden_ingress = [
    "ByteArray",
    "readBytes(",
    "openInputStream(",
    "decodeByteArray(",
]

errors = []
ui = UI.read_text(encoding="utf-8")
ingress = INGRESS.read_text(encoding="utf-8")

for token in required_ui:
    if token not in ui:
        errors.append(f"missing UI contract token: {token}")
for token in required_ingress:
    if token not in ingress:
        errors.append(f"missing ingress contract token: {token}")
for token in forbidden_ingress:
    if token in ingress:
        errors.append(f"forbidden source materialization token in ingress: {token}")

if "InputRoute.MULTI_CAPTURE_HDR" in ingress:
    errors.append("ingress must not auto-promote selection to HDR; route choice belongs to explicit UI/session state")

if errors:
    print("ADAPTIVE_UI_INGRESS_V0_1_CONTRACT_FAIL")
    for error in errors:
        print(f"- {error}")
    sys.exit(1)

print("ADAPTIVE_UI_INGRESS_V0_1_CONTRACT_PASS")
print("source_raw_full_materialization=0_by_ui_contract")
print("multi_select=EXPLICIT")
print("multi_capture_fusion=AUTO_DISABLED")
print("layout_axis=CURRENT_WINDOW")
print("science_authority=UPSTREAM_ONLY")
