#!/usr/bin/env python3
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GATE = ROOT / "evidence" / "UNCERTAINTY_RELATIVE_STORAGE_GATE_v0_7.json"
d = json.loads(GATE.read_text())

assert d["schema"] == "TruthRawUncertaintyRelativeStorageGate/0.7"
assert d["canonical_promoted"] is False
assert d["scientific_authority_changed"] is False
assert d["status"] == "OPEN_LOCAL_UNCERTAINTY_BINDING_REQUIRED"

# Fail-closed uncertainty doctrine.
assert d["doctrine"]["unknown_covariance"] == "REMAIN_NAN_AND_UNKNOWN"
assert d["doctrine"]["unknown_off_diagonal"] == "NEVER_ASSUME_ZERO"
assert "ONLY_EXPLICIT_GAUSSIAN_EQUIVALENT" in d["doctrine"]["gaussian_sigma"]
assert "NEVER_CONVERT_TO_SIGMA_OR_VARIANCE" in d["doctrine"]["quantile_anchor"]

# The current absolute storage result remains tiny but does not close the local
# scientific uncertainty gate by itself.
s = d["existing_storage_error"]
assert s["tested_files"] >= 8
assert s["max_f64_to_f32_abs_normalized"] < 1e-7
assert s["max_per_file_rms_normalized"] < 1e-8

q = d["prospective_tele_quantile_scale_check"]
assert q["may_be_called_sigma"] is False
assert q["may_close_local_storage_gate"] is False
assert q["max_storage_error_over_observed_median_error"] < 1e-4
assert q["max_storage_error_over_observed_p95_error"] < 1e-4

r = d["current_result"]
assert r["f64_compute_f32_storage_absolute_gate"].startswith("PROVISIONAL_PASS")
assert r["uncertainty_relative_local_gate"] == "OPEN"

# 200MP remains a future physical gate, not an inherited numerical claim.
assert any("16320x12288" in item for item in d["required_for_close"])

print("test_uncertainty_relative_storage_gate_v0_7 PASS")
