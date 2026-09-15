import math
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from adobe_hdr_roundtrip_v09 import (  # noqa: E402
    TONE_EXPANDED_REC709_V09,
    UNEDITED_P3_V09,
    UNEDITED_REC709_V09,
    compare_presentation_expansion_v09,
    compare_unedited_gamut_exports_v09,
)


class AdobeHdrRoundTripV09Tests(unittest.TestCase):
    def test_unedited_exports_are_real_pq_hdr(self):
        for obs in (UNEDITED_REC709_V09, UNEDITED_P3_V09):
            obs.validate()
            self.assertTrue(obs.hdr_edit_mode)
            self.assertEqual(obs.hdr_limit_ev, 8.0)
            self.assertEqual(obs.transfer, "SMPTE_ST_2084_PQ")
            self.assertEqual(obs.bit_depth, 10)
            self.assertEqual(obs.chroma, "4:4:4")
            self.assertTrue(obs.gainmap_stream_present)
            self.assertTrue(obs.tmap_compatible_brand_present)
            self.assertFalse(obs.scene_referred_export_metadata)
            self.assertFalse(obs.creates_new_measured_evidence)
            self.assertFalse(obs.scientific_authority_writeback_allowed)

    def test_gamut_only_export_keeps_luminance_nearly_identical(self):
        result = compare_unedited_gamut_exports_v09(UNEDITED_REC709_V09, UNEDITED_P3_V09)
        self.assertTrue(result["maxcll_equal"])
        self.assertLess(result["max_luminance_relative_delta"], 0.001)
        self.assertFalse(result["new_measured_evidence_created"])

    def test_tone_expansion_uses_same_source_and_same_hdr_limit(self):
        result = compare_presentation_expansion_v09(UNEDITED_REC709_V09, TONE_EXPANDED_REC709_V09)
        self.assertTrue(result.physical_source_same)
        self.assertTrue(result.hdr_limit_ev_same)
        self.assertEqual(result.classification, "ADOBE_TONE_EXPANSION_PRESENTATION_ONLY_NO_NEW_SENSOR_EVIDENCE")
        self.assertFalse(result.new_measured_dynamic_range_created)
        self.assertFalse(result.scientific_master_writeback_allowed)

    def test_tone_expansion_adds_about_1p92_ev_of_presentation_peak(self):
        result = compare_presentation_expansion_v09(UNEDITED_REC709_V09, TONE_EXPANDED_REC709_V09)
        self.assertAlmostEqual(result.max_luminance_ratio, 3.78425309524017, places=9)
        self.assertAlmostEqual(result.max_luminance_added_presentation_ev, 1.9200085810232714, places=9)
        self.assertAlmostEqual(result.maxcll_added_presentation_ev, 1.9269873403302, places=9)

    def test_average_brightness_change_is_presentation_not_sensor_truth(self):
        result = compare_presentation_expansion_v09(UNEDITED_REC709_V09, TONE_EXPANDED_REC709_V09)
        self.assertAlmostEqual(result.average_luminance_ratio, 2.6137268351418275, places=9)
        self.assertAlmostEqual(result.average_luminance_added_presentation_ev, 1.3861083705980763, places=9)
        self.assertFalse(result.new_measured_dynamic_range_created)

    def test_only_expected_global_tone_controls_changed_in_frozen_observation(self):
        self.assertEqual(UNEDITED_REC709_V09.exposure_2012, 0.0)
        self.assertEqual(TONE_EXPANDED_REC709_V09.exposure_2012, 0.0)
        self.assertEqual(UNEDITED_REC709_V09.whites_2012, 0.0)
        self.assertEqual(TONE_EXPANDED_REC709_V09.whites_2012, 2.0)
        self.assertEqual(UNEDITED_REC709_V09.tone_curve_name, "Linear")
        self.assertEqual(TONE_EXPANDED_REC709_V09.tone_curve_name, "Custom")
        self.assertEqual(TONE_EXPANDED_REC709_V09.extended_tone_curve_points[-1], (386, 500))


if __name__ == "__main__":
    unittest.main()
