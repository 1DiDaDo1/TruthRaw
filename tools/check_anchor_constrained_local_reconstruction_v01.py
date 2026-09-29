#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
base = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"
cpp = ROOT / "suite_android/app/src/main/cpp"
native = ROOT / "docs/research/anchor-constrained-local-reconstruction-v0.1/native"

h = (native / "anchor_constrained_local_reconstruction_v0_1.h").read_text()
cxx = (native / "anchor_constrained_local_reconstruction_v0_1.cpp").read_text()
bridge = (cpp / "anchor_constrained_local_reconstruction_bridge.cpp").read_text()
audit = (base / "AnchorConstrainedLocalReconstructionAudit.kt").read_text()
profiler = (base / "UniversalSourceProfiler.kt").read_text()
main = (base / "MainActivity.kt").read_text()
cmake = (cpp / "CMakeLists.txt").read_text()
state = (ROOT / "state/ANCHOR_CONSTRAINED_LOCAL_RECONSTRUCTION_STATE_2026-09-29.json").read_text()

for needle in [
    '"D.RAW/AnchorConstrainedLocalReconstructionAudit/0.1"',
    'kLatticeFractionBits = 20u',
    'kHoldoutPeriod = 8u',
    'kSupportRadius = 8',
    'kMinPlaneSamples = 12u',
    'targetValueUsedBySolver = false',
    'measuredAnchorsModified = false',
    'unanchoredValuesPromotedToMeasured = false',
    'reconstructedAuthorityOnly = true',
    'uncertaintyDiagnosticOnly = true',
    'noiseIndependenceAdmitted = false',
    'solverAppliedToScientificMaster = false',
    'scientificWritebackAllowed = false',
]:
    assert needle in h + cxx, f"missing native holdout invariant: {needle}"

for needle in [
    'INVERSE_VARIANCE_LOCAL_AFFINE_PLANE_V0_1',
    'CENTER_EXCLUDED_MULTISCALE_V0_2',
    'target_value_used_by_solver',
    'measured_anchors_modified',
    'unanchored_values_promoted_to_measured',
    'RECONSTRUCTED_PRIVATE_AUDIT_ONLY',
    'holdout_stream_sha256',
    'solver_mae',
    'baseline_mae',
    'holdout_records',
]:
    assert needle in cxx, f"missing holdout sidecar field: {needle}"

for needle in [
    'AnchorConstrainedLocalReconstructionBridge_exportAndVerify',
    'report.targetValueUsedBySolver',
    'report.measuredAnchorsModified',
    'report.unanchoredValuesPromotedToMeasured',
    'report.solverAppliedToScientificMaster',
    'pipeline::reverify(ctx)',
]:
    assert needle in bridge, f"missing bridge invariant: {needle}"

for needle in [
    '"D.RAW/Frontside/AnchorConstrainedLocalReconstruction/0.1"',
    '"AUDIT_ONLY_HOLDOUT_VALIDATION_AVAILABLE"',
    '"holdout_target_is_noisy_measurement", true',
    '"holdout_error_is_scene_truth_metric",\n                    false',
    '"lower_holdout_error_proves_denoising",\n                    false',
    '"target_value_used_by_solver",\n                    false',
    '"measured_anchors_modified", false',
    '"unanchored_values_promoted_to_measured",\n                    false',
    '"reconstructed_authority_only", true',
    '"uncertainty_diagnostic_only", true',
    '"noise_independence_admitted", false',
    '"solver_applied_to_scientific_master", false',
    '"can_enable_correction", false',
    '"scientific_writeback_allowed", false',
]:
    assert needle in audit, f"missing Android holdout invariant: {needle}"

assert 'AnchorConstrainedLocalReconstructionAudit.analyze' in profiler
assert '"anchor_constrained_local_reconstruction"' in profiler
assert 'anchor_constrained_local_reconstruction_v0_1' in profiler

assert 'ANCHOR_CONSTRAINED_LOCAL_RECON_V01' in cmake
assert 'anchor_constrained_local_reconstruction_bridge.cpp' in cmake
assert 'anchor_constrained_local_reconstruction_v0_1.cpp' in cmake

assert 'Anchor-Constrained Local Reconstruction v0.1 · HOLDOUT AUDIT' in main
assert 'Export Anchor-Constrained Reconstruction v0.1 · JSON' in main
assert 'REQUEST_SAVE_ANCHOR_RECONSTRUCTION = 4121' in main
assert 'AnchorConstrainedLocalReconstructionAudit.exportSidecar' in main

for needle in [
    '"target_value_used_by_solver": false',
    '"target_remains_measured": true',
    '"solver_output_authority": "RECONSTRUCTED_PRIVATE_AUDIT_ONLY"',
    '"lower_holdout_error_proves_denoising": false',
    '"measured_anchors_modified": false',
    '"solver_applied_to_scientific_master": false',
    '"scientific_writeback_allowed": false',
]:
    assert needle in state, f"missing state invariant: {needle}"

lower = (cxx + "\n" + bridge + "\n" + audit).lower()
for banned in ["tensorflow", "pytorch", "onnx", "neural network", "generative model"]:
    assert banned not in lower, f"banned learned-runtime token: {banned}"

print("Anchor-Constrained Local Reconstruction v0.1 integrity: PASS")
