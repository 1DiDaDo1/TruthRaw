#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

# Reconstruct v0.17 first. This preserves the proven v0.14 acquisition topology,
# the v0.16 post-HAL envelope, and the v0.17 pre-HAL airlock.
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

# v0.18 is an opt-in physical-focus request probe. AUTO is the default and intentionally leaves
# the proven capture request topology unchanged. Manual modes attempt physical-camera scoped AF OFF
# + LENS_FOCUS_DISTANCE and record both the write outcome and the returned physical/vendor focus state.
s = s.replace('v0.17 pre/post HAL airlock', 'v0.18 physical focus probe')
s = s.replace('v0.17 · Camera-5 airlock', 'v0.18 · Camera-5 physical focus probe')
s = s.replace('v0.17 airlock UI', 'v0.18 focus-probe UI')
s = s.replace('truthraw-200mp-v017', 'truthraw-200mp-v018')
s = s.replace('truthraw-v017-capability', 'truthraw-v018-capability')
s = s.replace('_v017.dng', '_v018.dng')
s = s.replace('_EVIDENCE_v017.json', '_EVIDENCE_v018.json')
s = s.replace('_v017.${if (contiguous)', '_v018.${if (contiguous)')
s = s.replace('staged-evidence.v0.17', 'staged-evidence.v0.18')

# UI/state fields.
field_needle = '''    private lateinit var captureButton: Button\n    private lateinit var saveRawButton: Button\n'''
field_replacement = '''    private lateinit var captureButton: Button\n    private lateinit var focusProbeButton: Button\n    private lateinit var saveRawButton: Button\n'''
if field_needle not in s:
    raise SystemExit('focus button field insertion point not found')
s = s.replace(field_needle, field_replacement, 1)

state_needle = '''    private var lastPreHalSessionGate: JSONObject? = null\n    private var lastPreHalRequestGate: JSONObject? = null\n\n    private var capturedRaw: File? = null\n'''
state_replacement = '''    private var lastPreHalSessionGate: JSONObject? = null\n    private var lastPreHalRequestGate: JSONObject? = null\n\n    // Focus probe mode is UI intent for the NEXT capture. Snapshot fields below are frozen when\n    // Stage 3 builds the request so later UI changes cannot rewrite capture provenance.\n    private var focusProbeModeIndex = 0\n    private var lastFocusProbeMode = "AUTO_DEFAULT"\n    private var lastFocusProbeMinFocusDistanceDiopters: Float? = null\n    private var lastFocusProbeRequestedDiopters: Float? = null\n    private var lastFocusProbePhysicalOverrideAdvertised = false\n    private var lastFocusProbePhysicalAfOffWritten = false\n    private var lastFocusProbePhysicalDistanceWritten = false\n    private var lastFocusProbePhysicalAfError: String? = null\n    private var lastFocusProbePhysicalDistanceError: String? = null\n\n    private var capturedRaw: File? = null\n'''
if state_needle not in s:
    raise SystemExit('focus state insertion point not found')
s = s.replace(state_needle, state_replacement, 1)

# UI button: selector only, no camera call. AUTO remains default.
ui_needle = '''        captureButton = button("Stap 3 · CAPTURE + SEAL 200MP RAW · 16320×12288") { capture200Mp() }.apply { isEnabled = false }\n        saveRawButton = button("Originele 200MP RAW buffer opslaan") { saveFile(capturedRaw, "application/octet-stream", REQUEST_SAVE_RAW) }.apply { isEnabled = false }\n'''
ui_replacement = '''        captureButton = button("Stap 3 · CAPTURE + SEAL 200MP RAW · 16320×12288") { capture200Mp() }.apply { isEnabled = false }\n        focusProbeButton = button("Focus probe · AUTO/default") { cycleFocusProbeMode() }.apply { isEnabled = false }\n        saveRawButton = button("Originele 200MP RAW buffer opslaan") { saveFile(capturedRaw, "application/octet-stream", REQUEST_SAVE_RAW) }.apply { isEnabled = false }\n'''
if ui_needle not in s:
    raise SystemExit('focus UI creation insertion point not found')
s = s.replace(ui_needle, ui_replacement, 1)

add_needle = '''        root.addView(previewButton)\n        root.addView(captureButton)\n        root.addView(saveRawButton)\n'''
add_replacement = '''        root.addView(previewButton)\n        root.addView(focusProbeButton)\n        root.addView(captureButton)\n        root.addView(saveRawButton)\n'''
if add_needle not in s:
    raise SystemExit('focus UI addView insertion point not found')
