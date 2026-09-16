import math
import unittest

from tools.open_world_dynamic_authority_v05 import (
    CensoringKind,
    DynamicAuthority,
    DynamicAuthoritySample,
)
from tools.open_world_foundations_v01 import ContractError
from tools.truthraw_hdr_projection_v15 import (
    HdrPresentationRequestV15,
    HdrPrimaries,
    HdrProjectionManifestV15,
    HdrTransfer,
    PiecewiseStopCurveV15,
    ScientificProjectionBindingV15,
    pq_code_from_nits,
    project_scientific_sample,
    summarize_scientific_headroom,
)


def binding():
    return ScientificProjectionBindingV15(
        source_dng_sha256="1" * 64,
        source_cfa_sha256="2" * 64,
        scientific_master_sha256="3" * 64,
        dynamic_authority_sha256="4" * 64,
        reference_l0=1.0,
    )


def request():
    curve = PiecewiseStopCurveV15(
        points=((-4.0, -4.0), (0.0, 0.0), (3.0, 3.0), (5.0, 5.0)),
        curve_id="neutral-stop-preserving-test",
    )
    return HdrPresentationRequestV15(
        primaries=HdrPrimaries.P3_D65,
        transfer=HdrTransfer.SMPTE_ST_2084_PQ,
        bit_depth=10,
        chroma="4:4:4",
        reference_white_nits=100.0,
        target_peak_nits=3200.0,
        curve=curve,
        adobe_interop_hdr_limit_ev=8.0,
    )


class TruthRawHdrProjectionV15Test(unittest.TestCase):
    def test_pq_scalar_encoder_respects_ceiling(self):
        self.assertEqual(pq_code_from_nits(0.0), 0)
        self.assertEqual(pq_code_from_nits(10000.0), 1023)
        self.assertLess(pq_code_from_nits(100.0), pq_code_from_nits(1000.0))

    def test_scientific_headroom_separates_supported_from_censored_bound(self):
        samples = [
            DynamicAuthoritySample(DynamicAuthority.MEASURED, 4.0, 1.0, 1.0, source_sample_index=1),
            DynamicAuthoritySample(DynamicAuthority.RECONSTRUCTED, 8.0, 2.0, 0.8),
            DynamicAuthoritySample(
                DynamicAuthority.CENSORED,
                None,
                support=0.0,
                censoring_kind=CensoringKind.HIGHLIGHT_SATURATION,
                censor_bound=16.0,
            ),
            DynamicAuthoritySample(DynamicAuthority.UNKNOWN, None, support=0.0),
        ]
        s = summarize_scientific_headroom(samples, 1.0)
        self.assertEqual(s.supported_sample_count, 2)
        self.assertEqual(s.censored_sample_count, 1)
        self.assertEqual(s.unknown_sample_count, 1)
        self.assertAlmostEqual(s.nominal_supported_peak_ev, 3.0)
        self.assertAlmostEqual(s.conservative_p95_supported_peak_ev, math.log2(6.0))
        self.assertAlmostEqual(s.censored_lower_bound_peak_ev, 4.0)

    def test_supported_sample_projects_without_authority_upgrade(self):
        sample = DynamicAuthoritySample(
            DynamicAuthority.CALIBRATED_ESTIMATE,
            4.0,
            uncertainty_p95=0.5,
            support=1.0,
            source_sample_index=2,
        )
        result = project_scientific_sample(sample, binding(), request())
        self.assertEqual(result.authority, DynamicAuthority.CALIBRATED_ESTIMATE)
        self.assertTrue(result.exact_projection_allowed)
        self.assertAlmostEqual(result.nominal_scene_ev, 2.0)
        self.assertIsNotNone(result.pq_code)
        self.assertFalse(result.creates_new_sensor_evidence)
        self.assertFalse(result.upgrades_scientific_authority)

    def test_censored_sample_keeps_bound_and_gets_no_exact_code(self):
        sample = DynamicAuthoritySample(
            DynamicAuthority.CENSORED,
            None,
            support=0.0,
            censoring_kind=CensoringKind.HIGHLIGHT_SATURATION,
            censor_bound=8.0,
        )
        result = project_scientific_sample(sample, binding(), request())
        self.assertFalse(result.exact_projection_allowed)
        self.assertIsNone(result.pq_code)
        self.assertAlmostEqual(result.censor_bound_ev, 3.0)

    def test_appearance_state_cannot_enter_scientific_projection(self):
        sample = DynamicAuthoritySample(
            DynamicAuthority.APPEARANCE_ONLY,
            4.0,
            support=0.0,
            parent_record_id="appearance-parent",
        )
        with self.assertRaises(ContractError):
            project_scientific_sample(sample, binding(), request())

    def test_adobe_hdr_limit_is_metadata_not_scientific_headroom(self):
        samples = [DynamicAuthoritySample(DynamicAuthority.RECONSTRUCTED, 4.0, 0.5, 1.0)]
        h = summarize_scientific_headroom(samples, 1.0)
        m = HdrProjectionManifestV15(binding=binding(), request=request(), headroom=h)
        m.validate()
        self.assertAlmostEqual(h.nominal_supported_peak_ev, 2.0)
        self.assertEqual(m.request.adobe_interop_hdr_limit_ev, 8.0)
        self.assertNotEqual(h.nominal_supported_peak_ev, m.request.adobe_interop_hdr_limit_ev)
        self.assertEqual(len(m.manifest_sha256), 64)

    def test_nonmonotonic_curve_and_over_ceiling_target_fail_closed(self):
        with self.assertRaises(ContractError):
            PiecewiseStopCurveV15(points=((0.0, 0.0), (1.0, -1.0)), curve_id="bad").validate()
        with self.assertRaises(ContractError):
            HdrPresentationRequestV15(
                primaries=HdrPrimaries.P3_D65,
                transfer=HdrTransfer.SMPTE_ST_2084_PQ,
                bit_depth=10,
                chroma="4:4:4",
                reference_white_nits=100.0,
                target_peak_nits=10001.0,
                curve=PiecewiseStopCurveV15(points=((0.0, 0.0), (1.0, 1.0)), curve_id="bad-peak"),
            ).validate()

    def test_curve_endpoint_clipping_is_presentation_only(self):
        sample = DynamicAuthoritySample(DynamicAuthority.RECONSTRUCTED, 64.0, 1.0, 1.0)
        result = project_scientific_sample(sample, binding(), request())
        self.assertTrue(result.presentation_clipped_high)
        self.assertFalse(result.creates_new_sensor_evidence)
        self.assertEqual(result.authority, DynamicAuthority.RECONSTRUCTED)


if __name__ == "__main__":
    unittest.main()
