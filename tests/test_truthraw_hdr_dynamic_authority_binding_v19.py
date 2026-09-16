import copy
import unittest

from tools.truthraw_hdr_dynamic_authority_binding_v19 import (
    DYNAMIC_AUTHORITY_FIELD_SHA256_V19,
    FROZEN_REAL_RUN_V19,
    PIXELS_V19,
    REAL_RECOMPUTATION_OBSERVATION_V19,
    RGB_SAMPLES_V19,
    SOURCE_WHITE_CENSORED_V19,
    AuthorityCountsV19,
    DynamicAuthorityBindingError,
    DynamicAuthorityRecomputationObservationV19,
    p3_projection_runtime_binding_v19,
    real_hdr_science_binding_v19,
    scientific_projection_binding_v19,
)
from tools.truthraw_hdr_scientific_master_binding_v18 import SCIENTIFIC_MASTER_SHA256_V18


class DynamicAuthorityBindingV19Tests(unittest.TestCase):
    def test_real_field_counts_are_complete_and_fail_closed_at_censored_sites(self):
        run = FROZEN_REAL_RUN_V19.validate()
        self.assertEqual(run.counts.total, RGB_SAMPLES_V19)
        self.assertEqual(run.counts.calibrated_estimate + run.counts.censored, PIXELS_V19)
        self.assertEqual(run.counts.censored, SOURCE_WHITE_CENSORED_V19)
        self.assertEqual(run.counts.unknown, 2 * SOURCE_WHITE_CENSORED_V19)
        self.assertEqual(run.counts.reconstructed, 2 * (PIXELS_V19 - SOURCE_WHITE_CENSORED_V19))

    def test_two_partition_sizes_produce_the_same_real_field_identity(self):
        obs = REAL_RECOMPUTATION_OBSERVATION_V19.validate()
        self.assertTrue(obs.partition_invariance_proven)
        self.assertNotEqual(obs.first_execution_band_rows, obs.second_execution_band_rows)
        self.assertEqual(obs.first_run, obs.second_run)
        self.assertEqual(obs.first_run.field_sha256, DYNAMIC_AUTHORITY_FIELD_SHA256_V19)

    def test_field_identity_is_bound_to_exact_scientific_master(self):
        run = FROZEN_REAL_RUN_V19.validate()
        self.assertEqual(run.scientific_master_verified_sha256, SCIENTIFIC_MASTER_SHA256_V18)
        self.assertNotEqual(run.field_sha256, SCIENTIFIC_MASTER_SHA256_V18)

    def test_complete_v16_science_binding_is_now_real_not_placeholder(self):
        b = scientific_projection_binding_v19()
        self.assertEqual(b.scientific_master_sha256, SCIENTIFIC_MASTER_SHA256_V18)
        self.assertEqual(b.dynamic_authority_sha256, DYNAMIC_AUTHORITY_FIELD_SHA256_V19)
        real = real_hdr_science_binding_v19()
        self.assertEqual(len(real.manifest_sha256), 64)

    def test_source_bound_p3_runtime_binding_is_complete_but_not_physical_color_calibration(self):
        runtime = p3_projection_runtime_binding_v19()
        self.assertEqual(runtime.science.dynamic_authority_sha256, DYNAMIC_AUTHORITY_FIELD_SHA256_V19)
        self.assertEqual(runtime.color.authority_label, "SOURCE_METADATA_BOUND_L1_NOT_PHYSICAL_CALIBRATION")

    def test_wrong_field_hash_fails_closed(self):
        bad = copy.copy(real_hdr_science_binding_v19())
        object.__setattr__(bad, "dynamic_authority_sha256", "0" * 64)
        with self.assertRaises(DynamicAuthorityBindingError):
            bad.validate()

    def test_nonidentical_partition_repeat_fails_closed(self):
        bad_run = copy.copy(FROZEN_REAL_RUN_V19)
        object.__setattr__(bad_run, "signed_nonpositive_estimate_count", FROZEN_REAL_RUN_V19.signed_nonpositive_estimate_count + 1)
        obs = DynamicAuthorityRecomputationObservationV19(
            source_dng_sha256=REAL_RECOMPUTATION_OBSERVATION_V19.source_dng_sha256,
            source_cfa_sha256=REAL_RECOMPUTATION_OBSERVATION_V19.source_cfa_sha256,
            scientific_master_sha256=REAL_RECOMPUTATION_OBSERVATION_V19.scientific_master_sha256,
            uncertainty_model_sha256=REAL_RECOMPUTATION_OBSERVATION_V19.uncertainty_model_sha256,
            uncertainty_binding_sha256=REAL_RECOMPUTATION_OBSERVATION_V19.uncertainty_binding_sha256,
            implementation_sha256=dict(REAL_RECOMPUTATION_OBSERVATION_V19.implementation_sha256),
            first_run=REAL_RECOMPUTATION_OBSERVATION_V19.first_run,
            second_run=bad_run,
            first_execution_band_rows=192,
            second_execution_band_rows=257,
        )
        with self.assertRaises(DynamicAuthorityBindingError):
            obs.validate()

    def test_count_object_rejects_incomplete_field(self):
        with self.assertRaises(DynamicAuthorityBindingError):
            AuthorityCountsV19(1, 2, 3, 4).validate(100)


if __name__ == "__main__":
    unittest.main()
