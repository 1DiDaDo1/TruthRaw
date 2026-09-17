#!/usr/bin/env python3
from pathlib import Path

p = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/FotoGraaf200MpStagedActivity.kt')

# v0.31 begins only after the complete v0.30 16/16 binary matrix produced no measured
# envelope/populated-prefix topology differential. Reconstruct the proven v0.30e device-usable
# source-first chain, then change only the controlled route-intervention scheduler/evidence labels.
#
# v0.31 bounded domain:
#   RawCbSourceType        INT32 values 0,2,3 (value 1 + UNSET already screened by v0.30)
#   HALOutputBufferCombined INT32 values 0,2,3 (value 1 + UNSET already screened by v0.30)
# Exactly one unknown vendor key is written per run. EnableIdealRAW and XCFA remain UNSET.
ns = {}
exec(
    compile(
        Path('tools/patch_fotograaf_v030e_fixed_controls_matrix_recovery.py').read_text(),
        'patch_fotograaf_v030e_fixed_controls_matrix_recovery.py',
        'exec',
    ),
    ns,
    ns,
)
s = p.read_text()

# Swap the completed binary-matrix scheduler for the bounded INT32 value-domain scheduler.
s = s.replace('Camera2VendorRouteFullFactorialMatrix', 'Camera2Int32ValueDomainSweep')

# Versioned output names/evidence are new; v0.20 source-first acquisition and Stage 3.6/3.7 code stay intact.
s = s.replace('v0.30e', 'v0.31')
s = s.replace('v0.30', 'v0.31')
s = s.replace('v030', 'v031')
s = s.replace(
    'TruthRaw · 200MP Tele Test v0.31 · fixed-control matrix recovery',
    'TruthRaw · 200MP Tele Test v0.31 · INT32 value-domain sweep',
)
s = s.replace(
    '16-run complement-paired 2^4 vendor-route matrix; low=UNSET, high=type-validated numeric 1; every RAW is independently sealed before unchanged Stage 3.6/3.7 topology audit',
    '6-run bounded INT32 value-domain sweep; RawCbSourceType and HALOutputBufferCombined test numeric 0/2/3 one key at a time; every RAW is independently sealed before unchanged Stage 3.6/3.7 topology audit',
)

# v0.31 profile IDs are Sxx_RAWCB_value or Sxx_HALCOMBINED_value; use them for cache recovery.
old_regex = 'Regex(".*_R(\\\\d{2})_ABCD_([01]{4})_EVIDENCE_v031\\\\.json$")'
new_regex = 'Regex(".*_S(\\\\d{2})_(RAWCB|HALCOMBINED)_(-?\\\\d+)_EVIDENCE_v031\\\\.json$")'
if old_regex not in s:
    raise SystemExit('v0.31 cache regex anchor not found')
s = s.replace(old_regex, new_regex, 1)

# Compact always-visible selector and device-facing instructions.
old_label = 'return "R${p.runIndex + 1}/16 · ABCD=${p.bits} · tik→volgende"'
new_label = 'return "S${p.runIndex + 1}/6 · ${p.bits} · tik→volgende"'
if old_label not in s:
    raise SystemExit('v0.31 selector label anchor not found')
s = s.replace(old_label, new_label, 1)

s = s.replace(
    'Matrix handmatig geselecteerd: run ${p.runIndex + 1}/16 · ABCD=${p.bits} · ${p.id}.\\nLow=UNSET; high=numeric 1 met bewezen native type. Geen vendorsemantiek aangenomen.',
    'Sweep handmatig geselecteerd: run ${p.runIndex + 1}/6 · ${p.bits} · ${p.id}.\\nAlleen één INT32 vendor-key wordt geschreven; waarde is stimulus, geen vendorsemantiek.',
)
s = s.replace(
    'Alle 16 matrixruns zijn in cache aanwezig; R01 is alleen als handmatige selector teruggezet.',
    'Alle 6 v0.31 sweepruns zijn in cache aanwezig; S01 is alleen als handmatige selector teruggezet.',
)
s = s.replace(
    'Eerste ontbrekende run=${next.runIndex + 1}/16 · ABCD=${next.bits} · ${next.id}.',
    'Eerste ontbrekende run=${next.runIndex + 1}/6 · ${next.bits} · ${next.id}.',
)
s = s.replace('matrix evidence JSON(s)', 'sweep evidence JSON(s)')
s = s.replace('Matrix evidence bundle klaar:', 'Sweep evidence bundle klaar:')
s = s.replace('Geen cached v0.31 matrix evidence JSONs gevonden; niets geëxporteerd.', 'Geen cached v0.31 sweep evidence JSONs gevonden; niets geëxporteerd.')
s = s.replace('Matrix · exporteer alle cached v0.31 evidence JSONs als ZIP', 'Sweep · exporteer cached v0.31 evidence JSONs als ZIP')
s = s.replace('3 · Matrix capture', '3 · Sweep capture')
s = s.replace('Stap 3 · MATRIX CAPTURE · PHYSICAL 5 · 16320×12288', 'Stap 3 · INT32 SWEEP CAPTURE · PHYSICAL 5 · 16320×12288')

