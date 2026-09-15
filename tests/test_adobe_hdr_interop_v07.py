import math
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from open_world_foundations_v01 import ContractError  # noqa: E402
from open_world_dynamic_authority_v05 import (  # noqa: E402
    CensoringKind,
    DynamicAuthority,
    DynamicAuthoritySample,
)
from adobe_hdr_interop_v07 import (  # noqa: E402
    AdobeHdrFormat,
    AdobeHdrRoute,
    build_adobe_hdr_interop_plan,
    derive_real_hdr_headroom,
)


def measured(value=4.0, p95=0.5):
    return DynamicAuthoritySample(
        authority=DynamicAuthority.MEASURED,
        scene_linear_estimate=value,
        uncertainty_p95=p95,
        support=1.0,
        source_sample_index=7,
    )


def calibrated(value=2.0, p95=0.1):
    return DynamicAuthoritySample(
        authority=DynamicAuthority.CALIBRATED_ESTIMATE,
        scene_linear_estimate=value,
        uncertainty_p95=p95,
        support=1.0,
        source_sample_index=8,
    )


def reconstructed(value=3.0, p95=0.25):
    return DynamicAuthoritySample(
        authority=DynamicAuthority.RECONSTRUCTED,
        scene_linear_estimate=value,
        uncertainty_p95=p95,
        support=0.8,
    )


class AdobeHdrInteropV07Tests(unittest.TestCase):
    def test_single_exposure_raw_route_is_valid_without_merge(self):
        plan = build_adobe_hdr_interop_plan(
            [measured()],
            route=AdobeHdrRoute.SINGLE_EXPOSURE_RAW,
            output_format=AdobeHdrFormat.DNG,
            sdr_white_scene_value=1.0,
        )
        self.assertEqual(plan.physical_frame_count, 1)
        self.assertFalse(plan.uses_exposure_merge)
        self.assertFalse(plan.fake_hdr_merge_metadata)
        self.assertEqual(plan.hdr_exchange_semantics, "ADOBE_RAW_PIPELINE_FROM_SINGLE_PHYSICAL_EXPOSURE")
        self.assertIsNone(plan.scientific_master_fixed_hdr_limit_ev)

    def test_exposure_merge_is_rejected(self):
        with self.assertRaises(ContractError):
            build_adobe_hdr_interop_plan(
                [measured()],
                route=AdobeHdrRoute.SINGLE_EXPOSURE_RAW,
                output_format=AdobeHdrFormat.DNG,
                sdr_white_scene_value=1.0,
                uses_exposure_merge=True,
            )

    def test_fake_merge_metadata_is_rejected(self):
        with self.assertRaises(ContractError):
            build_adobe_hdr_interop_plan(
                [measured()],
                route=AdobeHdrRoute.SINGLE_EXPOSURE_RAW,
                output_format=AdobeHdrFormat.DNG,
                sdr_white_scene_value=1.0,
                fake_hdr_merge_metadata=True,
            )

    def test_counterfactual_cannot_supply_scientific_hdr_headroom(self):
        counterfactual = DynamicAuthoritySample(
            authority=DynamicAuthority.COUNTERFACTUAL,
            scene_linear_estimate=64.0,
            parent_record_id="illumination-hypothesis-1",
        )
        with self.assertRaises(ContractError):
            derive_real_hdr_headroom([measured(), counterfactual], sdr_white_scene_value=1.0)

    def test_appearance_only_cannot_supply_scientific_hdr_headroom(self):
        appearance = DynamicAuthoritySample(
            authority=DynamicAuthority.APPEARANCE_ONLY,
            scene_linear_estimate=32.0,
            parent_record_id="look-1",
        )
        with self.assertRaises(ContractError):
            derive_real_hdr_headroom([appearance], sdr_white_scene_value=1.0)

    def test_headroom_uses_p95_lower_bound_for_conservative_claim(self):
        h = derive_real_hdr_headroom(
            [measured(4.0, 0.5), calibrated(2.0, 0.1), reconstructed(3.0, 0.25)],
            sdr_white_scene_value=1.0,
        )
        self.assertAlmostEqual(h.nominal_headroom_ev, 2.0)
        self.assertAlmostEqual(h.conservative_p95_supported_headroom_ev, math.log2(3.5))
        self.assertEqual(h.contributing_sample_count, 3)

    def test_censored_highlight_does_not_become_exact_hdr_radiance(self):
        clipped = DynamicAuthoritySample(
            authority=DynamicAuthority.CENSORED,
            scene_linear_estimate=None,
            support=0.0,
            source_sample_index=9,
            censoring_kind=CensoringKind.HIGHLIGHT_SATURATION,
            censor_bound=8.0,
        )
        h = derive_real_hdr_headroom([clipped], sdr_white_scene_value=1.0)
        self.assertIsNone(h.nominal_max_scene_value)
        self.assertIsNone(h.nominal_headroom_ev)
        self.assertEqual(h.ignored_censored_count, 1)

    def test_unknown_does_not_add_headroom(self):
        unknown = DynamicAuthoritySample(DynamicAuthority.UNKNOWN, None)
        h = derive_real_hdr_headroom([unknown, measured(2.0, 0.25)], sdr_white_scene_value=1.0)
        self.assertEqual(h.ignored_unknown_count, 1)
        self.assertAlmostEqual(h.nominal_headroom_ev, 1.0)

    def test_scene_master_render_may_not_masquerade_as_raw_dng(self):
        with self.assertRaises(ContractError):
            build_adobe_hdr_interop_plan(
                [measured()],
                route=AdobeHdrRoute.TRUTHRAW_SCENE_MASTER_RENDER,
                output_format=AdobeHdrFormat.DNG,
                sdr_white_scene_value=1.0,
            )

    def test_scene_master_render_keeps_gain_map_presentation_only(self):
        plan = build_adobe_hdr_interop_plan(
            [measured(8.0, 1.0)],
            route=AdobeHdrRoute.TRUTHRAW_SCENE_MASTER_RENDER,
            output_format=AdobeHdrFormat.AVIF,
            sdr_white_scene_value=1.0,
        )
        self.assertTrue(plan.gain_map_is_presentation_only)
        self.assertFalse(plan.scientific_master_writeback_from_adobe)
        self.assertEqual(plan.hdr_exchange_semantics, "TRUTHRAW_RENDERED_SCENE_HDR_EXCHANGE")

    def test_multiple_physical_frames_are_outside_truthraw_single_frame_route(self):
        with self.assertRaises(ContractError):
            build_adobe_hdr_interop_plan(
                [measured()],
                route=AdobeHdrRoute.SINGLE_EXPOSURE_RAW,
                output_format=AdobeHdrFormat.DNG,
                sdr_white_scene_value=1.0,
                physical_frame_count=2,
                independent_evidence_count=2,
            )


if __name__ == "__main__":
    unittest.main()
