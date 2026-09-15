import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from open_world_foundations_v01 import (  # noqa: E402
    Applicability,
    CalibrationDomain,
    CalibrationKind,
    CalibrationRecord,
    CaptureConditions,
    ContractError,
    HoldoutStatus,
    IlluminationAuthority,
    IlluminationRecord,
    RestorationClass,
    RestorationRecord,
    StructureEvidenceRecord,
    StructureSupportClass,
    validate_open_world_bundle,
)

H = "a" * 64
H2 = "b" * 64


class OpenWorldFoundationTests(unittest.TestCase):
    def test_measured_illumination_requires_evidence(self):
        with self.assertRaises(ContractError):
            IlluminationRecord("sun", IlluminationAuthority.MEASURED, "open landscape").validate()

    def test_counterfactual_does_not_need_to_be_a_sealed_room(self):
        r = IlluminationRecord(
            record_id="virtual-sun",
            authority=IlluminationAuthority.COUNTERFACTUAL,
            spatial_scope="unbounded outdoor scene graph",
            counterfactual_parent_id="scene-master-1",
        )
        r.validate()

    def test_calibration_registry_fails_closed_outside_domain(self):
        d = CalibrationDomain(
            device="HONOR BKQ-N49",
            physical_camera_id="5",
            sensor_pixel_mode="MAXIMUM_RESOLUTION",
            width=16320,
            height=12288,
            cfa="BGGR",
            iso_min=50,
            iso_max=800,
            temperature_c_min=15,
            temperature_c_max=35,
        )
        rec = CalibrationRecord(
            calibration_id="tele-color-001",
            kind=CalibrationKind.COLOR,
            domain=d,
            source_evidence_sha256=(H,),
            method="spectrally measured target",
            uncertainty_description="matrix + residual covariance",
            holdout_status=HoldoutStatus.PASS,
            holdout_report_sha256=H2,
        )
        inside = CaptureConditions(
            "HONOR BKQ-N49", "5", "MAXIMUM_RESOLUTION", 16320, 12288, "BGGR", iso=100, temperature_c=25
        )
        outside = CaptureConditions(
            "HONOR BKQ-N49", "5", "MAXIMUM_RESOLUTION", 16320, 12288, "BGGR", iso=3200, temperature_c=25
        )
        self.assertEqual(rec.applicability(inside), Applicability.APPLICABLE)
        self.assertEqual(rec.applicability(outside), Applicability.OUT_OF_DOMAIN)

    def test_structure_class_does_not_promote_reconstruction_to_measured(self):
        r = StructureEvidenceRecord(
            region_id="tile-3-edge-8",
            measured_support=0.0,
            reconstructed_support=0.8,
            mtf_support=0.9,
            censoring_risk=0.0,
            uncertainty=0.02,
            evidence_sha256=(H,),
        )
        self.assertEqual(r.support_class(), StructureSupportClass.RECONSTRUCTED_SUPPORTED)

    def test_restoration_cannot_overwrite_source(self):
        with self.assertRaises(ContractError):
            RestorationRecord(
                layer_id="retouch-1",
                restoration_class=RestorationClass.EVIDENCE_SUPPORTED_RECONSTRUCTION,
                source_evidence_sha256=(H,),
                mask_sha256=H2,
                method="local reconstruction",
                replaces_source=True,
                confidence=0.9,
            ).validate()

    def test_hypothetical_restoration_is_explicit(self):
        r = RestorationRecord(
            layer_id="historic-colour-hypothesis",
            restoration_class=RestorationClass.HYPOTHETICAL_VISUAL_RESTORATION,
            source_evidence_sha256=(H,),
            mask_sha256=H2,
            method="documented counterfactual recolouring",
            hypothesis_description="possible pre-fade appearance; not measured present colour",
        )
        validate_open_world_bundle(restoration=(r,))


if __name__ == "__main__":
    unittest.main()
