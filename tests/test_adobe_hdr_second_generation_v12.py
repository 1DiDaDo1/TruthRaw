import math
import unittest

from tools.adobe_hdr_second_generation_v12 import (
    OLD_TONE_EXPANDED_REC709_V09,
    SECOND_GENERATION_P3_V12,
    UNEDITED_P3_V09,
    compare_adobe_hdr_renders_v12,
    parent_route_is_proven_v12,
)


class AdobeHdrSecondGenerationV12Tests(unittest.TestCase):
    def test_second_generation_observation_validates(self):
        SECOND_GENERATION_P3_V12.validate()
        self.assertEqual(SECOND_GENERATION_P3_V12.transfer, "SMPTE_ST_2084_PQ")
        self.assertEqual(SECOND_GENERATION_P3_V12.primaries, "P3_D65")

    def test_same_nominal_controls_do_not_imply_same_render(self):
        c = compare_adobe_hdr_renders_v12(OLD_TONE_EXPANDED_REC709_V09, SECOND_GENERATION_P3_V12)
        self.assertTrue(c.same_nominal_hdr_limit)
        self.assertTrue(c.same_nominal_exposure)
        self.assertTrue(c.same_nominal_whites)
        self.assertFalse(c.same_tone_curve)
        self.assertFalse(c.same_extended_tone_curve)
        self.assertFalse(c.nominal_controls_are_sufficient_render_identity)
        self.assertFalse(c.scientific_authority_changed)

    def test_second_generation_peak_is_lower_than_original_tone_expanded_control(self):
        c = compare_adobe_hdr_renders_v12(OLD_TONE_EXPANDED_REC709_V09, SECOND_GENERATION_P3_V12)
        self.assertAlmostEqual(c.maxcll_ratio_b_over_a, 2878.0 / 5259.0, places=12)
        self.assertAlmostEqual(c.maxcll_delta_ev_b_minus_a, math.log2(2878.0 / 5259.0), places=12)
        self.assertLess(c.maxcll_delta_ev_b_minus_a, -0.86)
        self.assertEqual(c.luma_peak_code_delta_b_minus_a, -67)

    def test_second_generation_is_brighter_than_unedited_p3_control(self):
        c = compare_adobe_hdr_renders_v12(UNEDITED_P3_V09, SECOND_GENERATION_P3_V12)
        self.assertAlmostEqual(c.maxcll_delta_ev_b_minus_a, math.log2(2878.0 / 1383.0), places=12)
        self.assertGreater(c.maxcll_delta_ev_b_minus_a, 1.05)

    def test_immediate_parent_route_is_not_invented(self):
        self.assertFalse(parent_route_is_proven_v12(SECOND_GENERATION_P3_V12))
        self.assertTrue(parent_route_is_proven_v12(OLD_TONE_EXPANDED_REC709_V09))


if __name__ == "__main__":
    unittest.main()
