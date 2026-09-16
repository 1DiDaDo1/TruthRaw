import copy
import json
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from tools.dynamic_authority_v19_regeneration_readiness_v01 import (  # noqa: E402
    BLOCKED,
    LOCAL_PROBE,
    PASS_PROBE_GAP,
    PASS_WITH_PROBE,
    evaluate_readiness,
)
from tools.truthraw_hdr_dynamic_authority_binding_v19 import (  # noqa: E402
    RECOMPUTATION_IMPLEMENTATION_SHA256_V19,
)


class DynamicAuthorityV19RegenerationReadinessTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.state = json.loads(
            (ROOT / "state" / "TRUTHRAW_HDR_DYNAMIC_AUTHORITY_BINDING_V19_STATE.json").read_text(
                encoding="utf-8"
            )
        )

    def test_current_repository_lineage_is_ready_or_has_only_documented_probe_gap(self):
        r = evaluate_readiness(ROOT, state=self.state)
        self.assertTrue(r["pass"])
        self.assertIn(r["classification"], {PASS_PROBE_GAP, PASS_WITH_PROBE})
        self.assertTrue(r["implementation_lineage"]["required_repository_dependencies_exact"])
        self.assertEqual(r["implementation_lineage"]["dependency_entry_count"], 13)
        self.assertEqual(r["implementation_lineage"]["non_probe_dependency_count"], 12)
        self.assertFalse(
            r["scientific_interpretation"]["aggregate_counts_sufficient_to_regenerate_field"]
        )
        self.assertTrue(
            r["scientific_interpretation"][
                "exact_field_and_channel_digests_are_required_falsification_targets"
            ]
        )

    def test_probe_absence_is_provenance_gap_not_scientific_failure(self):
        r = evaluate_readiness(ROOT, state=self.state)
        probe = r["implementation_lineage"]["records"][LOCAL_PROBE]
        if probe["status"] == "ABSENT":
            self.assertEqual(r["classification"], PASS_PROBE_GAP)
            self.assertEqual(
                r["implementation_lineage"]["historical_local_probe_status"],
                "NOT_PERSISTED_IN_REPOSITORY",
            )
        self.assertFalse(
            r["scientific_interpretation"]["missing_local_probe_source_equals_scientific_failure"]
        )

    def test_dependency_hash_drift_blocks_readiness(self):
        bad = dict(RECOMPUTATION_IMPLEMENTATION_SHA256_V19)
        non_probe = next(k for k in bad if k != LOCAL_PROBE)
        bad[non_probe] = "0" * 64
        r = evaluate_readiness(ROOT, state=self.state, expected_implementation=bad)
        self.assertFalse(r["pass"])
        self.assertEqual(r["classification"], BLOCKED)
        self.assertFalse(r["implementation_lineage"]["required_repository_dependencies_exact"])

    def test_frozen_field_digest_drift_blocks_readiness(self):
        bad_state = copy.deepcopy(self.state)
        bad_state["dynamic_authority"]["field_sha256"] = "0" * 64
        r = evaluate_readiness(ROOT, state=bad_state)
        self.assertFalse(r["pass"])
        self.assertEqual(r["classification"], BLOCKED)
        self.assertFalse(r["frozen_v19_state"]["checks"]["dynamic_authority_field_sha256"])

    def test_payload_must_remain_documented_as_not_persisted(self):
        bad_state = copy.deepcopy(self.state)
        bad_state["dynamic_authority"]["payload_persisted_by_v19"] = True
        r = evaluate_readiness(ROOT, state=bad_state)
        self.assertFalse(r["pass"])
        self.assertFalse(r["frozen_v19_state"]["checks"]["payload_not_persisted"])

    def test_deterministic_recomputation_claim_must_remain_explicit(self):
        bad_state = copy.deepcopy(self.state)
        bad_state["dynamic_authority"]["identity_deterministically_recomputable"] = False
        r = evaluate_readiness(ROOT, state=bad_state)
        self.assertFalse(r["pass"])
        self.assertFalse(
            r["frozen_v19_state"]["checks"]["identity_deterministically_recomputable"]
        )

    def test_partition_invariance_evidence_is_required(self):
        bad_state = copy.deepcopy(self.state)
        bad_state["validation"]["partition_invariance"] = "FAIL"
        r = evaluate_readiness(ROOT, state=bad_state)
        self.assertFalse(r["pass"])
        self.assertFalse(r["frozen_v19_state"]["checks"]["partition_invariance_pass"])


if __name__ == "__main__":
    unittest.main()
