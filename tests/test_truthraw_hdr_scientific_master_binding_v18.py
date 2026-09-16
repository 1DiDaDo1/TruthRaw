import copy
import unittest

from tools.truthraw_hdr_scientific_master_binding_v18 import (
    FROZEN_REAL_RUN_V18,
    LEGACY_LINEARRAW_DEQUANTIZED_F32_SHA256_V18,
    LEGACY_LINEARRAW_FILE_SHA256_V18,
    LEGACY_LINEARRAW_PIXEL_PAYLOAD_SHA256_V18,
    REAL_RECOMPUTATION_OBSERVATION_V18,
    REFERENCE_L0_V18,
    SCIENTIFIC_MASTER_SHA256_V18,
    SOURCE_CFA_SHA256_V18,
    SOURCE_DNG_SHA256_V18,
    ScientificMasterBindingError,
    ScientificMasterRecomputationObservationV18,
    real_scientific_master_binding_v18,
    require_complete_v16_science_binding_v18,
)


class ScientificMasterBindingV18Tests(unittest.TestCase):
    def test_real_two_run_recomputation_is_exact_and_deterministic(self):
        obs = REAL_RECOMPUTATION_OBSERVATION_V18.validate()
        self.assertTrue(obs.deterministic_recomputation_proven)
        self.assertEqual(obs.first_run, obs.second_run)
        self.assertEqual(obs.first_run, FROZEN_REAL_RUN_V18)
        self.assertEqual(obs.scientific_master_sha256, SCIENTIFIC_MASTER_SHA256_V18)
        self.assertEqual(obs.first_run.reference_l0, REFERENCE_L0_V18)

    def test_scientific_master_identity_is_distinct_from_source_and_legacy_payloads(self):
        self.assertNotEqual(SCIENTIFIC_MASTER_SHA256_V18, SOURCE_DNG_SHA256_V18)
        self.assertNotEqual(SCIENTIFIC_MASTER_SHA256_V18, SOURCE_CFA_SHA256_V18)
        self.assertNotIn(
            SCIENTIFIC_MASTER_SHA256_V18,
            {
                LEGACY_LINEARRAW_FILE_SHA256_V18,
                LEGACY_LINEARRAW_PIXEL_PAYLOAD_SHA256_V18,
                LEGACY_LINEARRAW_DEQUANTIZED_F32_SHA256_V18,
            },
        )

    def test_binding_closes_master_identity_gate_but_not_dynamic_authority_gate(self):
        binding = real_scientific_master_binding_v18()
        self.assertTrue(binding.scientific_master_identity_bound)
        self.assertFalse(binding.scientific_master_payload_persisted_by_v18)
        self.assertFalse(binding.dynamic_authority_identity_bound)
        self.assertFalse(binding.v16_science_binding_complete)
        with self.assertRaises(ScientificMasterBindingError):
            require_complete_v16_science_binding_v18(binding)

    def test_complete_projection_kwargs_require_explicit_dynamic_authority_sha(self):
        authority_sha = "1" * 64
        binding = real_scientific_master_binding_v18(authority_sha)
        kwargs = require_complete_v16_science_binding_v18(binding)
        self.assertEqual(kwargs["scientific_master_sha256"], SCIENTIFIC_MASTER_SHA256_V18)
        self.assertEqual(kwargs["dynamic_authority_sha256"], authority_sha)
        self.assertEqual(kwargs["reference_l0"], REFERENCE_L0_V18)

    def test_wrong_source_hash_fails_closed(self):
        obs = ScientificMasterRecomputationObservationV18(
            source_dng_sha256="0" * 64,
            source_cfa_sha256=REAL_RECOMPUTATION_OBSERVATION_V18.source_cfa_sha256,
            archive_commit_sha=REAL_RECOMPUTATION_OBSERVATION_V18.archive_commit_sha,
            implementation_sha256=dict(REAL_RECOMPUTATION_OBSERVATION_V18.implementation_sha256),
            first_run=REAL_RECOMPUTATION_OBSERVATION_V18.first_run,
            second_run=REAL_RECOMPUTATION_OBSERVATION_V18.second_run,
        )
        with self.assertRaises(ScientificMasterBindingError):
            obs.validate()

    def test_nonidentical_repeat_fails_closed(self):
        bad_second = copy.copy(FROZEN_REAL_RUN_V18)
        object.__setattr__(bad_second, "tile_read_calls", FROZEN_REAL_RUN_V18.tile_read_calls + 1)
        obs = ScientificMasterRecomputationObservationV18(
            source_dng_sha256=REAL_RECOMPUTATION_OBSERVATION_V18.source_dng_sha256,
            source_cfa_sha256=REAL_RECOMPUTATION_OBSERVATION_V18.source_cfa_sha256,
            archive_commit_sha=REAL_RECOMPUTATION_OBSERVATION_V18.archive_commit_sha,
            implementation_sha256=dict(REAL_RECOMPUTATION_OBSERVATION_V18.implementation_sha256),
            first_run=REAL_RECOMPUTATION_OBSERVATION_V18.first_run,
            second_run=bad_second,
        )
        with self.assertRaises(ScientificMasterBindingError):
            obs.validate()

    def test_manifest_is_stable_and_contains_no_presentation_parameter(self):
        binding = real_scientific_master_binding_v18()
        self.assertEqual(len(binding.manifest_sha256), 64)
        self.assertNotIn("hdr", binding.gauge_id.lower())
        self.assertNotIn("adobe", binding.gauge_id.lower())


if __name__ == "__main__":
    unittest.main()
