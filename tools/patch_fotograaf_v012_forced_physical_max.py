#!/usr/bin/env python3
from pathlib import Path
import re

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')
s = p.read_text()

# Version/provenance labels.
s = s.replace('200MP test v0.11', '200MP test v0.12')
s = s.replace('TruthRaw · 200MP Tele Test v0.11', 'TruthRaw · 200MP Tele Test v0.12')
s = s.replace('STAGE 0 PASS · v0.11 UI', 'STAGE 0 PASS · v0.12 UI')
s = s.replace('truthraw-200mp-v011', 'truthraw-200mp-v012')
s = s.replace('truthraw-v011-capability', 'truthraw-v012-capability')
s = s.replace('_v011.dng', '_v012.dng')
s = s.replace('_EVIDENCE_v011.json', '_EVIDENCE_v012.json')
s = s.replace('_v011.${if (contiguous)', '_v012.${if (contiguous)')
s = s.replace('staged-evidence.v0.11', 'staged-evidence.v0.12')
s = s.replace('physical-5-scoped MAX still request', 'physical-5-scoped MAX still request with forced physical pixel-mode metadata')
s = s.replace('Stap 3 · PHYSICAL-SCOPED CAPTURE · 16320×12288', 'Stap 3 · FORCE PHYSICAL MAX · 16320×12288')

field_old = '    private var lastPhysicalPixelModeWritten = false\n'
field_new = (
    '    private var lastPhysicalPixelModeWritten = false\n'
    '    private var lastPhysicalPixelModeAdvertised = false\n'
    '    private var lastPhysicalPixelModeForceError: String? = null\n'
)
if field_old not in s:
    raise SystemExit('expected v0.11 physical-pixel-mode field not found')
s = s.replace(field_old, field_new, 1)

s = s.replace('submitPhysicalScopedStill(device, s, logical, physical)', 'submitForcedPhysicalMaxStill(device, s, logical)')

