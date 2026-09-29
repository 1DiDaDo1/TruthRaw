#!/usr/bin/env python3
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
base = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"
cpp = ROOT / "suite_android/app/src/main/cpp"
native = ROOT / "docs/research/universal-local-model-bank-holdout-v0.1/native"

h = (native / "universal_local_model_bank_holdout_v0_1.h").read_text()
cxx = (native / "universal_local_model_bank_holdout_v0_1.cpp").read_text()
bridge = (cpp / "universal_local_model_bank_holdout_bridge.cpp").read_text()
audit = (base / "UniversalLocalModelBankHoldoutV01.kt").read_text()
profiler = (base / "UniversalSourceProfiler.kt").read_text()
main = (base / "MainActivity.kt").read_text()
cmake = (cpp / "CMakeLists.txt").read_text()
readme = (ROOT / "docs/research/universal-local-model-bank-holdout-v0.1/README.md").read_text()
state = json.loads(
    (ROOT / "state/UNIVERSAL_LOCAL_MODEL_BANK_HOLDOUT_STATE_2026-09-29.json").read_text()
)

for needle in [
    '"D.RAW/UniversalLocalModelBankHoldoutAudit/0.1"',
    'kLatticeFractionBits = 20u',
    'kHoldoutPeriod = 64u',
    'kSupportRadius = 8',
    'RobustMedianConstant',
    'DirectionalLine',
    'AffinePlane',
    'QuadraticSurface',
    'targetValueUsedByModels = false',
    'targetValueUsedBySelector = false',
    'holdoutErrorUsedBySelector = false',
    'postRevealOracleUsedBySelector = false',
    'lensCalibrationUsed = false',
    'cameraModelUsed = false',
    'vendorMappingUsed = false',
    'measuredAnchorsModified = false',
    'unanchoredValuesPromotedToMeasured = false',
    'modelBankAppliedToScientificMaster = false',
    'scientificWritebackAllowed = false',
]:
    assert needle in h + cxx, f"missing native invariant: {needle}"

assert 'o << "}},\\n";' in cxx, "model-bank JSON global/models nesting must close with exactly two braces"
assert 'o << "}}},\\n";' not in cxx, "model-bank JSON has an extra global/models closing brace"

for needle in [
    'source_raster_used_only_for_noise',
    'source_raster_is_world_resolution_authority',
    'STRATIFIED_FULL_RESOLUTION_MEASURED_CFA_ANCHORS',
    'TARGET_BLIND_RESIDUAL_BIC_LIKE_COMPLEXITY_SCORE',
    'noise_profile_required_for_selection',
    'target_value_used_by_models',
    'target_value_used_by_selector',
    'holdout_error_used_by_selector',
    'post_reveal_oracle_used_by_selector',
    'post_reveal_oracle_is_diagnostic_only',
    'lens_calibration_used',
    'camera_model_used',
    'vendor_mapping_used',
    'model_bank_applied_to_scientific_master',
    'scientific_writeback_allowed',
    'oracle_best_model_post_reveal',
    'selector_regret_abs_error',
]:
    assert needle in cxx, f"missing sidecar contract: {needle}"

for needle in [
    'UniversalLocalModelBankHoldoutBridge_exportAndVerify',
    'report.targetValueUsedByModels',
    'report.targetValueUsedBySelector',
    'report.holdoutErrorUsedBySelector',
    'report.postRevealOracleUsedBySelector',
    'report.lensCalibrationUsed',
    'report.cameraModelUsed',
    'report.vendorMappingUsed',
    'pipeline::reverify(ctx)',
]:
    assert needle in bridge, f"missing bridge invariant: {needle}"

for needle in [
    '"D.RAW/UniversalLocalModelBankHoldout/0.1"',
    '"READY_FOR_EXPLICIT_EXPORT_AUDIT"',
    '"source_raster_used_only_for_noise", false',
    '"prospective_query_policy_required_for_full_resolution_audit",\n                false',
    '"dark_chroma_dependency_required",\n                false',
    '"noise_profile_required_for_selection",\n                false',
    '"new_independent_capture_required_for_scientific_conclusion",\n                true',
    '"target_value_used_by_models", false',
    '"target_value_used_by_selector", false',
    '"holdout_error_used_by_selector", false',
    '"post_reveal_oracle_used_by_selector", false',
    '"lens_calibration_used", false',
    '"camera_model_used", false',
    '"vendor_mapping_used", false',
    '"scientific_writeback_allowed", false',
]:
    assert needle in audit, f"missing Android audit invariant: {needle}"

assert 'UniversalLocalModelBankHoldoutV01.describe' in profiler
assert '"universal_local_model_bank_holdout"' in profiler
assert '"universal_local_model_bank_holdout_v0_1"' not in profiler

for needle in [
    'UNIVERSAL_LOCAL_MODEL_BANK_HOLDOUT_V01',
    'universal_local_model_bank_holdout_bridge.cpp',
    'universal_local_model_bank_holdout_v0_1.cpp',
]:
    assert needle in cmake, f"missing CMake integration: {needle}"

for needle in [
    'Export Universal Local Model Bank Holdout v0.1 · JSON',
    'REQUEST_SAVE_UNIVERSAL_MODEL_BANK_HOLDOUT = 4123',
    'UniversalLocalModelBankHoldoutV01.exportSidecar',
    'full-resolution CFA holdouts + target-blinde modelselectie',
]:
    assert needle in main, f"missing APK/UI integration: {needle}"

for needle in [
    "The original raster therefore supplies exact authority",
    "Target-blind candidates",
    "A new independent real-device RAW is required",
    "noise metadata is absent",
]:
    assert needle in readme, f"missing documentation: {needle}"

assert state["architecture"]["universal_input_required"] is True
assert state["architecture"]["lens_specific_calibration_required"] is False
assert state["architecture"]["source_raster_used_only_for_noise"] is False
assert state["architecture"]["noise_profile_required_for_selection"] is False
assert state["architecture"]["dark_chroma_dependency_required"] is False
assert state["architecture"]["prospective_query_policy_required"] is False
assert state["selector"]["target_value_used_by_models"] is False
assert state["selector"]["target_value_used_by_selector"] is False
assert state["selector"]["holdout_error_used_by_selector"] is False
assert state["selector"]["post_reveal_oracle_used_by_selector"] is False
assert state["safety"]["measured_anchors_modified"] is False
assert state["safety"]["unanchored_values_promoted_to_measured"] is False
assert state["safety"]["model_bank_applied_to_scientific_master"] is False
assert state["safety"]["candidate_applied"] is False
assert state["safety"]["scientific_writeback_allowed"] is False

lower = (h + "\n" + cxx + "\n" + bridge + "\n" + audit).lower()
for banned in [
    "tensorflow",
    "pytorch",
    "onnx",
    "neural network",
    "generative model",
    "honor magic",
]:
    assert banned not in lower, f"banned dependency/runtime token: {banned}"

print("Universal Local Model Bank Holdout v0.1 integrity: PASS")
