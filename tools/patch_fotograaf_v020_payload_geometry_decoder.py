#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

# Reconstruct exact v0.19 first. v0.20 is strictly downstream of the sealed-file full-raster audit.
ns = {}
exec(
    compile(
        Path('tools/patch_fotograaf_v019_full_raster_write_audit.py').read_text(),
        'patch_fotograaf_v019_full_raster_write_audit.py',
        'exec',
    ),
    ns,
    ns,
)
s = p.read_text()

# Proven acquisition and airlock remain unchanged. Only downstream interpretation/provenance is added.
s = s.replace('v0.19 sealed full-raster write audit', 'v0.20 payload geometry decoder')
s = s.replace('v0.19 · Camera-5 full-raster audit', 'v0.20 · Camera-5 payload geometry decoder')
s = s.replace('v0.19 full-raster-audit UI', 'v0.20 payload-geometry UI')
s = s.replace('truthraw-200mp-v019', 'truthraw-200mp-v020')
s = s.replace('truthraw-v019-capability', 'truthraw-v020-capability')
s = s.replace('_v019.dng', '_v020.dng')
s = s.replace('_EVIDENCE_v019.json', '_EVIDENCE_v020.json')
s = s.replace('_v019.${if (contiguous)', '_v020.${if (contiguous)')
s = s.replace('staged-evidence.v0.19', 'staged-evidence.v0.20')
s = s.replace(
    'physical-5-scoped MAX request; exact RAW bytes are sealed first, live HAL envelope is observed read-only, then the closed-source file is audited across all 12288 rows',
    'physical-5-scoped MAX request; exact RAW bytes are sealed first, HAL envelope and full raster are audited, then any populated prefix is matched read-only against advertised standard RAW geometries',
)

# UI/state for derived candidate payload + appearance-only geometry preview.
field_needle = '''    private lateinit var saveRawButton: Button\n    private lateinit var saveDngButton: Button\n    private lateinit var saveJsonButton: Button\n'''
field_replacement = '''    private lateinit var saveRawButton: Button\n    private lateinit var saveDngButton: Button\n    private lateinit var saveJsonButton: Button\n    private lateinit var saveGeometryPayloadButton: Button\n    private lateinit var saveGeometryPreviewButton: Button\n'''
if field_needle not in s:
    raise SystemExit('v0.20 button field anchor not found')
s = s.replace(field_needle, field_replacement, 1)

state_needle = '''    private var capturedRaw: File? = null\n    private var capturedDng: File? = null\n    private var capturedJson: File? = null\n'''
state_replacement = '''    private var capturedRaw: File? = null\n    private var capturedDng: File? = null\n    private var capturedJson: File? = null\n    private var capturedGeometryPayload: File? = null\n    private var capturedGeometryPreview: File? = null\n'''
if state_needle not in s:
    raise SystemExit('v0.20 state anchor not found')
s = s.replace(state_needle, state_replacement, 1)

ui_needle = '''        saveRawButton = button("Originele 200MP RAW buffer opslaan") { saveFile(capturedRaw, "application/octet-stream", REQUEST_SAVE_RAW) }.apply { isEnabled = false }\n        saveDngButton = button("Auxiliary 200MP DNG opslaan") { saveFile(capturedDng, "image/x-adobe-dng", REQUEST_SAVE_DNG) }.apply { isEnabled = false }\n        saveJsonButton = button("200MP evidence JSON opslaan") { saveFile(capturedJson, "application/json", REQUEST_SAVE_JSON) }.apply { isEnabled = false }\n'''
ui_replacement = '''        saveRawButton = button("Originele 200MP RAW buffer opslaan") { saveFile(capturedRaw, "application/octet-stream", REQUEST_SAVE_RAW) }.apply { isEnabled = false }\n        saveDngButton = button("Auxiliary 200MP DNG opslaan") { saveFile(capturedDng, "image/x-adobe-dng", REQUEST_SAVE_DNG) }.apply { isEnabled = false }\n        saveJsonButton = button("200MP evidence JSON opslaan") { saveFile(capturedJson, "application/json", REQUEST_SAVE_JSON) }.apply { isEnabled = false }\n        saveGeometryPayloadButton = button("Stage 3.7 · exact payload-prefix opslaan") { saveFile(capturedGeometryPayload, "application/octet-stream", REQUEST_SAVE_GEOMETRY_PAYLOAD) }.apply { isEnabled = false }\n        saveGeometryPreviewButton = button("Stage 3.7 · geometry diagnostic PNG opslaan") { saveFile(capturedGeometryPreview, "image/png", REQUEST_SAVE_GEOMETRY_PREVIEW) }.apply { isEnabled = false }\n'''
if ui_needle not in s:
    raise SystemExit('v0.20 UI creation anchor not found')
s = s.replace(ui_needle, ui_replacement, 1)

