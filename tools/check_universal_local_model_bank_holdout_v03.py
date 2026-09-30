#!/usr/bin/env python3
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
base = ROOT / 'suite_android/app/src/main/java/com/truthraw/adaptiveui'
cpp = ROOT / 'suite_android/app/src/main/cpp'
native = ROOT / 'docs/research/universal-local-model-bank-holdout-v0.3/native'

h = (native / 'universal_local_model_bank_holdout_v0_3.h').read_text()
cxx = (native / 'universal_local_model_bank_holdout_v0_3.cpp').read_text()
bridge = (cpp / 'universal_local_model_bank_holdout_v03_bridge.cpp').read_text()
audit = (base / 'UniversalLocalModelBankHoldoutV03.kt').read_text()
profiler = (base / 'UniversalSourceProfiler.kt').read_text()
main = (base / 'MainActivity.kt').read_text()
cmake = (cpp / 'CMakeLists.txt').read_text()
readme = (ROOT / 'docs/research/universal-local-model-bank-holdout-v0.3/README.md').read_text()
protocol = (ROOT / 'docs/research/universal-local-model-bank-holdout-v0.3/DEVICE_VALIDATION_PROTOCOL_2026-09-29.md').read_text()
state = json.loads((ROOT / 'state/UNIVERSAL_LOCAL_MODEL_BANK_HOLDOUT_STATE_2026-09-29_V03.json').read_text())

for needle in [
    'D.RAW/UniversalLocalModelBankHoldoutAudit/0.3',
    'kLatticeFractionBits = 20u',
    'kHoldoutPeriod = 64u',
    'kSupportRadius = 8',
    'DirectionalStripLine',
    'selectorUsesSupportCrossfit = true',
    'selectionUsesCommonValidationRms = true',
    'crossFamilyComplexityPenaltyApplied = false',
    'complexityUsedOnlyAsTieBreak = true',
    'targetValueUsedByModels = false',
    'targetValueUsedBySelector = false',
    'scientificWritebackAllowed = false',
]:
    assert needle in h + cxx, f'missing native v0.3 invariant: {needle}'

for needle in [
    'TARGET_BLIND_COMMON_VALIDATION_RMS_V0_3',
    'freeze_cross_family_validation_scores',
    'candidate->selectionScore = candidate->validationRms',
    'second_best_validation_rms',
    'selection_margin_rms',
    'selection_margin_relative',
    'selection_uses_common_validation_rms',
    'cross_family_complexity_penalty_applied',
    'complexity_used_only_as_tie_break',
]:
    assert needle in cxx, f'missing v0.3 selector contract: {needle}'

selection_pos = cxx.index('record.selected =')
target_read_pos = cxx.index('const double actual =\n                            workspace.stage2[centerIndex];')
assert selection_pos < target_read_pos, 'numeric target must remain unread before selector freeze'

# v0.3 must preserve v0.2 candidate geometry/fits and change only cross-family selection.
v02 = (ROOT / 'docs/research/universal-local-model-bank-holdout-v0.2/native/universal_local_model_bank_holdout_v0_2.cpp').read_text()
for needle in [
    'validation_offset(int dx, int dy)',
    'within_direction_strip(',
    'fit_directional_crossfit(',
    'kDirectionalStripHalfWidthSourcePx = 2.0',
]:
    assert needle in cxx and needle in v02, f'candidate geometry drifted: {needle}'

for needle in [
    'UniversalLocalModelBankHoldoutV03Bridge_exportAndVerify',
    '!report.selectionUsesCommonValidationRms',
    'report.crossFamilyComplexityPenaltyApplied',
    '!report.complexityUsedOnlyAsTieBreak',
    'pipeline::reverify(ctx)',
]:
    assert needle in bridge, f'missing v0.3 JNI gate: {needle}'

for needle in [
    'D.RAW/UniversalLocalModelBankHoldout/0.3',
    'D.RAW/UniversalLocalModelBankHoldoutAudit/0.3',
    'TARGET_BLIND_COMMON_VALIDATION_RMS_V0_3',
    'selection_uses_common_validation_rms',
    'cross_family_complexity_penalty_applied',
    'complexity_used_only_as_tie_break',
    'UniversalLocalModelBankHoldoutV03Bridge.exportAndVerify',
]:
    assert needle in audit, f'missing Android v0.3 contract: {needle}'

assert 'UniversalLocalModelBankHoldoutV03.describe' in profiler
assert 'universal_local_model_bank_holdout_v0_3' in profiler

for needle in [
    'UNIVERSAL_LOCAL_MODEL_BANK_HOLDOUT_V03',
    'universal_local_model_bank_holdout_v03_bridge.cpp',
    '${UNIVERSAL_LOCAL_MODEL_BANK_HOLDOUT_V02}/universal_local_model_bank_holdout_v0_2.cpp',
    '${UNIVERSAL_LOCAL_MODEL_BANK_HOLDOUT_V03}/universal_local_model_bank_holdout_v0_3.cpp',
    '${UNIVERSAL_LOCAL_MODEL_BANK_HOLDOUT_V03}',
]:
    assert needle in cmake, f'missing v0.3 CMake integration: {needle}'

assert '${UNIVERSAL_LOCAL_MODEL_BANK_HOLDOUT_V03}/universal_local_model_bank_holdout_v0_2.cpp' not in cmake

for needle in [
    'Export Universal Local Model Bank Holdout v0.3 · JSON',
    'REQUEST_SAVE_UNIVERSAL_MODEL_BANK_HOLDOUT_V03 = 4125',
    'UniversalLocalModelBankHoldoutV03.exportSidecar',
    'validation-RMS',
]:
    assert needle in main, f'missing v0.3 UI/export integration: {needle}'

for needle in [
    'changes only cross-family selection',
    'No second BIC/AIC/parameter-count penalty',
    'new, previously unscored sealed RAW/DNG',
]:
    assert needle in readme, f'missing v0.3 documentation: {needle}'

for needle in [
    'three-way device-validation protocol',
    'Export Universal Local Model Bank Holdout v0.1 JSON',
    'Export Universal Local Model Bank Holdout v0.2 JSON',
    'Export Universal Local Model Bank Holdout v0.3 JSON',
    'per-model estimates should be bit-identical',
]:
    assert needle in protocol, f'missing v0.3 validation protocol: {needle}'

assert state['architecture']['candidate_fits_frozen_from_v02'] is True
assert state['selector']['selection_uses_common_validation_rms'] is True
assert state['selector']['cross_family_complexity_penalty_applied'] is False
assert state['selector']['complexity_used_only_as_tie_break'] is True
assert state['retrospective_development_replay']['independent_validation'] is False
assert state['validation']['paired_v01_v02_v03_same_source_required'] is True
assert state['safety']['candidate_applied'] is False
assert state['safety']['scientific_writeback_allowed'] is False

lower = (h + '\n' + cxx + '\n' + bridge + '\n' + audit).lower()
for banned in ['tensorflow','pytorch','onnx','neural network','generative model','honor magic']:
    assert banned not in lower, f'banned dependency/runtime token: {banned}'

print('Universal Local Model Bank Holdout v0.3 integrity: PASS')