# Bundle identity is v0.31-specific and contains evidence JSONs only.
s = s.replace('TRUTHRAW_CAM5_V031_MATRIX_EVIDENCE_BUNDLE_', 'TRUTHRAW_CAM5_V031_INT32_SWEEP_EVIDENCE_BUNDLE_')
s = s.replace('TRUTHRAW_V031_MATRIX_BUNDLE_MANIFEST.json', 'TRUTHRAW_V031_INT32_SWEEP_BUNDLE_MANIFEST.json')
s = s.replace('truthraw.camera5-v031-matrix-evidence-bundle.v0.31d', 'truthraw.camera5-v031-int32-value-domain-sweep-evidence-bundle.v0.31')

# Correct experiment/evidence semantics: no full-factorial language remains in the v0.31 intervention record.
s = s.replace(
    'FULL_FACTORIAL_2_LEVEL_4_FACTOR_16_RUN_COMPLEMENT_PAIRED',
    'INT32_VALUE_DOMAIN_SWEEP_RAWCB_HALCOMBINED_VALUES_0_2_3',
)
old_levels = '''                    .put("controlledVendorInterventionLowLevel", "UNSET_NO_WRITE")\n                    .put("controlledVendorInterventionHighLevel", "NUMERIC_ONE_TYPE_VALIDATED__SEMANTICS_UNPROVEN")\n'''
new_levels = '''                    .put("controlledVendorInterventionValueDomain", "INT32_VALUES_0_2_3__VALUE_1_AND_UNSET_ALREADY_SCREENED_IN_V030")\n                    .put("controlledVendorInterventionSingleUnknownVendorKeyPerRun", true)\n'''
if old_levels not in s:
    raise SystemExit('v0.31 evidence level anchor not found')
s = s.replace(old_levels, new_levels, 1)
s = s.replace('"vendorRouteFullFactorialMatrix"', '"int32ValueDomainSweep"')
s = s.replace('"vendorRouteMatrixSemanticPromotionAllowed"', '"int32ValueDomainSemanticPromotionAllowed"')

# Block/final status text reflects value-domain sweep rather than the completed binary matrix.
s = s.replace(
    'STAGE 3 BLOCKED · v0.31 matrixprofiel ${matrixProfile.id} niet veilig toepasbaar.',
    'STAGE 3 BLOCKED · v0.31 sweep-profiel ${matrixProfile.id} niet veilig toepasbaar.',
)
s = s.replace('Geen capture ingediend; dezelfde matrixrun blijft geselecteerd.', 'Geen capture ingediend; dezelfde sweeprun blijft geselecteerd.')
s = s.replace(
    'Matrix ${activeMatrixProfileId()} · ABCD=${Camera2Int32ValueDomainSweep.profileForRun(activeMatrixRunIndex).bits} · keysWritten=${lastVendorRouteMatrixIntervention?.optInt("vendorKeysWritten", 0) ?: 0}',
    'Sweep ${activeMatrixProfileId()} · ${Camera2Int32ValueDomainSweep.profileForRun(activeMatrixRunIndex).bits} · keysWritten=${lastVendorRouteMatrixIntervention?.optInt("vendorKeysWritten", 0) ?: 0}',
)

# Recovery/export UI wording only; internal matrix-named variables are intentionally retained to minimize code churn.
s = s.replace('v0.31 cache recovery:', 'v0.31 INT32 sweep cache recovery:')
s = s.replace('matrix evidence JSON(s) gevonden', 'sweep evidence JSON(s) gevonden')
s = s.replace('matrixrun', 'sweeprun')
s = s.replace('Matrix capture', 'Sweep capture')

# Scientific and safety invariants.
assert 'TruthRaw · 200MP Tele Test v0.31 · INT32 value-domain sweep' in s
assert 'Camera2Int32ValueDomainSweep.applyProfile(' in s
assert s.count('Camera2Int32ValueDomainSweep.applyProfile(') == 1
assert 'INT32_VALUE_DOMAIN_SWEEP_RAWCB_HALCOMBINED_VALUES_0_2_3' in s
assert 'int32ValueDomainSweep' in s
assert 'int32ValueDomainSemanticPromotionAllowed' in s
assert 'S${p.runIndex + 1}/6' in s
assert 'TRUTHRAW_CAM5_V031_INT32_SWEEP_EVIDENCE_BUNDLE_' in s
assert '_EVIDENCE_v031.json' in s
assert 'staged-evidence.v0.31' in s
assert s.index('Camera2PreHalGate.observeSession(') < s.index('Camera2Int32ValueDomainSweep.applyProfile(')
assert s.index('Camera2Int32ValueDomainSweep.applyProfile(') < s.index('device.isSessionConfigurationSupported(config)')
assert s.index('persistOriginalRawBuffer(image, stamp)') < s.index('Camera2EnvelopeProbe.observe(image, physical, physicalResult)')
assert s.index('RawSensorRasterAudit.audit(') < s.index('RawPayloadGeometryDecoder.decode(')
assert 'MAXIMUM_RESOLUTION vereist' not in s

p.write_text(s)
print('patched v0.31 INT32 value-domain sweep', p)
print('bytes', p.stat().st_size)
