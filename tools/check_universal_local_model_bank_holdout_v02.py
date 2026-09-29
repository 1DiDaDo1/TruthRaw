#!/usr/bin/env python3
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
base = ROOT / 'suite_android/app/src/main/java/com/truthraw/adaptiveui'
cpp = ROOT / 'suite_android/app/src/main/cpp'
native = ROOT / 'docs/research/universal-local-model-bank-holdout-v0.2/native'

h = (native / 'universal_local_model_bank_holdout_v0_2.h').read_text()
cxx = (native / 'universal_local_model_bank_holdout_v0_2.cpp').read_text()
bridge = (cpp / 'universal_local_model_bank_holdout_v02_bridge.cpp').read_text()
audit = (base / 'UniversalLocalModelBankHoldoutV02.kt').read_text()
profiler = (base / 'UniversalSourceProfiler.kt').read_text()
main = (base / 'MainActivity.kt').read_text()
cmake = (cpp / 'CMakeLists.txt').read_text()
readme = (ROOT / 'docs/research/universal-local-model-bank-holdout-v0.2/README.md').read_text()
protocol = (ROOT / 'docs/research/universal-local-model-bank-holdout-v0.2/DEVICE_VALIDATION_PROTOCOL_2026-09-29.md').read_text()
state = json.loads((ROOT / 'state/UNIVERSAL_LOCAL_MODEL_BANK_HOLDOUT_STATE_2026-09-29_V02.json').read_text())

for needle in [
    'D.RAW/UniversalLocalModelBankHoldoutAudit/0.2',
    'kLatticeFractionBits = 20u',
    'kHoldoutPeriod = 64u',
    'kSupportRadius = 8',
    'kDirectionalStripHalfWidthSourcePx = 2.0',
    'DirectionalStripLine',
    'TARGET_BLIND_SUPPORT_CROSSFIT_PREDICTIVE_SCORE_V0_2',
    'validation_offset',
    'within_direction_strip',
    'fit_directional_crossfit',
    'fit_robust_constant_crossfit',
    'fit_least_squares_crossfit',
    'selectorUsesSupportCrossfit = true',
    'directionalSupportConditioned = true',
    'targetValueUsedByModels = false',
    'targetValueUsedBySelector = false',
    'scientificWritebackAllowed = false',
]:
    assert needle in h + cxx, f'missing native v0.2 invariant: {needle}'

assert 'DIRECTIONAL_LINE' not in cxx, 'v0.2 must not silently reuse v0.1 directional model name'
assert 'DIRECTIONAL_STRIP_LINE' in cxx
assert 'support_crossfit_partition' in cxx
assert 'directional_affine_both_valid' in cxx
assert 'directional_affine_bit_identical_estimate' in cxx
assert 'selector_uses_support_crossfit' in cxx
assert 'directional_support_conditioned' in cxx
assert 'directional_strip_half_width_source_px' in cxx

selection_pos = cxx.index('record.selected =')
target_read_pos = cxx.index('const double actual =\n                            workspace.stage2[centerIndex];')
assert selection_pos < target_read_pos, 'numeric target must be unread before selector freeze'
assert 's.validation =' in cxx and 'validation_offset(dx, dy)' in cxx

for needle in [
    'UniversalLocalModelBankHoldoutV02Bridge_exportAndVerify',
    '!report.selectorUsesSupportCrossfit',
    '!report.directionalSupportConditioned',
    'pipeline::reverify(ctx)',
]:
    assert needle in bridge, f'missing JNI v0.2 gate: {needle}'

for needle in [
    'D.RAW/UniversalLocalModelBankHoldout/0.2',
    'D.RAW/UniversalLocalModelBankHoldoutAudit/0.2',
    'selector_uses_support_crossfit',
    'directional_support_conditioned',
    'TARGET_BLIND_SUPPORT_CROSSFIT_PREDICTIVE_SCORE_V0_2',
    'DIRECTIONAL_STRIP_LINE',
    'UniversalLocalModelBankHoldoutV02Bridge.exportAndVerify',
]:
    assert needle in audit, f'missing Android v0.2 contract: {needle}'

assert 'UniversalLocalModelBankHoldoutV02.describe' in profiler
assert 'universal_local_model_bank_holdout_v0_2' in profiler

for needle in [
    'UNIVERSAL_LOCAL_MODEL_BANK_HOLDOUT_V02',
    'universal_local_model_bank_holdout_v02_bridge.cpp',
    'universal_local_model_bank_holdout_v0_2.cpp',
]:
    assert needle in cmake, f'missing v0.2 CMake integration: {needle}'

for needle in [
    'Export Universal Local Model Bank Holdout v0.2 · JSON',
    'REQUEST_SAVE_UNIVERSAL_MODEL_BANK_HOLDOUT_V02 = 4124',
    'UniversalLocalModelBankHoldoutV02.exportSidecar',
    'support-crossfit',
]:
    assert needle in main, f'missing v0.2 UI/export integration: {needle}'

for needle in [
    'support cross-fit',
    'DIRECTIONAL_STRIP_LINE',
    'tele capture is test provenance only',
    'new independent sealed RAW/DNG',
]:
    assert needle in readme, f'missing v0.2 documentation: {needle}'

for needle in [
    'paired device-validation protocol',
    'Export Universal Local Model Bank Holdout v0.1 JSON',
    'Export Universal Local Model Bank Holdout v0.2 JSON',
    'same holdout period',
]:
    assert needle in protocol, f'missing paired validation protocol: {needle}'

assert state['architecture']['universal_input_required'] is True
assert state['architecture']['lens_specific_calibration_required'] is False
assert state['architecture']['source_raster_used_only_for_noise'] is False
assert state['selector']['directional_support_conditioned'] is True
assert state['selector']['target_numeric_stage2_value_read_before_selection'] is False
assert state['selector']['target_value_used_by_selector'] is False
assert state['validation']['paired_v01_v02_same_capture_required'] is True
assert state['safety']['measured_anchors_modified'] is False
assert state['safety']['candidate_applied'] is False
assert state['safety']['scientific_writeback_allowed'] is False

lower = (h + '\n' + cxx + '\n' + bridge + '\n' + audit).lower()
for banned in ['tensorflow','pytorch','onnx','neural network','generative model','honor magic']:
    assert banned not in lower, f'banned dependency/runtime token: {banned}'

print('Universal Local Model Bank Holdout v0.2 integrity: PASS')
