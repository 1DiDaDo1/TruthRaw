import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from open_world_foundations_v01 import ContractError  # noqa: E402
from open_world_dynamic_authority_v05 import (  # noqa: E402
    CensoringKind,
    DynamicAuthority,
    DynamicAuthorityFieldBinding,
    DynamicAuthoritySample,
    DynamicFieldPurpose,
    HardwareBudget,
    StreamingDynamicAuthorityAccumulator,
    plan_dynamic_execution,
    truthrange_ev,
)

H = "a" * 64
H2 = "b" * 64


def binding(width=4, height=2, purpose=DynamicFieldPurpose.SCIENTIFIC_MASTER):
    return DynamicAuthorityFieldBinding(
        frame_id="open-world-frame-1",
        width=width,
        height=height,
        reference_l0=1.0,
        source_evidence_sha256=H,
        scientific_master_sha256=H2,
        purpose=purpose,
    )


def measured(value, i, p95=0.01):
    return DynamicAuthoritySample(
        DynamicAuthority.MEASURED,
        value,
        uncertainty_p95=p95,
        support=1.0,
        source_sample_index=i,
    )


def reconstructed(value, p95=0.02, support=0.8):
    return DynamicAuthoritySample(
        DynamicAuthority.RECONSTRUCTED,
        value,
        uncertainty_p95=p95,
        support=support,
    )


