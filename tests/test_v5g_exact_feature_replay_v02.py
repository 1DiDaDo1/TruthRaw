from __future__ import annotations

from pathlib import Path
import unittest

from tools.v5g_exact_feature_replay_v02 import evaluate

ROOT = Path(__file__).resolve().parents[1]


class V5GExactFeatureReplayV02Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.result = evaluate(ROOT)

    def test_exact_semantics_replay_passes(self):
        self.assertTrue(self.result["pass"])
        self.assertEqual(
            self.result["classification"],
            "PASS_EXACT_HISTORICAL_FEATURE_SEMANTICS_REPLAY",
        )

    def test_v01_failure_is_preserved_as_harness_issue(self):
        self.assertEqual(
            self.result["v01_harness_result"],
            "FAILED_SCALAR_CALL_TO_VECTORIZED_HISTORICAL_PREDICTOR",
        )
        self.assertEqual(
            self.result["v02_correction"],
            "ONE_ELEMENT_COORDINATE_ARRAYS_ONLY_NO_EXTRACTOR_CHANGE",
        )

    def test_anti_leak_all_roles(self):
        self.assertEqual(
            self.result["anti_leak_cases"],
            {"B": True, "G1": True, "G2": True, "R": True},
        )

    def test_historical_onehot_quirk_exact(self):
        obs = self.result["onehot_observed_by_role_code"]
        self.assertEqual(obs["3"], [1.0, 0.0, 0.0, 0.0])
        self.assertEqual(obs["1"], [0.0, 1.0, 0.0, 0.0])
        self.assertEqual(obs["2"], [0.0, 0.0, 1.0, 0.0])
        self.assertEqual(obs["0"], [0.0, 0.0, 0.0, 1.0])

    def test_source_white_exclusion_and_holdout_binding(self):
        self.assertTrue(
            self.result["checks"]["source_white_hidden_target_excluded_from_exact_scoring"]
        )
        self.assertTrue(
            self.result["checks"]["prospective_holdout_bound_to_same_extractor_and_v47i_core"]
        )
        self.assertFalse(
            self.result["historical_prospective_binding"]["holdout_reexecuted_in_this_gate"]
        )

    def test_current_f64_binding_still_open(self):
        self.assertFalse(
            self.result["scientific_boundary"]["current_f64_trace_bound_to_exact_features"]
        )


if __name__ == "__main__":
    unittest.main()
