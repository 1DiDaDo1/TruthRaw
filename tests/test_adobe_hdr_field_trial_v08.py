import math
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from open_world_foundations_v01 import ContractError  # noqa: E402
from adobe_hdr_field_trial_v08 import (  # noqa: E402
    LightroomHdrRoundTripObservationV08,
    REAL_TELE_FIELD_TRIAL_CANDIDATE_V08,
    RealTeleHdrCandidateV08,
    build_real_tele_lightroom_field_trial_v08,
    derive_real_tele_headroom_diagnostics_v08,
)


class AdobeHdrFieldTrialV08Tests(unittest.TestCase):
    def test_real_candidate_is_exact_single_frame_tele_source(self):
        c = REAL_TELE_FIELD_TRIAL_CANDIDATE_V08.validate()
        self.assertEqual(c.source_sha256, "7930ba5db0fe1b7b9dcc09e9337efbca76b8877fb78a6dc4667761bf25159b67")
        self.assertEqual((c.width, c.height, c.cfa), (4080, 3072, "BGGR"))
        self.assertEqual(c.physical_camera_id, "5")
        self.assertEqual(c.physical_frame_count, 1)
        self.assertEqual(c.independent_evidence_count, 1)
        self.assertFalse(c.uses_exposure_merge)
        self.assertFalse(c.fake_hdr_merge_metadata)

    def test_stage2_overrange_is_real_but_small_and_not_adobe_headroom(self):
        d = derive_real_tele_headroom_diagnostics_v08(REAL_TELE_FIELD_TRIAL_CANDIDATE_V08)
        self.assertAlmostEqual(d.stage2_over_unity_headroom_ev, math.log2(1.2977731227874756), places=12)
        self.assertAlmostEqual(d.q99_anchored_headroom_ev, math.log2(1.2977731227874756 / 0.9892458261670347), places=12)
        self.assertIn("do not predict Lightroom HDR", d.interpretation)

    def test_existing_sdr_preview_scalar_compresses_peak_below_unity(self):
        d = derive_real_tele_headroom_diagnostics_v08(REAL_TELE_FIELD_TRIAL_CANDIDATE_V08)
        self.assertLess(d.legacy_sdr_preview_peak, 1.0)
        self.assertLess(d.legacy_sdr_preview_peak_ev_vs_unity, 0.0)

    def test_censoring_is_kept_as_lower_bound_not_recovered_exact_radiance(self):
        d = derive_real_tele_headroom_diagnostics_v08(REAL_TELE_FIELD_TRIAL_CANDIDATE_V08)
        self.assertFalse(d.exact_censored_radiance_recovery_allowed)
        self.assertEqual(REAL_TELE_FIELD_TRIAL_CANDIDATE_V08.source_white_count, 217)
        self.assertGreater(d.source_white_fraction, 0.0)

    def test_plan_requires_manual_real_raw_hdr_activation_without_merge(self):
        plan = build_real_tele_lightroom_field_trial_v08(REAL_TELE_FIELD_TRIAL_CANDIDATE_V08)
        self.assertEqual(plan.hdr_activation, "MANUAL_EDIT_HDR_ON_REAL_SINGLE_EXPOSURE_DNG")
        self.assertFalse(plan.automatic_hdr_activation_assumed)
        self.assertFalse(plan.uses_exposure_merge)
        self.assertFalse(plan.fake_hdr_merge_metadata)
        self.assertFalse(plan.new_measured_evidence_created_by_hdr_editing)
        self.assertFalse(plan.scientific_master_writeback_from_adobe)

    def test_multiple_frame_or_merge_candidate_is_rejected(self):
        c = REAL_TELE_FIELD_TRIAL_CANDIDATE_V08
        bad = RealTeleHdrCandidateV08(**{**c.__dict__, "physical_frame_count": 2})
        with self.assertRaises(ContractError):
            bad.validate()
        bad_merge = RealTeleHdrCandidateV08(**{**c.__dict__, "uses_exposure_merge": True})
        with self.assertRaises(ContractError):
            bad_merge.validate()

    def test_roundtrip_observation_is_presentation_only(self):
        c = REAL_TELE_FIELD_TRIAL_CANDIDATE_V08
        o = LightroomHdrRoundTripObservationV08(
            source_sha256=c.source_sha256,
            lightroom_version="field-trial",
            hdr_button_available=True,
            hdr_mode_enabled=True,
            hdr_limit_ev_setting=4.0,
            visualize_hdr_headroom_ev_observed=1.5,
            display_headroom_ev_reported=2.0,
            profile_name="recorded-in-adobe",
            process_version="recorded-in-adobe",
            exposure_adjustment_ev=0.0,
            tone_controls_modified=False,
            local_masks_used=False,
            ai_scene_edit_used=False,
            exposure_merge_used=False,
        ).validate(c)
        self.assertFalse(o.scientific_authority_writeback_allowed)
        self.assertTrue(o.observed_adobe_headroom_is_presentation_evidence_only)

    def test_roundtrip_rejects_ai_or_exposure_merge(self):
        c = REAL_TELE_FIELD_TRIAL_CANDIDATE_V08
        base = dict(
            source_sha256=c.source_sha256,
            lightroom_version="field-trial",
            hdr_button_available=True,
            hdr_mode_enabled=True,
            hdr_limit_ev_setting=None,
            visualize_hdr_headroom_ev_observed=None,
            display_headroom_ev_reported=None,
            profile_name="p",
            process_version="pv",
            exposure_adjustment_ev=0.0,
            tone_controls_modified=False,
            local_masks_used=False,
            ai_scene_edit_used=False,
            exposure_merge_used=False,
        )
        with self.assertRaises(ContractError):
            LightroomHdrRoundTripObservationV08(**{**base, "ai_scene_edit_used": True}).validate(c)
        with self.assertRaises(ContractError):
            LightroomHdrRoundTripObservationV08(**{**base, "exposure_merge_used": True}).validate(c)


if __name__ == "__main__":
    unittest.main()
