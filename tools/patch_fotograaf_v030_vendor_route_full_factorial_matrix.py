#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

# Reconstruct the exact v0.20 source/payload authority first. v0.30 changes only the controlled
# pre-session vendor-factor profile selected for each capture; source sealing and Stage 3.6/3.7
# remain downstream and unchanged.
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
    'TruthRaw · 200MP Tele Test v0.30 · Camera-5 vendor route full-factorial matrix',
)
s = s.replace('v0.20 payload geometry decoder', 'v0.30 vendor route full-factorial matrix')
s = s.replace('v0.20 payload-geometry UI', 'v0.30 full-factorial matrix UI')
s = s.replace('truthraw-200mp-v020', 'truthraw-200mp-v030')
s = s.replace('truthraw-v020-capability', 'truthraw-v030-capability')
s = s.replace('_v020.dng', '_v030_${activeMatrixProfileId()}.dng')
s = s.replace('_EVIDENCE_v020.json', '_${activeMatrixProfileId()}_EVIDENCE_v030.json')
s = s.replace('_v020.${if (contiguous)', '_v030_${activeMatrixProfileId()}.${if (contiguous)')
s = s.replace('_v020.rawpayload', '_v030_${activeMatrixProfileId()}.rawpayload')
s = s.replace('_v020.png', '_v030_${activeMatrixProfileId()}.png')
s = s.replace('staged-evidence.v0.20', 'staged-evidence.v0.30')
s = s.replace(
    'physical-5-scoped MAX request; exact RAW bytes are sealed first, HAL envelope and full raster are audited, then any populated prefix is matched read-only against advertised standard RAW geometries',
    '16-run complement-paired 2^4 vendor-route matrix; low=UNSET, high=type-validated numeric 1; every RAW is independently sealed before unchanged Stage 3.6/3.7 topology audit',
)

# UI profile selector.
button_field_needle = '''    private lateinit var previewButton: Button\n    private lateinit var captureButton: Button\n    private lateinit var saveRawButton: Button\n'''
button_field_replacement = '''    private lateinit var previewButton: Button\n    private lateinit var captureButton: Button\n    private lateinit var matrixProfileButton: Button\n    private lateinit var saveRawButton: Button\n'''
if button_field_needle not in s:
    raise SystemExit('v0.30 matrix button field anchor not found')
s = s.replace(button_field_needle, button_field_replacement, 1)

state_needle = '''    private var lastPreHalSessionGate: JSONObject? = null\n    private var lastPreHalRequestGate: JSONObject? = null\n\n    private var capturedRaw: File? = null\n'''
state_replacement = '''    private var lastPreHalSessionGate: JSONObject? = null\n    private var lastPreHalRequestGate: JSONObject? = null\n    private var lastVendorRouteMatrixIntervention: JSONObject? = null\n    private var matrixRunIndex: Int = 0\n    private var activeMatrixRunIndex: Int = 0\n\n    private var capturedRaw: File? = null\n'''
if state_needle not in s:
    raise SystemExit('v0.30 matrix state anchor not found')
s = s.replace(state_needle, state_replacement, 1)

ui_create_needle = '''        previewButton = button("Stap 2 · start live beeld via logical 0 · 3.7×") { startLogicalPreview() }.apply { isEnabled = false }\n        captureButton = button("Stap 3 · PHYSICAL-SCOPED CAPTURE · 16320×12288") { capture200Mp() }.apply { isEnabled = false }\n        saveRawButton = button("Originele 200MP RAW buffer opslaan") { saveFile(capturedRaw, "application/octet-stream", REQUEST_SAVE_RAW) }.apply { isEnabled = false }\n'''
ui_create_replacement = '''        previewButton = button("Stap 2 · start live beeld via logical 0 · 3.7×") { startLogicalPreview() }.apply { isEnabled = false }\n        captureButton = button("Stap 3 · MATRIX CAPTURE · PHYSICAL 5 · 16320×12288") { capture200Mp() }.apply { isEnabled = false }\n        matrixProfileButton = button(matrixProfileLabel(matrixRunIndex)) {\n            matrixRunIndex = (matrixRunIndex + 1) % Camera2VendorRouteFullFactorialMatrix.TOTAL_RUNS\n            matrixProfileButton.text = matrixProfileLabel(matrixRunIndex)\n            val p = Camera2VendorRouteFullFactorialMatrix.profileForRun(matrixRunIndex)\n            setStatus("Matrix handmatig geselecteerd: run ${p.runIndex + 1}/16 · ABCD=${p.bits} · ${p.id}.\\nLow=UNSET; high=numeric 1 met bewezen native type. Geen vendorsemantiek aangenomen.")\n        }\n        saveRawButton = button("Originele 200MP RAW buffer opslaan") { saveFile(capturedRaw, "application/octet-stream", REQUEST_SAVE_RAW) }.apply { isEnabled = false }\n'''
if ui_create_needle not in s:
    raise SystemExit('v0.30 matrix UI creation anchor not found')
