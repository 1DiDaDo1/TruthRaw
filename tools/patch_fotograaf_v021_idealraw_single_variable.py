#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

# Reconstruct exact v0.20 first. v0.21 changes one upstream vendor session variable only;
# the trusted v0.14 -> v0.17 -> v0.19 -> v0.20 source-seal/audit chain remains intact.
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
    'TruthRaw · 200MP Tele Test v0.21 · Camera-5 IdealRAW single-variable',
)
s = s.replace('v0.20 payload geometry decoder', 'v0.21 IdealRAW single-variable route probe')
s = s.replace('v0.20 payload-geometry UI', 'v0.21 IdealRAW single-variable UI')
s = s.replace('truthraw-200mp-v020', 'truthraw-200mp-v021')
s = s.replace('truthraw-v020-capability', 'truthraw-v021-capability')
s = s.replace('_v020.dng', '_v021.dng')
s = s.replace('_EVIDENCE_v020.json', '_EVIDENCE_v021.json')
s = s.replace('_v020.${if (contiguous)', '_v021.${if (contiguous)')
s = s.replace('_v020.rawpayload', '_v021.rawpayload')
s = s.replace('_v020.png', '_v021.png')
s = s.replace('staged-evidence.v0.20', 'staged-evidence.v0.21')

field_needle = '''    private var lastPreHalSessionGate: JSONObject? = null\n    private var lastPreHalRequestGate: JSONObject? = null\n\n    private var capturedRaw: File? = null\n'''
field_replacement = '''    private var lastPreHalSessionGate: JSONObject? = null\n    private var lastPreHalRequestGate: JSONObject? = null\n    private var lastIdealRawSingleVariableExperiment: JSONObject? = null\n\n    private var capturedRaw: File? = null\n'''
if field_needle not in s:
    raise SystemExit('v0.21 state-field anchor not found')
s = s.replace(field_needle, field_replacement, 1)

reset_needle = '''        lastPreHalRequestGate = null\n\n        val config = SessionConfiguration(\n'''
reset_replacement = '''        lastPreHalRequestGate = null\n        lastIdealRawSingleVariableExperiment = null\n\n        val config = SessionConfiguration(\n'''
if reset_needle not in s:
    raise SystemExit('v0.21 experiment reset anchor not found')
s = s.replace(reset_needle, reset_replacement, 1)

support_needle = '''        val support = runCatching { device.isSessionConfigurationSupported(config) }.getOrNull()\n'''
intervention = '''        // v0.21 controlled intervention: after Gate A has frozen the unmodified request/session\n        // surface, change exactly one advertised vendor session key. The helper fails closed unless\n        // the runtime Java type is a safe one-byte domain and builder readback is exactly 1.\n        val idealRawAttempt = Camera2IdealRawSessionProbe.applyExperiment(\n            device = device,\n            logical = logical,\n            physical = physical,\n            config = config,\n        )\n        lastIdealRawSingleVariableExperiment = idealRawAttempt.evidence\n        if (!idealRawAttempt.applied) {\n            setStatus(\n                "STAGE 3 BLOCKED · v0.21 IdealRAW single-variable probe niet veilig toepasbaar.\\n" +\n                    "classification=${idealRawAttempt.evidence.optString(\"classification\", \"UNKNOWN\")}\\n" +\n                    "Geen vendor-key gok; v0.20 control blijft onaangeroerd.",\n            )\n            closeCameraResources(keepOutputs = true)\n            previewButton.isEnabled = true\n            return\n        }\n\n        val support = runCatching { device.isSessionConfigurationSupported(config) }.getOrNull()\n'''
if support_needle not in s:
    raise SystemExit('v0.21 intervention insertion anchor not found')
s = s.replace(support_needle, intervention, 1)

report_needle = '''                    .put("preHalGateChangedVendorKeys", false)\n                    .put("preHalGatePixelAccess", false)\n'''
report_replacement = '''                    .put("preHalGateChangedVendorKeys", false)\n                    .put("preHalGatePixelAccess", false)\n                    .put("controlledVendorInterventionAfterGateA", true)\n                    .put("controlledVendorInterventionKeyCount", 1)\n                    .put("idealRawSingleVariableExperiment", lastIdealRawSingleVariableExperiment ?: JSONObject.NULL)\n                    .put("idealRawExperimentSemanticPromotionAllowed", false)\n'''
if report_needle not in s:
    raise SystemExit('v0.21 evidence insertion anchor not found')
s = s.replace(report_needle, report_replacement, 1)

status_needle = '''                    "Gate A/B pre-HAL route fingerprint=${if (lastPreHalSessionGate != null && lastPreHalRequestGate != null) "captured" else "partial"}\\n" +\n'''
status_replacement = '''                    "Gate A baseline fingerprint=${if (lastPreHalSessionGate != null) "captured" else "missing"} · IdealRAW=1 session intervention=${if (lastIdealRawSingleVariableExperiment?.optBoolean("applied", false) == true) "applied" else "not applied"}\\n" +\n                    "Gate B request fingerprint=${if (lastPreHalRequestGate != null) "captured" else "missing"}\\n" +\n'''
if status_needle not in s:
    raise SystemExit('v0.21 status insertion anchor not found')
s = s.replace(status_needle, status_replacement, 1)

assert 'TruthRaw · 200MP Tele Test v0.21 · Camera-5 IdealRAW single-variable' in s
assert 'Camera2IdealRawSessionProbe.applyExperiment(' in s
assert s.index('Camera2PreHalGate.observeSession(') < s.index('Camera2IdealRawSessionProbe.applyExperiment(')
assert s.index('Camera2IdealRawSessionProbe.applyExperiment(') < s.index('device.isSessionConfigurationSupported(config)')
assert s.index('persistOriginalRawBuffer(image, stamp)') < s.index('Camera2EnvelopeProbe.observe(image, physical, physicalResult)')
assert s.index('RawSensorRasterAudit.audit(') < s.index('RawPayloadGeometryDecoder.decode(')
assert 'controlledVendorInterventionKeyCount' in s
assert 'idealRawExperimentSemanticPromotionAllowed' in s
assert 'MAXIMUM_RESOLUTION vereist' not in s

p.write_text(s)
print('patched', p)
print('bytes', p.stat().st_size)
