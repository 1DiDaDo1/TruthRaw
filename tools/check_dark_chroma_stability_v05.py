#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
base = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"
cpp = ROOT / "suite_android/app/src/main/cpp"

field_h = (ROOT / "docs/research/truthnegative-n2-structure-support-field-v0.1/native/truthnegative_n2_structure_support_field_v0_1.h").read_text()
field_cpp = (ROOT / "docs/research/truthnegative-n2-structure-support-field-v0.1/native/truthnegative_n2_structure_support_field_v0_1.cpp").read_text()
bridge_cpp = (cpp / "truthnegative_n2_structure_support_bridge.cpp").read_text()
binding = (base / "N2StructureSupportBindingAudit.kt").read_text()
v05 = (base / "DarkChromaStabilityV05Audit.kt").read_text()
profiler = (base / "UniversalSourceProfiler.kt").read_text()
main = (base / "MainActivity.kt").read_text()
cmake = (cpp / "CMakeLists.txt").read_text()

for needle in [
    '"D.RAW/TruthNegative/N2StructureSupportField/0.1"',
    'kTileEdge = 32u',
    'kSamplingPeriod = 8u',
    'sampleGridEvidenceOnly = true',
    'unsampledPixelsInferred = false',
    'canReduceProtection = false',
    'canEnableCorrection = false',
    'scientificWritebackAllowed = false',
]:
    assert needle in field_h + field_cpp, f"missing native field invariant: {needle}"

for needle in [
    'options.tileEdge = support::kTileEdge',
    'options.samplingPeriod = support::kSamplingPeriod',
    'report.canReduceProtection',
    'report.canEnableCorrection',
    'report.unsampledPixelsInferred',
    'pipeline::reverify(*ctx)',
]:
    assert needle in bridge_cpp, f"missing bridge invariant: {needle}"

for needle in [
    '"D.RAW/Frontside/N2StructureSupportBinding/0.1"',
    '"WHOLE_FINE_TILE_OVERLAP_CONTEXT_PLUS_EXACT_FULLY_CONTAINED_TILE_COUNTS"',
    '"unsampled_pixels_inferred", false',
    '"can_reduce_protection", false',
    '"can_enable_correction", false',
    '"chroma_correction_supported", false',
    '"private_ab_delta_allowed", false',
    '"scientific_writeback_allowed", false',
]:
    assert needle in binding, f"missing fine binding invariant: {needle}"

for needle in [
    '"D.RAW/Frontside/DarkChromaStability/0.5"',
    '"legacy_v0_4_protection_semantics_reduced"',
    '"fine_metrics_are_probability"',
    '"fine_metrics_can_reduce_protection"',
    '"fine_metrics_can_enable_correction"',
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
    assert needle in v05, f"missing v0.5 invariant: {needle}"

assert 'visibleDarkChromaCandidates > 0L' in profiler
assert '!v03State.startsWith("DARK_UNINFORMATIVE")' in profiler
assert 'N2StructureSupportBindingAudit.analyze' in profiler
assert 'DarkChromaStabilityV05Audit.analyze' in profiler
assert '"n2_structure_support_binding"' in profiler
assert '"dark_chroma_stability_v0_5"' in profiler

assert 'truthnegative_n2_structure_support_bridge.cpp' in cmake
assert 'TRUTHNEGATIVE_N2_STRUCTURE_SUPPORT_V01' in cmake
assert 'truthnegative_n2_structure_support_field_v0_1.cpp' in cmake

assert 'N2 Structure Support v0.1 · FINE 32×32 SOURCE GRID' in main
assert 'Dark Chroma Stability v0.5 · STRUCTURE RESOLUTION REFINEMENT' in main
assert 'Deze laag mag bescherming niet verminderen' in main

lower = (field_cpp + "\n" + bridge_cpp + "\n" + binding + "\n" + v05).lower()
for banned in ["tensorflow", "pytorch", "onnx", "neural network", "generative model"]:
    assert banned not in lower, f"banned learned-runtime token: {banned}"

print("Dark Chroma Stability v0.5 fine structure-support integrity: PASS")
