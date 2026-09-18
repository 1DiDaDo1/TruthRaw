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

needle = '''                    capabilitySource = packed.second.selectedSource
                    capabilityReady = true
                    previewButton.isEnabled = preview.isAvailable
                    val r = packed.second
                    setStatus(
                        "STAGE 1 PASS · exact 16320×12288 RAW_SENSOR via ${r.selectedSource}.\n" +
                            "standard.out=[${routeText(r.standardOutput)}]\n" +
                            "standard.high=[${routeText(r.standardHigh)}]\n" +
                            "maximum.out=[${routeText(r.maximumOutput)}]\n" +
                            "maximum.high=[${routeText(r.maximumHigh)}]\n" +
                            "Druk nu Stap 2.",
                    )
'''
replacement = '''                    capabilitySource = packed.second.selectedSource
                    capabilityReady = false
                    previewButton.isEnabled = false
                    captureButton.isEnabled = false
                    val r = packed.second
                    setStatus(
                        "STAGE 1 PASS · Camera-5 RAW routes gelezen via ${r.selectedSource}.\n" +
                            "STAGE 1.5 · v0.32 multi-key native-type oracle bezig.\n" +
                            "Geen session parameters · geen capture session · geen capture submit · geen RAW pixel access.",
                    )
                    Thread({
                        val oracle = runCatching {
                            Camera2MultiKeyNativeTypeOracle.probe(
                                manager = packed.first.first,
                                logicalCameraId = LOGICAL_ID,
                                physicalCameraId = PHYSICAL_ID,
                            )
                        }.getOrElse { e ->
                            JSONObject()
                                .put("schema", "truthraw.camera2-v032-multi-key-native-type-oracle.v0.32")
                                .put("classification", "V032_MULTI_KEY_NATIVE_TYPE_ORACLE_AGGREGATOR_ERROR")
                                .put("error", "${e.javaClass.simpleName}: ${e.message}")
                                .put("sessionCreated", false)
                                .put("sessionParametersAttached", false)
                                .put("captureSubmitted", false)
                                .put("vendorModifiedRequestSubmittedToHal", false)
                                .put("rawPixelAccess", false)
                                .put("sourceMutation", false)
                                .put("semanticPromotionAllowed", false)
                                .put("interventionAllowed", false)
                        }

                        oracle.put("experimentVersion", "v0.32")
                            .put("controlReference", "TruthRaw v0.20 unchanged")
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

                        val oracleFile = File(
                            cacheDir,
                            "TRUTHRAW_CAM5_MULTI_KEY_NATIVE_TYPE_ORACLE_v032.json",
                        )
                        runCatching { oracleFile.writeText(oracle.toString(2)) }
                        capturedJson = oracleFile.takeIf { it.exists() && it.length() > 0L }

                        runOnUiThread {
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
                        }
                    }, "truthraw-v032-multi-key-native-type-oracle").start()
'''
if needle not in s:
    raise SystemExit('v0.32 Stage-1 success anchor not found')
s = s.replace(needle, replacement, 1)

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
print('patched', p)
print('bytes', p.stat().st_size)