s = s.replace(ui_create_needle, ui_create_replacement, 1)

ui_add_needle = '''        root.addView(capabilityButton)\n        root.addView(previewButton)\n        root.addView(captureButton)\n        root.addView(saveRawButton)\n'''
ui_add_replacement = '''        root.addView(capabilityButton)\n        root.addView(previewButton)\n        root.addView(matrixProfileButton)\n        root.addView(captureButton)\n        root.addView(saveRawButton)\n'''
if ui_add_needle not in s:
    raise SystemExit('v0.30 matrix UI addView anchor not found')
s = s.replace(ui_add_needle, ui_add_replacement, 1)

# Small helpers keep run/profile identity deterministic and visible in every output filename.
method_anchor = '''    private fun readCapability() {\n'''
method_block = '''    private fun matrixProfileLabel(runIndex: Int): String {\n        val p = Camera2VendorRouteFullFactorialMatrix.profileForRun(runIndex)\n        return "Matrix run ${p.runIndex + 1}/16 · ABCD=${p.bits} · ${p.id} · tik = volgende"\n    }\n\n    private fun activeMatrixProfileId(): String =\n        Camera2VendorRouteFullFactorialMatrix.profileForRun(activeMatrixRunIndex).id\n\n    private fun readCapability() {\n'''
if method_anchor not in s:
    raise SystemExit('v0.30 helper-method anchor not found')
s = s.replace(method_anchor, method_block, 1)

# Freeze the selected matrix run when capture starts. UI changes after this point cannot alter the
# active capture profile.
capture_start_needle = '''        captureButton.isEnabled = false\n        previewButton.isEnabled = false\n        clearOutputs()\n'''
capture_start_replacement = '''        captureButton.isEnabled = false\n        previewButton.isEnabled = false\n        activeMatrixRunIndex = matrixRunIndex\n        lastVendorRouteMatrixIntervention = null\n        clearOutputs()\n'''
if capture_start_needle not in s:
    raise SystemExit('v0.30 capture-start anchor not found')
s = s.replace(capture_start_needle, capture_start_replacement, 1)

# Gate A from v0.17 has already frozen the untouched app-visible session surface. Apply exactly the
# selected full-factorial profile after Gate A and before session support/configuration.
support_needle = '''        val support = runCatching { device.isSessionConfigurationSupported(config) }.getOrNull()\n'''
matrix_block = '''        val matrixProfile = Camera2VendorRouteFullFactorialMatrix.profileForRun(activeMatrixRunIndex)\n        val matrixAttempt = Camera2VendorRouteFullFactorialMatrix.applyProfile(\n            device = device,\n            logical = logical,\n            physical = physical,\n            config = config,\n            runIndex = activeMatrixRunIndex,\n        )\n        lastVendorRouteMatrixIntervention = matrixAttempt.evidence\n        if (!matrixAttempt.applied) {\n            setStatus(\n                "STAGE 3 BLOCKED · v0.30 matrixprofiel ${matrixProfile.id} niet veilig toepasbaar.\\n" +\n                    "classification=${matrixAttempt.evidence.optString(\"classification\", \"UNKNOWN\")}\\n" +\n                    "Geen capture ingediend; dezelfde matrixrun blijft geselecteerd.",\n            )\n            closeCameraResources(keepOutputs = true)\n            previewButton.isEnabled = true\n            return\n        }\n\n        val support = runCatching { device.isSessionConfigurationSupported(config) }.getOrNull()\n'''
if support_needle not in s:
    raise SystemExit('v0.30 matrix insertion anchor not found')
s = s.replace(support_needle, matrix_block, 1)

# Add designed-intervention provenance to the evidence sidecar. Gate A itself remains observation-only.
report_needle = '''                    .put("preHalGateChangedVendorKeys", false)\n                    .put("preHalGatePixelAccess", false)\n                    .put("halBufferEnvelope", halEnvelope ?: JSONObject.NULL)\n'''
report_replacement = '''                    .put("preHalGateChangedVendorKeys", false)\n                    .put("preHalGatePixelAccess", false)\n                    .put("controlledVendorInterventionAfterGateA", true)\n                    .put("controlledVendorInterventionDesign", "FULL_FACTORIAL_2_LEVEL_4_FACTOR_16_RUN_COMPLEMENT_PAIRED")\n                    .put("controlledVendorInterventionLowLevel", "UNSET_NO_WRITE")\n                    .put("controlledVendorInterventionHighLevel", "NUMERIC_ONE_TYPE_VALIDATED__SEMANTICS_UNPROVEN")\n                    .put("controlledVendorInterventionRunIndex", activeMatrixRunIndex)\n                    .put("controlledVendorInterventionProfileId", activeMatrixProfileId())\n                    .put("controlledVendorInterventionKeyCount", lastVendorRouteMatrixIntervention?.optInt("vendorKeysWritten", 0) ?: 0)\n                    .put("vendorRouteFullFactorialMatrix", lastVendorRouteMatrixIntervention ?: JSONObject.NULL)\n                    .put("vendorRouteMatrixSemanticPromotionAllowed", false)\n                    .put("halBufferEnvelope", halEnvelope ?: JSONObject.NULL)\n'''
if report_needle not in s:
    raise SystemExit('v0.30 evidence insertion anchor not found')
