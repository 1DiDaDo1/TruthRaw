#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

# Reconstruct the exact v0.20 trusted acquisition/audit chain. v0.22 is deliberately
# diagnostic-only: it performs an app-side Camera2 type/marshalling dry-run and stops
# before any SessionConfiguration is created/submitted with a vendor intervention.
ns = {}
exec(
    compile(
        Path('tools/patch_fotograaf_v020_payload_geometry_decoder.py').read_text(),
        'patch_fotograaf_v020_payload_geometry_decoder.py',
        'exec',
    ),
    ns,
    ns,
)
s = p.read_text()

s = s.replace(
    'TruthRaw · 200MP Tele Test v0.20 · Camera-5 payload geometry decoder',
    'TruthRaw · 200MP Tele Test v0.22 · IdealRAW type dry-run',
)
s = s.replace('v0.20 payload geometry decoder', 'v0.22 IdealRAW type dry-run')
s = s.replace('v0.20 payload-geometry UI', 'v0.22 IdealRAW type-dry-run UI')
s = s.replace('truthraw-200mp-v020', 'truthraw-200mp-v022')
s = s.replace('truthraw-v020-capability', 'truthraw-v022-capability')

field_needle = '''    private var lastPreHalSessionGate: JSONObject? = null\n    private var lastPreHalRequestGate: JSONObject? = null\n\n    private var capturedRaw: File? = null\n'''
field_replacement = '''    private var lastPreHalSessionGate: JSONObject? = null\n    private var lastPreHalRequestGate: JSONObject? = null\n    private var lastIdealRawTypeDryRun: JSONObject? = null\n\n    private var capturedRaw: File? = null\n'''
if field_needle not in s:
    raise SystemExit('v0.22 state-field anchor not found')
s = s.replace(field_needle, field_replacement, 1)

needle = '''        lastPreHalRequestGate = null\n\n        val config = SessionConfiguration(\n'''
replacement = '''        lastPreHalRequestGate = null\n        lastIdealRawTypeDryRun = null\n\n        // v0.22 diagnostic gate: use only disposable app-side CaptureRequest builders to test\n        // candidate value representations for EnableIdealRAW. Nothing is attached to a session,\n        // nothing is submitted to HAL, and no capture is issued in this build.\n        val typeDryRun = Camera2IdealRawTypeDryRun.probe(\n            device = device,\n            logical = logical,\n            physical = physical,\n        )\n        lastIdealRawTypeDryRun = typeDryRun.evidence\n        val passingKinds = if (typeDryRun.passingKinds.isEmpty()) "none" else typeDryRun.passingKinds.joinToString(",")\n        setStatus(\n            "STAGE 3 DIAGNOSTIC STOP · v0.22 IdealRAW type dry-run\\n" +\n                "classification=${typeDryRun.evidence.optString(\"classification\", \"UNKNOWN\")}\\n" +\n                "passingKinds=$passingKinds\\n" +\n                "HAL/session submission=false · capture=false · v0.20 control onaangeroerd.",\n        )\n        closeCameraResources(keepOutputs = true)\n        previewButton.isEnabled = true\n        return\n\n        @Suppress("UNREACHABLE_CODE")\n        val config = SessionConfiguration(\n'''
if needle not in s:
    raise SystemExit('v0.22 diagnostic insertion anchor not found')
s = s.replace(needle, replacement, 1)

assert 'TruthRaw · 200MP Tele Test v0.22 · IdealRAW type dry-run' in s
assert 'Camera2IdealRawTypeDryRun.probe(' in s
probe_i = s.index('Camera2IdealRawTypeDryRun.probe(')
gate_i = s.index('Camera2PreHalGate.observeSession(')
raw_config_i = s.index('val config = SessionConfiguration(', probe_i)
assert gate_i < probe_i < raw_config_i
assert 'HAL/session submission=false' in s
assert 'MAXIMUM_RESOLUTION vereist' not in s

p.write_text(s)
print('patched', p)
print('bytes', p.stat().st_size)