start = s.index('    private fun submitPhysicalScopedStill(')
end = s.index('    private fun finalizeIfPaired()', start)
new_method = r'''    private fun submitForcedPhysicalMaxStill(
        device: CameraDevice,
        s: CameraCaptureSession,
        logical: CameraCharacteristics,
    ) {
        val reader = rawReader ?: return
        try {
            // v0.11 proved the scoped request exists, but Android rejected submission because
            // the physical Camera-5 request metadata remained in DEFAULT sensor pixel mode while
            // the attached stream was configured MAXIMUM_RESOLUTION-only.  AOSP checks that
            // consistency before filtering unsupported physical override keys.  v0.12 therefore
            // writes the physical SENSOR_PIXEL_MODE explicitly and verifies builder read-back.
            val requestBuilder = device.createCaptureRequest(
                CameraDevice.TEMPLATE_STILL_CAPTURE,
                setOf(PHYSICAL_ID),
            )
            lastScopedRequestUsed = true
            lastScopedRequestError = null
            requestBuilder.addTarget(reader.surface)

            lastGlobalPixelModeWritten = false
            if (logical.availableCaptureRequestKeys?.contains(CaptureRequest.SENSOR_PIXEL_MODE) == true) {
                requestBuilder.set(
                    CaptureRequest.SENSOR_PIXEL_MODE,
                    CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION,
                )
                lastGlobalPixelModeWritten =
                    requestBuilder.get(CaptureRequest.SENSOR_PIXEL_MODE) == CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION
            }

            lastPhysicalPixelModeAdvertised = physicalOverrideSupported(logical, CaptureRequest.SENSOR_PIXEL_MODE)
            lastPhysicalPixelModeWritten = false
            lastPhysicalPixelModeForceError = null
            runCatching {
                requestBuilder.setPhysicalCameraKey(
                    CaptureRequest.SENSOR_PIXEL_MODE,
                    CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION,
                    PHYSICAL_ID,
                )
                lastPhysicalPixelModeWritten =
                    requestBuilder.getPhysicalCameraKey(CaptureRequest.SENSOR_PIXEL_MODE, PHYSICAL_ID) ==
                        CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION
            }.onFailure { e ->
                lastPhysicalPixelModeForceError = "${e.javaClass.simpleName}: ${e.message}"
            }

            if (!lastGlobalPixelModeWritten || !lastPhysicalPixelModeWritten) {
                setStatusAny(
                    "STAGE 3 BLOCKED BEFORE SUBMIT · MAX metadata kon niet exact worden gezet.\n" +
                        "globalMAX=$lastGlobalPixelModeWritten · physicalMAX=$lastPhysicalPixelModeWritten · " +
                        "physicalOverrideAdvertised=$lastPhysicalPixelModeAdvertised" +
                        (lastPhysicalPixelModeForceError?.let { "\nphysicalForceError=$it" } ?: ""),
                )
                runOnUiThread { previewButton.isEnabled = true }
                return
            }

            // Deliberately do not add AE/AF/NR/EDGE overrides here.  Keep the request at the
            // TEMPLATE_STILL_CAPTURE defaults plus only the two pixel-mode declarations so a HAL
            // failure cannot be blamed on optional processing controls.
            val request = requestBuilder.build()
            setStatusAny(
                "STAGE 3 CAPTURE SENT · scopedRequest=true · globalMAX=true · physicalMAX=true · " +
                    "physicalOverrideAdvertised=$lastPhysicalPixelModeAdvertised\n" +
                    "Minimal still request; wachten op RAW Image + physical Camera-5 result…",
            )

            s.capture(request, object : CameraCaptureSession.CaptureCallback() {
                override fun onCaptureCompleted(
                    session: CameraCaptureSession,
                    request: CaptureRequest,
                    result: TotalCaptureResult,
                ) {
                    synchronized(pairLock) { pendingResult = result }
                    finalizeIfPaired()
                }

                override fun onCaptureFailed(
                    session: CameraCaptureSession,
                    request: CaptureRequest,
                    failure: CaptureFailure,
                ) {
                    setStatusAny(
                        "STAGE 3 CAPTURE FAIL · reason=${failure.reason} · wasImageCaptured=${failure.wasImageCaptured()} · " +
                            "sequenceId=${failure.sequenceId} · frameNumber=${failure.frameNumber}\n" +
                            "scopedRequest=true globalMAX=$lastGlobalPixelModeWritten " +
                            "physicalMAX=$lastPhysicalPixelModeWritten advertised=$lastPhysicalPixelModeAdvertised" +
                            (lastPhysicalPixelModeForceError?.let { "\nphysicalForceError=$it" } ?: ""),
                    )
                    runOnUiThread {
                        previewButton.isEnabled = true
                        captureButton.isEnabled = false
                    }
                }
            }, cameraHandler)
        } catch (e: Throwable) {
            setStatusAny(
                "STAGE 3 request FAIL · ${e.javaClass.simpleName}: ${e.message}\n" +
                    "scopedRequest=$lastScopedRequestUsed globalMAX=$lastGlobalPixelModeWritten " +
                    "physicalMAX=$lastPhysicalPixelModeWritten advertised=$lastPhysicalPixelModeAdvertised" +
                    (lastPhysicalPixelModeForceError?.let { "\nphysicalForceError=$it" } ?: ""),
            )
            runOnUiThread { previewButton.isEnabled = true }
        }
    }

'''
s = s[:start] + new_method + s[end:]

needle = '                .put("physicalSensorPixelModeWritten", lastPhysicalPixelModeWritten)\n'
replacement = (
    '                .put("physicalSensorPixelModeWritten", lastPhysicalPixelModeWritten)\n'
    '                .put("physicalSensorPixelModeOverrideAdvertised", lastPhysicalPixelModeAdvertised)\n'
    '                .put("physicalSensorPixelModeForceError", lastPhysicalPixelModeForceError ?: JSONObject.NULL)\n'
)
if needle not in s:
    raise SystemExit('evidence requestTopology marker not found')
s = s.replace(needle, replacement, 1)

# Ensure old implementation is completely gone and the intended experiment is present.
assert 'private fun submitPhysicalScopedStill' not in s
assert 'private fun submitForcedPhysicalMaxStill' in s
assert 'setPhysicalCameraKey(' in s
assert 'getPhysicalCameraKey(' in s
assert 'physicalSensorPixelModeOverrideAdvertised' in s
assert 'v0.12' in s

p.write_text(s)
print('patched', p)
print('bytes', p.stat().st_size)