add_needle = '''        root.addView(saveRawButton)\n        root.addView(saveDngButton)\n        root.addView(saveJsonButton)\n'''
add_replacement = '''        root.addView(saveRawButton)\n        root.addView(saveDngButton)\n        root.addView(saveJsonButton)\n        root.addView(saveGeometryPayloadButton)\n        root.addView(saveGeometryPreviewButton)\n'''
if add_needle not in s:
    raise SystemExit('v0.20 UI addView anchor not found')
s = s.replace(add_needle, add_replacement, 1)

# Stage 3.7: only after Stage 3.6 has completed against the closed, sealed file.
audit_anchor = '''            val rasterClassification = rasterAudit?.optString("classification", "UNCLASSIFIED")\n                ?: "AUDIT_UNAVAILABLE"\n\n            // Enrich only the JSON sidecar. Never reopen the source file for writing.\n'''
geometry_block = '''            val rasterClassification = rasterAudit?.optString("classification", "UNCLASSIFIED")\n                ?: "AUDIT_UNAVAILABLE"\n\n            setStatusAny(\n                "STAGE 3.6 raster audit=$rasterClassification\\n" +\n                    "STAGE 3.7 PAYLOAD GEOMETRY DECODER bezig · sealed source read-only…",\n            )\n            val standardRawSizes = physical.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)\n                ?.getOutputSizes(ImageFormat.RAW_SENSOR)?.toList().orEmpty()\n            val geometryPayloadCandidate = File(\n                cacheDir,\n                "TRUTHRAW_${stamp}_CAM5_PAYLOAD_STANDARD_RAW_BYTE_MATCH_v020.rawpayload",\n            )\n            val geometryPreviewCandidate = File(\n                cacheDir,\n                "TRUTHRAW_${stamp}_CAM5_PAYLOAD_GEOMETRY_DIAGNOSTIC_v020.png",\n            )\n            val geometryAttempt = if (rasterAudit != null) runCatching {\n                RawPayloadGeometryDecoder.decode(\n                    source = rawEvidence.file,\n                    rasterAudit = rasterAudit,\n                    declaredWidth = TARGET_W,\n                    declaredHeight = TARGET_H,\n                    advertisedStandardRawSizes = standardRawSizes,\n                    candidatePayloadFile = geometryPayloadCandidate,\n                    diagnosticPreviewFile = geometryPreviewCandidate,\n                )\n            } else null\n            val geometryReport = geometryAttempt?.getOrNull()\n            val geometryError = geometryAttempt?.exceptionOrNull()?.let {\n                "${it.javaClass.simpleName}: ${it.message}"\n            } ?: if (rasterAudit == null) "Stage 3.6 unavailable; Stage 3.7 not run" else null\n            val geometryClassification = geometryReport?.optString("classification", "UNCLASSIFIED")\n                ?: "GEOMETRY_DECODER_UNAVAILABLE"\n            capturedGeometryPayload = geometryPayloadCandidate.takeIf { it.exists() && it.length() > 0L }\n            capturedGeometryPreview = geometryPreviewCandidate.takeIf { it.exists() && it.length() > 0L }\n\n            // Enrich only the JSON sidecar. Never reopen the source file for writing.\n'''
if audit_anchor not in s:
    raise SystemExit('v0.20 Stage 3.6 anchor not found')
s = s.replace(audit_anchor, geometry_block, 1)

json_needle = '''                    .put("stage36AuditModifiedSource", false)\n                    .put("stage36AuditSemanticPromotionAllowed", false)\n'''
json_replacement = '''                    .put("stage36AuditModifiedSource", false)\n                    .put("stage36AuditSemanticPromotionAllowed", false)\n                    .put("stage37PayloadGeometryDecoder", geometryReport ?: JSONObject.NULL)\n                    .put("stage37PayloadGeometryDecoderError", geometryError ?: JSONObject.NULL)\n                    .put("stage37ReadsSealedFileOnly", true)\n                    .put("stage37SourceModified", false)\n                    .put("stage37CandidatePayloadIsExactPrefixCopy", capturedGeometryPayload != null)\n                    .put("stage37DiagnosticPreviewAppearanceOnly", capturedGeometryPreview != null)\n                    .put("stage37GeometrySemanticPromotionAllowed", false)\n'''
if json_needle not in s:
    raise SystemExit('v0.20 JSON enrichment anchor not found')
s = s.replace(json_needle, json_replacement, 1)

# Extend success status with geometry result.
status_needle = '''                    "Stage 3.6 raster audit=$rasterClassification · sealedShaIdentity=" +\n                    (rasterAudit?.optBoolean("sealedSha256IdentityPass", false) ?: false),\n'''
status_replacement = '''                    "Stage 3.6 raster audit=$rasterClassification · sealedShaIdentity=" +\n                    (rasterAudit?.optBoolean("sealedSha256IdentityPass", false) ?: false) + "\\n" +\n                    "Stage 3.7 geometry=$geometryClassification · exactPrefix=" +\n                    (capturedGeometryPayload?.length() ?: 0L) + " bytes",\n'''
if status_needle not in s:
    raise SystemExit('v0.20 final status anchor not found')
s = s.replace(status_needle, status_replacement, 1)

