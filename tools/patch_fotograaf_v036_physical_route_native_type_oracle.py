#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

# Reconstruct exact v0.20 authority first. v0.36 is diagnostic only.
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
    'TruthRaw · Camera-5 v0.36 · physical-route native type oracle',
)
s = s.replace('truthraw-200mp-v020', 'truthraw-200mp-v036')
s = s.replace('truthraw-v020-capability', 'truthraw-v036-capability')
s = s.replace(
    'saveJsonButton = button("200MP evidence JSON opslaan")',
    'saveJsonButton = button("v0.36 physical-route oracle JSON opslaan")',
)

start_marker = '    private fun readCapability() {\n'
end_marker = '\n    private fun startLogicalPreview() {\n'
start = s.find(start_marker)
end = s.find(end_marker, start)
if start < 0 or end < 0:
    raise SystemExit('v0.36 readCapability boundaries not found')

replacement = r'''    private fun readCapability() {
        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            setStatus("Stap 1 geblokkeerd: CAMERA permission ontbreekt.")
            return
        }
        capabilityButton.isEnabled = false
        capabilityReady = false
        previewButton.isEnabled = false
        captureButton.isEnabled = false
        setStatus(
            "v0.36 STAGE 1 · physical-vs-logical route availability controleren en " +
                "vier physical-5-only QTI keys representation-only screenen; geen submit…",
        )

        Thread({
            val result = runCatching {
                val m = getSystemService(CameraManager::class.java)
                val logical = m.getCameraCharacteristics(LOGICAL_ID)
                require(logical.physicalCameraIds.contains(PHYSICAL_ID)) {
                    "logical 0 meldt physical 5 niet; physicalIds=${logical.physicalCameraIds}"
                }
                val physical = m.getCameraCharacteristics(PHYSICAL_ID)
                val standard = physical.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
                val maximum = physical.get(
                    CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION,
                )

                val standardOutput = safeSizes { standard?.getOutputSizes(ImageFormat.RAW_SENSOR) }
                val maximumOutput = safeSizes { maximum?.getOutputSizes(ImageFormat.RAW_SENSOR) }
                val maximumHigh = safeSizes { maximum?.getHighResolutionOutputSizes(ImageFormat.RAW_SENSOR) }
                require(containsTarget(maximumHigh) || containsTarget(maximumOutput)) {
                    "16320×12288 MAX RAW route ontbreekt"
                }

                val oracle = Camera2PhysicalRouteNativeTypeOracle.probe(
                    manager = m,
                    logicalCameraId = LOGICAL_ID,
                    physicalCameraId = PHYSICAL_ID,
                )
                oracle.put("experimentVersion", "v0.36")
                    .put("controlReference", "TruthRaw v0.20 unchanged")
                    .put("standardRawOutputs", JSONArray(standardOutput.map { "${it.width}x${it.height}" }))
                    .put("maximumRawOutputs", JSONArray(maximumOutput.map { "${it.width}x${it.height}" }))
                    .put("maximumHighResolutionRawOutputs", JSONArray(maximumHigh.map { "${it.width}x${it.height}" }))
                    .put("interventionPerformed", false)
                    .put("directPhysicalCameraOpenAttempted", false)

                Triple(m, logical, physical) to oracle
            }

            runOnUiThread {
                capabilityButton.isEnabled = true
                result.onSuccess { packed ->
                    manager = packed.first.first
                    logicalCharacteristics = packed.first.second
                    physical5Characteristics = packed.first.third
                    capabilityReady = false
                    previewButton.isEnabled = false
                    captureButton.isEnabled = false

                    val oracle = packed.second
                    val oracleFile = File(
                        cacheDir,
                        "TRUTHRAW_CAM5_PHYSICAL_ROUTE_NATIVE_TYPE_ORACLE_v036.json",
                    )
                    val writeError = runCatching { oracleFile.writeText(oracle.toString(2)) }.exceptionOrNull()
                    capturedJson = oracleFile.takeIf {
                        writeError == null && it.exists() && it.length() > 0L
                    }
                    saveJsonButton.isEnabled = capturedJson != null

                    val lines = mutableListOf<String>()
                    val results = oracle.optJSONArray("results")
                    if (results != null) {
                        for (i in 0 until results.length()) {
                            val item = results.optJSONObject(i) ?: continue
                            lines += "${item.optString("candidateSymbol", "?")} · " +
                                "${item.optString("keyName", "?").substringAfterLast('.')} · " +
                                "type=${item.optString("resolvedNativeType", "UNRESOLVED")} · " +
                                "tag=${item.optString("tagIdHex", "unknown")} · " +
                                "physicalOnly=${item.optBoolean("availabilityTopologyPass", false)}"
                        }
                    }

                    setStatus(
                        "STAGE 1.5 DIAGNOSTIC STOP · v0.36 physical-route oracle\n" +
                            "classification=${oracle.optString("classification", "UNKNOWN")}\n" +
                            "resolved=${oracle.optInt("resolvedCount", 0)} · " +
                            "unresolved=${oracle.optInt("unresolvedCount", 0)} · " +
                            "ambiguous=${oracle.optInt("ambiguousCount", 0)}\n" +
                            lines.joinToString("\n") + "\n" +
                            "directPhysicalOpen=false · sessionCreated=false · " +
                            "sessionParametersAttached=false · captureSubmitted=false.\n" +
                            "Geen interventie uitgevoerd; v0.20 authority blijft onaangeroerd.",
                    )
                }.onFailure { e ->
                    capabilityReady = false
                    previewButton.isEnabled = false
                    captureButton.isEnabled = false
                    setStatus(
                        "v0.36 DIAGNOSTIC BLOCKED · ${e.javaClass.simpleName}: ${e.message}\n" +
                            "Geen session/capture submit en geen RAW-toegang uitgevoerd.",
                    )
                }
            }
        }, "truthraw-v036-physical-route-native-type-oracle").start()
    }
'''

s = s[:start] + replacement + s[end:]

assert 'TruthRaw · Camera-5 v0.36 · physical-route native type oracle' in s
assert 'Camera2PhysicalRouteNativeTypeOracle.probe(' in s
assert 'TRUTHRAW_CAM5_PHYSICAL_ROUTE_NATIVE_TYPE_ORACLE_v036.json' in s
assert 'STAGE 1.5 DIAGNOSTIC STOP · v0.36' in s
assert 'capabilityReady = false' in s
assert 'previewButton.isEnabled = false' in s
assert 'captureButton.isEnabled = false' in s
assert 'directPhysicalOpen=false' in s
assert 'sessionCreated=false' in s
assert 'captureSubmitted=false' in s
assert 'Camera2McxMasterCbInt32SessionProbe.applyExperiment(' not in s

p.write_text(s)
print('patched v0.36 physical-route oracle', p)
print('bytes', p.stat().st_size)
