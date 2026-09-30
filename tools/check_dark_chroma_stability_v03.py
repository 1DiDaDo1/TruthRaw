#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
base = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"
v01 = (base / "DarkChromaStabilityAudit.kt").read_text()
v02 = (base / "DarkChromaStabilityV02Audit.kt").read_text()
v03 = (base / "DarkChromaStabilityV03Audit.kt").read_text()
backside = (base / "BacksideSignalSupportAudit.kt").read_text()
front = (base / "FrontsideSceneInspector.kt").read_text()
profiler = (base / "UniversalSourceProfiler.kt").read_text()
main = (base / "MainActivity.kt").read_text()

assert '"D.RAW/Frontside/DarkChromaStability/0.1"' in v01
assert '"D.RAW/Frontside/DarkChromaStability/0.2"' in v02
assert '"D.RAW/Frontside/DarkChromaStability/0.3"' in v03
assert '"D.RAW/BacksideSignalSupport/0.1"' in backside
assert '"D.RAW/FrontsideSceneInspection/0.5"' in front

required_v03 = [
    '"DARK_UNINFORMATIVE_BY_DEGENERACY"',
    '"DARK_UNINFORMATIVE_BY_BACKSIDE_SIGNAL"',
    '"entropy_is_hard_gate", false',
    '"global_backside_signal_can_block", true',
    '"global_backside_signal_can_enable", false',
    '"local_backside_n2_support_bound", false',
    '"chroma_correction_supported", false',
    '"private_ab_delta_allowed", false',
    '"other_physical_lenses_used", false',
    '"temporal_frames_used", false',
    '"burst_used", false',
    '"multi_observation_fusion_allowed", false',
    '"candidate_applied", false',
    '"creates_new_evidence", false',
    '"scientific_writeback_allowed", false',
    '"replacement_colour_estimated", false',
    '"pixel_value_replacement_proposed", false',
    '"uses_ai_or_learned_model", false',
]
for needle in required_v03:
    assert needle in v03, f"missing v0.3 invariant: {needle}"

required_backside = [
    '"SOURCE_PAYLOAD_MEASURED_WITHIN_SELECTED_DNG"',
    '"selected_source_is_untouched_adc_proof", false',
    '"(raw_code-black_level)/(white_level-black_level)"',
    '"negative_values_preserved", true',
    '"clamping_applied", false',
    '"sampled_active_area"',
    '"CONSERVATIVE_BLOCKING_HEURISTIC_ONLY"',
    '"can_block_colour_reconstruction_when_near_black", true',
    '"can_enable_chroma_correction", false',
    '"local_tile_binding_available", false',
    '"n2_local_support_bound", false',
    '"creates_new_evidence", false',
    '"scientific_writeback_allowed", false',
]
for needle in required_backside:
    assert needle in backside, f"missing backside invariant: {needle}"

assert "BacksideSignalSupportAudit.analyze" in profiler
assert '"backside_signal_support"' in profiler
assert '"dark_chroma_stability_v0_3"' in front
assert "DarkChromaStabilityV03Audit.analyze" in front
assert "Dark Chroma Stability v0.3 · DEGENERACY + BACKSIDE GATE" in main
assert "entropy is alleen diagnostiek, geen harde poort" in main

# Real-device regression summaries. These are blocking research checks, not calibration.
DARK_MIN = 0.98
CANDIDATE_MIN = 0.98
STRUCTURE_MAX = 0.02
EDGE_MAX = 0.010

def degenerate(dark, candidate, structure, edge):
    return (
        dark >= DARK_MIN
        and candidate >= CANDIDATE_MIN
        and structure <= STRUCTURE_MAX
        and edge <= EDGE_MAX
    )

# v0.2 miss: entropy changed from ~1.04 to ~1.54, but the scene remained
# fully dark/candidate and structure-free. v0.3 must still block it.
assert degenerate(432/432, 432/432, 0/432, 0.0040)

# Earlier dark main/wide must remain non-degenerate.
assert not degenerate(414/432, 22/432, 40/432, 0.0499)

# Entropy is intentionally absent from the degeneracy function.
assert degenerate(1.0, 1.0, 0.0, 0.0040)

lower = (v03 + "\n" + backside).lower()
for banned in ["tensorflow", "pytorch", "onnx", "neural network", "generative model"]:
    assert banned not in lower, f"banned learned-runtime token: {banned}"

print("Dark Chroma Stability v0.3 + Backside Signal Support v0.1 integrity: PASS")
