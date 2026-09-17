#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

# Reconstruct exact v0.20 control first. v0.26 changes exactly one upstream session variable.
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
    'TruthRaw · 200MP Tele Test v0.26 · Camera-5 RawCbSourceType INT32 intervention',
)
s = s.replace('v0.20 payload geometry decoder', 'v0.26 RawCbSourceType INT32 single-variable intervention')
s = s.replace('v0.20 payload-geometry UI', 'v0.26 RawCbSourceType INT32 intervention UI')
s = s.replace('truthraw-200mp-v020', 'truthraw-200mp-v026')
s = s.replace('truthraw-v020-capability', 'truthraw-v026-capability')
s = s.replace('_v020.dng', '_v026.dng')
s = s.replace('_EVIDENCE_v020.json', '_EVIDENCE_v026.json')
s = s.replace('_v020.${if (contiguous)', '_v026.${if (contiguous)')
s = s.replace('_v020.rawpayload', '_v026.rawpayload')
s = s.replace('_v020.png', '_v026.png')
s = s.replace('staged-evidence.v0.20', 'staged-evidence.v0.26')

field_needle = '''    private var lastPreHalSessionGate: JSONObject? = null\n    private var lastPreHalRequestGate: JSONObject? = null\n\n    private var capturedRaw: File? = null\n'''
field_replacement = '''    private var lastPreHalSessionGate: JSONObject? = null\n    private var lastPreHalRequestGate: JSONObject? = null\n    private var lastRawCbInt32Intervention: JSONObject? = null\n\n    private var capturedRaw: File? = null\n'''
if field_needle not in s:
    raise SystemExit('v0.26 state-field anchor not found')
s = s.replace(field_needle, field_replacement, 1)

reset_needle = '''        lastPreHalRequestGate = null\n\n        val config = SessionConfiguration(\n'''
reset_replacement = '''        lastPreHalRequestGate = null\n        lastRawCbInt32Intervention = null\n\n        val config = SessionConfiguration(\n'''
if reset_needle not in s:
    raise SystemExit('v0.26 intervention reset anchor not found')
s = s.replace(reset_needle, reset_replacement, 1)

support_needle = '''        val support = runCatching { device.isSessionConfigurationSupported(config) }.getOrNull()\n'''
intervention = '''        // v0.26 controlled intervention: Gate A has already frozen the untouched request/session\n        // surface. v0.25 resolved RawCbSourceType as native camera_metadata INT32 (tag 0x801F0009),\n        // so this build changes exactly one advertised logical session key to one INT32 value = 1.\n        // Numeric value 1 has no promoted vendor semantics; it is only the controlled A/B intervention.\n        val rawCbAttempt = Camera2RawCbSourceTypeInt32SessionProbe.applyExperiment(\n            device = device,\n            logical = logical,\n            physical = physical,\n            config = config,\n        )\n        lastRawCbInt32Intervention = rawCbAttempt.evidence\n        if (!rawCbAttempt.applied) {\n            setStatus(\n                "STAGE 3 BLOCKED · v0.26 RawCbSourceType INT32 intervention niet veilig toepasbaar.\\n" +\n                    "classification=${rawCbAttempt.evidence.optString(\"classification\", \"UNKNOWN\")}\\n" +\n                    "Geen tweede vendor-key; v0.20 control blijft onaangeroerd.",\n            )\n            closeCameraResources(keepOutputs = true)\n            previewButton.isEnabled = true\n            return\n        }\n\n        val support = runCatching { device.isSessionConfigurationSupported(config) }.getOrNull()\n'''
if support_needle not in s:
    raise SystemExit('v0.26 intervention insertion anchor not found')
s = s.replace(support_needle, intervention, 1)

report_needle = '''                    .put("preHalGateChangedVendorKeys", false)\n                    .put("preHalGatePixelAccess", false)\n'''
report_replacement = '''                    .put("preHalGateChangedVendorKeys", false)\n                    .put("preHalGatePixelAccess", false)\n                    .put("controlledVendorInterventionAfterGateA", true)\n                    .put("controlledVendorInterventionKeyCount", 1)\n                    .put("rawCbSourceTypeNativeTypeOracleTagHex", "0x801F0009")\n                    .put("rawCbSourceTypeNativeTypeOracleResolvedType", "INT32")\n                    .put("rawCbSourceTypeInt32Intervention", lastRawCbInt32Intervention ?: JSONObject.NULL)\n                    .put("rawCbSourceTypeExperimentSemanticPromotionAllowed", false)\n'''
if report_needle not in s:
    raise SystemExit('v0.26 evidence insertion anchor not found')
s = s.replace(report_needle, report_replacement, 1)

status_needle = '''                    "Gate A/B pre-HAL route fingerprint=${if (lastPreHalSessionGate != null && lastPreHalRequestGate != null) "captured" else "partial"}\\n" +\n'''
status_replacement = '''                    "Gate A baseline=${if (lastPreHalSessionGate != null) "captured" else "missing"} · RawCbSourceType INT32(1)=${if (lastRawCbInt32Intervention?.optBoolean("applied", false) == true) "attached" else "not attached"}\\n" +\n                    "Gate B request fingerprint=${if (lastPreHalRequestGate != null) "captured" else "missing"}\\n" +\n'''
if status_needle not in s:
    raise SystemExit('v0.26 status insertion anchor not found')
s = s.replace(status_needle, status_replacement, 1)

assert 'TruthRaw · 200MP Tele Test v0.26 · Camera-5 RawCbSourceType INT32 intervention' in s
assert 'Camera2RawCbSourceTypeInt32SessionProbe.applyExperiment(' in s
assert s.index('Camera2PreHalGate.observeSession(') < s.index('Camera2RawCbSourceTypeInt32SessionProbe.applyExperiment(')
assert s.index('Camera2RawCbSourceTypeInt32SessionProbe.applyExperiment(') < s.index('device.isSessionConfigurationSupported(config)')
assert s.index('persistOriginalRawBuffer(image, stamp)') < s.index('Camera2EnvelopeProbe.observe(image, physical, physicalResult)')
assert s.index('RawSensorRasterAudit.audit(') < s.index('RawPayloadGeometryDecoder.decode(')
assert s.count('Camera2RawCbSourceTypeInt32SessionProbe.applyExperiment(') == 1
assert 'controlledVendorInterventionKeyCount' in s
assert 'rawCbSourceTypeNativeTypeOracleResolvedType' in s
assert 'MAXIMUM_RESOLUTION vereist' not in s

p.write_text(s)
print('patched', p)
print('bytes', p.stat().st_size)
