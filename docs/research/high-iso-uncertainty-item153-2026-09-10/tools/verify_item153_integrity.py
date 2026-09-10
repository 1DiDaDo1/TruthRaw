#!/usr/bin/env python3
from pathlib import Path
import hashlib, json

ROOT = Path(__file__).resolve().parents[4]
MOD = ROOT / "docs/research/high-iso-uncertainty-item153-2026-09-10"

def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()

frozen = {
    "canonical/uncertainty/v5.0g/UNCERTAINTY_MODEL_v5_0g.json": "8831bee921999e823466cfc462812e40620b2834080c0f7d64c6f11d7ead626f",
    "canonical/uncertainty/v5.0g/BACKEND_BINDING_v5_0g.json": "d0134dc5322cf721778d1b5e4607c2e82e0dde45b909acfa2e6ea2c8e588d42d",
    "canonical/uncertainty/v5.0g/source/uncertainty_core_v5_0g.py": "b1cfbf061a32aa4abccb9d8bb86a9b5b9257f88498081eb9aa659f9402d0919d",
    "canonical/uncertainty/v5.0g/source/prospective_holdout_v5_0g.py": "5c9e3bcfb146933634bc65ad167f1911edc61503d75df02dd7a748a79c51618c",
    "canonical/reconstruction/v4.7i/native/src/core.cpp": "68f52c4896d47c4605adc1cf670e55f95d42a687e18c06a167028a64ce37b94c",
    "canonical/reconstruction/v4.7i/native/include/truthraw/core.h": "b7f6e2189d6ecccfc5fda80084041990bd12757145c5ff2c40f2ae0d0046b167",
}
for rel, expected in frozen.items():
    p = ROOT / rel
    assert p.is_file(), rel
    assert sha256(p) == expected, rel

state = json.loads((MOD / "state/STATE_ITEM153.json").read_text())
assert state["status"] == "SCORED_MIXED_FAIL_GENERALIZATION_REVIEW_OPEN_NO_RETUNE"
assert state["frozen_model_retuned"] is False
assert state["broader_generalization_blocker_open"] is True
assert state["historical_094414_failure_binding"] is True
assert state["independent_promotion_holdout"] is False
assert state["literal_decisions"] == ["FAIL", "FAIL", "FAIL", "PASS"]
assert state["deterministic_rerun_byte_identical"] is True

metrics = json.loads((MOD / "evidence/METRICS_ITEM153_COMPACT.json").read_text())
assert metrics["status"] == state["status"]
assert metrics["evidence_class"] == state["evidence_class"]
assert metrics["deterministic_rerun_byte_exact"] is True
assert [x["iso"] for x in metrics["captures"]] == [1600, 3200, 6400, 12800]
assert [x["literal_scorer_decision"].endswith("PASS") for x in metrics["captures"]] == [False, False, False, True]
ratios = [x["q5_ratio"] for x in metrics["captures"]]
assert ratios[0] < 0.75 and ratios[1] < 0.75 and ratios[2] < 0.75 and ratios[3] >= 0.75
assert len(metrics["raw_result_json_sha256"]) == 4

print("TruthRaw High-ISO Item 153 Evidence Integrity: PASS")
