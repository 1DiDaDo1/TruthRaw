from __future__ import annotations

import hashlib
import importlib.util
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "verify_fotograaf_c0_capture_identity_v0_1.py"
CONTRACT = ROOT / "docs" / "research" / "fotograaf-scene-metrology-v0.1" / "TRUTHRAW_FOTOGRAAF_C0_CAPTURE_IDENTITY_CONTRACT_V0_1.json"

spec = importlib.util.spec_from_file_location("c0_gate", TOOL)
mod = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(mod)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


class C0CaptureIdentityTests(unittest.TestCase):
    def setUp(self) -> None:
        self.contract = mod.load_json(CONTRACT)
        mod.validate_contract(self.contract)

    def make_record(self, source: bytes, metadata: bytes) -> dict:
        scope = {
            "deviceMake": "HONOR",
            "deviceModel": "BKQ-N49",
            "cameraSystemId": "5",
            "physicalCameraId": "5-tele-physical-capture-id",
            "lensRole": "TELE",
            "captureApiDomain": "CAMERA2_RAW_SENSOR_CAPTURE_WITH_DNG_WRAPPER",
            "captureMode": "4080x3072_DIRECT_CFA_TARGET",
            "rawWidth": 4080,
            "rawHeight": 3072,
            "cfaPattern": "BGGR",
            "sampleRepresentation": "RAW10_MEASUREMENT_IN_16BIT_DNG_STORAGE",
            "captureSampleDomainId": "BKQ-N49_TELE_DIRECT_CFA_DOMAIN_A",
            "firmwareBuildId": "HONORBKQ-N49/10.0.0.199C636E4R106P1",
            "focusStateClass": "FIXED_CALIBRATION_FOCUS_STATE_A",
            "stabilizationState": "OIS_OFF_EIS_OFF",
            "protocolVersion": "FOTOGRAAF_CALIBRATION_ACQUISITION_V0_1",
        }

        def obs(field: str, method: str) -> dict:
            return {"value": str(scope[field]), "method": method, "evidenceId": f"capture-sidecar:{field}"}

        record = {
            "schema": "truthraw.fotograaf-c0-capture-identity-record.v0.1",
            "recordId": "HONOR_TELE_C0_RUN_SYNTHETIC_TEST",
            "scope": scope,
            "scopeKeySha256": mod.canonical_sha256(scope),
            "sourceEvidence": {
                "fileName": "capture.dng",
                "sha256": sha(source),
                "byteLength": len(source),
            },
            "metadataSnapshot": {
                "fileName": "capture-metadata.json",
                "sha256": sha(metadata),
                "byteLength": len(metadata),
            },
            "identityObservations": {
                "cameraSystemId": obs("cameraSystemId", "CAMERA2_LOGICAL_CAMERA_ID_QUERY"),
                "physicalCameraId": obs("physicalCameraId", "CAMERA2_PHYSICAL_CAMERA_RESULT"),
                "captureApiDomain": obs("captureApiDomain", "CAPTURE_APPLICATION_ROUTE"),
                "captureSampleDomainId": obs("captureSampleDomainId", "RAW_PAYLOAD_AND_METADATA_CLASSIFIER"),
                "firmwareBuildId": obs("firmwareBuildId", "ANDROID_BUILD_FINGERPRINT"),
                "focusStateClass": obs("focusStateClass", "CAPTURE_CONTROL_AND_RESULT"),
                "stabilizationState": obs("stabilizationState", "CAPTURE_CONTROL_AND_RESULT"),
            },
            "topologyEvidence": {
                "rawWidth": 4080,
                "rawHeight": 3072,
                "cfaPattern": "BGGR",
                "sampleRepresentation": "RAW10_MEASUREMENT_IN_16BIT_DNG_STORAGE",
                "directCfaMeasurement": True,
                "processedRgbInput": False,
                "multiFrameEvidenceMerged": False,
                "parserBackendId": "truthraw-c0-test-parser-v0.1",
            },
            "physicalFrameCountForLaterScene": 1,
            "independentEvidenceCountForLaterScene": 1,
            "calibrationAuthorityGrantedByC0": False,
        }
        record["c0RecordSha256"] = mod.canonical_record_sha256(record)
        return record

    def validate(self, record: dict, source: bytes = b"raw-source-bytes", metadata: bytes = b'{"camera":"5"}') -> dict:
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            source_path = root / "capture.dng"
            metadata_path = root / "capture-metadata.json"
            source_path.write_bytes(source)
            metadata_path.write_bytes(metadata)
            return mod.validate_record(record, self.contract, source_path, metadata_path)

    def test_valid_capture_identity_seals_exact_scope(self) -> None:
        source = b"raw-source-bytes"
        metadata = b'{"camera":"5"}'
        result = self.validate(self.make_record(source, metadata), source, metadata)
        self.assertEqual(result["decision"], "C0_IDENTITY_SEALED")
        self.assertTrue(result["honorTeleProfileMatched"])
        self.assertFalse(result["calibrationAuthorityGrantedByC0"])

    def test_placeholder_physical_camera_id_fails(self) -> None:
        source = b"raw-source-bytes"
        metadata = b'{"camera":"5"}'
        record = self.make_record(source, metadata)
        record["scope"]["physicalCameraId"] = "UNRESOLVED_MUST_BE_CAPTURE_BOUND"
        record["scopeKeySha256"] = mod.canonical_sha256(record["scope"])
        record["identityObservations"]["physicalCameraId"]["value"] = record["scope"]["physicalCameraId"]
        record["c0RecordSha256"] = mod.canonical_record_sha256(record)
        with self.assertRaises(mod.C0Error):
            self.validate(record, source, metadata)

    def test_camera_system_id_must_match_observation(self) -> None:
        source = b"raw-source-bytes"
        metadata = b'{"camera":"5"}'
        record = self.make_record(source, metadata)
        record["identityObservations"]["cameraSystemId"]["value"] = "4"
        record["c0RecordSha256"] = mod.canonical_record_sha256(record)
        with self.assertRaises(mod.C0Error):
            self.validate(record, source, metadata)

    def test_physical_camera_id_may_not_be_inferred_from_focal_length(self) -> None:
        source = b"raw-source-bytes"
        metadata = b'{"camera":"5"}'
        record = self.make_record(source, metadata)
        record["identityObservations"]["physicalCameraId"]["method"] = "INFERRED_FROM_FOCAL_LENGTH"
        record["c0RecordSha256"] = mod.canonical_record_sha256(record)
        with self.assertRaises(mod.C0Error):
            self.validate(record, source, metadata)

    def test_capture_sample_domain_may_not_be_inferred_from_iso(self) -> None:
        source = b"raw-source-bytes"
        metadata = b'{"camera":"5"}'
        record = self.make_record(source, metadata)
        record["identityObservations"]["captureSampleDomainId"]["method"] = "INFERRED_FROM_ISO"
        record["c0RecordSha256"] = mod.canonical_record_sha256(record)
        with self.assertRaises(mod.C0Error):
            self.validate(record, source, metadata)

    def test_tampered_source_bytes_fail(self) -> None:
        source = b"raw-source-bytes"
        metadata = b'{"camera":"5"}'
        record = self.make_record(source, metadata)
        with self.assertRaises(mod.C0Error):
            self.validate(record, source + b"-tampered", metadata)

    def test_tampered_metadata_snapshot_fails(self) -> None:
        source = b"raw-source-bytes"
        metadata = b'{"camera":"5"}'
        record = self.make_record(source, metadata)
        with self.assertRaises(mod.C0Error):
            self.validate(record, source, metadata + b" ")

    def test_multiframe_or_processed_input_fails(self) -> None:
        source = b"raw-source-bytes"
        metadata = b'{"camera":"5"}'
        for field, value in (("multiFrameEvidenceMerged", True), ("processedRgbInput", True), ("directCfaMeasurement", False)):
            record = self.make_record(source, metadata)
            record["topologyEvidence"][field] = value
            record["c0RecordSha256"] = mod.canonical_record_sha256(record)
            with self.subTest(field=field):
                with self.assertRaises(mod.C0Error):
                    self.validate(record, source, metadata)

    def test_wrong_honor_tele_profile_fails(self) -> None:
        source = b"raw-source-bytes"
        metadata = b'{"camera":"5"}'
        record = self.make_record(source, metadata)
        record["scope"]["cameraSystemId"] = "4"
        record["scopeKeySha256"] = mod.canonical_sha256(record["scope"])
        record["identityObservations"]["cameraSystemId"]["value"] = "4"
        record["c0RecordSha256"] = mod.canonical_record_sha256(record)
        with self.assertRaises(mod.C0Error):
            self.validate(record, source, metadata)


if __name__ == "__main__":
    unittest.main()
