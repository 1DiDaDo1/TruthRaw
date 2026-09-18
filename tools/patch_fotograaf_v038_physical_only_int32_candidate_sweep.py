#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

# Reconstruct the proven v0.31 fixed-control sweep UI/source-first chain, then replace
# only the intervention scheduler and v0.38 evidence/UI labels. v0.20 acquisition,
# seal-first ordering and Stage 3.6/3.7 remain unchanged.
ns = {}
exec(
    compile(
        Path('tools/patch_fotograaf_v031b_int32_value_domain_sweep.py').read_text(),
        'patch_fotograaf_v031b_int32_value_domain_sweep.py',
        'exec',
    ),
    ns,
    ns,
)
s = p.read_text()

s = s.replace('Camera2Int32ValueDomainSweep', 'Camera2PhysicalOnlyInt32CandidateSweep')
s = s.replace('v0.31', 'v0.38')
s = s.replace('v031', 'v038')
s = s.replace('V031', 'V038')

s = s.replace(
    'TruthRaw · 200MP Tele Test v0.38 · INT32 value-domain sweep',
    'TruthRaw · 200MP Tele Test v0.38 · physical-only INT32 candidate sweep',
)
s = s.replace(
    '6-run bounded INT32 value-domain sweep; RawCbSourceType and HALOutputBufferCombined test numeric 0/2/3 one key at a time; every RAW is independently sealed before unchanged Stage 3.6/3.7 topology audit',
    '3-run physical-only INT32 candidate sweep; v0.36 candidates I/J/K test numeric 1 one key at a time; every RAW is independently sealed before unchanged Stage 3.6/3.7 topology audit',
)

old_regex = 'Regex(".*_S(\\d{2})_(RAWCB|HALCOMBINED)_(-?\\d+)_EVIDENCE_v038\\.json$")'
new_regex = 'Regex(".*_P(\\d{2})_([IJK])_EVIDENCE_v038\\.json$")'
if old_regex not in s:
    raise SystemExit('v0.38 cache regex anchor not found')
s = s.replace(old_regex, new_regex, 1)

s = s.replace(
    'return "S${p.runIndex + 1}/6 · ${p.bits} · tik→volgende"',
    'return "P${p.runIndex + 1}/3 · ${p.bits} · tik→volgende"',
)
s = s.replace(
    'Sweep handmatig geselecteerd: run ${p.runIndex + 1}/6 · ${p.bits} · ${p.id}.\\nAlleen één INT32 vendor-key wordt geschreven; waarde is stimulus, geen vendorsemantiek.',
    'Candidate handmatig geselecteerd: run ${p.runIndex + 1}/3 · ${p.bits} · ${p.id}.\\nAlleen één physical-only INT32 vendor-key wordt geschreven; waarde 1 is stimulus, geen vendorsemantiek.',
)
s = s.replace(
    'Alle 6 v0.38 sweepruns zijn in cache aanwezig; S01 is alleen als handmatige selector teruggezet.',
    'Alle 3 v0.38 candidate-runs zijn in cache aanwezig; P01 is alleen als handmatige selector teruggezet.',
)
s = s.replace(
    'Eerste ontbrekende run=${next.runIndex + 1}/6 · ${next.bits} · ${next.id}.',
    'Eerste ontbrekende run=${next.runIndex + 1}/3 · ${next.bits} · ${next.id}.',
)

s = s.replace('sweep evidence JSON(s)', 'candidate evidence JSON(s)')
s = s.replace('Sweep evidence bundle klaar:', 'Candidate evidence bundle klaar:')
s = s.replace('Geen cached v0.38 sweep evidence JSONs gevonden; niets geëxporteerd.', 'Geen cached v0.38 candidate evidence JSONs gevonden; niets geëxporteerd.')
s = s.replace('Sweep · exporteer cached v0.38 evidence JSONs als ZIP', 'Candidates · exporteer cached v0.38 evidence JSONs als ZIP')
s = s.replace('3 · Sweep capture', '3 · Candidate capture')
s = s.replace('Stap 3 · INT32 SWEEP CAPTURE · PHYSICAL 5 · 16320×12288', 'Stap 3 · PHYSICAL-ONLY INT32 CANDIDATE CAPTURE · PHYSICAL 5 · 16320×12288')

s = s.replace('TRUTHRAW_CAM5_V038_INT32_SWEEP_EVIDENCE_BUNDLE_', 'TRUTHRAW_CAM5_V038_PHYSICAL_INT32_CANDIDATE_BUNDLE_')
s = s.replace('TRUTHRAW_V038_INT32_SWEEP_BUNDLE_MANIFEST.json', 'TRUTHRAW_V038_PHYSICAL_INT32_CANDIDATE_BUNDLE_MANIFEST.json')
s = s.replace('truthraw.camera5-v038-int32-value-domain-sweep-evidence-bundle.v0.38', 'truthraw.camera5-v038-physical-only-int32-candidate-sweep-evidence-bundle.v0.38')

