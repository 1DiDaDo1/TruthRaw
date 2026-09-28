#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
audit = (ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui/DarkChromaStabilityAudit.kt").read_text()
front = (ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui/FrontsideSceneInspector.kt").read_text()
main = (ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui/MainActivity.kt").read_text()

required_audit = [
    '"D.RAW/Frontside/DarkChromaStability/0.1"',
    '"authority", "APPEARANCE_DERIVED_ONLY"',
    '"source_observation_count", 1',
    '"other_physical_lenses_used", false',
    '"temporal_frames_used", false',
    '"burst_used", false',
    '"multi_observation_fusion_allowed", false',
    '"drawnegative_target_count", 1',
    '"candidate_applied", false',
    '"creates_new_evidence", false',
    '"scientific_writeback_allowed", false',
    '"replacement_colour_estimated", false',
    '"pixel_value_replacement_proposed", false',
    '"requires_backside_noise_model_or_measurement", true',
    '"binding_role", "D.RAWNEGATIVE_DIAGNOSTIC_CONSTRAINT_ONLY"',
    '"uses_ai_or_learned_model", false',
]
for needle in required_audit:
    assert needle in audit, f"missing Dark Chroma invariant: {needle}"

assert '"D.RAW/FrontsideSceneInspection/0.3"' in front
assert "DarkChromaStabilityAudit.analyze(bitmap, sourceSha256)" in front
assert '"dark_chroma_stability_v0_1"' in front
assert '"STRUCTURAL_VISION_V0_3"' in front

assert "Dark Chroma Stability v0.1 · AUDIT ONLY" in main
assert "otherLenses=" in main
assert "candidateApplied=false" in main

# Runtime source must stay classical/deterministic.
lower = audit.lower()
for banned in ["tensorflow", "pytorch", "onnx", "neural network", "generative model"]:
    assert banned not in lower, f"banned learned-runtime token: {banned}"

print("Dark Chroma Stability v0.1 integrity: PASS")
