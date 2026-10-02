#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
base = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"
binding = (base / "N2LocalSpatialBindingAudit.kt").read_text()
v04 = (base / "DarkChromaStabilityV04Audit.kt").read_text()
profiler = (base / "UniversalSourceProfiler.kt").read_text()
main = (base / "MainActivity.kt").read_text()

required_binding = [
    '"D.RAW/Frontside/N2LocalSpatialBinding/0.1"',
    '"D.RAW/TruthNegative/N2FactoredConfidenceState/0.3.1"',
    '"exact_confidence_field_binding_verified"',
    '"promotion_eligible"',
    '"candidate_applied"',
    '"scientific_writeback_allowed"',
    '"support_distance_admitted"',
    '"local_binding_verified"',
    '"all_candidates_predictable"',
    '"center_outlier_free"',
    '"pair_rejection_free"',
    '"scale_rejection_free"',
    '"structure_protection_present"',
    '"censor_protection_present"',
    '"strict_local_support_vector"',
    '"chroma_correction_supported", false',
    '"private_ab_delta_allowed", false',
    '"creates_new_evidence", false',
]
for needle in required_binding:
    assert needle in binding, f"missing local binding invariant: {needle}"

required_v04 = [
    '"D.RAW/Frontside/DarkChromaStability/0.4"',
    '"BLOCKED_DARK_UNINFORMATIVE"',
    '"LOCAL_N2_BINDING_PENDING"',
    '"LOCAL_N2_PROTECTION_BLOCK"',
    '"LOCAL_N2_STRICT_VECTOR_BOUND_NOT_PROMOTED"',
    '"LOCAL_N2_FACTORS_BOUND_NOT_SUFFICIENT"',
    '"dark_uninformative_can_be_overridden_by_local_n2"',
    '"local_n2_factors_are_probability"',
    '"local_n2_factors_can_enable_correction"',
    '"chroma_correction_supported", false',
    '"private_ab_delta_allowed", false',
    '"candidate_applied", false',
    '"scientific_writeback_allowed", false',
    '"other_physical_lenses_used", false',
    '"temporal_frames_used", false',
    '"burst_used", false',
    '"multi_observation_fusion_allowed", false',
    '"uses_ai_or_learned_model", false',
]
for needle in required_v04:
    assert needle in v04, f"missing v0.4 invariant: {needle}"

assert "N2LocalSpatialBindingAudit.analyze" in profiler
assert "DarkChromaStabilityV04Audit.analyze" in profiler
assert '"n2_local_spatial_binding"' in profiler
assert '"dark_chroma_stability_v0_4"' in profiler
service = (base / "TruthRawMediaProcessingForegroundService.kt").read_text()
assert "UniversalSourceProfiler.profile" in main
assert "UniversalSourceProfiler.profile" in service
assert "ResearchUniversalProfileStoreV01.save" in service
assert "N2 Local Spatial Binding v0.1 · SAME OBSERVATION" in main
assert "Dark Chroma Stability v0.4 · LOCAL N2 BINDING" in main
assert "Private chroma A/B/Δ blijft uit" in main

# No learned runtime may enter this research path.
lower = (binding + "\n" + v04).lower()
for banned in ["tensorflow", "pytorch", "onnx", "neural network", "generative model"]:
    assert banned not in lower, f"banned learned-runtime token: {banned}"

print("Dark Chroma Stability v0.4 local N2 binding integrity: PASS")