class DynamicAuthorityV05Tests(unittest.TestCase):
    def test_truthrange_is_open_ended_not_artificially_clamped(self):
        self.assertAlmostEqual(truthrange_ev(2.0 ** -80, 1.0), -80.0)
        self.assertAlmostEqual(truthrange_ev(2.0 ** 80, 1.0), 80.0)
        self.assertIsNone(truthrange_ev(0.0, 1.0))
        self.assertIsNone(truthrange_ev(-0.25, 1.0))

    def test_scientific_field_separates_dynamic_authority_classes(self):
        samples = [
            measured(2.0 ** -10, 0),
            measured(2.0 ** 10, 1),
            DynamicAuthoritySample(
                DynamicAuthority.CALIBRATED_ESTIMATE,
                2.0 ** 12,
                uncertainty_p95=0.02,
                support=0.9,
                source_sample_index=2,
            ),
            reconstructed(2.0 ** 20),
            DynamicAuthoritySample(
                DynamicAuthority.CENSORED,
                None,
                support=0.0,
                censoring_kind=CensoringKind.HIGHLIGHT_SATURATION,
                censor_bound=2.0 ** 20,
            ),
            DynamicAuthoritySample(DynamicAuthority.UNKNOWN, None),
            reconstructed(-0.01),
            measured(1.0, 7),
        ]
        acc = StreamingDynamicAuthorityAccumulator(binding())
        acc.append_chunk(0, samples)
        out = acc.finalize()

        self.assertEqual(out.sample_count, 8)
        self.assertEqual(out.counts["MEASURED"], 3)
        self.assertEqual(out.counts["CALIBRATED_ESTIMATE"], 1)
        self.assertEqual(out.counts["RECONSTRUCTED"], 2)
        self.assertEqual(out.counts["CENSORED"], 1)
        self.assertEqual(out.counts["UNKNOWN"], 1)
        self.assertAlmostEqual(out.evidence_supported.minimum_ev, -10.0)
        self.assertAlmostEqual(out.evidence_supported.maximum_ev, 12.0)
        self.assertAlmostEqual(out.evidence_supported.span_ev, 22.0)
        self.assertAlmostEqual(out.reconstruction_supported.maximum_ev, 20.0)
        self.assertAlmostEqual(out.scientific_supported.maximum_ev, 20.0)
        self.assertEqual(out.signed_nonpositive_estimate_count, 1)
        self.assertIsNone(out.fixed_dynamic_range_limit_ev)
        self.assertTrue(out.scientific_master_write_allowed)

    def test_counterfactual_and_appearance_cannot_enter_scientific_master_field(self):
        acc = StreamingDynamicAuthorityAccumulator(binding(width=1, height=1))
        with self.assertRaises(ContractError):
            acc.append_chunk(
                0,
                [DynamicAuthoritySample(
                    DynamicAuthority.COUNTERFACTUAL,
                    4.0,
                    parent_record_id="scene-master-1",
                )],
            )

        cf = StreamingDynamicAuthorityAccumulator(
            binding(width=1, height=1, purpose=DynamicFieldPurpose.COUNTERFACTUAL_VIEW)
        )
        cf.append_chunk(
            0,
            [DynamicAuthoritySample(
                DynamicAuthority.COUNTERFACTUAL,
                4.0,
                parent_record_id="scene-master-1",
            )],
        )
        out = cf.finalize()
        self.assertFalse(out.scientific_master_write_allowed)
        self.assertEqual(out.counts["COUNTERFACTUAL"], 1)

    def test_stream_hash_and_summary_do_not_depend_on_chunk_size(self):
        samples = [
            measured(0.25, 0),
            reconstructed(0.5),
            measured(1.0, 2),
            reconstructed(2.0),
            measured(4.0, 4),
            reconstructed(8.0),
            measured(16.0, 6),
            reconstructed(32.0),
        ]
        a = StreamingDynamicAuthorityAccumulator(binding())
        a.append_chunk(0, samples)
        sa = a.finalize()

        b = StreamingDynamicAuthorityAccumulator(binding())
        b.append_chunk(0, samples[:2])
        b.append_chunk(2, samples[2:5])
        b.append_chunk(5, samples[5:])
        sb = b.finalize()

        self.assertEqual(sa.content_sha256, sb.content_sha256)
        self.assertEqual(sa.scientific_policy_sha256, sb.scientific_policy_sha256)
        self.assertEqual(sa.representation, sb.representation)
        self.assertEqual(sa.scientific_supported, sb.scientific_supported)

    def test_streaming_requires_global_raster_order(self):
        acc = StreamingDynamicAuthorityAccumulator(binding(width=2, height=1))
        with self.assertRaises(ContractError):
            acc.append_chunk(1, [measured(1.0, 0)])

    def test_cheap_and_heavy_hardware_change_execution_not_science(self):
        b = DynamicAuthorityFieldBinding(
            frame_id="camera5-200mp-scene",
            width=16320,
            height=12288,
            reference_l0=1.0,
            source_evidence_sha256=H,
            scientific_master_sha256=H2,
        )
        cheap = plan_dynamic_execution(
            b,
            HardwareBudget(
                working_memory_bytes=96 * 1024 * 1024,
                logical_cores=4,
                bytes_per_working_pixel=64,
                halo_pixels=16,
            ),
        )
        heavy = plan_dynamic_execution(
            b,
            HardwareBudget(
                working_memory_bytes=2 * 1024 * 1024 * 1024,
                logical_cores=16,
                bytes_per_working_pixel=64,
                halo_pixels=16,
            ),
        )
        self.assertEqual(cheap.scientific_policy_sha256, heavy.scientific_policy_sha256)
        self.assertLessEqual(cheap.estimated_peak_working_bytes, cheap.available_working_bytes)
        self.assertLessEqual(heavy.estimated_peak_working_bytes, heavy.available_working_bytes)
        self.assertGreaterEqual(heavy.tile_core_pixels * heavy.tile_core_pixels * heavy.concurrency,
                                cheap.tile_core_pixels * cheap.tile_core_pixels * cheap.concurrency)
        self.assertNotEqual(cheap.execution_plan_sha256, heavy.execution_plan_sha256)

    def test_censored_value_cannot_be_relabelled_as_supported_reconstruction_by_contract(self):
        with self.assertRaises(ContractError):
            DynamicAuthoritySample(
                DynamicAuthority.CENSORED,
                100.0,
                support=0.4,
                censoring_kind=CensoringKind.HIGHLIGHT_SATURATION,
                censor_bound=100.0,
            ).validate()


if __name__ == "__main__":
    unittest.main()
