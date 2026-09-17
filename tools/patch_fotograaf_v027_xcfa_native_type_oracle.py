#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

# Reconstruct exact v0.20 source/payload authority. v0.27 is Stage-1-only diagnostics.
# It reuses the proven v0.25 disposable NDK metadata validator for exactly one different
# advertised vendor key: EnableXCFAOptimization. No session parameters or capture are submitted.
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
    'TruthRaw · Camera-5 v0.27 · XCFA native type oracle',
)
s = s.replace('truthraw-200mp-v020', 'truthraw-200mp-v027')
s = s.replace('truthraw-v020-capability', 'truthraw-v027-capability')
s = s.replace(
    'physical-5-scoped MAX request; exact RAW bytes are sealed first, HAL envelope and full raster are audited, then any populated prefix is matched read-only against advertised standard RAW geometries',
    'Stage-1-only diagnostic: resolve EnableXCFAOptimization native metadata type with disposable NDK request metadata; no session parameters, capture session or capture submit',
)
s = s.replace(
    'saveJsonButton = button("200MP evidence JSON opslaan")',
    'saveJsonButton = button("v0.27 XCFA native type-oracle JSON opslaan")',
)

needle = '''                    capabilitySource = packed.second.selectedSource\n                    capabilityReady = true\n                    previewButton.isEnabled = preview.isAvailable\n                    val r = packed.second\n                    setStatus(\n                        "STAGE 1 PASS · exact 16320×12288 RAW_SENSOR via ${r.selectedSource}.\\n" +\n                            "standard.out=[${routeText(r.standardOutput)}]\\n" +\n                            "standard.high=[${routeText(r.standardHigh)}]\\n" +\n                            "maximum.out=[${routeText(r.maximumOutput)}]\\n" +\n                            "maximum.high=[${routeText(r.maximumHigh)}]\\n" +\n                            "Druk nu Stap 2.",\n                    )\n'''
replacement = '''                    capabilitySource = packed.second.selectedSource\n                    capabilityReady = false\n                    previewButton.isEnabled = false\n                    captureButton.isEnabled = false\n                    val r = packed.second\n                    setStatus(\n                        "STAGE 1 PASS · Camera-5 RAW routes gelezen.\\n" +\n                            "standard.out=[${routeText(r.standardOutput)}] · maximum.high=[${routeText(r.maximumHigh)}]\\n" +\n                            "STAGE 1.5 · EnableXCFAOptimization native metadata-type oracle bezig; geen session/capture submit…",\n                    )\n                    Thread({\n                        val keyName = "org.codeaurora.qcamera3.sessionParameters.EnableXCFAOptimization"\n                        val oracle = Camera2RawCbSourceTypeNativeTypeOracle.probeKey(\n                            cameraId = LOGICAL_ID,\n                            keyName = keyName,\n                            schema = "truthraw.camera2-xcfa-native-type-oracle.v0.27",\n                            bridgeErrorClassification = "XCFA_NATIVE_TYPE_ORACLE_BRIDGE_ERROR",\n                        )\n                        oracle.put("experimentVersion", "v0.27")\n                            .put("controlReference", "TruthRaw v0.20 unchanged")\n                            .put("gateBObservedDefaultOrCurrentValueClass", "byte[] length 1 preview [0] in v0.26 evidence")\n                            .put("gateBObservationIsNotNativeTypeProof", true)\n                            .put("testValuePurpose", "TYPE_VALIDATION_ONLY_NOT_VENDOR_VALUE_SEMANTICS")\n                            .put("semanticMeaningAssumed", false)\n                            .put("semanticPromotionAllowed", false)\n                        val oracleFile = File(cacheDir, "TRUTHRAW_CAM5_XCFA_NATIVE_TYPE_ORACLE_v027.json")\n                        runCatching { oracleFile.writeText(oracle.toString(2)) }\n                        capturedJson = oracleFile.takeIf { it.exists() && it.length() > 0L }\n                        runOnUiThread {\n                            saveJsonButton.isEnabled = capturedJson != null\n                            val classification = oracle.optString("classification", "UNKNOWN")\n                            val resolved = oracle.optString("resolvedNativeType", "UNRESOLVED")\n                            val tagHex = oracle.optString("tagIdHex", "unknown")\n                            val acceptedCount = oracle.optInt("acceptedTypeCount", 0)\n                            val u8Accepted = oracle.optJSONObject("u8Test")?.optBoolean("accepted", false) ?: false\n                            val i32Accepted = oracle.optJSONObject("i32Test")?.optBoolean("accepted", false) ?: false\n                            val f32Accepted = oracle.optJSONObject("floatTest")?.optBoolean("accepted", false) ?: false\n                            val i64Accepted = oracle.optJSONObject("i64Test")?.optBoolean("accepted", false) ?: false\n                            val f64Accepted = oracle.optJSONObject("doubleTest")?.optBoolean("accepted", false) ?: false\n                            val rationalAccepted = oracle.optJSONObject("rationalTest")?.optBoolean("accepted", false) ?: false\n                            setStatus(\n                                "STAGE 1.5 DIAGNOSTIC STOP · v0.27 EnableXCFAOptimization native type oracle\\n" +\n                                    "classification=$classification\\n" +\n                                    "resolvedNativeType=$resolved · tag=$tagHex · acceptedTypeCount=$acceptedCount\\n" +\n                                    "BYTE=$u8Accepted · INT32=$i32Accepted · FLOAT=$f32Accepted\\n" +\n                                    "INT64=$i64Accepted · DOUBLE=$f64Accepted · RATIONAL=$rationalAccepted\\n" +\n                                    "sessionCreated=false · sessionParametersAttached=false · captureSubmitted=false.\\n" +\n                                    "v0.20 source authority blijft onaangeroerd.",\n                            )\n                        }\n                    }, "truthraw-v027-xcfa-native-type-oracle").start()\n'''
if needle not in s:
    raise SystemExit('v0.27 Stage-1 success anchor not found')
s = s.replace(needle, replacement, 1)

assert 'TruthRaw · Camera-5 v0.27 · XCFA native type oracle' in s
assert 'org.codeaurora.qcamera3.sessionParameters.EnableXCFAOptimization' in s
assert 'Camera2RawCbSourceTypeNativeTypeOracle.probeKey(' in s
assert 'TRUTHRAW_CAM5_XCFA_NATIVE_TYPE_ORACLE_v027.json' in s
assert 'STAGE 1.5 DIAGNOSTIC STOP' in s
assert 'capabilityReady = false' in s
assert 'previewButton.isEnabled = false' in s
assert 'captureButton.isEnabled = false' in s
assert 'captureSubmitted=false' in s
assert 'MAXIMUM_RESOLUTION vereist' not in s

p.write_text(s)
print('patched', p)
print('bytes', p.stat().st_size)
