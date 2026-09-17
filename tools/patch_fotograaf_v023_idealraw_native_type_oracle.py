#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

# Reconstruct exact v0.20 trusted acquisition/audit baseline. v0.23 is diagnostic-only and
# stops during Stage 1 before Java preview/session/capture. It uses a native disposable-request
# metadata validator to resolve BYTE vs INT32 for EnableIdealRAW without submitting that value.
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
    'TruthRaw · Camera-5 v0.23 · IdealRAW native type oracle',
)
s = s.replace('truthraw-200mp-v020', 'truthraw-200mp-v023')
s = s.replace('truthraw-v020-capability', 'truthraw-v023-capability')
s = s.replace(
    'physical-5-scoped MAX request; exact RAW bytes are sealed first, HAL envelope and full raster are audited, then any populated prefix is matched read-only against advertised standard RAW geometries',
    'Stage-1-only diagnostic: resolve the native metadata type of EnableIdealRAW with disposable NDK request metadata; no session is created and no capture is submitted',
)
s = s.replace(
    'saveJsonButton = button("200MP evidence JSON opslaan")',
    'saveJsonButton = button("v0.23 native type-oracle JSON opslaan")',
)

needle = '''                    capabilitySource = packed.second.selectedSource\n                    capabilityReady = true\n                    previewButton.isEnabled = preview.isAvailable\n                    val r = packed.second\n                    setStatus(\n                        "STAGE 1 PASS · exact 16320×12288 RAW_SENSOR via ${r.selectedSource}.\\n" +\n                            "standard.out=[${routeText(r.standardOutput)}]\\n" +\n                            "standard.high=[${routeText(r.standardHigh)}]\\n" +\n                            "maximum.out=[${routeText(r.maximumOutput)}]\\n" +\n                            "maximum.high=[${routeText(r.maximumHigh)}]\\n" +\n                            "Druk nu Stap 2.",\n                    )\n'''
replacement = '''                    capabilitySource = packed.second.selectedSource\n                    capabilityReady = false\n                    previewButton.isEnabled = false\n                    captureButton.isEnabled = false\n                    val r = packed.second\n                    setStatus(\n                        "STAGE 1 PASS · Camera-5 RAW routes gelezen.\\n" +\n                            "standard.out=[${routeText(r.standardOutput)}] · maximum.high=[${routeText(r.maximumHigh)}]\\n" +\n                            "STAGE 1.5 · native IdealRAW metadata-type oracle bezig; geen session/capture submit…",\n                    )\n                    Thread({\n                        val oracle = Camera2IdealRawNativeTypeOracle.probe(LOGICAL_ID)\n                        val oracleFile = File(cacheDir, "TRUTHRAW_CAM5_IDEALRAW_NATIVE_TYPE_ORACLE_v023.json")\n                        runCatching { oracleFile.writeText(oracle.toString(2)) }\n                        capturedJson = oracleFile.takeIf { it.exists() && it.length() > 0L }\n                        runOnUiThread {\n                            saveJsonButton.isEnabled = capturedJson != null\n                            val classification = oracle.optString("classification", "UNKNOWN")\n                            val resolved = oracle.optString("resolvedNativeType", "UNRESOLVED")\n                            val tagHex = oracle.optString("tagIdHex", "unknown")\n                            val u8Accepted = oracle.optJSONObject("u8Test")?.optBoolean("accepted", false) ?: false\n                            val i32Accepted = oracle.optJSONObject("i32Test")?.optBoolean("accepted", false) ?: false\n                            setStatus(\n                                "STAGE 1.5 DIAGNOSTIC STOP · v0.23 native type oracle\\n" +\n                                    "classification=$classification\\n" +\n                                    "resolvedNativeType=$resolved · tag=$tagHex\\n" +\n                                    "u8Accepted=$u8Accepted · i32Accepted=$i32Accepted\\n" +\n                                    "sessionCreated=false · captureSubmitted=false · vendor request submit=false.\\n" +\n                                    "v0.20 source authority blijft onaangeroerd.",\n                            )\n                        }\n                    }, "truthraw-v023-native-type-oracle").start()\n'''
if needle not in s:
    raise SystemExit('v0.23 Stage-1 success anchor not found')
s = s.replace(needle, replacement, 1)

assert 'TruthRaw · Camera-5 v0.23 · IdealRAW native type oracle' in s
assert 'Camera2IdealRawNativeTypeOracle.probe(LOGICAL_ID)' in s
assert 'STAGE 1.5 DIAGNOSTIC STOP' in s
assert 'capabilityReady = false' in s
assert 'previewButton.isEnabled = false' in s
assert 'captureButton.isEnabled = false' in s
assert 'vendor request submit=false' in s
assert 'MAXIMUM_RESOLUTION vereist' not in s

p.write_text(s)
print('patched', p)
print('bytes', p.stat().st_size)
