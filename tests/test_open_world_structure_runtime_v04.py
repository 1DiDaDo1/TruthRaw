import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from open_world_foundations_v01 import StructureSupportClass  # noqa: E402
from open_world_integration_v02 import DetailExecutionMode  # noqa: E402
from open_world_structure_runtime_v04 import (  # noqa: E402
    OpticsMtfSupport,
    RuntimeStructureStatus,
    RuntimeStructureTile,
    derive_runtime_structure_evidence,
    load_canonical_structure_bindings,
)

H = "a" * 64
H2 = "b" * 64
H3 = "c" * 64
H4 = "d" * 64


def color_at_bggr(x, y):
    return (2, 1, 1, 0)[(y & 1) * 2 + (x & 1)]


def fixture_tile(*, source_width=4080, source_height=3072, break_reinjection=False, censor_all=False):
    w, h = 6, 6
    stage2 = []
    sigma = []
    rgb = []
    p95 = []
    censored = []
    for y in range(h):
        for x in range(w):
            # Strong, smooth measured structure; same-colour samples two pixels
            # apart differ well above the synthetic source-noise sigma.
            s = 0.10 + 0.025 * x + 0.035 * y
            stage2.append(s)
            sigma.append(0.001)
            c = color_at_bggr(x, y)
            q = [s - 0.004, s + 0.002, s + 0.006]
            q[c] = s
            rgb.extend(q)
            p95.extend([0.010, 0.010, 0.010])
            censored.append(bool(censor_all))
    if break_reinjection:
        # BGGR at (0,0) measures B -> component index 2.
        rgb[2] += 1e-6

    return RuntimeStructureTile(
        region_id="tele-runtime-tile-0",
        device_make="HONOR",
        device_model="BKQ-N49",
        physical_camera_id="5",
        focal_length_mm=22.48,
        source_width=source_width,
        source_height=source_height,
        cfa="BGGR",
        tile_x=0,
        tile_y=0,
        tile_width=w,
        tile_height=h,
        source_evidence_sha256=H,
        scientific_master_sha256=H2,
        reconstruction_backend_name="ResearchEdgeAwareMeasuredPreservingReconstruction",
        production_backend_combined_sha256="8d3a2ad2a97729c2a6a72028099dca7eda6190fcc4df3a3a4020f1c92e15c7ca",
        uncertainty_binding_sha256="61c99b0e29316730ca1323fd3e91fd069ebbc50cba73127cd81b9803b10178a0",
        stage2_cfa=tuple(stage2),
        source_sigma_cfa=tuple(sigma),
        reconstructed_rgb=tuple(rgb),
        uncertainty_p95_rgb=tuple(p95),
        censored_cfa=tuple(censored),
    )


