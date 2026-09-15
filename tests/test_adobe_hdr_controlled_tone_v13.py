import unittest

from tools.adobe_hdr_controlled_tone_v13 import (
    HdrRenderObservation,
    compare_controlled_tone_pair,
    curve_from_pairs,
)

SOURCE_CFA = "883cbe13fd2a5afe7e9f3f4147df3a8ed3be681340a3c0422933f42d0f4d719c"
SCALARS = "same-lightroom-crs-scalars-except-tone-curves"


def obs(*, name, primaries, tone, extended, maxcll, ymax, satfrac):
    return HdrRenderObservation(
        file_name=name,
        source_cfa_sha256=SOURCE_CFA,
        primaries=primaries,
        transfer="SMPTE_ST_2084_PQ",
        pixel_format="YUV444P10LE",
        hdr_limit_ev=8.0,
        exposure_ev=0.0,
        whites=2.0,
        scalar_settings_fingerprint=SCALARS,
        tone_curve=curve_from_pairs(tone),
        extended_tone_curve=curve_from_pairs(extended),
        maxcll_nits=maxcll,
        decoded_luma_max_code=ymax,
        decoded_luma_code_ceiling=1023,
        decoded_luma_ceiling_fraction=satfrac,
    )


class AdobeHdrControlledToneV13Tests(unittest.TestCase):
    def test_p3_pair_is_clean_one_variable_tone_test(self):
        extreme = obs(
            name="IMG_BNC_TRUTHRAW20260907_094449_565 (8).avif",
            primaries="P3_D65",
            tone=[(0, 0), (17, 255), (34, 255), (51, 255), (68, 255), (85, 255), (102, 255), (119, 255), (136, 255), (153, 255), (170, 255), (187, 255), (204, 255), (221, 255), (238, 255), (255, 255)],
            extended=[(0, 0), (25, 500)],
            maxcll=10000,
            ymax=1023,
            satfrac=0.8806715265181185,
        )
        moderate = obs(
            name="IMG_BNC_TRUTHRAW20260907_094449_565 (9).avif",
            primaries="P3_D65",
            tone=[(0, 28), (139, 215), (255, 255)],
            extended=[(0, 28), (139, 215), (329, 427)],
            maxcll=4652,
            ymax=943,
            satfrac=0.0,
        )
        r = compare_controlled_tone_pair(moderate, extreme)
        self.assertTrue(r.controlled_tone_only)
        self.assertEqual(r.classification, "CONTROLLED_TONE_ONLY_WITH_PRESENTATION_CEILING_SATURATION")
        self.assertFalse(r.a_ceiling_saturated)
        self.assertTrue(r.b_ceiling_saturated)
        self.assertAlmostEqual(r.maxcll_ratio_b_over_a, 10000 / 4652, places=12)
        self.assertAlmostEqual(r.maxcll_delta_ev_b_over_a, 1.104076998076231, places=12)
        self.assertFalse(r.scientific_authority_changed)
        self.assertFalse(r.creates_new_measured_dynamic_range)

    def test_gamut_change_is_not_tone_only(self):
        rec709 = obs(
            name="IMG_BNC_TRUTHRAW20260907_094449_565 (6).avif",
            primaries="REC709",
            tone=[(0, 23), (17, 47), (34, 70), (51, 94), (68, 117), (85, 140), (102, 163), (119, 186), (136, 209), (153, 231), (170, 252), (187, 255), (204, 255), (221, 255), (238, 255), (255, 255)],
            extended=[(0, 23), (176, 260), (384, 500)],
            maxcll=5383,
            ymax=959,
            satfrac=0.0,
        )
        p3 = obs(
            name="IMG_BNC_TRUTHRAW20260907_094449_565 (9).avif",
            primaries="P3_D65",
            tone=[(0, 28), (139, 215), (255, 255)],
            extended=[(0, 28), (139, 215), (329, 427)],
            maxcll=4652,
            ymax=943,
            satfrac=0.0,
        )
        r = compare_controlled_tone_pair(rec709, p3)
        self.assertFalse(r.controlled_tone_only)
        self.assertEqual(r.classification, "NOT_A_CONTROLLED_TONE_ONLY_COMPARISON")

    def test_same_curve_is_not_a_tone_change_test(self):
        a = obs(name="a", primaries="P3_D65", tone=[(0, 0), (255, 255)], extended=[(0, 0), (500, 500)], maxcll=1000, ymax=800, satfrac=0)
        b = obs(name="b", primaries="P3_D65", tone=[(0, 0), (255, 255)], extended=[(0, 0), (500, 500)], maxcll=1100, ymax=810, satfrac=0)
        r = compare_controlled_tone_pair(a, b)
        self.assertFalse(r.controlled_tone_only)


if __name__ == "__main__":
    unittest.main()
