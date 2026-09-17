#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

# Reconstruct the proven v0.16 source first. v0.16 itself reconstructs the exact v0.14
# acquisition route, then adds the read-only post-HAL HardwareBuffer/vendor envelope.
ns = {}
exec(
    compile(
        Path('tools/patch_fotograaf_v016_hal_buffer_envelope.py').read_text(),
        'patch_fotograaf_v016_hal_buffer_envelope.py',
        'exec',
    ),
    ns,
    ns,
)
s = p.read_text()

# Proven acquisition topology is unchanged. v0.17 adds request-side observation only.
s = s.replace('v0.16 HAL buffer envelope', 'v0.17 pre/post HAL airlock')
s = s.replace('v0.16 · HAL envelope', 'v0.17 · Camera-5 airlock')
s = s.replace('v0.16 HAL-envelope UI', 'v0.17 airlock UI')
s = s.replace('truthraw-200mp-v016', 'truthraw-200mp-v017')
s = s.replace('truthraw-v016-capability', 'truthraw-v017-capability')
s = s.replace('_v016.dng', '_v017.dng')
s = s.replace('_EVIDENCE_v016.json', '_EVIDENCE_v017.json')
s = s.replace('_v016.${if (contiguous)', '_v017.${if (contiguous)')
s = s.replace('staged-evidence.v0.16', 'staged-evidence.v0.17')

# v0.12 added the advertised/error fields between lastPhysicalPixelModeWritten and capturedRaw.
field_needle = '''    private var lastPhysicalPixelModeForceError: String? = null\n\n    private var capturedRaw: File? = null\n'''
field_replacement = '''    private var lastPhysicalPixelModeForceError: String? = null\n    private var lastPreHalSessionGate: JSONObject? = null\n    private var lastPreHalRequestGate: JSONObject? = null\n\n    private var capturedRaw: File? = null\n'''
if field_needle not in s:
    raise SystemExit('request-state field insertion point not found')
s = s.replace(field_needle, field_replacement, 1)

session_needle = '''        val config = SessionConfiguration(\n'''
session_replacement = '''        // Gate A: our request-side door before the vendor session is created. This cannot access\n        // pre-HAL sensor pixels; it freezes the app-visible control surface/intention before HONOR/QTI\n        // selects and executes the internal session pipeline. Unknown vendor keys are never written.\n        lastPreHalSessionGate = runCatching {\n            Camera2PreHalGate.observeSession(\n                logical = logical,\n                physical = physical,\n                physicalId = PHYSICAL_ID,\n                width = TARGET_W,\n                height = TARGET_H,\n                format = ImageFormat.RAW_SENSOR,\n                outputPhysicalBinding = true,\n                outputMaximumResolutionModeDeclared = true,\n            )\n        }.getOrNull()\n        lastPreHalRequestGate = null\n\n        val config = SessionConfiguration(\n'''
if session_needle not in s:
    raise SystemExit('session gate insertion point not found')
s = s.replace(session_needle, session_replacement, 1)

request_needle = '''            val request = requestBuilder.build()\n'''
request_replacement = '''            // Gate B: freeze the exact request-side route state immediately before build/submit.\n            // This is observation-only: no vendor request key is set from its name or guessed meaning.\n            val preHalPhysical = physical5Characteristics ?: error("physical characteristics ontbreken")\n            lastPreHalRequestGate = runCatching {\n                Camera2PreHalGate.observeRequest(\n                    builder = requestBuilder,\n                    logical = logical,\n                    physical = preHalPhysical,\n                    physicalId = PHYSICAL_ID,\n                    scopedPhysicalRequestUsed = lastScopedRequestUsed,\n                    globalPixelModeWritten = lastGlobalPixelModeWritten,\n                    physicalPixelModeWritten = lastPhysicalPixelModeWritten,\n                )\n            }.getOrNull()\n\n            val request = requestBuilder.build()\n'''
if request_needle not in s:
    raise SystemExit('request gate insertion point not found')
s = s.replace(request_needle, request_replacement, 1)

report_needle = '''                    .put("halBufferEnvelope", halEnvelope ?: JSONObject.NULL)\n'''
report_replacement = '''                    .put("preHalSessionGate", lastPreHalSessionGate ?: JSONObject.NULL)\n                    .put("preHalRequestGate", lastPreHalRequestGate ?: JSONObject.NULL)\n                    .put("preHalGateChangedVendorKeys", false)\n                    .put("preHalGatePixelAccess", false)\n                    .put("halBufferEnvelope", halEnvelope ?: JSONObject.NULL)\n'''
if report_needle not in s:
    raise SystemExit('evidence gate insertion point not found')
s = s.replace(report_needle, report_replacement, 1)

status_needle = '''                    "Stage 3.5 HAL envelope=${if (halEnvelope != null) "captured" else "unavailable"} · source already sealed",\n'''
status_replacement = '''                    "Gate A/B pre-HAL route fingerprint=${if (lastPreHalSessionGate != null && lastPreHalRequestGate != null) "captured" else "partial"}\\n" +\n                    "Stage 3.5 post-HAL envelope=${if (halEnvelope != null) "captured" else "unavailable"} · source already sealed",\n'''
if status_needle not in s:
    raise SystemExit('status airlock insertion point not found')
s = s.replace(status_needle, status_replacement, 1)

assert 'TruthRaw · 200MP Tele Test v0.17 · Camera-5 airlock' in s
assert 'Camera2PreHalGate.observeSession(' in s
assert 'Camera2PreHalGate.observeRequest(' in s
assert s.index('Camera2PreHalGate.observeSession(') < s.index('device.createCaptureSession(config)')
assert s.index('Camera2PreHalGate.observeRequest(') < s.index('val request = requestBuilder.build()')
assert s.index('persistOriginalRawBuffer(image, stamp)') < s.index('Camera2EnvelopeProbe.observe(image, physical, physicalResult)')
assert 'preHalGateChangedVendorKeys' in s
assert 'APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF' in s
assert 'MAXIMUM_RESOLUTION vereist' not in s

p.write_text(s)
print('patched', p)
print('bytes', p.stat().st_size)
