import copy
import pathlib
import sys
import tempfile
import unittest
from unittest import mock

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import camera5_200mp_runtime_gate_v09 as gate  # noqa: E402


RAW_SHA = "af3ad73e5919b816881a661f00ffd84a7b537f23198a5877c242717c5d7526de"


def qualifying_evidence():
    return {
        "schema": "truthraw.fotograaf-camera5-200mp-staged-evidence.v0.14",
        "authority": "CAMERA2_ACQUISITION_OBSERVATION_ONLY",
        "calibrationAuthorityGranted": False,
        "scientificMasterModified": False,
        "physicalFrameCount": 1,
        "independentEvidenceCount": 1,
        "capability": {
            "discoverySource": "MAXIMUM_MAP_HIGH_RESOLUTION",
            "targetWidth": 16320,
            "targetHeight": 12288,
        },
        "preview": {
            "logicalCameraId": "0",
            "requestedZoomRatio": 3.7,
            "lastActivePhysicalId": "5",
            "previewCreatesEvidence": False,
        },
        "requestTopology": {
            "openedCameraId": "0",
            "requestedPhysicalCameraId": "5",
            "physicalScopedRequestUsed": True,
            "globalSensorPixelModeWritten": False,
            "physicalSensorPixelModeWritten": True,
            "physicalSensorPixelModeOverrideAdvertised": False,
            "outputPhysicalBinding": True,
            "outputMaximumResolutionModeDeclared": True,
        },
        "captureRoute": {
            "reportedPhysicalIds": ["5"],
            "physicalResultCameraId": "5",
            "requestedMaximumResolution": True,
            "outputMaximumResolutionModeDeclared": True,
            "captureResultSensorPixelMode": 0,
            "resultPixelModeIsIndependentObservation": True,
            "width": 16320,
            "height": 12288,
            "sampleCount": 200540160,
        },
        "captureResult": {
            "sensorTimestampNs": 691266629782500,
            "imageTimestampNs": 691266629782500,
            "timestampIdentityPass": True,
            "sensorPixelMode": 0,
            "rawBinningFactorUsed": True,
            "noiseReductionMode": 2,
            "edgeMode": 2,
        },
        "rawPayload": {
            "file": "TRUTHRAW_1789600559711_CAM5_200MP_16320x12288_v014.rawsensor",
            "sha256": RAW_SHA,
            "bytes": 401080320,
            "accessibleBufferBytes": 401080320,
            "rowStride": 32640,
            "pixelStride": 2,
            "canonicalContiguousRawSensor": True,
            "expectedContiguousBytes": 401080320,
        },
        "returnedSensorPixelModeIsMaximumResolution": False,
        "returnedSensorPixelModeMismatchPreserved": True,
        "sealBeforePixelModeInterpretation": True,
        "boundary": "APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF",
    }


class Camera5200MPRuntimeGateV09Tests(unittest.TestCase):
    def make_sparse_raw(self, directory: str) -> pathlib.Path:
        path = pathlib.Path(directory) / "cam5.rawsensor"
        with path.open("wb") as f:
            f.truncate(gate.EXPECTED_CONTIGUOUS)
        return path

    def evaluate_with_declared_hash(self, evidence, raw):
        with mock.patch.object(gate, "sha256_file", return_value=RAW_SHA):
            return gate.evaluate(evidence, raw)

    def test_qualifying_v014_logical0_physical5_capture_passes(self):
        with tempfile.TemporaryDirectory() as td:
            raw = self.make_sparse_raw(td)
            report = self.evaluate_with_declared_hash(qualifying_evidence(), raw)
        self.assertTrue(report["pass"])
        self.assertEqual(report["classification"], gate.PASS_CLASS)
        self.assertFalse(report["untouched_photodiode_adc_raw_proven"])
        self.assertEqual(report["observed"]["capture_result_sensor_pixel_mode"], 0)
        self.assertTrue(report["observed"]["returned_sensor_pixel_mode_mismatch_preserved"])

    def test_direct_open_camera5_does_not_satisfy_proven_v014_topology(self):
        evidence = qualifying_evidence()
        evidence["requestTopology"]["openedCameraId"] = "5"
        with tempfile.TemporaryDirectory() as td:
            raw = self.make_sparse_raw(td)
            report = self.evaluate_with_declared_hash(evidence, raw)
        self.assertFalse(report["pass"])
        self.assertFalse(report["checks"]["opened_logical0"])

    def test_returned_default_pixel_mode_is_not_rewritten_or_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            raw = self.make_sparse_raw(td)
            report = self.evaluate_with_declared_hash(qualifying_evidence(), raw)
        self.assertTrue(report["pass"])
        self.assertFalse(report["observed"]["returned_sensor_pixel_mode_is_maximum_resolution"])
        self.assertTrue(report["checks"]["returned_pixel_mode_observation_preserved"])

    def test_timestamp_mismatch_fails_closed(self):
        evidence = qualifying_evidence()
        evidence["captureResult"]["sensorTimestampNs"] += 1
        with tempfile.TemporaryDirectory() as td:
            raw = self.make_sparse_raw(td)
            report = self.evaluate_with_declared_hash(evidence, raw)
        self.assertFalse(report["pass"])
        self.assertFalse(report["checks"]["timestamp_exact_equality"])

    def test_payload_hash_mismatch_fails_closed(self):
        with tempfile.TemporaryDirectory() as td:
            raw = self.make_sparse_raw(td)
            with mock.patch.object(gate, "sha256_file", return_value="0" * 64):
                report = gate.evaluate(qualifying_evidence(), raw)
        self.assertFalse(report["pass"])
        self.assertFalse(report["checks"]["payload_file_sha256_matches_evidence"])

    def test_calibration_or_master_promotion_is_rejected(self):
        evidence = qualifying_evidence()
        evidence["calibrationAuthorityGranted"] = True
        evidence["scientificMasterModified"] = True
        with tempfile.TemporaryDirectory() as td:
            raw = self.make_sparse_raw(td)
            report = self.evaluate_with_declared_hash(evidence, raw)
        self.assertFalse(report["pass"])
        self.assertFalse(report["checks"]["calibration_authority_not_granted"])
        self.assertFalse(report["checks"]["scientific_master_unmodified"])

    def test_missing_raw_file_never_passes(self):
        report = gate.evaluate(qualifying_evidence(), None)
        self.assertFalse(report["pass"])
        self.assertFalse(report["checks"]["payload_file_provided"])
        self.assertFalse(report["checks"]["payload_file_exists"])


if __name__ == "__main__":
    unittest.main()