# Export buttons become available only if derived files were actually created.
enable_needle = '''                saveDngButton.isEnabled = capturedDng != null\n                saveJsonButton.isEnabled = true\n                previewButton.isEnabled = true\n'''
enable_replacement = '''                saveDngButton.isEnabled = capturedDng != null\n                saveJsonButton.isEnabled = true\n                saveGeometryPayloadButton.isEnabled = capturedGeometryPayload?.exists() == true\n                saveGeometryPreviewButton.isEnabled = capturedGeometryPreview?.exists() == true\n                previewButton.isEnabled = true\n'''
if enable_needle not in s:
    raise SystemExit('v0.20 success button anchor not found')
s = s.replace(enable_needle, enable_replacement, 1)

# Fail-closed path also preserves already-created downstream diagnostics if a later auxiliary step fails.
fail_enable_needle = '''                saveDngButton.isEnabled = capturedDng?.exists() == true\n                saveJsonButton.isEnabled = capturedJson?.exists() == true\n                previewButton.isEnabled = true\n'''
fail_enable_replacement = '''                saveDngButton.isEnabled = capturedDng?.exists() == true\n                saveJsonButton.isEnabled = capturedJson?.exists() == true\n                saveGeometryPayloadButton.isEnabled = capturedGeometryPayload?.exists() == true\n                saveGeometryPreviewButton.isEnabled = capturedGeometryPreview?.exists() == true\n                previewButton.isEnabled = true\n'''
if fail_enable_needle not in s:
    raise SystemExit('v0.20 fail button anchor not found')
s = s.replace(fail_enable_needle, fail_enable_replacement, 1)

clear_needle = '''        capturedRaw = null\n        capturedDng = null\n        capturedJson = null\n        if (::saveRawButton.isInitialized) saveRawButton.isEnabled = false\n        if (::saveDngButton.isInitialized) saveDngButton.isEnabled = false\n        if (::saveJsonButton.isInitialized) saveJsonButton.isEnabled = false\n'''
clear_replacement = '''        capturedRaw = null\n        capturedDng = null\n        capturedJson = null\n        capturedGeometryPayload = null\n        capturedGeometryPreview = null\n        if (::saveRawButton.isInitialized) saveRawButton.isEnabled = false\n        if (::saveDngButton.isInitialized) saveDngButton.isEnabled = false\n        if (::saveJsonButton.isInitialized) saveJsonButton.isEnabled = false\n        if (::saveGeometryPayloadButton.isInitialized) saveGeometryPayloadButton.isEnabled = false\n        if (::saveGeometryPreviewButton.isInitialized) saveGeometryPreviewButton.isEnabled = false\n'''
if clear_needle not in s:
    raise SystemExit('v0.20 clearOutputs anchor not found')
s = s.replace(clear_needle, clear_replacement, 1)

result_needle = '''            REQUEST_SAVE_RAW -> capturedRaw\n            REQUEST_SAVE_DNG -> capturedDng\n            REQUEST_SAVE_JSON -> capturedJson\n            else -> null\n'''
result_replacement = '''            REQUEST_SAVE_RAW -> capturedRaw\n            REQUEST_SAVE_DNG -> capturedDng\n            REQUEST_SAVE_JSON -> capturedJson\n            REQUEST_SAVE_GEOMETRY_PAYLOAD -> capturedGeometryPayload\n            REQUEST_SAVE_GEOMETRY_PREVIEW -> capturedGeometryPreview\n            else -> null\n'''
if result_needle not in s:
    raise SystemExit('v0.20 activity result anchor not found')
s = s.replace(result_needle, result_replacement, 1)

const_needle = '''        private const val REQUEST_SAVE_RAW = 5801\n        private const val REQUEST_SAVE_DNG = 5802\n        private const val REQUEST_SAVE_JSON = 5803\n'''
const_replacement = '''        private const val REQUEST_SAVE_RAW = 5801\n        private const val REQUEST_SAVE_DNG = 5802\n        private const val REQUEST_SAVE_JSON = 5803\n        private const val REQUEST_SAVE_GEOMETRY_PAYLOAD = 5804\n        private const val REQUEST_SAVE_GEOMETRY_PREVIEW = 5805\n'''
if const_needle not in s:
    raise SystemExit('v0.20 constants anchor not found')
s = s.replace(const_needle, const_replacement, 1)

assert 'TruthRaw · 200MP Tele Test v0.20 · Camera-5 payload geometry decoder' in s
assert 'RawPayloadGeometryDecoder.decode(' in s
assert s.index('RawSensorRasterAudit.audit(') < s.index('RawPayloadGeometryDecoder.decode(')
assert s.index('image.close()') < s.index('RawSensorRasterAudit.audit(')
assert 'stage37CandidatePayloadIsExactPrefixCopy' in s
assert 'stage37GeometrySemanticPromotionAllowed' in s
assert 'REQUEST_SAVE_GEOMETRY_PAYLOAD' in s
assert 'MAXIMUM_RESOLUTION vereist' not in s

p.write_text(s)
print('patched', p)
print('bytes', p.stat().st_size)
