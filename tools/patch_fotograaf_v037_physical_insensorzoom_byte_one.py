#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

# Reconstruct exact v0.20 source/payload authority first.
# v0.37 changes exactly one physical-only vendor variable, but only after
# Gate A and only if attachment feasibility passes.
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
    'TruthRaw · 200MP Tele Test v0.37 · physical-only inSensorZoomEnable BYTE(1)',
)
s = s.replace('v0.20 payload geometry decoder', 'v0.37 physical-only BYTE attachment + conditional capture')
s = s.replace('v0.20 payload-geometry UI', 'v0.37 physical-only BYTE conditional intervention UI')
s = s.replace('truthraw-200mp-v020', 'truthraw-200mp-v037')
s = s.replace('truthraw-v020-capability', 'truthraw-v037-capability')
s = s.replace('_v020.dng', '_v037.dng')
s = s.replace('_EVIDENCE_v020.json', '_EVIDENCE_v037.json')
s = s.replace('_v020.${if (contiguous)', '_v037.${if (contiguous)')
s = s.replace('_v020.rawpayload', '_v037.rawpayload')
s = s.replace('_v020.png', '_v037.png')
s = s.replace('staged-evidence.v0.20', 'staged-evidence.v0.37')

field_needle = '''    private var lastPreHalSessionGate: JSONObject? = null
    private var lastPreHalRequestGate: JSONObject? = null

    private var capturedRaw: File? = null
'''
field_replacement = '''    private var lastPreHalSessionGate: JSONObject? = null
    private var lastPreHalRequestGate: JSONObject? = null
    private var lastPhysicalInSensorZoomByteIntervention: JSONObject? = null

    private var capturedRaw: File? = null
'''
if field_needle not in s:
    raise SystemExit('v0.37 state-field anchor not found')
s = s.replace(field_needle, field_replacement, 1)

reset_needle = '''        lastPreHalRequestGate = null

        val config = SessionConfiguration(
'''
reset_replacement = '''        lastPreHalRequestGate = null
        lastPhysicalInSensorZoomByteIntervention = null

        val config = SessionConfiguration(
'''
if reset_needle not in s:
    raise SystemExit('v0.37 intervention reset anchor not found')
s = s.replace(reset_needle, reset_replacement, 1)

support_needle = '''        val support = runCatching { device.isSessionConfigurationSupported(config) }.getOrNull()
'''
intervention = r'''        // v0.37 controlled physical-only attachment-feasibility experiment.
        // Gate A has already frozen the untouched request/session surface. v0.36 resolved
        // inSensorZoomEnable as native BYTE (tag 0x801F0029) and proved it is advertised
        // only on physical Camera-5 request/session surfaces. The vendor name is NOT used
        // as semantic evidence. We write exactly one BYTE(1) stimulus.
        //
        // Fail-closed rule:
        // - if builder/request representation fails -> no capture
        // - if SessionConfiguration.setSessionParameters rejects the request -> no capture
        // - only successful attachment permits the unchanged v0.20 capture chain to continue
        val physicalByteAttempt = Camera2PhysicalInSensorZoomByteSessionProbe.applyExperiment(
            device = device,
            logical = logical,
            physical = physical,
            config = config,
        )
        lastPhysicalInSensorZoomByteIntervention = physicalByteAttempt.evidence
        if (!physicalByteAttempt.applied) {
            val blockedJson = JSONObject()
                .put("schema", "truthraw.camera5-v037-attachment-feasibility-result.v0.37")
                .put("experimentVersion", "v0.37")
                .put("controlReference", "TruthRaw v0.20 unchanged")
                .put("intervention", physicalByteAttempt.evidence)
                .put("capturePerformed", false)
                .put("rawPixelAccessAfterIntervention", false)
                .put("sourceMutation", false)
                .put("semanticPromotionAllowed", false)
            val blockedFile = File(cacheDir, "TRUTHRAW_CAM5_V037_ATTACHMENT_RESULT.json")
            runCatching { blockedFile.writeText(blockedJson.toString(2)) }
            capturedJson = blockedFile.takeIf { it.exists() && it.length() > 0L }
            saveJsonButton.isEnabled = capturedJson != null

            setStatus(
                "STAGE 3 STOP · v0.37 physical-only BYTE attachment niet toegepast.\n" +
                    "classification=${physicalByteAttempt.evidence.optString("classification", "UNKNOWN")}\n" +
                    "Dit is geldige evidence. Geen capture, geen RAW gelezen, v0.20 authority onaangeroerd.",
            )
            closeCameraResources(keepOutputs = true)
            previewButton.isEnabled = true
            return
        }

        val support = runCatching { device.isSessionConfigurationSupported(config) }.getOrNull()
'''
if support_needle not in s:
    raise SystemExit('v0.37 intervention insertion anchor not found')