s = s.replace(report_needle, report_replacement, 1)

# Make current run visible in the final status while leaving Stage 3.6/3.7 wording intact.
status_needle = '''                    "Gate A/B pre-HAL route fingerprint=${if (lastPreHalSessionGate != null && lastPreHalRequestGate != null) "captured" else "partial"}\\n" +\n'''
status_replacement = '''                    "Matrix ${activeMatrixProfileId()} · ABCD=${Camera2VendorRouteFullFactorialMatrix.profileForRun(activeMatrixRunIndex).bits} · keysWritten=${lastVendorRouteMatrixIntervention?.optInt("vendorKeysWritten", 0) ?: 0}\\n" +\n                    "Gate A/B pre-HAL route fingerprint=${if (lastPreHalSessionGate != null && lastPreHalRequestGate != null) "captured" else "partial"}\\n" +\n'''
if status_needle not in s:
    raise SystemExit('v0.30 final status anchor not found')
s = s.replace(status_needle, status_replacement, 1)

# After a successful sealed/audited capture, move to the next complement-paired run automatically.
# Saved output references still point at the completed run, so the user can export them before Step 2.
success_needle = '''                saveGeometryPayloadButton.isEnabled = capturedGeometryPayload?.exists() == true\n                saveGeometryPreviewButton.isEnabled = capturedGeometryPreview?.exists() == true\n                previewButton.isEnabled = true\n'''
success_replacement = '''                saveGeometryPayloadButton.isEnabled = capturedGeometryPayload?.exists() == true\n                saveGeometryPreviewButton.isEnabled = capturedGeometryPreview?.exists() == true\n                matrixRunIndex = (activeMatrixRunIndex + 1) % Camera2VendorRouteFullFactorialMatrix.TOTAL_RUNS\n                matrixProfileButton.text = matrixProfileLabel(matrixRunIndex)\n                previewButton.isEnabled = true\n'''
if success_needle not in s:
    raise SystemExit('v0.30 success advance anchor not found')
s = s.replace(success_needle, success_replacement, 1)

# Fail-closed captures do not advance the matrix profile.
fail_needle = '''                saveGeometryPayloadButton.isEnabled = capturedGeometryPayload?.exists() == true\n                saveGeometryPreviewButton.isEnabled = capturedGeometryPreview?.exists() == true\n                previewButton.isEnabled = true\n'''
# The first identical block was already replaced above; the remaining occurrence is the failure path.
if fail_needle not in s:
    raise SystemExit('v0.30 failure-path anchor not found')
fail_replacement = '''                saveGeometryPayloadButton.isEnabled = capturedGeometryPayload?.exists() == true\n                saveGeometryPreviewButton.isEnabled = capturedGeometryPreview?.exists() == true\n                matrixRunIndex = activeMatrixRunIndex\n                matrixProfileButton.text = matrixProfileLabel(matrixRunIndex)\n                previewButton.isEnabled = true\n'''
s = s.replace(fail_needle, fail_replacement, 1)

assert 'TruthRaw · 200MP Tele Test v0.30 · Camera-5 vendor route full-factorial matrix' in s
assert 'Camera2VendorRouteFullFactorialMatrix.applyProfile(' in s
assert s.index('Camera2PreHalGate.observeSession(') < s.index('Camera2VendorRouteFullFactorialMatrix.applyProfile(')
assert s.index('Camera2VendorRouteFullFactorialMatrix.applyProfile(') < s.index('device.isSessionConfigurationSupported(config)')
assert s.index('persistOriginalRawBuffer(image, stamp)') < s.index('Camera2EnvelopeProbe.observe(image, physical, physicalResult)')
assert s.index('RawSensorRasterAudit.audit(') < s.index('RawPayloadGeometryDecoder.decode(')
assert s.count('Camera2VendorRouteFullFactorialMatrix.applyProfile(') == 1
assert 'FULL_FACTORIAL_2_LEVEL_4_FACTOR_16_RUN_COMPLEMENT_PAIRED' in s
assert 'UNSET_NO_WRITE' in s
assert 'vendorRouteMatrixSemanticPromotionAllowed' in s
assert 'MAXIMUM_RESOLUTION vereist' not in s

p.write_text(s)
print('patched', p)
print('bytes', p.stat().st_size)
