#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

# Reconstruct the exact proven v0.14 build-time source first. Never derive this probe from
# the checked-in v0.11 base alone; the working route is v0.11 -> v0.12 -> v0.13 -> v0.14.
ns = {}
exec(
    compile(
        Path('tools/patch_fotograaf_v014_seal_raw_before_result_mode_gate.py').read_text(),
        'patch_fotograaf_v014_seal_raw_before_result_mode_gate.py',
        'exec',
    ),
    ns,
    ns,
)
s = p.read_text()

# Version/provenance labels only; acquisition topology remains the proven v0.14 route.
s = s.replace('200MP test v0.14', '200MP test v0.16 HAL buffer envelope')
s = s.replace('TruthRaw · 200MP Tele Test v0.14', 'TruthRaw · 200MP Tele Test v0.16 · HAL envelope')
s = s.replace('STAGE 0 PASS · v0.14 UI', 'STAGE 0 PASS · v0.16 HAL-envelope UI')
s = s.replace('truthraw-200mp-v014', 'truthraw-200mp-v016')
s = s.replace('truthraw-v014-capability', 'truthraw-v016-capability')
s = s.replace('_v014.dng', '_v016.dng')
s = s.replace('_EVIDENCE_v014.json', '_EVIDENCE_v016.json')
s = s.replace('_v014.${if (contiguous)', '_v016.${if (contiguous)')
s = s.replace('staged-evidence.v0.14', 'staged-evidence.v0.16')
s = s.replace(
    'physical-5-scoped MAX request; exact RAW bytes are sealed before interpreting returned pixel-mode metadata',
    'physical-5-scoped MAX request; exact RAW bytes are sealed first, then the live Android HardwareBuffer envelope and visible vendor metadata are observed read-only',
)

# Preserve v0.14's strongest rule: the original Plane[0] bytes are sealed BEFORE any new probe.
needle = '''            val stamp = System.currentTimeMillis()\n            val rawEvidence = persistOriginalRawBuffer(image, stamp)\n            capturedRaw = rawEvidence.file\n\n            // Returned pixel mode is an observation, not a retroactive veto on delivered bytes.\n'''
replacement = '''            val stamp = System.currentTimeMillis()\n            val rawEvidence = persistOriginalRawBuffer(image, stamp)\n            capturedRaw = rawEvidence.file\n\n            // Stage 3.5: observe the still-live Android buffer envelope only AFTER the exact\n            // Plane[0] source bytes are safely persisted + SHA-256 sealed, and BEFORE DNG.\n            // The probe does not lock/map/write the HardwareBuffer and never becomes source truth.\n            val physical = physical5Characteristics ?: error("physical characteristics ontbreken")\n            val halEnvelopeAttempt = runCatching {\n                Camera2EnvelopeProbe.observe(image, physical, physicalResult)\n            }\n            val halEnvelope = halEnvelopeAttempt.getOrNull()\n            val halEnvelopeError = halEnvelopeAttempt.exceptionOrNull()?.let {\n                "${it.javaClass.simpleName}: ${it.message}"\n            }\n\n            // Returned pixel mode is an observation, not a retroactive veto on delivered bytes.\n'''
if needle not in s:
    raise SystemExit('v0.14 seal-first insertion point not found')
s = s.replace(needle, replacement, 1)

# The physical characteristics reference now exists before Stage 3.5.
old_physical = '            val physical = physical5Characteristics ?: error("physical characteristics ontbreken")\n            var dng: File? = null\n'
new_physical = '            var dng: File? = null\n'
if old_physical not in s:
    raise SystemExit('v0.14 physical characteristics marker not found')
s = s.replace(old_physical, new_physical, 1)

old_report = '''                    .put("sealBeforePixelModeInterpretation", true)\n                    .toString(2)\n'''
new_report = '''                    .put("sealBeforePixelModeInterpretation", true)\n                    .put("halBufferEnvelope", halEnvelope ?: JSONObject.NULL)\n                    .put("halBufferEnvelopeError", halEnvelopeError ?: JSONObject.NULL)\n                    .put("halBufferEnvelopeCollectedAfterRawSealBeforeDng", true)\n                    .put("halBufferProbeLockedOrMappedBuffer", false)\n                    .put("halBufferProbeModifiedSource", false)\n                    .toString(2)\n'''
if old_report not in s:
    raise SystemExit('v0.14 evidence report chain not found')
s = s.replace(old_report, new_report, 1)

old_status = '''                    "returned SENSOR_PIXEL_MODE=${returnedPixelMode ?: "null"} · requested/output MAX=true · mismatchPreserved=${!returnedPixelModeIsMaximum}",\n'''
new_status = '''                    "returned SENSOR_PIXEL_MODE=${returnedPixelMode ?: "null"} · requested/output MAX=true · mismatchPreserved=${!returnedPixelModeIsMaximum}\\n" +\n                    "Stage 3.5 HAL envelope=${if (halEnvelope != null) "captured" else "unavailable"} · source already sealed",\n'''
if old_status not in s:
    raise SystemExit('v0.14 success status marker not found')
s = s.replace(old_status, new_status, 1)

assert 'TruthRaw · 200MP Tele Test v0.16 · HAL envelope' in s
assert 'Camera2EnvelopeProbe.observe(image, physical, physicalResult)' in s
assert s.index('persistOriginalRawBuffer(image, stamp)') < s.index('Camera2EnvelopeProbe.observe(image, physical, physicalResult)')
assert s.index('Camera2EnvelopeProbe.observe(image, physical, physicalResult)') < s.index('DngCreator(physical, physicalResult)')
assert 'halBufferProbeLockedOrMappedBuffer' in s
assert 'APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF' in s
assert 'MAXIMUM_RESOLUTION vereist' not in s

p.write_text(s)
print('patched', p)
print('bytes', p.stat().st_size)
