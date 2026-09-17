#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

# Reconstruct the proven v0.17 pre/post-HAL airlock. v0.19 intentionally branches from v0.17,
# not from the experimental v0.18 focus probe: first settle whether the sealed 401 MB source file
# is actually populated across the declared 16320x12288 raster.
ns = {}
exec(
    compile(
        Path('tools/patch_fotograaf_v017_pre_hal_airlock.py').read_text(),
        'patch_fotograaf_v017_pre_hal_airlock.py',
        'exec',
    ),
    ns,
    ns,
)
s = p.read_text()

# Version/provenance labels. Acquisition topology remains the proven v0.14/v0.17 route.
s = s.replace('v0.17 pre/post HAL airlock', 'v0.19 sealed full-raster write audit')
s = s.replace('v0.17 · Camera-5 airlock', 'v0.19 · Camera-5 full-raster audit')
s = s.replace('v0.17 airlock UI', 'v0.19 full-raster-audit UI')
s = s.replace('truthraw-200mp-v017', 'truthraw-200mp-v019')
s = s.replace('truthraw-v017-capability', 'truthraw-v019-capability')
s = s.replace('_v017.dng', '_v019.dng')
s = s.replace('_EVIDENCE_v017.json', '_EVIDENCE_v019.json')
s = s.replace('_v017.${if (contiguous)', '_v019.${if (contiguous)')
s = s.replace('staged-evidence.v0.17', 'staged-evidence.v0.19')
s = s.replace(
    'physical-5-scoped MAX request; exact RAW bytes are sealed first, then the live Android HardwareBuffer envelope and visible vendor metadata are observed read-only',
    'physical-5-scoped MAX request; exact RAW bytes are sealed first, live HAL envelope is observed read-only, then the closed-source file is audited across all 12288 rows',
)

# After v0.17 has already sealed the source, observed the live envelope, produced the auxiliary DNG
# and written the first evidence JSON, close Image + app camera objects BEFORE reading the saved RAW
# file back. This keeps Stage 3.6 strictly downstream from acquisition and source sealing.
needle = '''            capturedDng = dng\n            capturedJson = report\n            image.close()\n\n            setStatusAny(\n'''
replacement = '''            capturedDng = dng\n            capturedJson = report\n            image.close()\n\n            // Stage 3.6 must operate on the sealed FILE, not Image/HardwareBuffer. Request closure\n            // of the app-side camera objects first so the audit cannot participate in acquisition.\n            runCatching { session?.close() }\n            session = null\n            runCatching { rawReader?.close() }\n            rawReader = null\n            runCatching { camera?.close() }\n            camera = null\n\n            setStatusAny(\n                "STAGE 3 RAW SEALED · source + HAL envelope captured.\\n" +\n                    "STAGE 3.6 FULL RASTER WRITE AUDIT bezig · sealed file read-only · 12,288 rijen…",\n            )\n\n            val rasterAuditAttempt = runCatching {\n                RawSensorRasterAudit.audit(\n                    file = rawEvidence.file,\n                    width = TARGET_W,\n                    height = TARGET_H,\n                    pixelBytes = 2,\n                    expectedSealedSha256 = rawEvidence.sha256,\n                )\n            }\n            val rasterAudit = rasterAuditAttempt.getOrNull()\n            val rasterAuditError = rasterAuditAttempt.exceptionOrNull()?.let {\n                "${it.javaClass.simpleName}: ${it.message}"\n            }\n            val rasterClassification = rasterAudit?.optString("classification", "UNCLASSIFIED")\n                ?: "AUDIT_UNAVAILABLE"\n\n            // Enrich only the JSON sidecar. Never reopen the source file for writing.\n            runCatching {\n                val enriched = JSONObject(report.readText())\n                    .put("stage36FullRasterWriteAudit", rasterAudit ?: JSONObject.NULL)\n                    .put("stage36FullRasterWriteAuditError", rasterAuditError ?: JSONObject.NULL)\n                    .put("stage36AuditReadsSealedFileOnly", true)\n                    .put("stage36ImageClosedBeforeAudit", true)\n                    .put("stage36AppCameraObjectsClosedBeforeAudit", true)\n                    .put("stage36AuditModifiedSource", false)\n                    .put("stage36AuditSemanticPromotionAllowed", false)\n                report.writeText(enriched.toString(2))\n            }.onFailure { e ->\n                // The sealed RAW remains valid even if the auxiliary JSON enrichment fails.\n                setStatusAny("STAGE 3.6 JSON enrichment FAIL · ${e.javaClass.simpleName}: ${e.message}")\n            }\n\n            setStatusAny(\n'''
if needle not in s:
    raise SystemExit('v0.17 post-image-close insertion point not found')
s = s.replace(needle, replacement, 1)

# Append Stage 3.6 classification to the existing v0.17 success status.
status_needle = '''                    "Stage 3.5 post-HAL envelope=${if (halEnvelope != null) "captured" else "unavailable"} · source already sealed",\n'''
status_replacement = '''                    "Stage 3.5 post-HAL envelope=${if (halEnvelope != null) "captured" else "unavailable"} · source already sealed\\n" +\n                    "Stage 3.6 raster audit=$rasterClassification · sealedShaIdentity=" +\n                    (rasterAudit?.optBoolean("sealedSha256IdentityPass", false) ?: false),\n'''
if status_needle not in s:
    raise SystemExit('v0.17 success status marker not found')
s = s.replace(status_needle, status_replacement, 1)

# Evidence semantics remain conservative. Stage 3.6 answers payload population/repetition only.
assert 'TruthRaw · 200MP Tele Test v0.19 · Camera-5 full-raster audit' in s
assert 'RawSensorRasterAudit.audit(' in s
assert s.index('persistOriginalRawBuffer(image, stamp)') < s.index('Camera2EnvelopeProbe.observe(image, physical, physicalResult)')
assert s.index('Camera2EnvelopeProbe.observe(image, physical, physicalResult)') < s.index('image.close()')
assert s.index('image.close()') < s.index('RawSensorRasterAudit.audit(')
assert 'stage36AuditReadsSealedFileOnly' in s
assert 'stage36AuditModifiedSource' in s
assert 'MAXIMUM_RESOLUTION vereist' not in s
assert 'APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF' in s

p.write_text(s)
print('patched', p)
print('bytes', p.stat().st_size)
