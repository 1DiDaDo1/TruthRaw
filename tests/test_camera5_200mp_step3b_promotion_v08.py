import copy
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from camera5_200mp_step3b_promotion_v08 import (  # noqa: E402
    BLOCKED,
    PASS,
    TARGET_SAMPLES,
    evaluate_step3b,
)

H1 = "1" * 64
H2 = "2" * 64
H3 = "3" * 64


def good_manifest():
    return {
        "evidence_class": "DEVICE_RUNTIME_CAPTURE",
        "device": {"make": "HONOR", "model": "BKQ-N49"},
        "physical_camera_id": "5",
        "opened_camera_id": "5",
        "source_identity_sha256": H1,
        "requested": {"sensor_pixel_mode": "MAXIMUM_RESOLUTION"},
        "raw_output": {
            "format": "RAW_SENSOR",
            "width": 16320,
            "height": 12288,
            "pixel_stride": 2,
            "row_stride": 32640,
            "payload_bytes": 401080320,
            "payload_sha256": H1,
            "image_timestamp_ns": 123456789,
            "timestamp_matches_capture_result": True,
        },
        "camera_characteristics": {
            "cfa_arrangement": 3,
            "black_level_pattern": [64, 64, 64, 64],
            "white_level": 1023,
        },
        "capture_result": {
            "result_camera_id": "5",
            "sensor_pixel_mode": "MAXIMUM_RESOLUTION",
            "sensor_timestamp_ns": 123456789,
            "sensor_exposure_time_ns": 10_000_000,
            "sensor_sensitivity_iso": 200,
            "lens_focus_distance_diopters": 0.4,
            "lens_optical_stabilization_mode": "ON",
            "sensor_raw_binning_factor_used": False,
            "noise_profile": [0.01, 0.0001, 0.01, 0.0001, 0.01, 0.0001, 0.01, 0.0001],
            "lens_shading_map": {"width": 4, "height": 3, "channels": 4},
        },
        "metadata_availability": {
            "noise_profile": "PRESENT",
            "lens_shading_map": "PRESENT",
        },
    }


def good_runtime():
    return {
        "schema": "TruthRawCamera5_200MPRuntimeGate/0.7",
        "classification": "FULL_SENSOR_200MP_APP_VISIBLE_RAW_CAPTURE_PROVEN",
        "pass": True,
    }


def good_bundle():
    return {
        "schema": "TruthRawCamera5_200MPEvidenceBundle/0.3",
        "classification": "CAMERA5_200MP_CAPTURE_CHAIN_CFA_TOPOLOGY_BOUND_APP_VISIBLE",
        "pass": True,
        "bindings": {
            "source_identity_sha256": H1,
            "canonical_cfa_sha256": H2,
            "dng_file_sha256": H3,
            "canonical_samples": TARGET_SAMPLES,
            "camera2_cfa_arrangement_code": 3,
            "camera2_cfa_pattern": "BGGR",
            "dng_cfa_pattern": "BGGR",
            "sensor_raw_binning_factor_used": False,
        },
    }


class Camera5200MPStep3BPromotionV08Tests(unittest.TestCase):
    def test_complete_real_capture_contract_promotes_only_bounded_claim(self):
        r = evaluate_step3b(good_manifest(), good_runtime(), good_bundle())
        self.assertTrue(r["pass"])
        self.assertEqual(r["classification"], PASS)
        self.assertEqual(r["allowed_claim"], PASS)
        self.assertEqual(r["forbidden_claim"], "UNTOUCHED_NATIVE_200MP_ADC")
        self.assertEqual(r["physicalFrameCount"], 1)
        self.assertEqual(r["independentEvidenceCount"], 1)
        self.assertEqual(r["bindings"]["samples"], 200_540_160)
        self.assertEqual(r["dng_role"], "AUXILIARY_DERIVED_CONTAINER_WITH_SEPARATE_CFA_IDENTITY_CHECK")

    def test_missing_exposure_blocks_promotion(self):
        m = good_manifest()
        m["capture_result"]["sensor_exposure_time_ns"] = None
        r = evaluate_step3b(m, good_runtime(), good_bundle())
        self.assertFalse(r["pass"])
        self.assertEqual(r["classification"], BLOCKED)
        self.assertIn("exposure_time_present", r["missing_or_failed_requirements"])

    def test_cfa_topology_bundle_is_mandatory(self):
        b = good_bundle()
        b["pass"] = False
        r = evaluate_step3b(good_manifest(), good_runtime(), b)
        self.assertFalse(r["pass"])
        self.assertFalse(r["checks"]["topology_bundle_pass"])

    def test_source_identity_mismatch_blocks(self):
        b = good_bundle()
        b["bindings"]["source_identity_sha256"] = "9" * 64
        r = evaluate_step3b(good_manifest(), good_runtime(), b)
        self.assertFalse(r["pass"])
        self.assertFalse(r["checks"]["topology_bundle_source_bound"])

    def test_optional_noise_profile_may_be_explicitly_unavailable(self):
        m = good_manifest()
        m["metadata_availability"]["noise_profile"] = "UNAVAILABLE_REPORTED"
        m["capture_result"].pop("noise_profile")
        r = evaluate_step3b(m, good_runtime(), good_bundle())
        self.assertTrue(r["pass"])
        self.assertTrue(r["checks"]["noise_profile_availability_recorded"])
        self.assertTrue(r["checks"]["noise_profile_present_when_declared"])

    def test_optional_metadata_must_not_disappear_silently(self):
        m = good_manifest()
        m["metadata_availability"].pop("noise_profile")
        m["capture_result"].pop("noise_profile")
        r = evaluate_step3b(m, good_runtime(), good_bundle())
        self.assertFalse(r["pass"])
        self.assertIn("noise_profile_availability_recorded", r["missing_or_failed_requirements"])

    def test_explicit_raw_binning_true_blocks_current_topology_claim(self):
        m = good_manifest()
        m["capture_result"]["sensor_raw_binning_factor_used"] = True
        r = evaluate_step3b(m, good_runtime(), good_bundle())
        self.assertFalse(r["pass"])
        self.assertFalse(r["checks"]["no_explicit_raw_binning_true"])

    def test_missing_black_level_blocks(self):
        m = good_manifest()
        m["camera_characteristics"]["black_level_pattern"] = None
        r = evaluate_step3b(m, good_runtime(), good_bundle())
        self.assertFalse(r["pass"])
        self.assertIn("black_level_pattern_present", r["missing_or_failed_requirements"])

    def test_runtime_must_be_real_pass(self):
        rt = good_runtime()
        rt["classification"] = "SIMULATION_ONLY"
        rt["pass"] = False
        r = evaluate_step3b(good_manifest(), rt, good_bundle())
        self.assertFalse(r["pass"])
        self.assertFalse(r["checks"]["runtime_gate_pass"])


if __name__ == "__main__":
    unittest.main()
