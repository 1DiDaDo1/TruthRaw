#!/usr/bin/env python3
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
state = json.loads((ROOT / "state/STATE_v1.json").read_text())
assert state["status"] == "EXACT_REPARAMETERIZATION_CONTRACT_LOCAL_PASS_MODEL_SELECTION_ADMISSION_OPEN"
assert state["best_observation_semantics"] == "BEST_CONDITIONING_GAUGE_NOT_BEST_EVIDENCE"
assert state["physical_frame_count_delta"] == 0
assert state["independent_evidence_count_delta"] == 0
assert state["snr_gain_claim_from_virtual_ev"] == 0.0
assert state["information_gain_claim_from_virtual_ev"] == 0.0
assert state["zero_line_redefined"] is False
assert state["scientific_master_modified_by_exact_mode"] is False
assert state["legacy_multi_ev_reconstruction_candidate"] == "CANDIDATE_REJECTED"

probe = json.loads((ROOT / "evidence/REAL_094423_MEASUREMENT_CONDITIONING_PROBE_v1.json").read_text())
assert probe["source_capture_metadata_used_for_gauge_selection"] is False
assert probe["sampled_values"] == 3133440
assert probe["bit_exact_roundtrip_mean_fraction"] == 1.0
assert probe["bit_exact_roundtrip_sigma_fraction"] == 1.0
assert probe["max_snr_delta"] == 0.0
assert probe["max_standardized_residual_delta"] == 0.0

metrics = (ROOT / "evidence/TEST_METRICS_v1.txt").read_text()
assert "roundtrip_tested=250000 exact=250000" in metrics
assert "fuzz_valid=992347 fuzz_success=992347" in metrics
assert "candidate_fallback=CANDIDATE_REJECTED" in metrics

print("TruthRaw Manifold Conditioning v1 integrity: PASS")
