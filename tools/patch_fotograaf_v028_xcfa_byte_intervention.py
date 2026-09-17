#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

# Reconstruct exact v0.20 control first. v0.28 changes exactly one upstream session variable.
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
    'TruthRaw · 200MP Tele Test v0.28 · Camera-5 XCFA BYTE intervention',
)
s = s.replace('v0.20 payload geometry decoder', 'v0.28 XCFA BYTE single-variable intervention')
s = s.replace('v0.20 payload-geometry UI', 'v0.28 XCFA BYTE intervention UI')
s = s.replace('truthraw-200mp-v020', 'truthraw-200mp-v028')
s = s.replace('truthraw-v020-capability', 'truthraw-v028-capability')
s = s.replace('_v020.dng', '_v028.dng')
s = s.replace('_EVIDENCE_v020.json', '_EVIDENCE_v028.json')
s = s.replace('_v020.${if (contiguous)', '_v028.${if (contiguous)')
s = s.replace('_v020.rawpayload', '_v028.rawpayload')
s = s.replace('_v020.png', '_v028.png')
s = s.replace('staged-evidence.v0.20', 'staged-evidence.v0.28')

field_needle = '''    private var lastPreHalSessionGate: JSONObject? = null\n    private var lastPreHalRequestGate: JSONObject? = null\n\n    private var capturedRaw: File? = null\n'''
field_replacement = '''    private var lastPreHalSessionGate: JSONObject? = null\n    private var lastPreHalRequestGate: JSONObject? = null\n    private var lastXcfaByteIntervention: JSONObject? = null\n\n    private var capturedRaw: File? = null\n'''
if field_needle not in s:
    raise SystemExit('v0.28 state-field anchor not found')
s = s.replace(field_needle, field_replacement, 1)

reset_needle = '''        lastPreHalRequestGate = null\n\n        val config = SessionConfiguration(\n'''
reset_replacement = '''        lastPreHalRequestGate = null\n        lastXcfaByteIntervention = null\n\n        val config = SessionConfiguration(\n'''
if reset_needle not in s:
    raise SystemExit('v0.28 intervention reset anchor not found')
s = s.replace(reset_needle, reset_replacement, 1)

support_needle = '''        val support = runCatching { device.isSessionConfigurationSupported(config) }.getOrNull()\n'''
intervention = '''        // v0.28 controlled intervention: Gate A has already frozen the untouched request/session\n        // surface. v0.27 resolved EnableXCFAOptimization as native camera_metadata BYTE\n        // (tag 0x801F0036), so this build changes exactly one advertised session key to BYTE(1).\n        val xcfaAttempt = Camera2XcfaByteSessionProbe.applyExperiment(\n            device = device,\n            logical = logical,\n            physical = physical,\n            config = config,\n        )\n        lastXcfaByteIntervention = xcfaAttempt.evidence\n        if (!xcfaAttempt.applied) {\n            setStatus(\n                "STAGE 3 BLOCKED · v0.28 XCFA BYTE intervention niet veilig toepasbaar.\\n" +\n                    "classification=${xcfaAttempt.evidence.optString(\"classification\", \"UNKNOWN\")}\\n" +\n                    "Geen tweede vendor-key; v0.20 control blijft onaangeroerd.",\n            )\n            closeCameraResources(keepOutputs = true)\n            previewButton.isEnabled = true\n            return\n        }\n\n        val support = runCatching { device.isSessionConfigurationSupported(config) }.getOrNull()\n'''
if support_needle not in s:
    raise SystemExit('v0.28 intervention insertion anchor not found')
s = s.replace(support_needle, intervention, 1)

report_needle = '''                    .put("preHalGateChangedVendorKeys", false)\n                    .put("preHalGatePixelAccess", false)\n'''
report_replacement = '''                    .put("preHalGateChangedVendorKeys", false)\n                    .put("preHalGatePixelAccess", false)\n                    .put("controlledVendorInterventionAfterGateA", true)\n                    .put("controlledVendorInterventionKeyCount", 1)\n                    .put("xcfaNativeTypeOracleTagHex", "0x801F0036")\n                    .put("xcfaNativeTypeOracleResolvedType", "BYTE")\n                    .put("xcfaByteIntervention", lastXcfaByteIntervention ?: JSONObject.NULL)\n                    .put("xcfaExperimentSemanticPromotionAllowed", false)\n'''
if report_needle not in s:
    raise SystemExit('v0.28 evidence insertion anchor not found')
s = s.replace(report_needle, report_replacement, 1)

status_needle = '''                    "Gate A/B pre-HAL route fingerprint=${if (lastPreHalSessionGate != null && lastPreHalRequestGate != null) "captured" else "partial"}\\n" +\n'''
status_replacement = '''                    "Gate A baseline=${if (lastPreHalSessionGate != null) "captured" else "missing"} · XCFA BYTE(1)=${if (lastXcfaByteIntervention?.optBoolean("applied", false) == true) "attached" else "not attached"}\\n" +\n                    "Gate B request fingerprint=${if (lastPreHalRequestGate != null) "captured" else "missing"}\\n" +\n'''
if status_needle not in s:
    raise SystemExit('v0.28 status insertion anchor not found')
s = s.replace(status_needle, status_replacement, 1)

assert 'TruthRaw · 200MP Tele Test v0.28 · Camera-5 XCFA BYTE intervention' in s
assert 'Camera2XcfaByteSessionProbe.applyExperiment(' in s
assert s.index('Camera2PreHalGate.observeSession(') < s.index('Camera2XcfaByteSessionProbe.applyExperiment(')
assert s.index('Camera2XcfaByteSessionProbe.applyExperiment(') < s.index('device.isSessionConfigurationSupported(config)')
assert s.index('persistOriginalRawBuffer(image, stamp)') < s.index('Camera2EnvelopeProbe.observe(image, physical, physicalResult)')
assert s.index('RawSensorRasterAudit.audit(') < s.index('RawPayloadGeometryDecoder.decode(')
assert s.count('Camera2XcfaByteSessionProbe.applyExperiment(') == 1
assert 'controlledVendorInterventionKeyCount' in s
assert 'xcfaNativeTypeOracleResolvedType' in s
assert 'MAXIMUM_RESOLUTION vereist' not in s

p.write_text(s)
print('patched', p)
print('bytes', p.stat().st_size)