s = s.replace(add_needle, add_replacement, 1)

# Enable selector only after physical Camera-5 characteristics were actually discovered.
enable_needle = '''                    previewButton.isEnabled = preview.isAvailable\n                    val r = packed.second\n'''
enable_replacement = '''                    previewButton.isEnabled = preview.isAvailable\n                    focusProbeButton.isEnabled = true\n                    updateFocusProbeButton()\n                    val r = packed.second\n'''
if enable_needle not in s:
    raise SystemExit('focus selector enable point not found')
s = s.replace(enable_needle, enable_replacement, 1)

# Helper methods before startLogicalPreview().
helper_needle = '''    private fun startLogicalPreview() {\n'''
helper_replacement = '''    private fun focusProbeModeName(index: Int = focusProbeModeIndex): String = when (index) {\n        0 -> "AUTO_DEFAULT"\n        1 -> "INFINITY_0D"\n        2 -> "MID_50PCT_MIN_FOCUS"\n        else -> "NEAR_85PCT_MIN_FOCUS"\n    }\n\n    private fun focusProbeRequestedDistance(physical: CameraCharacteristics, index: Int = focusProbeModeIndex): Float? {\n        val minFocus = physical.get(CameraCharacteristics.LENS_INFO_MINIMUM_FOCUS_DISTANCE)\n        return when (index) {\n            0 -> null\n            1 -> 0.0f\n            2 -> minFocus?.takeIf { it > 0.0f }?.times(0.50f)\n            else -> minFocus?.takeIf { it > 0.0f }?.times(0.85f)\n        }\n    }\n\n    private fun updateFocusProbeButton() {\n        if (!::focusProbeButton.isInitialized) return\n        val physical = physical5Characteristics\n        val minFocus = physical?.get(CameraCharacteristics.LENS_INFO_MINIMUM_FOCUS_DISTANCE)\n        val requested = physical?.let { focusProbeRequestedDistance(it) }\n        focusProbeButton.text = when (focusProbeModeIndex) {\n            0 -> "Focus probe · AUTO/default (baseline)"\n            1 -> "Focus probe · INFINITY · 0.000 D"\n            2 -> "Focus probe · MID · ${requested?.let { String.format(Locale.ROOT, "%.4f D", it) } ?: "unsupported"}"\n            else -> "Focus probe · NEAR · ${requested?.let { String.format(Locale.ROOT, "%.4f D", it) } ?: "unsupported"}"\n        }\n        focusProbeButton.contentDescription = "${focusProbeModeName()} minFocus=${minFocus ?: "unknown"} requested=${requested ?: "AUTO"}"\n    }\n\n    private fun cycleFocusProbeMode() {\n        focusProbeModeIndex = (focusProbeModeIndex + 1) % 4\n        updateFocusProbeButton()\n        val requested = physical5Characteristics?.let { focusProbeRequestedDistance(it) }\n        setStatus(\n            "FOCUS PROBE SELECTED · ${focusProbeModeName()} · requested=${requested?.let { String.format(Locale.ROOT, "%.6f D", it) } ?: "AUTO/default"}.\\n" +\n                "Geen capture uitgevoerd; deze keuze geldt alleen voor de volgende Stage-3 RAW."\n        )\n    }\n\n    private fun startLogicalPreview() {\n'''
if helper_needle not in s:
    raise SystemExit('focus helper insertion point not found')
s = s.replace(helper_needle, helper_replacement, 1)

