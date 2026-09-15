import unittest

from tools.adobe_hdr_counterbalanced_tone_v14 import (
    HdrRenderObservation,
    compare_counterbalanced_tone,
    decoded_render_is_identical,
    raw_carrier_chain_is_exact,
)

CFA = "883cbe13fd2a5afe7e9f3f4147df3a8ed3be681340a3c0422933f42d0f4d719c"


def obs(**kw):
    base = dict(
        source_cfa_sha256=CFA,
        file_sha256="1" * 64,
        decoded_color_sha256="2" * 64,
        decoded_gainmap_sha256="3" * 64,
        primaries="P3_D65",
        transfer="SMPTE_ST_2084_PQ",
        pixel_format="YUV444P10LE",
        hdr_limit_ev=8.0,
        exposure_ev=0.0,
        highlights=0.0,
        whites=2.0,
        tone_curve=((0, 0), (255, 255)),
        extended_tone_curve=((0, 0), (500, 500)),
        maxcll_nits=1391,
        y_median_code=495.0,
        y_p99_code=697.0,
        y_p999_code=722.0,
        y_p9999_code=732.0,
        y_max_code=810,
        y_code_ceiling=1023,
        y_ceiling_fraction=0.0,
    )
    base.update(kw)
    return HdrRenderObservation(**base)


class AdobeHdrCounterbalancedToneV14Test(unittest.TestCase):
    def test_counterbalanced_curve_darkens_bulk_but_extends_tail(self):
        control = obs()
        extreme = obs(
            file_sha256="4" * 64,
            decoded_color_sha256="5" * 64,
            decoded_gainmap_sha256="6" * 64,
            exposure_ev=-5.0,
            highlights=-100.0,
            whites=-100.0,
            tone_curve=((0, 11), (17, 255), (255, 255)),
            extended_tone_curve=((0, 11), (25, 500)),
            maxcll_nits=10000,
            y_median_code=483.0,
            y_p99_code=684.0,
            y_p999_code=816.0,
            y_p9999_code=889.0,
            y_max_code=1023,
            y_ceiling_fraction=2.0532088675433894e-05,
        )
        result = compare_counterbalanced_tone(control, extreme)
        self.assertTrue(result.bulk_darker_tail_brighter)
        self.assertTrue(result.sparse_output_ceiling_contact)
        self.assertAlmostEqual(result.maxcll_ratio, 10000 / 1391)
        self.assertFalse(result.creates_new_sensor_evidence)
        self.assertFalse(result.recovers_censored_radiance)
        self.assertFalse(result.scientific_master_writeback_allowed)

    def test_container_can_differ_while_decoded_render_is_identical(self):
        a = obs(file_sha256="a" * 64)
        b = obs(file_sha256="b" * 64)
        self.assertTrue(decoded_render_is_identical(a, b))

    def test_changed_gainmap_breaks_decoded_identity(self):
        a = obs()
        b = obs(decoded_gainmap_sha256="9" * 64)
        self.assertFalse(decoded_render_is_identical(a, b))

    def test_raw_carrier_exact_chain_requires_prior_source_proof_and_same_tiles(self):
        h = "830f530916444f8a90826b5b74ea87c1dce525ba7f59cf2f1fd09d9b0082ded5"
        self.assertTrue(raw_carrier_chain_is_exact(h, h, previous_adobe_dng_was_cfa_exact_to_immutable_source=True, tile_count_previous=192, tile_count_new=192))
        self.assertFalse(raw_carrier_chain_is_exact(h, h, previous_adobe_dng_was_cfa_exact_to_immutable_source=False, tile_count_previous=192, tile_count_new=192))
        self.assertFalse(raw_carrier_chain_is_exact(h, "0" * 64, previous_adobe_dng_was_cfa_exact_to_immutable_source=True, tile_count_previous=192, tile_count_new=192))

    def test_mismatched_transport_fails_closed(self):
        with self.assertRaises(ValueError):
            compare_counterbalanced_tone(obs(), obs(primaries="REC709"))


if __name__ == "__main__":
    unittest.main()
