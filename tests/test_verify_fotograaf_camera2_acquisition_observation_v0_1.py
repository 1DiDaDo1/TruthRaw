from __future__ import annotations

import copy
import hashlib
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "verify_fotograaf_camera2_acquisition_observation_v0_1.py"
CONTRACT_PATH = ROOT / "docs" / "research" / "fotograaf-scene-metrology-v0.1" / "TRUTHRAW_FOTOGRAAF_CAMERA2_ACQUISITION_OBSERVATION_CONTRACT_V0_1.json"

spec = importlib.util.spec_from_file_location("camera2_observation", TOOL)
mod = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(mod)


class Camera2AcquisitionObservationTest(unittest.TestCase):
    def setUp(self) -> None:
        self.contract = json.loads(CONTRACT_PATH.read_text(encoding="utf-8"))
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.source = self.root / "capture.dng"
        self.source.write_bytes(b"II*\x00truthraw-camera2-test-source")
        source_bytes = self.source.read_bytes()
        self.observation = {
            "schema": mod.OBSERVATION_SCHEMA,
            "captureId": "HONOR_TELE_C0_0001",
            "createdAtUtc": "2026-09-14T14:00:00Z",
            "source": {
                "displayName": "capture.dng",
                "sha256": hashlib.sha256(source_bytes).hexdigest(),
                "byteLength": len(source_bytes),
                "hashTiming": "AFTER_DNG_FINALIZATION",
            },
            "device": {
                "manufacturer": "HONOR",
                "model": "BKQ-N49",
                "buildFingerprint": "HONOR/BKQ-N49/test:16/build:user/release-keys",
            },
            "camera": {
                "logicalCameraId": "0",
                "logicalMultiCamera": True,
                "requestedPhysicalCameraId": "5",
                "advertisedPhysicalCameraIds": ["2", "4", "5"],
                "physicalCameraObservation": {
                    "status": "MEASURED_PHYSICAL_OUTPUT_RESULT",
                    "value": "5",
                    "method": "CAMERA2_PHYSICAL_OUTPUT_RESULT",
                },
                "resultCameraId": "5",
            },
            "topology": {
                "format": "RAW_SENSOR",
                "rawWidth": 4080,
                "rawHeight": 3072,
                "cfaPattern": "BGGR",
                "rawCapability": True,
                "directCfaMeasurement": True,
                "processedRgbInput": False,
                "multiFrameEvidenceMerged": False,
            },
            "request": {
                "sensorSensitivityIso": 100,
                "exposureTimeNs": 16367398,
                "focusDistanceDiopters": 0.0,
                "oisRequestedOff": True,
            },
            "result": {
                "sensorSensitivityIso": 100,
                "exposureTimeNs": 16367398,
                "sensorTimestampNs": 123456789000,
                "imageTimestampNs": 123456789000,
                "timestampMatch": True,
                "focusDistanceDiopters": 0.0,
                "oisMode": 0,
                "noiseReductionMode": 0,
            },
            "authority": {
                "recordClass": "CAMERA2_ACQUISITION_OBSERVATION_ONLY",
                "calibrationAuthorityGranted": False,
                "c0EnvelopeReady": False,
                "physicalFrameCountForLaterScene": 1,
                "independentEvidenceCountForLaterScene": 1,
                "missingBeforeC0Envelope": [
                    "cameraSystemIdMapping",
                    "captureSampleDomainId",
                    "gainReadoutStateId",
                    "focusStateClass",
                    "stabilizationState"
                ],
            },
        }

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def test_contract_and_valid_observation(self) -> None:
        mod.validate_contract(self.contract)
        result = mod.validate_observation(self.observation, self.contract, self.source)
        self.assertTrue(result["valid"])
        self.assertTrue(result["sourceBytesVerified"])
        self.assertTrue(result["physicalCameraIdentityMeasured"])
        self.assertTrue(result["honorTeleTopologyCandidate"])
        self.assertFalse(result["c0EnvelopeReady"])
        self.assertFalse(result["calibrationAuthorityGranted"])

    def test_direct_non_logical_camera_result_is_admitted(self) -> None:
        value = copy.deepcopy(self.observation)
        value["camera"] = {
            "logicalCameraId": "5",
            "logicalMultiCamera": False,
            "requestedPhysicalCameraId": None,
            "advertisedPhysicalCameraIds": [],
            "physicalCameraObservation": {
                "status": "MEASURED_DIRECT_CAMERA_RESULT",
                "value": "5",
                "method": "CAMERA2_DIRECT_CAMERA_RESULT",
            },
            "resultCameraId": "5",
        }
        result = mod.validate_observation(value, self.contract, self.source)
        self.assertTrue(result["physicalCameraIdentityMeasured"])
        self.assertEqual(result["physicalCameraObservationStatus"], "MEASURED_DIRECT_CAMERA_RESULT")

    def test_tampered_source_fails(self) -> None:
        self.source.write_bytes(self.source.read_bytes() + b"tamper")
        with self.assertRaises(mod.ObservationError):
            mod.validate_observation(self.observation, self.contract, self.source)

    def test_timestamp_mismatch_fails(self) -> None:
        bad = copy.deepcopy(self.observation)
        bad["result"]["imageTimestampNs"] += 1
        with self.assertRaises(mod.ObservationError):
            mod.validate_observation(bad, self.contract, self.source)

    def test_sample_domain_cannot_be_sneaked_into_observation(self) -> None:
        bad = copy.deepcopy(self.observation)
        bad["camera"]["captureSampleDomainId"] = "ISO_8192_DOMAIN"
        with self.assertRaises(mod.ObservationError):
            mod.validate_observation(bad, self.contract, self.source)

    def test_gain_readout_state_cannot_be_sneaked_into_observation(self) -> None:
        bad = copy.deepcopy(self.observation)
        bad["result"]["gainReadoutStateId"] = "HIGH_GAIN"
        with self.assertRaises(mod.ObservationError):
            mod.validate_observation(bad, self.contract, self.source)

    def test_requested_physical_id_must_match_physical_result(self) -> None:
        bad = copy.deepcopy(self.observation)
        bad["camera"]["physicalCameraObservation"]["value"] = "4"
        bad["camera"]["resultCameraId"] = "4"
        with self.assertRaises(mod.ObservationError):
            mod.validate_observation(bad, self.contract, self.source)

    def test_unavailable_physical_id_is_valid_observation_but_not_c0_ready(self) -> None:
        value = copy.deepcopy(self.observation)
        value["camera"]["requestedPhysicalCameraId"] = None
        value["camera"]["physicalCameraObservation"] = {
            "status": "NOT_AVAILABLE",
            "value": None,
            "method": "CAMERA2_RESULT_KEY_NOT_AVAILABLE",
        }
        value["camera"]["resultCameraId"] = "0"
        result = mod.validate_observation(value, self.contract, self.source)
        self.assertTrue(result["valid"])
        self.assertFalse(result["physicalCameraIdentityMeasured"])
        self.assertFalse(result["c0EnvelopeReady"])


if __name__ == "__main__":
    unittest.main()
