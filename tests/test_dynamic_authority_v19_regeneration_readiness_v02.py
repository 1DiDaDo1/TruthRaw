import copy
import json
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(ROOT / "tools"))

from tools.dynamic_authority_v19_regeneration_readiness_v02 import (  # noqa: E402
    BLOCKED,
    HISTORICAL_LOCAL_SOURCES,
    PASS_LOCAL_GAPS,
    evaluate_readiness,
)
from tools.truthraw_hdr_dynamic_authority_binding_v19 import (  # noqa: E402
    RECOMPUTATION_IMPLEMENTATION_SHA256_V19,
)


class DynamicAuthorityV19RegenerationReadinessV02Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.state = json.loads(
            (ROOT / "state" / "TRUTHRAW_HDR_DYNAMIC_AUTHORITY_BINDING_V19_STATE.json").read_text(
                encoding="utf-8"
            )
        )

    def test_current_state_is_coherent_but_not_regeneration_ready(self):
        r = evaluate_readiness(ROOT, state=self.state)
        self.assertTrue(r["pass"])
        self.assertEqual(r["classification"], PASS_LOCAL_GAPS)
        self.assertFalse(r["regeneration_ready"])
        self.assertTrue(r["independent_reimplementation_permitted"])
        self.assertEqual(r["implementation_lineage"]["dependency_entry_count"], 13)
        self.assertEqual(r["implementation_lineage"]["persisted_dependency_count"], 8)
        self.assertEqual(r["implementation_lineage"]["historical_local_source_count"], 5)
        self.assertEqual(
            set(r["implementation_lineage"]["historical_local_sources_missing"]),
            set(HISTORICAL_LOCAL_SOURCES),
        )
        self.assertTrue(r["implementation_lineage"]["persisted_repository_dependencies_exact"])

    def test_counts_are_not_a_regeneration_source(self):
        r = evaluate_readiness(ROOT, state=self.state)
        sci = r["scientific_interpretation"]
        self.assertFalse(sci["aggregate_counts_sufficient_to_regenerate_field"])
        self.assertTrue(sci["aggregate_counts_may_be_used_only_as_falsification_checks"])
        self.assertTrue(sci["exact_field_and_channel_digests_required_for_equivalence"])
        self.assertFalse(sci["independent_regenerator_equivalent_before_digest_match"])

    def test_persisted_dependency_hash_drift_blocks_state_classification(self):
        bad = dict(RECOMPUTATION_IMPLEMENTATION_SHA256_V19)
        persisted = next(k for k in bad if k not in HISTORICAL_LOCAL_SOURCES)
        bad[persisted] = "0" * 64
        r = evaluate_readiness(ROOT, state=self.state, expected_implementation=bad)
        self.assertFalse(r["pass"])
        self.assertEqual(r["classification"], BLOCKED)
        self.assertFalse(r["implementation_lineage"]["persisted_repository_dependencies_exact"])

    def test_frozen_field_digest_drift_blocks(self):
        bad_state = copy.deepcopy(self.state)
        bad_state["dynamic_authority"]["field_sha256"] = "0" * 64
        r = evaluate_readiness(ROOT, state=bad_state)
        self.assertFalse(r["pass"])
        self.assertEqual(r["classification"], BLOCKED)

    def test_payload_persistence_history_must_not_be_rewritten(self):
        bad_state = copy.deepcopy(self.state)
        bad_state["dynamic_authority"]["payload_persisted_by_v19"] = True
        r = evaluate_readiness(ROOT, state=bad_state)
        self.assertFalse(r["pass"])
        self.assertFalse(r["frozen_v19_state"]["checks"]["payload_not_persisted"])

    def test_deterministic_recomputation_history_must_remain_explicit(self):
        bad_state = copy.deepcopy(self.state)
        bad_state["dynamic_authority"]["identity_deterministically_recomputable"] = False
        r = evaluate_readiness(ROOT, state=bad_state)
        self.assertFalse(r["pass"])

    def test_partition_invariance_is_part_of_frozen_evidence(self):
        bad_state = copy.deepcopy(self.state)
        bad_state["validation"]["partition_invariance"] = "FAIL"
        r = evaluate_readiness(ROOT, state=bad_state)
        self.assertFalse(r["pass"])


if __name__ == "__main__":
    unittest.main()
