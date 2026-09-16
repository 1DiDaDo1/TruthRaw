from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from tools.v5g_exact_feature_replay_v01 import (
    EXPECTED_EXTRACTOR_SHA256,
    EXPECTED_FEATURE_SCHEMA_SHA256,
    evaluate,
    load_exact_extractor,
)

ROOT = Path(__file__).resolve().parents[1]


class V5GExactFeatureReplayV01Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.result = evaluate(ROOT)

    def test_exact_historical_semantics_replay_passes(self):
        r = self.result
        self.assertTrue(r["pass"])
        self.assertEqual(r["classification"], "PASS_EXACT_HISTORICAL_FEATURE_SEMANTICS_REPLAY")
        self.assertEqual(r["extractor"]["sha256"], EXPECTED_EXTRACTOR_SHA256)
        self.assertEqual(r["extractor"]["feature_schema_sha256"], EXPECTED_FEATURE_SCHEMA_SHA256)

    def test_hidden_target_independence_is_replayed_for_all_roles(self):
        self.assertEqual(
            self.result["anti_leak_cases"],
            {"B": True, "G1": True, "G2": True, "R": True},
        )

    def test_historical_onehot_quirk_is_preserved(self):
        # role code mapping is R=0,G1=1,G2=2,B=3, while extractor loop order
        # is B,G1,G2,R. This intentionally produces the frozen B/R reversal
        # relative to the emitted feature labels role_R,...,role_B.
        obs = self.result["onehot_observed_by_role_code"]
        self.assertEqual(obs["3"], [1.0, 0.0, 0.0, 0.0])  # B -> role_R-labelled slot
        self.assertEqual(obs["1"], [0.0, 1.0, 0.0, 0.0])
        self.assertEqual(obs["2"], [0.0, 0.0, 1.0, 0.0])
        self.assertEqual(obs["0"], [0.0, 0.0, 0.0, 1.0])  # R -> role_B-labelled slot

    def test_source_white_target_exclusion_is_executed_not_inferred(self):
        self.assertTrue(
            self.result["checks"]["source_white_hidden_target_excluded_from_exact_scoring"]
        )

    def test_holdout_is_bound_but_not_falsely_claimed_reexecuted(self):
        h = self.result["historical_prospective_binding"]
        self.assertEqual(h["decision"], "BACKEND_BOUND_UNCERTAINTY_PROSPECTIVE_PASS")
        self.assertFalse(h["holdout_reexecuted_in_this_gate"])
        self.assertFalse(self.result["scientific_boundary"]["prospective_holdout_reexecuted"])

    def test_byte_drift_is_rejected_before_import(self):
        source = ROOT / "canonical/uncertainty/v5.0g/source/uncertainty_core_v5_0g.py"
        with tempfile.TemporaryDirectory() as td:
            altered = Path(td) / "uncertainty_core_v5_0g.py"
            raw = bytearray(source.read_bytes())
            raw[-1] ^= 1
            altered.write_bytes(raw)
            with self.assertRaises(RuntimeError):
                load_exact_extractor(altered)


if __name__ == "__main__":
    unittest.main()
