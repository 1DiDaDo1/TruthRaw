#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
JAVA_ROOT = ROOT / "app/android/truthraw-adaptive-ui-v01/app/src/main/java/com/truthraw/adaptiveui"
POLICY = JAVA_ROOT / "OutputModePolicy.kt"
ACTIVITY = JAVA_ROOT / "OutputModeActivity.kt"
PROCESSING = JAVA_ROOT / "MainActivity.kt"
MANIFEST = ROOT / "app/android/truthraw-adaptive-ui-v01/app/src/main/AndroidManifest.xml"

policy = POLICY.read_text(encoding="utf-8")
activity = ACTIVITY.read_text(encoding="utf-8")
processing = PROCESSING.read_text(encoding="utf-8")
manifest = MANIFEST.read_text(encoding="utf-8")

required_modes = {
    'JPG("JPG"',
    'JPG_XL("JPG_XL"',
    'TRUTHRAW_PURE("TRUTHRAW_PURE"',
    'TRUTHRAW_ADVANCED("TRUTHRAW_ADVANCED"',
}
for token in required_modes:
    assert token in policy, f"missing output mode token: {token}"

assert re.search(r'TRUTHRAW_PURE\("TRUTHRAW_PURE"[^\n]*false\)', policy), \
    "TRUTHRAW PURE must not expose appearance options"
assert 'const val JPEG_XL_ENCODER_VALIDATED: Boolean = false' in policy, \
    "JPEG XL must stay blocked until encoder validation is promoted"
assert 'TruthRawOutputMode.TRUTHRAW_PURE -> setOf(RawProjectionKind.TRUTHRAW_PURE_FLOAT32_DNG)' in policy, \
    "TRUTHRAW PURE must resolve only to the float32 scientific DNG projection"
assert 'TruthRawOutputMode.TRUTHRAW_ADVANCED -> RawProjectionKind.entries.toSet()' in policy, \
    "TRUTHRAW ADVANCED must retain explicit access to downstream projection choices"
assert 'fun scientificAuthorityMayChange' in policy and 'Boolean = false' in policy, \
    "presentation policy must state that scientific authority cannot change"
assert 'appearance = if (mode.supportsAppearanceOptions) requested else AppearanceIntent()' in policy, \
    "unsupported appearance controls must sanitize to neutral"

assert 'private var selectedMode = TruthRawOutputMode.TRUTHRAW_PURE' in activity, \
    "launcher must default to TRUTHRAW PURE"
assert 'OutputModePolicy.selection(selectedMode, colourful, detailed, soft, hdr)' in activity, \
    "launcher must sanitize through shared policy before handoff"
assert 'OutputModePolicy.JPEG_XL_ENCODER_VALIDATED' in activity, \
    "launcher must expose JPEG XL pending state from policy"

assert 'private var outputSelection: OutputModeSelection = OutputModePolicy.fromWireName(null)' in processing, \
    "processing UI must fail safe to TRUTHRAW PURE when no handoff is present"
assert 'intent.getStringExtra(OutputModeActivity.EXTRA_OUTPUT_MODE)' in processing, \
    "processing UI must consume the launcher output-mode handoff"
assert 'OutputModePolicy.allowsJpeg(outputSelection)' in processing, \
    "JPEG export must be policy-gated"
assert processing.count('kind !in OutputModePolicy.allowedRawProjectionKinds(outputSelection)') >= 2, \
    "RAW/DNG export must be policy-gated before and after the document dialog"
assert 'RawProjectionKind.entries.filter { it in allowed }' in processing, \
    "processing UI must expose only policy-allowed RAW/DNG projectors"
assert 'JPG XL blijft fail-closed' in processing, \
    "JPEG XL must visibly remain blocked while validation is pending"
assert 'Appearance-intent:' in processing and 'nog niet gekoppeld' in processing, \
    "unvalidated appearance semantics must be presented as intent-only"
assert 'Scientific Master, TruthRange, zero-line, evidence-counts en Backplane blijven onveranderd.' in processing, \
    "processing UI must preserve the authority boundary in user-facing text"

assert '.OutputModeActivity' in manifest and 'android.intent.action.MAIN' in manifest, \
    "OutputModeActivity must remain the launcher"
assert '<activity\n            android:name=".MainActivity"\n            android:exported="false"' in manifest, \
    "processing activity must remain non-exported"
assert 'android.permission.CAMERA' not in manifest, \
    "output-mode UI must not acquire camera/capture authority"

locale_paths = [
    ROOT / "app/android/truthraw-adaptive-ui-v01/app/src/main/res/values/strings.xml",
    ROOT / "app/android/truthraw-adaptive-ui-v01/app/src/main/res/values-nl/strings.xml",
    ROOT / "app/android/truthraw-adaptive-ui-v01/app/src/main/res/values-de/strings.xml",
    ROOT / "app/android/truthraw-adaptive-ui-v01/app/src/main/res/values-fr/strings.xml",
]
required_strings = [
    'name="mode_jpg"',
    'name="mode_jxl"',
    'name="mode_pure"',
    'name="mode_advanced"',
    'name="appearance_options_hint"',
    'name="jxl_pending"',
    'name="certificate_hint"',
]
for path in locale_paths:
    text = path.read_text(encoding="utf-8")
    for token in required_strings:
        assert token in text, f"{path}: missing localized resource {token}"

print("OUTPUT_MODE_POLICY_PASS")
print("modes=4")
print("launcher_to_processing_handoff=ENFORCED")
print("pure_appearance=LOCKED_NEUTRAL")
print("pure_raw_projection=TRUTHRAW_PURE_FLOAT32_DNG_ONLY")
print("jpeg_xl_encoder_validated=false")
print("scientific_authority_mutable=false")
