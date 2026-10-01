#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
base = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"
cpp = ROOT / "suite_android/app/src/main/cpp"
native = ROOT / "docs/research/truthnegative-n2-sample-support-distance-v0.1/native"

h = (native / "truthnegative_n2_support_distance_v0_1.h").read_text()
cxx = (native / "truthnegative_n2_support_distance_v0_1.cpp").read_text()
bridge = (cpp / "truthnegative_n2_support_distance_bridge.cpp").read_text()
audit = (base / "N2SampleSupportDistanceAudit.kt").read_text()
v06 = (base / "DarkChromaStabilityV06Audit.kt").read_text()
profiler = (base / "UniversalSourceProfiler.kt").read_text()
main = (base / "MainActivity.kt").read_text()
cmake = (cpp / "CMakeLists.txt").read_text()

for needle in [
    '"D.RAW/TruthNegative/N2SampleSupportDistance/0.1"',
    'kEvaluationTileEdge = 64u',
    'kSamplingPeriod = 8u',
    'kCenterRadiiPx{{8u,16u,32u,64u}}',
    'kRectMarginRadiiPx{{0u,8u,16u,32u}}',
    'exactSampleCoordinatesRecorded = true',
    'unsampledPixelsInferred = false',
    'scalarProbabilityCreated = false',
    'canReduceProtection = false',
    'canEnableCorrection = false',
    'scientificWritebackAllowed = false',
]:
    assert needle in h + cxx, f"missing native distance invariant: {needle}"

for needle in [
    'BASE64_LE_U32_XY_PAIRS_SOURCE_NATIVE',
    'exact_sample_coordinates_recorded',
    'unsampled_pixels_inferred',
    'scalar_probability_created',
    'can_reduce_protection',
    'can_enable_correction',
    'promotion_eligible',
    'scientific_writeback_allowed',
    'support_point_stream_sha256',
    'nearest_structure_from_center',
    'nearest_structure_to_rect',
]:
    assert needle in cxx, f"missing sidecar invariant: {needle}"

for needle in [
    'TruthNegativeN2SupportDistanceBridge_exportAndVerify',
    'report.exactSampleCoordinatesRecorded',
    'report.unsampledPixelsInferred',
    'report.scalarProbabilityCreated',
    'report.canReduceProtection',
    'report.canEnableCorrection',
    'pipeline::reverify(*ctx)',
]:
    assert needle in bridge, f"missing bridge invariant: {needle}"

for needle in [
    '"D.RAW/Frontside/N2SampleSupportDistance/0.1"',
    '"AUDIT_ONLY_DISTANCE_BINDING_AVAILABLE"',
    '"exact_sample_coordinates_recorded", true',
    '"sample_grid_evidence_only", true',
    '"unsampled_pixels_inferred", false',
    '"scalar_probability_created", false',
    '"can_reduce_protection", false',
    '"can_enable_correction", false',
    '"chroma_correction_supported", false',
    '"private_ab_delta_allowed", false',
    '"v0_5_aggregate_parity_verified"',
]:
    assert needle in audit, f"missing Android distance invariant: {needle}"

for needle in [
    '"D.RAW/Frontside/DarkChromaStability/0.6"',
    '"distance_metrics_are_probability"',
    '"distance_threshold_admitted"',
    '"distance_can_reduce_protection"',
    '"distance_can_enable_correction"',
    '"dark_uninformative_can_be_overridden"',
    '"chroma_correction_supported", false',
    '"private_ab_delta_allowed", false',
    '"candidate_applied", false',
    '"scientific_writeback_allowed", false',
    '"other_physical_lenses_used", false',
    '"temporal_frames_used", false',
    '"burst_used", false',
    '"multi_observation_fusion_allowed", false',
    '"uses_ai_or_learned_model", false',
]:
    assert needle in v06, f"missing v0.6 invariant: {needle}"

assert 'N2SampleSupportDistanceAudit.analyze' in profiler
assert 'DarkChromaStabilityV06Audit.analyze' in profiler
assert '"n2_sample_support_distance"' in profiler
assert '"dark_chroma_stability_v0_6"' in profiler
assert 'truthnegative_n2_support_distance_bridge.cpp' in cmake
assert 'TRUTHNEGATIVE_N2_SUPPORT_DISTANCE_V01' in cmake
assert 'truthnegative_n2_support_distance_v0_1.cpp' in cmake
assert 'N2 Sample Support Distance v0.1 · EXACT SAMPLED GEOMETRY' in main
assert 'Dark Chroma Stability v0.6 · SAMPLE-LEVEL SUPPORT DISTANCE' in main
assert 'Export N2 Sample Support Distance v0.1 · JSON' in main
assert 'REQUEST_SAVE_N2_SUPPORT_DISTANCE = 4120' in main
assert 'N2SampleSupportDistanceAudit.exportSidecar' in main
assert 'fun exportSidecar(' in audit

lower = (cxx + "\n" + bridge + "\n" + audit + "\n" + v06).lower()
for banned in ["tensorflow", "pytorch", "onnx", "neural network", "generative model"]:
    assert banned not in lower, f"banned learned-runtime token: {banned}"

print("Dark Chroma Stability v0.6 exact sampled support-distance integrity: PASS")
