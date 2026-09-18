#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

# Reconstruct exact v0.20 source/payload authority first. v0.34 changes exactly one
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
    'TruthRaw · 200MP Tele Test v0.34 · SnapshotOnlyInsensorZoom INT32 intervention',
)
s = s.replace('v0.20 payload geometry decoder', 'v0.34 SnapshotOnlyInsensorZoom INT32 single-variable intervention')
s = s.replace('v0.20 payload-geometry UI', 'v0.34 SnapshotOnlyInsensorZoom INT32 intervention UI')
s = s.replace('truthraw-200mp-v020', 'truthraw-200mp-v034')
s = s.replace('truthraw-v020-capability', 'truthraw-v034-capability')
s = s.replace('_v020.dng', '_v034.dng')
s = s.replace('_EVIDENCE_v020.json', '_EVIDENCE_v034.json')
s = s.replace('_v020.${if (contiguous)', '_v034.${if (contiguous)')
s = s.replace('_v020.rawpayload', '_v034.rawpayload')
s = s.replace('_v020.png', '_v034.png')
s = s.replace('staged-evidence.v0.20', 'staged-evidence.v0.34')

field_needle = '''    private var lastPreHalSessionGate: JSONObject? = null
    private var lastPreHalRequestGate: JSONObject? = null

    private var capturedRaw: File? = null
'''
field_replacement = '''    private var lastPreHalSessionGate: JSONObject? = null
    private var lastPreHalRequestGate: JSONObject? = null
    private var lastSnapshotOnlyInsensorZoomInt32Intervention: JSONObject? = null

    private var capturedRaw: File? = null
'''
if field_needle not in s:
    raise SystemExit('v0.34 state-field anchor not found')
s = s.replace(field_needle, field_replacement, 1)

reset_needle = '''        lastPreHalRequestGate = null

        val config = SessionConfiguration(
'''
reset_replacement = '''        lastPreHalRequestGate = null
        lastSnapshotOnlyInsensorZoomInt32Intervention = null

        val config = SessionConfiguration(
'''
if reset_needle not in s:
    raise SystemExit('v0.34 intervention reset anchor not found')
s = s.replace(reset_needle, reset_replacement, 1)

support_needle = '''        val support = runCatching { device.isSessionConfigurationSupported(config) }.getOrNull()
'''
intervention = r'''        // v0.34 controlled intervention: Gate A has already frozen the untouched request/session
        // surface. v0.32 resolved EnableSnapshotOnlyInsensorZoom as native camera_metadata
        // INT32 (tag 0x801F0014). This build therefore changes exactly one advertised logical
        // session key to numeric INT32(1). The name/value remain stimuli only; no vendor
        // semantics are promoted. EnableInsensorZoom and EnableMCXMasterCb remain UNSET.
        val snapshotOnlyInsensorZoomAttempt =
            Camera2SnapshotOnlyInsensorZoomInt32SessionProbe.applyExperiment(
                device = device,
                logical = logical,
                physical = physical,
                config = config,
            )
        lastSnapshotOnlyInsensorZoomInt32Intervention = snapshotOnlyInsensorZoomAttempt.evidence
        if (!snapshotOnlyInsensorZoomAttempt.applied) {
            setStatus(
                "STAGE 3 BLOCKED · v0.34 SnapshotOnlyInsensorZoom INT32 intervention niet veilig toepasbaar.\n" +
                    "classification=${snapshotOnlyInsensorZoomAttempt.evidence.optString("classification", "UNKNOWN")}\n" +
                    "Geen tweede vendor-key geschreven; v0.20 control blijft onaangeroerd.",
            )
            closeCameraResources(keepOutputs = true)
            previewButton.isEnabled = true
            return
        }

        val support = runCatching { device.isSessionConfigurationSupported(config) }.getOrNull()
'''
if support_needle not in s:
    raise SystemExit('v0.34 intervention insertion anchor not found')
s = s.replace(support_needle, intervention, 1)

report_needle = '''                    .put("preHalGateChangedVendorKeys", false)
                    .put("preHalGatePixelAccess", false)
'''
report_replacement = '''                    .put("preHalGateChangedVendorKeys", false)
                    .put("preHalGatePixelAccess", false)
                    .put("controlledVendorInterventionAfterGateA", true)
                    .put("controlledVendorInterventionKeyCount", 1)
                    .put("snapshotOnlyInsensorZoomNativeTypeOracleTagHex", "0x801F0014")
                    .put("snapshotOnlyInsensorZoomNativeTypeOracleResolvedType", "INT32")
                    .put("snapshotOnlyInsensorZoomNativeTypeEvidenceSource", "TRUTHRAW_CAM5_MULTI_KEY_NATIVE_TYPE_ORACLE_v032.json")
                    .put("snapshotOnlyInsensorZoomInt32Intervention", lastSnapshotOnlyInsensorZoomInt32Intervention ?: JSONObject.NULL)
                    .put("enableInsensorZoomWritten", false)
                    .put("enableMcxMasterCbWritten", false)
                    .put("otherV032CandidatesWritten", false)
                    .put("snapshotOnlyInsensorZoomExperimentSemanticPromotionAllowed", false)
                    .put("predecessorV033TopologyDifferential", false)
'''
if report_needle not in s:
    raise SystemExit('v0.34 evidence insertion anchor not found')
s = s.replace(report_needle, report_replacement, 1)

assert 'TruthRaw · 200MP Tele Test v0.34 · SnapshotOnlyInsensorZoom INT32 intervention' in s
assert 'Camera2SnapshotOnlyInsensorZoomInt32SessionProbe.applyExperiment(' in s
assert s.index('Camera2PreHalGate.observeSession(') < s.index('Camera2SnapshotOnlyInsensorZoomInt32SessionProbe.applyExperiment(')
assert s.index('Camera2SnapshotOnlyInsensorZoomInt32SessionProbe.applyExperiment(') < s.index('device.isSessionConfigurationSupported(config)')
assert s.index('persistOriginalRawBuffer(image, stamp)') < s.index('Camera2EnvelopeProbe.observe(image, physical, physicalResult)')
assert s.index('RawSensorRasterAudit.audit(') < s.index('RawPayloadGeometryDecoder.decode(')
assert s.count('Camera2SnapshotOnlyInsensorZoomInt32SessionProbe.applyExperiment(') == 1
assert 'Camera2InsensorZoomInt32SessionProbe.applyExperiment(' not in s
assert 'controlledVendorInterventionKeyCount' in s
assert 'snapshotOnlyInsensorZoomNativeTypeOracleResolvedType' in s
assert 'enableInsensorZoomWritten' in s
assert 'enableMcxMasterCbWritten' in s
assert 'otherV032CandidatesWritten' in s
assert 'MAXIMUM_RESOLUTION vereist' not in s

p.write_text(s)
print('patched', p)
print('bytes', p.stat().st_size)
