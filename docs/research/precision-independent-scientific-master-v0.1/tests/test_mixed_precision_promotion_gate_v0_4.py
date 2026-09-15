#!/usr/bin/env python3
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GATE = ROOT / "evidence" / "MIXED_PRECISION_PROMOTION_GATE_v0_4.json"

d = json.loads(GATE.read_text())
assert d["schema"] == "TruthRawMixedPrecisionPromotionGate/0.4"
assert d["canonical_promoted"] is False
assert d["scientific_authority_changed"] is False

# The gate must never silently restore Float32 as scientific compute authority.
assert d["decisions"]["f32_branch_sensitive_compute"] == "REJECT_AS_SCIENTIFIC_REFERENCE"
assert d["decisions"]["f64_branch_sensitive_compute"] == "REQUIRED_SCIENTIFIC_REFERENCE"
assert d["gate_result"]["f32_compute_reference"] == "FAIL"
assert d["gate_result"]["f64_compute_reference"] == "PASS"

# Evidence breadth.
a = d["aggregate_full_file_sweep"]
b = d["research_acceptance_budget"]
assert a["files"] >= b["minimum_real_files"]
assert len(set(a["source_classes"])) >= b["minimum_source_classes"]
assert a["total_direction_divergence"] > 0
assert a["max_f32_f64_rgb_abs"] > 1e-2  # real branch amplification exists
assert a["measured_channel_violations_f64"] == b["measured_channel_violations_f64_compute"]

# Provisional F64-compute -> F32-storage gate.
assert a["max_f64_to_f32_storage_abs"] <= b["storage_max_abs_normalized"]
assert a["max_f64_to_f32_storage_rms"] <= b["storage_rms_normalized"]
assert d["gate_result"]["f64_compute_f32_storage"] == "PROVISIONAL_PASS"

# All retained real files must individually satisfy the same storage budget.
assert len(d["per_file"]) >= b["minimum_real_files"]
for item in d["per_file"]:
    assert item["storage_max_abs"] <= b["storage_max_abs_normalized"], item["file"]
    assert item["storage_rms"] <= b["storage_rms_normalized"], item["file"]

# Deterministic C++ branch-hazard fixtures are mandatory. The full-file host
# sweep is locator evidence; these fixtures are what prevents regression of the
# branch-sensitive conclusion in CI.
if b["require_cpp_branch_hazard_fixtures"]:
    fixtures = d["cpp_ci"]["fixtures"]
    assert len(fixtures) >= 3
    assert all(f["f32_direction"] != f["f64_direction"] for f in fixtures)

# 200MP remains explicitly unproven until a real 16320x12288 payload exists.
assert any("16320x12288" in x for x in d["gate_result"]["not_proven"])

print("test_mixed_precision_promotion_gate_v0_4 PASS")
