import math
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from open_world_real_tele_sidecar_v07 import (  # noqa: E402
    AUTH_CALIBRATED,
    SidecarError,
    bilinear_gain_scalar,
    canonical_authority_region,
    encode_flags,
    gainmap_axis_coordinate,
    source_noise_sigma_stage2,
)


class RealTeleSidecarV07Tests(unittest.TestCase):
    def test_authority_regions_are_source_anchored_not_compute_anchored(self):
        a = canonical_authority_region(127, 127, 4080, 3072)
        b = canonical_authority_region(128, 127, 4080, 3072)
        self.assertEqual(a[0], 0)
        self.assertEqual(a[1], (0, 0, 128, 128))
        self.assertEqual(b[0], 1)
        self.assertEqual(b[1], (128, 0, 256, 128))

    def test_selected_lattice_last_sample_reaches_gainmap_edge(self):
        self.assertEqual(
            gainmap_axis_coordinate(1535, 1536, 0.0, 1.0 / 12.0, 13),
            (12, 12, 0.0),
        )

    def test_gainmap_bilinear_reference(self):
        grid = [[1.0, 2.0], [3.0, 5.0]]
        got = bilinear_gain_scalar(grid, 1, 3, 1, 3, 0.0, 0.0, 1.0, 1.0)
        self.assertAlmostEqual(got, 2.75)

    def test_noise_profile_output_is_sigma_not_p95(self):
        got = source_noise_sigma_stage2(0.02, 1.5, 0.0008, 0.0000012)
        expected = 1.5 * math.sqrt(0.0008 * 0.02 + 0.0000012)
        self.assertAlmostEqual(got, expected)

    def test_negative_stage2_uses_nonnegative_physical_signal_for_variance(self):
        got = source_noise_sigma_stage2(-0.01, 2.0, 0.0008, 0.0000012)
        self.assertAlmostEqual(got, 2.0 * math.sqrt(0.0000012))

    def test_flags_store_phase_and_direct_authority_only(self):
        self.assertEqual(encode_flags(3, AUTH_CALIBRATED), 3)
        with self.assertRaises(SidecarError):
            encode_flags(4, AUTH_CALIBRATED)


if __name__ == "__main__":
    unittest.main()
