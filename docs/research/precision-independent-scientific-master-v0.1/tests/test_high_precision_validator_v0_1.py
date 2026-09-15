#!/usr/bin/env python3

import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MODULE = ROOT / "tools" / "high_precision_validator_v0_1.py"
spec = importlib.util.spec_from_file_location("truthraw_precision_oracle_v01", MODULE)
mod = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(mod)


def test_authority_boundary_and_precision_order():
    r = mod.evaluate()
    assert r["authority"] == "NUMERICAL_REFERENCE_ONLY_NO_EVIDENCE_UPGRADE"
    s = r["summary"]
    assert s["float64_not_worse_on_normalization_max_abs"] is True
    assert s["float64_not_worse_on_covariance_max_abs"] is True
    assert s["normalization_max_abs_error_float64"] < 1e-12
    assert s["covariance_max_abs_error_float64"] < 1e-12


def test_float32_and_float64_share_same_source_evidence_cases():
    r = mod.evaluate()
    for row in r["normalization"]:
        assert isinstance(row["input"]["code"], int)
        assert 0 <= row["input"]["code"] <= 65535
        assert row["float64_abs_error"] <= row["float32_abs_error"] + 1e-18


def test_covariance_oracle_is_symmetric():
    r = mod.evaluate()
    vals = [float(x["oracle"]) for x in r["covariance"]]
    m = [vals[0:3], vals[3:6], vals[6:9]]
    for i in range(3):
        for j in range(3):
            assert abs(m[i][j] - m[j][i]) < 1e-12


if __name__ == "__main__":
    test_authority_boundary_and_precision_order()
    test_float32_and_float64_share_same_source_evidence_cases()
    test_covariance_oracle_is_symmetric()
    print("test_high_precision_validator_v0_1 PASS")