s = s.replace(
    'INT32_VALUE_DOMAIN_SWEEP_RAWCB_HALCOMBINED_VALUES_0_2_3',
    'PHYSICAL_ONLY_INT32_CANDIDATE_SWEEP_I_J_K_NUMERIC_ONE',
)
s = s.replace(
    '.put("controlledVendorInterventionValueDomain", "INT32_VALUES_0_2_3__VALUE_1_AND_UNSET_ALREADY_SCREENED_IN_V030")',
    '.put("controlledVendorInterventionKeyDomain", "V036_PHYSICAL_ONLY_INT32_CANDIDATES_I_J_K__NUMERIC_ONE__ONE_KEY_PER_RUN")',
)
s = s.replace(
    '.put("controlledVendorInterventionSingleUnknownVendorKeyPerRun", true)',
    '.put("controlledVendorInterventionSingleUnknownVendorKeyPerRun", true)\n' +
    '                    .put("controlledVendorInterventionNoCombinations", true)\n' +
    '                    .put("controlledVendorInterventionStopOnTopologyHit", true)',
)
s = s.replace('"int32ValueDomainSweep"', '"physicalOnlyInt32CandidateSweep"')
s = s.replace('"int32ValueDomainSemanticPromotionAllowed"', '"physicalOnlyInt32CandidateSemanticPromotionAllowed"')

s = s.replace(
    'STAGE 3 BLOCKED · v0.38 sweep-profiel ${matrixProfile.id} niet veilig toepasbaar.',
    'STAGE 3 BLOCKED · v0.38 physical-only candidate ${matrixProfile.id} niet veilig toepasbaar.',
)
s = s.replace('Geen capture ingediend; dezelfde sweeprun blijft geselecteerd.', 'Geen capture ingediend; dezelfde candidate-run blijft geselecteerd.')
s = s.replace(
    'Sweep ${activeMatrixProfileId()} · ${Camera2PhysicalOnlyInt32CandidateSweep.profileForRun(activeMatrixRunIndex).bits} · keysWritten=${lastVendorRouteMatrixIntervention?.optInt("vendorKeysWritten", 0) ?: 0}',
    'Candidate ${activeMatrixProfileId()} · ${Camera2PhysicalOnlyInt32CandidateSweep.profileForRun(activeMatrixRunIndex).bits} · keysWritten=${lastVendorRouteMatrixIntervention?.optInt("vendorKeysWritten", 0) ?: 0}',
)
s = s.replace('v0.38 INT32 sweep cache recovery:', 'v0.38 physical-only candidate cache recovery:')
s = s.replace('sweep evidence JSON(s) gevonden', 'candidate evidence JSON(s) gevonden')
s = s.replace('sweeprun', 'candidate-run')
s = s.replace('Sweep capture', 'Candidate capture')

# Persist a blocked attachment/representation attempt without advancing the selected candidate.
# The BLOCKED filename deliberately does not match the completion-regex, so recovery does not
# silently count a blocked run as completed before its evidence has been reviewed.
block_needle = '''        if (!matrixAttempt.applied) {
            setStatus(
'''
block_replacement = '''        if (!matrixAttempt.applied) {
            val blockedJson = JSONObject()
                .put("schema", "truthraw.camera5-v038-physical-only-int32-candidate-blocked.v0.38")
                .put("experimentVersion", "v0.38")
                .put("controlReference", "TruthRaw v0.20 unchanged")
                .put("candidateRunIndex", activeMatrixRunIndex)
                .put("candidateProfileId", matrixProfile.id)
                .put("intervention", matrixAttempt.evidence)
                .put("capturePerformed", false)
                .put("rawPixelAccessAfterIntervention", false)
                .put("sourceMutation", false)
                .put("semanticPromotionAllowed", false)
            val blockedFile = File(
                cacheDir,
                "TRUTHRAW_V038_BLOCKED_${System.currentTimeMillis()}_${matrixProfile.id}.json",
            )
            runCatching { blockedFile.writeText(blockedJson.toString(2)) }
            capturedJson = blockedFile.takeIf { it.exists() && it.length() > 0L }
            saveJsonButton.isEnabled = capturedJson != null
            setStatus(
'''
if block_needle not in s:
    raise SystemExit('v0.38 blocked-evidence anchor not found')
s = s.replace(block_needle, block_replacement, 1)

# Safety / scientific invariants.
assert 'TruthRaw · 200MP Tele Test v0.38 · physical-only INT32 candidate sweep' in s
assert 'Camera2PhysicalOnlyInt32CandidateSweep.applyProfile(' in s
assert s.count('Camera2PhysicalOnlyInt32CandidateSweep.applyProfile(') == 1
assert 'PHYSICAL_ONLY_INT32_CANDIDATE_SWEEP_I_J_K_NUMERIC_ONE' in s
assert 'physicalOnlyInt32CandidateSweep' in s
assert 'physicalOnlyInt32CandidateSemanticPromotionAllowed' in s
assert 'P${p.runIndex + 1}/3' in s
assert 'TRUTHRAW_CAM5_V038_PHYSICAL_INT32_CANDIDATE_BUNDLE_' in s
assert '_EVIDENCE_v038.json' in s
assert 'staged-evidence.v0.38' in s
assert 'TRUTHRAW_V038_BLOCKED_' in s
assert s.index('Camera2PreHalGate.observeSession(') < s.index('Camera2PhysicalOnlyInt32CandidateSweep.applyProfile(')
assert s.index('Camera2PhysicalOnlyInt32CandidateSweep.applyProfile(') < s.index('device.isSessionConfigurationSupported(config)')
assert s.index('persistOriginalRawBuffer(image, stamp)') < s.index('Camera2EnvelopeProbe.observe(image, physical, physicalResult)')
assert s.index('RawSensorRasterAudit.audit(') < s.index('RawPayloadGeometryDecoder.decode(')
assert 'MAXIMUM_RESOLUTION vereist' not in s

p.write_text(s)
print('patched v0.38 physical-only INT32 candidate sweep', p)
print('bytes', p.stat().st_size)
