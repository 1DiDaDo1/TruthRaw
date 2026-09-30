#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
v01 = (ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui/DarkChromaStabilityAudit.kt").read_text()
v02 = (ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui/DarkChromaStabilityV02Audit.kt").read_text()
front = (ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui/FrontsideSceneInspector.kt").read_text()
profiler = (ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui/UniversalSourceProfiler.kt").read_text()
main = (ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui/MainActivity.kt").read_text()

assert '"D.RAW/Frontside/DarkChromaStability/0.1"' in v01
assert '"D.RAW/Frontside/DarkChromaStability/0.2"' in v02
assert (
    '"D.RAW/FrontsideSceneInspection/0.4"' in front
    or '"D.RAW/FrontsideSceneInspection/0.5"' in front
), "v0.2 must remain wired into a compatible FrontsideSceneInspection successor"
assert '"dark_chroma_stability_v0_1"' in front
assert '"dark_chroma_stability_v0_2"' in front
assert "DarkChromaStabilityV02Audit.analyze" in front

required_v02 = [
    '"DARK_UNINFORMATIVE"',
    '"CHROMA_INSTABILITY_VISIBLE"',
    '"STRUCTURE_PROTECTED"',
    '"BACKSIDE_CONFIRMATION_PENDING"',
    '"chroma_correction_supported", false',
    '"local_noise_confirmation_available", false',
    '"n2_local_support_bound", false',
    '"frontside_can_prove_sensor_noise", false',
    '"frontside_can_choose_replacement_colour", false',
    '"candidate_applied", false',
    '"creates_new_evidence", false',
    '"scientific_writeback_allowed", false',
    '"replacement_colour_estimated", false',
    '"pixel_value_replacement_proposed", false',
    '"other_physical_lenses_used", false',
    '"temporal_frames_used", false',
    '"burst_used", false',
    '"multi_observation_fusion_allowed", false',
    '"uses_ai_or_learned_model", false',
]
for needle in required_v02:
    assert needle in v02, f"missing v0.2 invariant: {needle}"

assert '"SOURCE_METADATA_BOUND_HINT_ONLY"' in profiler
assert '"noise_profile_present", noiseProfile != null' in profiler
assert '"local_noise_confirmation_available", false' in profiler

assert "Dark Chroma Stability v0.2 · INFORMATION GATE" in main
assert "DARK_UNINFORMATIVE = geen verborgen kleur reconstrueren" in main
assert "correction-supported=" in main

# Regression sanity checks from the two real-device frontside summaries that
# motivated v0.2. These are not scientific calibration constants.
dark_fraction_min = 0.95
entropy_max = 1.50
edge_density_max = 0.005

def globally_uninformative(dark_fraction, entropy, edge_density):
    return (
        dark_fraction >= dark_fraction_min
        and entropy <= entropy_max
        and edge_density <= edge_density_max
    )

assert globally_uninformative(1.0, 1.04, 0.0007)
assert not globally_uninformative(414/432, 2.33, 0.0499)

lower = v02.lower()
for banned in ["tensorflow", "pytorch", "onnx", "neural network", "generative model"]:
    assert banned not in lower, f"banned learned-runtime token: {banned}"

print("Dark Chroma Stability v0.2 integrity: PASS")
