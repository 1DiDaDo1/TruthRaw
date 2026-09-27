import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from draw_calibration_binding_v01 import (  # noqa: E402
    BindingDomain,
    BoundCalibration,
    CalibrationBindingDecision,
    CaptureBindingContext,
    evaluate_binding,
)
from open_world_foundations_v01 import (  # noqa: E402
    Applicability,
    CalibrationDomain,
    CalibrationKind,
    CalibrationRecord,
    CaptureConditions,
    HoldoutStatus,
)

H1="a"*64
H2="b"*64
PAYLOAD="c"*64
SOURCE="d"*64

def domain():
    return BindingDomain(
        calibration_domain=CalibrationDomain(
            device="HONOR BKQ-N49",
            physical_camera_id="5",
            sensor_pixel_mode="DEFAULT",
            width=4080,
            height=3072,
            cfa="BGGR",
            iso_min=50,
            iso_max=800,
            exposure_s_min=1/10000,
            exposure_s_max=1.0,
            temperature_c_min=10,
            temperature_c_max=45,
            focus_distance_diopters_min=0,
            focus_distance_diopters_max=20,
        ),
        source_route_id="HONOR_VENDOR_DNG_ROUTE",
        sample_domain_id="HONOR_VENDOR_DNG_4080x3072",
        readout_domain_id="CAM5_DEFAULT_4080x3072",
    )

def calibration(holdout=HoldoutStatus.PASS, payload=PAYLOAD):
    return BoundCalibration(
        record=CalibrationRecord(
            calibration_id="synthetic-tele-noise-001",
            kind=CalibrationKind.GAIN_NOISE,
            domain=domain().calibration_domain,
            source_evidence_sha256=(H1,),
            method="synthetic-test-method",
            uncertainty_description="synthetic covariance fixture",
            holdout_status=holdout,
            holdout_report_sha256=H2 if holdout is HoldoutStatus.PASS else None,
        ),
        binding_domain=domain(),
        calibration_payload_sha256=payload,
    )

def capture(**kw):
    values=dict(
        device="HONOR BKQ-N49",
        physical_camera_id="5",
        sensor_pixel_mode="DEFAULT",
        width=4080,
        height=3072,
        cfa="BGGR",
        iso=100,
        exposure_s=0.01,
        temperature_c=25,
        focus_distance_diopters=2,
    )
    values.update(kw)
    return CaptureBindingContext(
        observation_id="DRAW_OBS_SYNTHETIC",
        source_evidence_sha256=SOURCE,
        capture=CaptureConditions(**values),
        source_route_id="HONOR_VENDOR_DNG_ROUTE",
        sample_domain_id="HONOR_VENDOR_DNG_4080x3072",
        readout_domain_id="CAM5_DEFAULT_4080x3072",
    )

class CalibrationBindingTests(unittest.TestCase):
    def test_exact_domain_holdout_pass_is_admitted(self):
        out=evaluate_binding(calibration(),capture())
        self.assertEqual(out.applicability,Applicability.APPLICABLE)
        self.assertTrue(out.admitted)
        self.assertFalse(out.creates_new_evidence)
        self.assertFalse(out.source_evidence_mutated)
        self.assertFalse(out.cross_observation_relation_granted)
        self.assertFalse(out.calibration_transfer_implied)
        self.assertEqual(out.physical_frame_count_increment,0)
        self.assertEqual(out.independent_evidence_count_increment,0)
        self.assertEqual(len(out.binding_sha256),64)

    def test_hashes_are_deterministic_and_payload_sensitive(self):
        a=calibration()
        b=calibration()
        self.assertEqual(a.binding_domain.sha256(),b.binding_domain.sha256())
        self.assertEqual(a.record_sha256(),b.record_sha256())
        changed=calibration(payload="e"*64)
        self.assertNotEqual(a.record_sha256(),changed.record_sha256())

    def test_physical_camera_transfer_fails_closed(self):
        out=evaluate_binding(calibration(),capture(physical_camera_id="2"))
        self.assertEqual(out.applicability,Applicability.OUT_OF_DOMAIN)
        self.assertFalse(out.admitted)

    def test_readout_and_source_domain_transfer_fail_closed(self):
        c=capture()
        bad=CaptureBindingContext(
            observation_id=c.observation_id,
            source_evidence_sha256=c.source_evidence_sha256,
            capture=c.capture,
            source_route_id="CAMERA2_DERIVED_DNG_ROUTE",
            sample_domain_id=c.sample_domain_id,
            readout_domain_id=c.readout_domain_id,
        )
        self.assertEqual(
            evaluate_binding(calibration(),bad).applicability,
            Applicability.OUT_OF_DOMAIN,
        )

        bad2=CaptureBindingContext(
            observation_id=c.observation_id,
            source_evidence_sha256=c.source_evidence_sha256,
            capture=c.capture,
            source_route_id=c.source_route_id,
            sample_domain_id="CAMERA2_RAW_SENSOR_DERIVED_DNG_4080x3072",
            readout_domain_id=c.readout_domain_id,
        )
        self.assertEqual(
            evaluate_binding(calibration(),bad2).applicability,
            Applicability.OUT_OF_DOMAIN,
        )

        bad3=CaptureBindingContext(
            observation_id=c.observation_id,
            source_evidence_sha256=c.source_evidence_sha256,
            capture=c.capture,
            source_route_id=c.source_route_id,
            sample_domain_id=c.sample_domain_id,
            readout_domain_id="CAM5_MAXIMUM_RESOLUTION_16320x12288",
        )
        self.assertEqual(
            evaluate_binding(calibration(),bad3).applicability,
            Applicability.OUT_OF_DOMAIN,
        )

    def test_range_and_missing_condition_fail_closed(self):
        self.assertEqual(
            evaluate_binding(calibration(),capture(iso=3200)).applicability,
            Applicability.OUT_OF_DOMAIN,
        )
        self.assertEqual(
            evaluate_binding(calibration(),capture(iso=None)).applicability,
            Applicability.OUT_OF_DOMAIN,
        )

    def test_holdout_is_required(self):
        out=evaluate_binding(calibration(HoldoutStatus.UNTESTED),capture())
        self.assertEqual(out.applicability,Applicability.UNCERTIFIED)
        self.assertFalse(out.admitted)

    def test_capture_context_hash_changes_with_observation_source(self):
        a=capture()
        b=CaptureBindingContext(
            observation_id="DRAW_OBS_SYNTHETIC_2",
            source_evidence_sha256="f"*64,
            capture=a.capture,
            source_route_id=a.source_route_id,
            sample_domain_id=a.sample_domain_id,
            readout_domain_id=a.readout_domain_id,
        )
        self.assertNotEqual(a.sha256(),b.sha256())

if __name__ == "__main__":
    unittest.main()
