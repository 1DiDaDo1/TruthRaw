import unittest

from tools.open_world_dynamic_authority_v05 import DynamicAuthority
from tools.open_world_foundations_v01 import ContractError
from tools.truthraw_hdr_projection_v15 import (
    HdrPresentationRequestV15,
    HdrPrimaries,
    HdrTransfer,
    PiecewiseStopCurveV15,
    ScientificProjectionBindingV15,
)
from tools.truthraw_hdr_projection_runtime_v16 import (
    ColorimetricProjectionBindingV16,
    ProjectionRuntimeBindingV16,
    ScientificRgbPixelV16,
    StreamingHdrProjectionAccumulatorV16,
    project_rgb_pixel_v16,
)

SHA_SOURCE = "7930ba5db0fe1b7b9dcc09e9337efbca76b8877fb78a6dc4667761bf25159b67"
SHA_CFA = "883cbe13fd2a5afe7e9f3f4147df3a8ed3be681340a3c0422933f42d0f4d719c"
SHA_MASTER = "1" * 64
SHA_DAF = "2" * 64
SHA_COLOR = "3" * 64


def science_binding():
    return ScientificProjectionBindingV15(
        source_dng_sha256=SHA_SOURCE,
        source_cfa_sha256=SHA_CFA,
        scientific_master_sha256=SHA_MASTER,
        dynamic_authority_sha256=SHA_DAF,
        reference_l0=1.0,
    )


def color_binding(primaries=HdrPrimaries.P3_D65, matrix=None):
    if matrix is None:
        matrix = ((1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0))
    return ColorimetricProjectionBindingV16(
        transform_sha256=SHA_COLOR,
        target_primaries=primaries,
        matrix_rgb_to_target=matrix,
        authority_label="SOURCE_METADATA_BOUND_TEST_FIXTURE",
    )


def request(adobe_limit=8.0, peak=2000.0):
    return HdrPresentationRequestV15(
        primaries=HdrPrimaries.P3_D65,
        transfer=HdrTransfer.SMPTE_ST_2084_PQ,
        bit_depth=10,
        chroma="RGB",
        reference_white_nits=200.0,
        target_peak_nits=peak,
        curve=PiecewiseStopCurveV15(
            points=((-4.0, -4.0), (0.0, 0.0), (1.0, 0.8), (2.0, 1.5), (4.0, 3.0)),
            curve_id="V16_TEST_MONOTONIC",
        ),
        adobe_interop_hdr_limit_ev=adobe_limit,
    )


def runtime_binding(color=None):
    return ProjectionRuntimeBindingV16(science_binding(), color or color_binding())


def measured(rgb=(1.0, 1.0, 1.0), uncertainty=(0.01, 0.01, 0.01)):
    return ScientificRgbPixelV16(
        rgb=rgb,
        authority=(DynamicAuthority.MEASURED,) * 3,
        uncertainty_p95=uncertainty,
    )


