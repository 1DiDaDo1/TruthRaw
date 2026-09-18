#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

# Reconstruct exact v0.20 source/payload control first. v0.33 changes exactly one
# upstream vendor session variable, justified by v0.32 representation evidence.
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
    'TruthRaw · 200MP Tele Test v0.33 · EnableInsensorZoom INT32 intervention',
)
s = s.replace('v0.20 payload geometry decoder', 'v0.33 EnableInsensorZoom INT32 single-variable intervention')
s = s.replace('v0.20 payload-geometry UI', 'v0.33 EnableInsensorZoom INT32 intervention UI')
s = s.replace('truthraw-200mp-v020', 'truthraw-200mp-v033')
s = s.replace('truthraw-v020-capability', 'truthraw-v033-capability')
s = s.replace('_v020.dng', '_v033.dng')
s = s.replace('_EVIDENCE_v020.json', '_EVIDENCE_v033.json')
s = s.replace('_v020.${if (contiguous)', '_v033.${if (contiguous)')
s = s.replace('_v020.rawpayload', '_v033.rawpayload')
s = s.replace('_v020.png', '_v033.png')
s = s.replace('staged-evidence.v0.20', 'staged-evidence.v0.33')

field_needle = '''    private var lastPreHalSessionGate: JSONObject? = null
    private var lastPreHalRequestGate: JSONObject? = null

    private var capturedRaw: File? = null
'''
field_replacement = '''    private var lastPreHalSessionGate: JSONObject? = null
    private var lastPreHalRequestGate: JSONObject? = null
    private var lastInsensorZoomInt32Intervention: JSONObject? = null

    private var capturedRaw: File? = null
'''
if field_needle not in s:
    raise SystemExit('v0.33 state-field anchor not found')
s = s.replace(field_needle, field_replacement, 1)

reset_needle = '''        lastPreHalRequestGate = null

        val config = SessionConfiguration(
'''
reset_replacement = '''        lastPreHalRequestGate = null
        lastInsensorZoomInt32Intervention = null

        val config = SessionConfiguration(
'''
if reset_needle not in s:
    raise SystemExit('v0.33 intervention reset anchor not found')
s = s.replace(reset_needle, reset_replacement, 1)

support_needle = '''        val support = runCatching { device.isSessionConfigurationSupported(config) }.getOrNull()
'''
intervention = '''        // v0.33 controlled intervention: Gate A has already frozen the untouched request/session
        // surface. v0.32 resolved EnableInsensorZoom as native camera_metadata INT32
        // (tag 0x801F0013), so this build changes exactly one advertised logical session key
        // to numeric INT32(1). The key name and value are stimuli only; no vendor semantics
        // are promoted. The two other v0.32 candidates remain completely UNSET.
        val insensorZoomAttempt = Camera2InsensorZoomInt32SessionProbe.applyExperiment(
            device = device,
            logical = logical,
            physical = physical,
            config = config,
        )
        lastInsensorZoomInt32Intervention = insensorZoomAttempt.evidence
        if (!insensorZoomAttempt.applied) {
            setStatus(
                "STAGE 3 BLOCKED · v0.33 EnableInsensorZoom INT32 intervention niet veilig toepasbaar.\n" +
                    "classification=${insensorZoomAttempt.evidence.optString("classification", "UNKNOWN")}\n" +
                    "Geen tweede vendor-key geschreven; v0.20 control blijft onaangeroerd.",
            )
            closeCameraResources(keepOutputs = true)
            previewButton.isEnabled = true
            return
        }

        val support = runCatching { device.isSessionConfigurationSupported(config) }.getOrNull()
'''
if support_needle not in s:
    raise SystemExit('v0.33 intervention insertion anchor not found')
s = s.replace(support_needle, intervention, 1)

report_needle = '''                    .put("preHalGateChangedVendorKeys", false)
                    .put("preHalGatePixelAccess", false)
'''
report_replacement = '''                    .put("preHalGateChangedVendorKeys", false)
                    .put("preHalGatePixelAccess", false)
                    .put("controlledVendorInterventionAfterGateA", true)
                    .put("controlledVendorInterventionKeyCount", 1)
                    .put("insensorZoomNativeTypeOracleTagHex", "0x801F0013")
                    .put("insensorZoomNativeTypeOracleResolvedType", "INT32")
                    .put("insensorZoomNativeTypeEvidenceSource", "TRUTHRAW_CAM5_MULTI_KEY_NATIVE_TYPE_ORACLE_v032.json")
                    .put("insensorZoomInt32Intervention", lastInsensorZoomInt32Intervention ?: JSONObject.NULL)
                    .put("otherV032CandidatesWritten", false)
                    .put("insensorZoomExperimentSemanticPromotionAllowed", false)
'''
if report_needle not in s:
    raise SystemExit('v0.33 evidence insertion anchor not found')
s = s.replace(report_needle, report_replacement, 1)

status_needle = '''                    "Gate A/B pre-HAL route fingerprint=${if (lastPreHalSessionGate != null && lastPreHalRequestGate != null) "captured" else "partial"}\n" +
'''
status_replacement = '''                    "Gate A baseline=${if (lastPreHalSessionGate != null) "captured" else "missing"} · EnableInsensorZoom INT32(1)=${if (lastInsensorZoomInt32Intervention?.optBoolean("applied", false) == true) "attached" else "not attached"}\n" +
                    "Gate B request fingerprint=${if (lastPreHalRequestGate != null) "captured" else "missing"}\n" +
'''
if status_needle not in s:
    raise SystemExit('v0.33 status insertion anchor not found')
s = s.replace(status_needle, status_replacement, 1)

assert 'TruthRaw · 200MP Tele Test v0.33 · EnableInsensorZoom INT32 intervention' in s
assert 'Camera2InsensorZoomInt32SessionProbe.applyExperiment(' in s
assert s.index('Camera2PreHalGate.observeSession(') < s.index('Camera2InsensorZoomInt32SessionProbe.applyExperiment(')
assert s.index('Camera2InsensorZoomInt32SessionProbe.applyExperiment(') < s.index('device.isSessionConfigurationSupported(config)')
assert s.index('persistOriginalRawBuffer(image, stamp)') < s.index('Camera2EnvelopeProbe.observe(image, physical, physicalResult)')
assert s.index('RawSensorRasterAudit.audit(') < s.index('RawPayloadGeometryDecoder.decode(')
assert s.count('Camera2InsensorZoomInt32SessionProbe.applyExperiment(') == 1
assert 'controlledVendorInterventionKeyCount' in s
assert 'insensorZoomNativeTypeOracleResolvedType' in s
assert 'otherV032CandidatesWritten' in s
assert 'MAXIMUM_RESOLUTION vereist' not in s

p.write_text(s)
print('patched', p)
print('bytes', p.stat().st_size)