class OpenWorldStructureRuntimeV04Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.bindings = load_canonical_structure_bindings(ROOT)

    def test_real_canonical_bindings_are_loaded_without_topology_upgrade(self):
        b = self.bindings
        self.assertEqual(b.reconstruction_backend_name, "ResearchEdgeAwareMeasuredPreservingReconstruction")
        self.assertEqual((b.uncertainty_source_width, b.uncertainty_source_height), (4080, 3072))
        self.assertEqual(b.uncertainty_cfa, "BGGR")
        self.assertFalse(b.topology_certified)
        self.assertGreater(b.prospective_p95_coverage, 0.95)
        self.assertLess(b.topology_proxy_by_channel[1], b.topology_proxy_by_channel[0])
        self.assertEqual(len(b.binding_sha256), 64)

    def test_runtime_derives_real_support_but_without_optics_detail_stays_neutral(self):
        r = derive_runtime_structure_evidence(fixture_tile(), self.bindings)
        self.assertEqual(r.status, RuntimeStructureStatus.BLOCKED_OPTICS_CALIBRATION_MISSING)
        self.assertTrue(r.diagnostics.exact_measured_reinjection)
        self.assertTrue(r.diagnostics.uncertainty_domain_certified)
        self.assertFalse(r.diagnostics.topology_certified)
        self.assertFalse(r.diagnostics.optics_calibrated)
        self.assertGreater(r.diagnostics.measured_cfa_structure_support, 0.0)
        self.assertGreater(r.diagnostics.reconstructed_topology_proxy_support, 0.0)
        # The pre-optics diagnostics can contain measured/reconstruction support,
        # but v0.3 lower-envelope admission collapses to zero until optics exists.
        self.assertEqual(r.record.measured_support, 0.0)
        self.assertEqual(r.record.reconstructed_support, 0.0)
        self.assertEqual(r.detail_decision.detail_v47j_mode, DetailExecutionMode.NEUTRAL_ONLY)
        self.assertFalse(r.detail_decision.scientific_master_write_allowed)

    def test_exact_measured_reinjection_is_a_hard_gate(self):
        r = derive_runtime_structure_evidence(
            fixture_tile(break_reinjection=True),
            self.bindings,
        )
        self.assertEqual(r.status, RuntimeStructureStatus.BLOCKED_MEASURED_REINJECTION_MISMATCH)
        self.assertIsNone(r.record)
        self.assertFalse(r.diagnostics.exact_measured_reinjection)

    def test_200mp_route_cannot_reuse_4080x3072_uncertainty_certification(self):
        r = derive_runtime_structure_evidence(
            fixture_tile(source_width=16320, source_height=12288),
            self.bindings,
        )
        self.assertEqual(r.status, RuntimeStructureStatus.BLOCKED_UNCERTAINTY_OUT_OF_DOMAIN)
        self.assertIsNone(r.record)
        self.assertFalse(r.diagnostics.uncertainty_domain_certified)

    def test_independent_optics_support_can_admit_appearance_but_never_science_writeback(self):
        tile = fixture_tile()
        optics = OpticsMtfSupport(
            calibration_id="test-fixture-mtf-heldout",
            source_width=4080,
            source_height=3072,
            cfa="BGGR",
            focal_length_mm=22.48,
            support=0.80,
            calibration_evidence_sha256=H3,
            holdout_report_sha256=H4,
        )
        r = derive_runtime_structure_evidence(tile, self.bindings, optics=optics)
        self.assertEqual(r.status, RuntimeStructureStatus.ADMITTED_APPEARANCE_ONLY)
        self.assertTrue(r.diagnostics.optics_calibrated)
        self.assertGreater(r.record.measured_support, 0.0)
        self.assertGreater(r.record.reconstructed_support, 0.0)
        self.assertEqual(r.record.support_class(), StructureSupportClass.MEASURED_SUPPORTED)
        self.assertEqual(r.detail_decision.detail_v47j_mode, DetailExecutionMode.ADAPTIVE_APPEARANCE)
        self.assertFalse(r.detail_decision.scientific_master_write_allowed)
        self.assertFalse(r.detail_decision.measured_detail_claim_allowed_from_output)

    def test_full_censoring_removes_structure_support(self):
        tile = fixture_tile(censor_all=True)
        optics = OpticsMtfSupport(
            calibration_id="test-fixture-mtf-heldout",
            source_width=4080,
            source_height=3072,
            cfa="BGGR",
            focal_length_mm=22.48,
            support=1.0,
            calibration_evidence_sha256=H3,
            holdout_report_sha256=H4,
        )
        r = derive_runtime_structure_evidence(tile, self.bindings, optics=optics)
        self.assertEqual(r.status, RuntimeStructureStatus.NEUTRAL_WEAK_STRUCTURE)
        self.assertEqual(r.diagnostics.censoring_risk, 1.0)
        self.assertEqual(r.record.measured_support, 0.0)
        self.assertEqual(r.record.reconstructed_support, 0.0)
        self.assertEqual(r.detail_decision.detail_v47j_mode, DetailExecutionMode.NEUTRAL_ONLY)


if __name__ == "__main__":
    unittest.main()
