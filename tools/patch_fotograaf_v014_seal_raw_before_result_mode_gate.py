#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

# Reconstruct the exact v0.13 build-time source first.
ns = {}
exec(compile(Path('tools/patch_fotograaf_v013_physical_only_max_submit.py').read_text(), 'patch_fotograaf_v013_physical_only_max_submit.py', 'exec'), ns, ns)
s = p.read_text()

# Version/provenance labels.
s = s.replace('200MP test v0.13', '200MP test v0.14')
s = s.replace('TruthRaw · 200MP Tele Test v0.13', 'TruthRaw · 200MP Tele Test v0.14')
s = s.replace('STAGE 0 PASS · v0.13 UI', 'STAGE 0 PASS · v0.14 UI')
s = s.replace('truthraw-200mp-v013', 'truthraw-200mp-v014')
s = s.replace('truthraw-v013-capability', 'truthraw-v014-capability')
s = s.replace('_v013.dng', '_v014.dng')
s = s.replace('_EVIDENCE_v013.json', '_EVIDENCE_v014.json')
s = s.replace('_v013.${if (contiguous)', '_v014.${if (contiguous)')
s = s.replace('staged-evidence.v0.13', 'staged-evidence.v0.14')
s = s.replace(
    'physical-5-scoped MAX still request; physical MAX metadata is authoritative for the physical-bound high-res stream',
    'physical-5-scoped MAX request; exact RAW bytes are sealed before interpreting returned pixel-mode metadata',
)
s = s.replace('Stap 3 · SUBMIT PHYSICAL MAX · 16320×12288', 'Stap 3 · CAPTURE + SEAL 200MP RAW · 16320×12288')