class TruthRawHdrProjectionRuntimeV16Tests(unittest.TestCase):
    def test_supported_pixel_projects_and_preserves_authority_uncertainty(self):
        px = measured((2.0, 2.0, 2.0), (0.1, 0.2, 0.3))
        out = project_rgb_pixel_v16(px, runtime_binding(), request())
        self.assertTrue(out.exact_projection_allowed)
        self.assertIsNotNone(out.pq_rgb_codes)
        self.assertEqual(out.source_authority, px.authority)
        self.assertEqual(out.source_uncertainty_p95, px.uncertainty_p95)
        self.assertFalse(out.creates_new_sensor_evidence)
        self.assertFalse(out.upgrades_scientific_authority)
        self.assertFalse(out.scientific_master_writeback_allowed)

    def test_censored_or_unknown_channel_withholds_exact_projection(self):
        for authority in (DynamicAuthority.CENSORED, DynamicAuthority.UNKNOWN):
            px = ScientificRgbPixelV16(
                rgb=(1.0, 1.0, 1.0),
                authority=(DynamicAuthority.MEASURED, authority, DynamicAuthority.RECONSTRUCTED),
                uncertainty_p95=(0.1, None, 0.2),
            )
            out = project_rgb_pixel_v16(px, runtime_binding(), request())
            self.assertFalse(out.exact_projection_allowed)
            self.assertIsNone(out.pq_rgb_codes)
            self.assertIsNone(out.display_luminance_nits)

    def test_counterfactual_and_appearance_are_rejected(self):
        for authority in (DynamicAuthority.COUNTERFACTUAL, DynamicAuthority.APPEARANCE_ONLY):
            px = ScientificRgbPixelV16(
                rgb=(1.0, 1.0, 1.0),
                authority=(DynamicAuthority.MEASURED, authority, DynamicAuthority.MEASURED),
                uncertainty_p95=(0.1, 0.1, 0.1),
            )
            with self.assertRaises(ContractError):
                project_rgb_pixel_v16(px, runtime_binding(), request())

    def test_negative_target_component_is_explicit_presentation_gamut_clip(self):
        matrix = ((1.0, -2.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0))
        px = measured((1.0, 0.6, 0.5))
        out = project_rgb_pixel_v16(px, runtime_binding(color_binding(matrix=matrix)), request())
        self.assertTrue(out.negative_gamut_clip)
        self.assertTrue(out.exact_projection_allowed)
        self.assertIsNotNone(out.pq_rgb_codes)
        self.assertFalse(out.creates_new_sensor_evidence)

    def test_channel_peak_clip_is_presentation_only(self):
        # Strong red transform creates an out-of-display channel while luminance stays finite.
        matrix = ((12.0, 0.0, 0.0), (0.0, 0.1, 0.0), (0.0, 0.0, 0.1))
        out = project_rgb_pixel_v16(
            measured((4.0, 1.0, 1.0)),
            runtime_binding(color_binding(matrix=matrix)),
            request(peak=1000.0),
        )
        self.assertTrue(out.channel_peak_clip)
        self.assertTrue(out.presentation_clipped_high)
        self.assertLessEqual(max(out.display_rgb_nits), 1000.0)
        self.assertFalse(out.upgrades_scientific_authority)

    def test_target_primaries_must_match_color_binding(self):
        bad = runtime_binding(color_binding(primaries=HdrPrimaries.REC709))
        with self.assertRaises(ContractError):
            project_rgb_pixel_v16(measured(), bad, request())

    def test_adobe_hdr_limit_metadata_does_not_change_projected_pixels(self):
        px = measured((1.7, 1.2, 0.8))
        a = project_rgb_pixel_v16(px, runtime_binding(), request(adobe_limit=2.0))
        b = project_rgb_pixel_v16(px, runtime_binding(), request(adobe_limit=8.0))
        self.assertEqual(a.pq_rgb_codes, b.pq_rgb_codes)
        self.assertEqual(a.display_rgb_nits, b.display_rgb_nits)

    def test_streaming_hash_is_chunk_boundary_independent(self):
        pixels = [
            measured((0.5, 0.5, 0.5)),
            measured((1.0, 0.8, 0.6)),
            measured((2.0, 1.5, 1.0), (0.2, 0.2, 0.2)),
            ScientificRgbPixelV16(
                rgb=(1.0, 1.0, 1.0),
                authority=(DynamicAuthority.MEASURED, DynamicAuthority.UNKNOWN, DynamicAuthority.RECONSTRUCTED),
                uncertainty_p95=(0.1, None, 0.2),
            ),
        ]
        one = StreamingHdrProjectionAccumulatorV16(runtime_binding(), request())
        one.push_many(pixels)
        s1 = one.finish()

        two = StreamingHdrProjectionAccumulatorV16(runtime_binding(), request())
        two.push_many(pixels[:1])
        two.push_many(pixels[1:3])
        two.push_many(pixels[3:])
        s2 = two.finish()

        self.assertEqual(s1.content_sha256, s2.content_sha256)
        self.assertEqual(s1.pixel_count, 4)
        self.assertEqual(s1.exact_projected_count, 3)
        self.assertEqual(s1.withheld_authority_count, 1)
        self.assertIsNone(s1.fixed_scene_dynamic_range_ceiling_ev)
        self.assertFalse(s1.scientific_master_writeback_allowed)

    def test_empty_stream_fails_closed(self):
        acc = StreamingHdrProjectionAccumulatorV16(runtime_binding(), request())
        with self.assertRaises(ContractError):
            acc.finish()


if __name__ == "__main__":
    unittest.main()
