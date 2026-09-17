#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

# Reconstruct exact v0.20 control first. v0.24 changes exactly one upstream session variable.
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
    'TruthRaw · 200MP Tele Test v0.24 · Camera-5 IdealRAW BYTE intervention',
)
s = s.replace('v0.20 payload geometry decoder', 'v0.24 IdealRAW BYTE single-variable intervention')
s = s.replace('v0.20 payload-geometry UI', 'v0.24 IdealRAW BYTE intervention UI')
s = s.replace('truthraw-200mp-v020', 'truthraw-200mp-v024')
s = s.replace('truthraw-v020-capability', 'truthraw-v024-capability')
s = s.replace('_v020.dng', '_v024.dng')
s = s.replace('_EVIDENCE_v020.json', '_EVIDENCE_v024.json')
s = s.replace('_v020.${if (contiguous)', '_v024.${if (contiguous)')
s = s.replace('_v020.rawpayload', '_v024.rawpayload')
s = s.replace('_v020.png', '_v024.png')
s = s.replace('staged-evidence.v0.20', 'staged-evidence.v0.24')

field_needle = '''    private var lastPreHalSessionGate: JSONObject? = null\n    private var lastPreHalRequestGate: JSONObject? = null\n\n    private var capturedRaw: File? = null\n'''
field_replacement = '''    private var lastPreHalSessionGate: JSONObject? = null\n    private var lastPreHalRequestGate: JSONObject? = null\n    private var lastIdealRawByteIntervention: JSONObject? = null\n\n    private var capturedRaw: File? = null\n'''
if field_needle not in s:
    raise SystemExit('v0.24 state-field anchor not found')
s = s.replace(field_needle, field_replacement, 1)

reset_needle = '''        lastPreHalRequestGate = null\n\n        val config = SessionConfiguration(\n'''
reset_replacement = '''        lastPreHalRequestGate = null\n        lastIdealRawByteIntervention = null\n\n        val config = SessionConfiguration(\n'''
if reset_needle not in s:
    raise SystemExit('v0.24 intervention reset anchor not found')
s = s.replace(reset_needle, reset_replacement, 1)

support_needle = '''        val support = runCatching { device.isSessionConfigurationSupported(config) }.getOrNull()\n'''
intervention = '''        // v0.24 controlled intervention: Gate A has already frozen the untouched request/session\n        // surface. v0.23 resolved EnableIdealRAW as native camera_metadata BYTE (tag 0x801F0027),\n        // so this build changes exactly one advertised logical session key to one BYTE value = 1.\n        val idealRawAttempt = Camera2IdealRawByteSessionProbe.applyExperiment(\n            device = device,\n            logical = logical,\n            physical = physical,\n            config = config,\n        )\n        lastIdealRawByteIntervention = idealRawAttempt.evidence\n        if (!idealRawAttempt.applied) {\n            setStatus(\n                "STAGE 3 BLOCKED · v0.24 IdealRAW BYTE intervention niet veilig toepasbaar.\\n" +\n                    "classification=${idealRawAttempt.evidence.optString(\"classification\", \"UNKNOWN\")}\\n" +\n                    "Geen tweede vendor-key; v0.20 control blijft onaangeroerd.",\n            )\n            closeCameraResources(keepOutputs = true)\n            previewButton.isEnabled = true\n            return\n        }\n\n        val support = runCatching { device.isSessionConfigurationSupported(config) }.getOrNull()\n'''
if support_needle not in s:
    raise SystemExit('v0.24 intervention insertion anchor not found')
s = s.replace(support_needle, intervention, 1)

report_needle = '''                    .put("preHalGateChangedVendorKeys", false)\n                    .put("preHalGatePixelAccess", false)\n'''
report_replacement = '''                    .put("preHalGateChangedVendorKeys", false)\n                    .put("preHalGatePixelAccess", false)\n                    .put("controlledVendorInterventionAfterGateA", true)\n                    .put("controlledVendorInterventionKeyCount", 1)\n                    .put("idealRawNativeTypeOracleTagHex", "0x801F0027")\n                    .put("idealRawNativeTypeOracleResolvedType", "BYTE")\n                    .put("idealRawByteIntervention", lastIdealRawByteIntervention ?: JSONObject.NULL)\n                    .put("idealRawExperimentSemanticPromotionAllowed", false)\n'''
if report_needle not in s:
    raise SystemExit('v0.24 evidence insertion anchor not found')
s = s.replace(report_needle, report_replacement, 1)

status_needle = '''                    "Gate A/B pre-HAL route fingerprint=${if (lastPreHalSessionGate != null && lastPreHalRequestGate != null) "captured" else "partial"}\\n" +\n'''
status_replacement = '''                    "Gate A baseline=${if (lastPreHalSessionGate != null) "captured" else "missing"} · IdealRAW BYTE(1)=${if (lastIdealRawByteIntervention?.optBoolean("applied", false) == true) "attached" else "not attached"}\\n" +\n                    "Gate B request fingerprint=${if (lastPreHalRequestGate != null) "captured" else "missing"}\\n" +\n'''
if status_needle not in s:
    raise SystemExit('v0.24 status insertion anchor not found')
s = s.replace(status_needle, status_replacement, 1)

assert 'TruthRaw · 200MP Tele Test v0.24 · Camera-5 IdealRAW BYTE intervention' in s
assert 'Camera2IdealRawByteSessionProbe.applyExperiment(' in s
assert s.index('Camera2PreHalGate.observeSession(') < s.index('Camera2IdealRawByteSessionProbe.applyExperiment(')
assert s.index('Camera2IdealRawByteSessionProbe.applyExperiment(') < s.index('device.isSessionConfigurationSupported(config)')
assert s.index('persistOriginalRawBuffer(image, stamp)') < s.index('Camera2EnvelopeProbe.observe(image, physical, physicalResult)')
assert s.index('RawSensorRasterAudit.audit(') < s.index('RawPayloadGeometryDecoder.decode(')
assert s.count('Camera2IdealRawByteSessionProbe.applyExperiment(') == 1
assert 'controlledVendorInterventionKeyCount' in s
assert 'idealRawNativeTypeOracleResolvedType' in s
assert 'MAXIMUM_RESOLUTION vereist' not in s

p.write_text(s)
print('patched', p)
print('bytes', p.stat().st_size)