s = s.replace(support_needle, intervention, 1)

report_needle = '''                    .put("preHalGateChangedVendorKeys", false)
                    .put("preHalGatePixelAccess", false)
'''
report_replacement = '''                    .put("preHalGateChangedVendorKeys", false)
                    .put("preHalGatePixelAccess", false)
                    .put("controlledVendorInterventionAfterGateA", true)
                    .put("controlledVendorInterventionKeyCount", 1)
                    .put("physicalInSensorZoomNativeTypeOracleTagHex", "0x801F0029")
                    .put("physicalInSensorZoomNativeTypeOracleResolvedType", "BYTE")
                    .put("physicalInSensorZoomNativeTypeEvidenceSource", "TRUTHRAW_CAM5_PHYSICAL_ROUTE_NATIVE_TYPE_ORACLE_v036.json")
                    .put("physicalInSensorZoomSelectionBasis", "PHYSICAL5_ONLY_AVAILABILITY_PLUS_ONLY_BYTE_MEMBER_OF_V036_SET")
                    .put("physicalInSensorZoomSelectionUsesVendorNameSemantics", false)
                    .put("physicalInSensorZoomByteIntervention", lastPhysicalInSensorZoomByteIntervention ?: JSONObject.NULL)
                    .put("otherV036CandidatesWritten", false)
                    .put("physicalInSensorZoomExperimentSemanticPromotionAllowed", false)
'''
if report_needle not in s:
    raise SystemExit('v0.37 evidence insertion anchor not found')
s = s.replace(report_needle, report_replacement, 1)

assert 'TruthRaw · 200MP Tele Test v0.37 · physical-only inSensorZoomEnable BYTE(1)' in s
assert 'Camera2PhysicalInSensorZoomByteSessionProbe.applyExperiment(' in s
assert s.index('Camera2PreHalGate.observeSession(') < s.index('Camera2PhysicalInSensorZoomByteSessionProbe.applyExperiment(')
assert s.index('Camera2PhysicalInSensorZoomByteSessionProbe.applyExperiment(') < s.index('device.isSessionConfigurationSupported(config)')
assert s.index('persistOriginalRawBuffer(image, stamp)') < s.index('Camera2EnvelopeProbe.observe(image, physical, physicalResult)')
assert s.index('RawSensorRasterAudit.audit(') < s.index('RawPayloadGeometryDecoder.decode(')
assert s.count('Camera2PhysicalInSensorZoomByteSessionProbe.applyExperiment(') == 1
assert 'controlledVendorInterventionKeyCount' in s
assert 'physicalInSensorZoomNativeTypeOracleResolvedType' in s
assert 'otherV036CandidatesWritten' in s
assert 'TRUTHRAW_CAM5_V037_ATTACHMENT_RESULT.json' in s
assert 'MAXIMUM_RESOLUTION vereist' not in s

p.write_text(s)
print('patched v0.37', p)
print('bytes', p.stat().st_size)