# Apply manual focus only to the physical Camera-5 scoped request. No vendor key is touched.
request_needle = '''            setIfSupported(requestBuilder, CaptureRequest.CONTROL_ENABLE_ZSL, false, logical)\n            setIfSupported(requestBuilder, CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO, logical)\n            setIfSupported(requestBuilder, CaptureRequest.CONTROL_AE_MODE, CameraMetadata.CONTROL_AE_MODE_ON, logical)\n'''
request_replacement = '''            // v0.18 focus-probe snapshot. AUTO/default writes nothing and therefore reproduces\n            // v0.17 focus behavior. Manual modes deliberately attempt only standard Camera2 keys\n            // as PHYSICAL_CAMERA_5 overrides; failure is recorded and never hidden by a global fallback.\n            lastFocusProbeMode = focusProbeModeName()\n            lastFocusProbeMinFocusDistanceDiopters = physical.get(CameraCharacteristics.LENS_INFO_MINIMUM_FOCUS_DISTANCE)\n            lastFocusProbeRequestedDiopters = focusProbeRequestedDistance(physical)\n            lastFocusProbePhysicalOverrideAdvertised =\n                logical.availablePhysicalCameraRequestKeys?.contains(CaptureRequest.LENS_FOCUS_DISTANCE) == true\n            lastFocusProbePhysicalAfOffWritten = false\n            lastFocusProbePhysicalDistanceWritten = false\n            lastFocusProbePhysicalAfError = null\n            lastFocusProbePhysicalDistanceError = null\n\n            lastFocusProbeRequestedDiopters?.let { requestedFocus ->\n                runCatching {\n                    requestBuilder.setPhysicalCameraKey(\n                        CaptureRequest.CONTROL_AF_MODE,\n                        CameraMetadata.CONTROL_AF_MODE_OFF,\n                        PHYSICAL_ID,\n                    )\n                    lastFocusProbePhysicalAfOffWritten = true\n                }.onFailure { t ->\n                    lastFocusProbePhysicalAfError = "${t.javaClass.simpleName}: ${t.message}"\n                }\n                runCatching {\n                    requestBuilder.setPhysicalCameraKey(\n                        CaptureRequest.LENS_FOCUS_DISTANCE,\n                        requestedFocus,\n                        PHYSICAL_ID,\n                    )\n                    lastFocusProbePhysicalDistanceWritten = true\n                }.onFailure { t ->\n                    lastFocusProbePhysicalDistanceError = "${t.javaClass.simpleName}: ${t.message}"\n                }\n            }\n\n            setIfSupported(requestBuilder, CaptureRequest.CONTROL_ENABLE_ZSL, false, logical)\n            setIfSupported(requestBuilder, CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO, logical)\n            setIfSupported(requestBuilder, CaptureRequest.CONTROL_AE_MODE, CameraMetadata.CONTROL_AE_MODE_ON, logical)\n'''
if request_needle not in s:
    raise SystemExit('focus request insertion point not found')
s = s.replace(request_needle, request_replacement, 1)

# Include focus write state in pre-submit status.
status_needle = '''                "STAGE 3 CAPTURE SENT · scopedRequest=$lastScopedRequestUsed · globalMAX=$lastGlobalPixelModeWritten · physicalMAX=$lastPhysicalPixelModeWritten" +\n'''
status_replacement = '''                "STAGE 3 CAPTURE SENT · scopedRequest=$lastScopedRequestUsed · globalMAX=$lastGlobalPixelModeWritten · physicalMAX=$lastPhysicalPixelModeWritten" +\n                    " · focusMode=$lastFocusProbeMode · focusWrite=$lastFocusProbePhysicalDistanceWritten" +\n'''
if status_needle not in s:
    raise SystemExit('focus status insertion point not found')
s = s.replace(status_needle, status_replacement, 1)

# Helpers to decode known focus-related vendor observations without assigning undocumented semantics.
result_helper_needle = '''    private fun buildEvidence(\n'''
result_helper_replacement = '''    @Suppress("UNCHECKED_CAST")\n    private fun resultValueByName(result: CaptureResult, name: String): Any? {\n        val key = result.keys.firstOrNull { it.name == name } ?: return null\n        return runCatching { result.get(key as CaptureResult.Key<Any>) }.getOrNull()\n    }\n\n    private fun firstIntLike(value: Any?): Int? = when (value) {\n        is Int -> value\n        is Byte -> value.toInt() and 0xff\n        is Short -> value.toInt()\n        is Long -> value.toInt()\n        is IntArray -> value.firstOrNull()\n        is ByteArray -> value.firstOrNull()?.toInt()?.and(0xff)\n        is ShortArray -> value.firstOrNull()?.toInt()\n        is LongArray -> value.firstOrNull()?.toInt()\n        else -> null\n    }\n\n    private fun buildEvidence(\n'''
if result_helper_needle not in s:
    raise SystemExit('focus result helper insertion point not found')
s = s.replace(result_helper_needle, result_helper_replacement, 1)

