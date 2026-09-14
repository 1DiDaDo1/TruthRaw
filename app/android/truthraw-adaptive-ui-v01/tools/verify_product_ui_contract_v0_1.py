#!/usr/bin/env python3
from pathlib import Path
import sys
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[4]
APP = ROOT / "app/android/truthraw-adaptive-ui-v01/app"
MAIN = APP / "src/main"
JAVA = MAIN / "java/com/truthraw/adaptiveui"
RES = MAIN / "res"


def fail(message: str) -> None:
    print(f"TRUTHRAW_PRODUCT_UI_CONTRACT_FAIL: {message}")
    raise SystemExit(1)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def string_names(path: Path) -> set[str]:
    root = ET.parse(path).getroot()
    return {node.attrib["name"] for node in root.findall("string") if "name" in node.attrib}


required_strings = {
    "app_name",
    "choose_output",
    "choose_output_subtitle",
    "mode_jpg",
    "mode_jpg_subtitle",
    "mode_jxl",
    "mode_jxl_subtitle",
    "mode_pure",
    "mode_pure_subtitle",
    "mode_advanced",
    "mode_advanced_subtitle",
    "appearance_options",
    "appearance_options_hint",
    "option_colourful",
    "option_detailed",
    "option_soft",
    "option_hdr",
    "pure_locked_hint",
    "continue_to_raw",
    "selected_mode",
    "jxl_pending",
    "certificate_hint",
}

english = RES / "values/strings.xml"
require(english.exists(), "English fallback strings.xml missing")
require(required_strings <= string_names(english), "English fallback is missing required product strings")

for qualifier in ("values-nl", "values-de", "values-fr"):
    path = RES / qualifier / "strings.xml"
    require(path.exists(), f"{qualifier} strings.xml missing")
    missing = required_strings - string_names(path)
    require(not missing, f"{qualifier} missing strings: {sorted(missing)}")

locale_path = RES / "xml/locales_config.xml"
require(locale_path.exists(), "Android localeConfig missing")
android_ns = "{http://schemas.android.com/apk/res/android}"
locale_root = ET.parse(locale_path).getroot()
locales = [node.attrib.get(android_ns + "name") for node in locale_root.findall("locale")]
require(locales == ["en", "nl", "de", "fr"], f"unexpected locale order/content: {locales}")

manifest_text = (MAIN / "AndroidManifest.xml").read_text(encoding="utf-8")
for expected in (
    'android:icon="@mipmap/ic_launcher"',
    'android:roundIcon="@mipmap/ic_launcher_round"',
    'android:localeConfig="@xml/locales_config"',
    'android:name=".OutputModeActivity"',
    'android.intent.category.LAUNCHER',
):
    require(expected in manifest_text, f"manifest missing {expected}")

for path in (
    RES / "mipmap-anydpi/ic_launcher.xml",
    RES / "mipmap-anydpi-v26/ic_launcher.xml",
    RES / "mipmap-anydpi-v26/ic_launcher_round.xml",
    RES / "drawable/truthraw_launcher_art.xml",
):
    require(path.exists(), f"launcher resource missing: {path.relative_to(ROOT)}")

policy = (JAVA / "OutputModePolicy.kt").read_text(encoding="utf-8")
for expected in (
    'JPG("JPG", R.string.mode_jpg, R.string.mode_jpg_subtitle, true)',
    'JPG_XL("JPG_XL", R.string.mode_jxl, R.string.mode_jxl_subtitle, true)',
    'TRUTHRAW_PURE("TRUTHRAW_PURE", R.string.mode_pure, R.string.mode_pure_subtitle, false)',
    'TRUTHRAW_ADVANCED("TRUTHRAW_ADVANCED", R.string.mode_advanced, R.string.mode_advanced_subtitle, true)',
    'const val JPEG_XL_ENCODER_VALIDATED: Boolean = false',
    'TruthRawOutputMode.TRUTHRAW_PURE -> setOf(RawProjectionKind.TRUTHRAW_PURE_FLOAT32_DNG)',
):
    require(expected in policy, f"output-mode policy drift: {expected}")

activity = (JAVA / "OutputModeActivity.kt").read_text(encoding="utf-8")
for expected in (
    'modeRow(TruthRawOutputMode.JPG, TruthRawOutputMode.JPG_XL)',
    'modeRow(TruthRawOutputMode.TRUTHRAW_PURE, TruthRawOutputMode.TRUTHRAW_ADVANCED)',
    'if (selectedMode.supportsAppearanceOptions)',
    'optionCheckBox(R.string.option_colourful',
    'optionCheckBox(R.string.option_detailed',
    'optionCheckBox(R.string.option_soft',
    'optionCheckBox(R.string.option_hdr',
):
    require(expected in activity, f"launcher UI contract drift: {expected}")

build = (APP / "build.gradle.kts").read_text(encoding="utf-8")
require('versionName = "0.7-four-mode-certificate"' in build, "unexpected Android versionName")

print("TRUTHRAW_PRODUCT_UI_CONTRACT_PASS")
print("modes=JPG,JPG_XL,TRUTHRAW_PURE,TRUTHRAW_ADVANCED")
print("appearance_options=COLOURFUL,DETAILED,SOFT,HDR")
print("pure_appearance_options=DISABLED")
print("jpeg_xl_encoder=FAIL_CLOSED_PENDING_VALIDATION")
print("locales=en,nl,de,fr")
print("launcher_icon=TRUTHRAW_ADAPTIVE_ICON")
