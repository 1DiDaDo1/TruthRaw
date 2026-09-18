#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

# Reconstruct the exact v0.20 source/payload authority first. v0.32 is diagnostics-only:
# it inspects three already-observed upstream route-key names with the proven disposable
# NDK native metadata-type oracle. It creates no capture session, attaches no session
# parameters, submits no capture, reads no RAW pixels and cannot promote vendor semantics.
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
    'TruthRaw · Camera-5 v0.32 · multi-key native type oracle',
)
s = s.replace('truthraw-200mp-v020', 'truthraw-200mp-v032')
s = s.replace('truthraw-v020-capability', 'truthraw-v032-capability')
s = s.replace(
    'physical-5-scoped MAX request; exact RAW bytes are sealed first, HAL envelope and full raster are audited, then any populated prefix is matched read-only against advertised standard RAW geometries',
    'Stage-1-only diagnostic: screen three v0.20-observed upstream route-key names for native metadata representation; no session parameters, capture session, capture submit or RAW pixel access',
)
s = s.replace(
    'saveJsonButton = button("200MP evidence JSON opslaan")',
    'saveJsonButton = button("v0.32 multi-key type-oracle JSON opslaan")',
)

# Replace the complete Stage-1 capability function rather than depending on one historical
# success-message text anchor. This keeps v0.20 reconstruction as the immutable parent while
# making the v0.32 diagnostic insertion robust to prior source-first wording changes.
start_marker = '    private fun readCapability() {\n'
end_marker = '\n    private fun startLogicalPreview() {\n'
start = s.find(start_marker)
end = s.find(end_marker, start)
if start < 0 or end < 0:
    raise SystemExit('v0.32 readCapability function boundaries not found')

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
            "v0.32 STAGE 1 · Camera-5 routes lezen en daarna drie upstream vendor-keynamen " +
                "representation-only screenen; geen session/capture submit…",
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
                val standardHigh = safeSizes { standard?.getHighResolutionOutputSizes(ImageFormat.RAW_SENSOR) }
                val maximumOutput = safeSizes { maximum?.getOutputSizes(ImageFormat.RAW_SENSOR) }
                val maximumHigh = safeSizes { maximum?.getHighResolutionOutputSizes(ImageFormat.RAW_SENSOR) }

                val selected = when {
                    containsTarget(maximumHigh) -> "MAXIMUM_MAP_HIGH_RESOLUTION"
                    containsTarget(maximumOutput) -> "MAXIMUM_MAP_OUTPUT"
                    containsTarget(standardHigh) -> "STANDARD_MAP_HIGH_RESOLUTION"
                    containsTarget(standardOutput) -> "STANDARD_MAP_OUTPUT"
                    else -> error(
                        "16320×12288 RAW_SENSOR ontbreekt; standard.out=[$standardOutput] " +
                            "standard.high=[$standardHigh] maximum.out=[$maximumOutput] " +
                            "maximum.high=[$maximumHigh]",
                    )
                }

                val oracle = Camera2MultiKeyNativeTypeOracle.probe(
                    manager = m,
                    logicalCameraId = LOGICAL_ID,
                    physicalCameraId = PHYSICAL_ID,
                )
                oracle.put("experimentVersion", "v0.32")
                    .put("controlReference", "TruthRaw v0.20 unchanged")
                    .put("capabilityRouteSource", selected)
                    .put("standardRawOutputs", JSONArray(standardOutput.map { "${it.width}x${it.height}" }))
                    .put("maximumRawOutputs", JSONArray(maximumOutput.map { "${it.width}x${it.height}" }))
                    .put("maximumHighResolutionRawOutputs", JSONArray(maximumHigh.map { "${it.width}x${it.height}" }))
                    .put(
                        "closedPredecessor",
                        "v0.31 bounded INT32 domain {UNSET,0,1,2,3} closed with no measured RAW topology differential",
                    )
                    .put(
                        "candidateSet",
                        "EnableInsensorZoom; EnableSnapshotOnlyInsensorZoom; EnableMCXMasterCb",
                    )
                    .put("candidateNamesAreSemanticProof", false)
                    .put("routeEffectAssumed", false)
                    .put("interventionPerformed", false)

                Triple(m, logical, physical) to oracle
            }

            runOnUiThread {
                capabilityButton.isEnabled = true
                result.onSuccess { packed ->
                    manager = packed.first.first
                    logicalCharacteristics = packed.first.second
                    physical5Characteristics = packed.first.third
                    capabilitySource = packed.second.optString("capabilityRouteSource", "UNKNOWN")
                    capabilityReady = false
                    previewButton.isEnabled = false
                    captureButton.isEnabled = false

                    val oracle = packed.second
                    val oracleFile = File(
                        cacheDir,
                        "TRUTHRAW_CAM5_MULTI_KEY_NATIVE_TYPE_ORACLE_v032.json",
                    )
                    val writeError = runCatching { oracleFile.writeText(oracle.toString(2)) }.exceptionOrNull()
                    capturedJson = oracleFile.takeIf {
                        writeError == null && it.exists() && it.length() > 0L
                    }
                    saveJsonButton.isEnabled = capturedJson != null

                    val classification = oracle.optString("classification", "UNKNOWN")
                    val resolvedCount = oracle.optInt("resolvedCount", 0)
                    val unresolvedCount = oracle.optInt("unresolvedCount", 0)
                    val ambiguousCount = oracle.optInt("ambiguousCount", 0)
                    val lines = mutableListOf<String>()
                    val results = oracle.optJSONArray("results")
                    if (results != null) {
                        for (i in 0 until results.length()) {
                            val item = results.optJSONObject(i) ?: continue
                            lines += "${item.optString("candidateSymbol", "?")} · " +
                                "${item.optString("keyName", "?").substringAfterLast('.')} · " +
                                "type=${item.optString("resolvedNativeType", "UNRESOLVED")} · " +
                                "tag=${item.optString("tagIdHex", "unknown")}"
                        }
                    }

                    setStatus(
                        "STAGE 1.5 DIAGNOSTIC STOP · v0.32 multi-key native type oracle\n" +
                            "classification=$classification\n" +
                            "resolved=$resolvedCount · unresolved=$unresolvedCount · ambiguous=$ambiguousCount\n" +
                            lines.joinToString("\n") + "\n" +
                            "sessionCreated=false · sessionParametersAttached=false · captureSubmitted=false.\n" +
                            "Geen interventie uitgevoerd; v0.20 source/payload authority blijft onaangeroerd.",
                    )
                }.onFailure { e ->
                    capabilityReady = false
                    capabilitySource = null
                    previewButton.isEnabled = false
                    captureButton.isEnabled = false
                    setStatus(
                        "v0.32 DIAGNOSTIC BLOCKED · ${e.javaClass.simpleName}: ${e.message}\n" +
                            "Geen capture session gemaakt en geen vendor-modified request naar HAL gestuurd.",
                    )
                }
            }
        }, "truthraw-v032-multi-key-native-type-oracle").start()
    }
'''

s = s[:start] + replacement + s[end:]

assert 'TruthRaw · Camera-5 v0.32 · multi-key native type oracle' in s
assert 'Camera2MultiKeyNativeTypeOracle.probe(' in s
assert 'TRUTHRAW_CAM5_MULTI_KEY_NATIVE_TYPE_ORACLE_v032.json' in s
assert 'STAGE 1.5 DIAGNOSTIC STOP · v0.32' in s
assert 'capabilityReady = false' in s
assert 'previewButton.isEnabled = false' in s
assert 'captureButton.isEnabled = false' in s
assert 'sessionParametersAttached=false' in s
assert 'captureSubmitted=false' in s
assert 'Geen interventie uitgevoerd' in s

p.write_text(s)
print('patched v0.32 robust Stage-1 diagnostic', p)
print('bytes', p.stat().st_size)