# Machine-readable focus provenance. This is deliberately not a proof of optical focus movement until
# two or more differential captures show reproducible request/result/lens-position changes.
report_needle = '''            .put("rawPayload", JSONObject()\n'''
report_replacement = '''            .put("focusProbe", JSONObject()\n                .put("mode", lastFocusProbeMode)\n                .put("manualRequestAttempted", lastFocusProbeRequestedDiopters != null)\n                .put("minimumFocusDistanceDiopters", lastFocusProbeMinFocusDistanceDiopters ?: JSONObject.NULL)\n                .put("requestedFocusDistanceDiopters", lastFocusProbeRequestedDiopters ?: JSONObject.NULL)\n                .put("physicalFocusOverrideAdvertised", lastFocusProbePhysicalOverrideAdvertised)\n                .put("physicalAfOffWritten", lastFocusProbePhysicalAfOffWritten)\n                .put("physicalFocusDistanceWritten", lastFocusProbePhysicalDistanceWritten)\n                .put("physicalAfOffError", lastFocusProbePhysicalAfError ?: JSONObject.NULL)\n                .put("physicalFocusDistanceError", lastFocusProbePhysicalDistanceError ?: JSONObject.NULL)\n                .put("reportedAfMode", physicalResult.get(CaptureResult.CONTROL_AF_MODE) ?: JSONObject.NULL)\n                .put("reportedAfState", physicalResult.get(CaptureResult.CONTROL_AF_STATE) ?: JSONObject.NULL)\n                .put("reportedLensState", physicalResult.get(CaptureResult.LENS_STATE) ?: JSONObject.NULL)\n                .put("reportedFocusDistanceDiopters", physicalResult.get(CaptureResult.LENS_FOCUS_DISTANCE) ?: JSONObject.NULL)\n                .put("reportedFocusRange", physicalResult.get(CaptureResult.LENS_FOCUS_RANGE)?.toString() ?: JSONObject.NULL)\n                .put("vendorLensPosObserved", firstIntLike(resultValueByName(physicalResult, "org.quic.camera.afData.lenspos")) ?: JSONObject.NULL)\n                .put("vendorPdEnableObserved", firstIntLike(resultValueByName(physicalResult, "org.quic.camera.afData.isPDEnable")) ?: JSONObject.NULL)\n                .put("vendorPdTypeObserved", firstIntLike(resultValueByName(physicalResult, "org.quic.camera.afData.PDType")) ?: JSONObject.NULL)\n                .put("vendorHonorAfStateObserved", firstIntLike(resultValueByName(physicalResult, "com.hihonor.capture.metadata.afState")) ?: JSONObject.NULL)\n                .put("vendorSemanticsPromoted", false)\n                .put("controlProofStatus", if (lastFocusProbeRequestedDiopters == null)\n                    "BASELINE_AUTO_NO_MANUAL_FOCUS_WRITE"\n                else if (lastFocusProbePhysicalDistanceWritten)\n                    "REQUEST_WRITE_ACCEPTED_DIFFERENTIAL_PHYSICAL_MOVEMENT_PROOF_PENDING"\n                else\n                    "PHYSICAL_FOCUS_WRITE_NOT_ACCEPTED"))\n            .put("rawPayload", JSONObject()\n'''
if report_needle not in s:
    raise SystemExit('focus report insertion point not found')
s = s.replace(report_needle, report_replacement, 1)

# Add a visible completion line but never label manual focus as proven from one capture.
complete_needle = '''                    "Stage 3.5 post-HAL envelope=${if (halEnvelope != null) "captured" else "unavailable"} · source already sealed",\n'''
complete_replacement = '''                    "Stage 3.5 post-HAL envelope=${if (halEnvelope != null) "captured" else "unavailable"} · source already sealed\\n" +\n                    "Focus probe=$lastFocusProbeMode · requested=${lastFocusProbeRequestedDiopters ?: "AUTO"} · physicalWrite=$lastFocusProbePhysicalDistanceWritten",\n'''
if complete_needle not in s:
    raise SystemExit('focus completion status insertion point not found')
s = s.replace(complete_needle, complete_replacement, 1)

# Scientific invariants / build-time guards.
assert 'TruthRaw · 200MP Tele Test v0.18 · Camera-5 physical focus probe' in s
assert 'focusProbeButton = button(' in s
assert 'requestBuilder.setPhysicalCameraKey(' in s
assert 'CaptureRequest.LENS_FOCUS_DISTANCE' in s
assert 'REQUEST_WRITE_ACCEPTED_DIFFERENTIAL_PHYSICAL_MOVEMENT_PROOF_PENDING' in s
assert s.index('persistOriginalRawBuffer(image, stamp)') < s.index('Camera2EnvelopeProbe.observe(image, physical, physicalResult)')
assert 'preHalGateChangedVendorKeys' in s
assert 'vendorKeysWritten", false' not in s  # lives in helper, not generated activity
assert 'APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF' in s
assert 'MAXIMUM_RESOLUTION vereist' not in s

p.write_text(s)
print('patched', p)
print('bytes', p.stat().st_size)