start = s.index('    private fun finalizeCapture(image: Image, logicalResult: TotalCaptureResult) {')
end = s.index('    private fun persistOriginalRawBuffer(', start)
new_finalize = r'''    private fun finalizeCapture(image: Image, logicalResult: TotalCaptureResult) {
        try {
            require(image.width == TARGET_W && image.height == TARGET_H) {
                "RAW dimensions ${image.width}×${image.height} != ${TARGET_W}×${TARGET_H}"
            }
            val physicalResult = logicalResult.physicalCameraResults[PHYSICAL_ID]
                ?: error("physical Camera-5 TotalCaptureResult ontbreekt; ids=${logicalResult.physicalCameraResults.keys}")
            val sensorTs = physicalResult.get(CaptureResult.SENSOR_TIMESTAMP)
                ?: error("physical Camera-5 SENSOR_TIMESTAMP ontbreekt")
            require(sensorTs == image.timestamp) {
                "Image.timestamp=${image.timestamp} != physical5 SENSOR_TIMESTAMP=$sensorTs"
            }

            // Structural RAW checks come before semantic interpretation of returned metadata.
            // v0.13 proved that HONOR can deliver an exact 16320x12288 RAW_SENSOR Image with a
            // timestamp-identical physical Camera-5 result while that result reports
            // SENSOR_PIXEL_MODE=DEFAULT(0).  Preserve the actual evidence first; never destroy a
            // successfully delivered physical RAW frame because one result field is surprising.
            val plane = image.planes.singleOrNull() ?: error("RAW_SENSOR planeCount=${image.planes.size}, exact 1 vereist")
            require(plane.pixelStride == 2) { "RAW pixelStride=${plane.pixelStride}, 2 vereist" }
            require(plane.rowStride >= TARGET_W * 2) { "RAW rowStride=${plane.rowStride} te klein" }

            val stamp = System.currentTimeMillis()
            val rawEvidence = persistOriginalRawBuffer(image, stamp)
            capturedRaw = rawEvidence.file

            // Returned pixel mode is an observation, not a retroactive veto on delivered bytes.
            // Keep the request/output MAX declarations and the returned result value separate.
            val returnedPixelMode = physicalResult.get(CaptureResult.SENSOR_PIXEL_MODE)
            val returnedPixelModeIsMaximum =
                returnedPixelMode == CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION

            val physical = physical5Characteristics ?: error("physical characteristics ontbreken")
            var dng: File? = null
            var dngSha: String? = null
            var dngError: String? = null
            runCatching {
                val candidate = File(cacheDir, "TRUTHRAW_${stamp}_CAM5_200MP_${TARGET_W}x${TARGET_H}_v014.dng")
                FileOutputStream(candidate).use { out ->
                    DngCreator(physical, physicalResult).use { creator ->
                        creator.setOrientation(1)
                        creator.writeImage(out, image)
                    }
                }
                dng = candidate
                dngSha = sha256File(candidate)
            }.onFailure { e ->
                dngError = "${e.javaClass.simpleName}: ${e.message}"
            }

            val report = File(cacheDir, "TRUTHRAW_${stamp}_CAM5_200MP_EVIDENCE_v014.json")
            report.writeText(
                buildEvidence(logicalResult, physicalResult, image, rawEvidence, dng, dngSha, dngError)
                    .put("returnedSensorPixelModeIsMaximumResolution", returnedPixelModeIsMaximum)
                    .put("returnedSensorPixelModeMismatchPreserved", !returnedPixelModeIsMaximum)
                    .put("sealBeforePixelModeInterpretation", true)
                    .toString(2)
            )
            capturedDng = dng
            capturedJson = report
            image.close()

            setStatusAny(
                "STAGE 3 RAW SEALED · physical 5 · 16320×12288 · 200,540,160 samples · timestamp exact.\n" +
                    "Originele app-visible RAW buffer bewaard vóór DNG en vóór pixel-mode interpretatie.\n" +
                    "returned SENSOR_PIXEL_MODE=${returnedPixelMode ?: "null"} · requested/output MAX=true · mismatchPreserved=${!returnedPixelModeIsMaximum}",
            )
            runOnUiThread {
                saveRawButton.isEnabled = true
                saveDngButton.isEnabled = capturedDng != null
                saveJsonButton.isEnabled = true
                previewButton.isEnabled = true
            }
        } catch (e: Throwable) {
            runCatching { image.close() }
            setStatusAny("STAGE 3 FAIL CLOSED · ${e.javaClass.simpleName}: ${e.message}")
            runOnUiThread {
                // If sealing succeeded before a later auxiliary failure, keep export controls live.
                saveRawButton.isEnabled = capturedRaw?.exists() == true
                saveDngButton.isEnabled = capturedDng?.exists() == true
                saveJsonButton.isEnabled = capturedJson?.exists() == true
                previewButton.isEnabled = true
            }
        } finally {
            runCatching { session?.close() }
            session = null
            runCatching { rawReader?.close() }
            rawReader = null
            runCatching { camera?.close() }
            camera = null
        }
    }

'''
s = s[:start] + new_finalize + s[end:]

# Strengthen evidence semantics without changing authority.
old = '''                .put("captureResultSensorPixelMode", physicalResult.get(CaptureResult.SENSOR_PIXEL_MODE) ?: JSONObject.NULL))'''
new = '''                .put("captureResultSensorPixelMode", physicalResult.get(CaptureResult.SENSOR_PIXEL_MODE) ?: JSONObject.NULL)
                .put("requestedMaximumResolution", true)
                .put("outputMaximumResolutionModeDeclared", true)
                .put("resultPixelModeIsIndependentObservation", true))'''
if old not in s:
    raise SystemExit('captureRoute pixel-mode marker not found')
s = s.replace(old, new, 1)

s = s.replace(
    '.put("boundary", "APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF")',
    '.put("boundary", "APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF")\n'
    '            .put("classification", "DELIVERED_200MP_PHYSICAL_CAMERA5_RAW_SENSOR_WITH_EXACT_TIMESTAMP_BINDING; RETURNED_PIXEL_MODE_RECORDED_NOT_ASSUMED")',
    1,
)

assert 'TruthRaw · 200MP Tele Test v0.14' in s
assert 'STAGE 3 RAW SEALED' in s
assert 'returnedSensorPixelModeMismatchPreserved' in s
assert 'persistOriginalRawBuffer(image, stamp)' in s
assert 'MAXIMUM_RESOLUTION vereist' not in s
assert 'APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF' in s

p.write_text(s)
print('patched', p)
print('bytes', p.stat().st_size)
