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
assert d["doctrine"]["censored_uncertainty"] == "NOT_ADMITTED_TO_STORAGE_SAFETY_CLAIM"
assert "F32_OVERFLOW_OR_NONFINITE_STORAGE_FAILS_CLOSED" in d["doctrine"]["free_scientific_space"]

# Exact canonical v5.0g-p1 identity. Quantiles from another model/schema may not
# silently enter this storage-safety claim.
b = d["bindings"]
assert b["v5g_p1_uncertainty_binding_sha256"] == "61c99b0e29316730ca1323fd3e91fd069ebbc50cba73127cd81b9803b10178a0"
assert b["v5g_p1_feature_schema_sha256"] == "8c8e56b762b83a5a846c5201ab3ba43c9a2549d896873cc0ceb66574e74a83e3"
assert b["v5g_p1_runtime_output_units"] == "Stage-2 normalized scene-linear absolute-error bands"
assert "NOT_GAUSSIAN_SIGMA" in b["v5g_p1_quantile_semantics"]

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

f = d["runtime_fail_closed_rules"]
assert f["binding_mismatch"] == "UNKNOWN_NOT_ADMITTED"
assert f["feature_schema_mismatch"] == "UNKNOWN_NOT_ADMITTED"
assert f["censored_sample"] == "UNKNOWN_NOT_ADMITTED"
assert f["unknown_off_diagonal"] == "PRESERVE_NAN"
assert f["finite_f64_to_nonfinite_f32"] == "STORAGE_OVERFLOW_NOT_ADMITTED"
assert "EXACTLY_LOSSLESS" in f["zero_uncertainty"]

r = d["current_result"]
assert r["f64_compute_f32_storage_absolute_gate"].startswith("PROVISIONAL_PASS")
assert r["uncertainty_relative_local_gate"] == "OPEN"

# 200MP remains a future physical gate, not an inherited numerical claim.
assert any("16320x12288" in item for item in d["required_for_close"])

print("test_uncertainty_relative_storage_gate_v0_7 PASS")
