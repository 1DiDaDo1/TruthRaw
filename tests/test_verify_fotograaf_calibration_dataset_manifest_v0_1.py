#!/usr/bin/env python3
import copy
import hashlib
import tempfile
import unittest
from pathlib import Path

from tools.verify_fotograaf_calibration_dataset_manifest_v0_1 import (
    ManifestError,
    canonical_manifest_sha256,
    canonical_scope_sha256,
    load_json,
    validate_contract,
    validate_manifest,
)

ROOT = Path(__file__).resolve().parents[1]
CONTRACT_PATH = ROOT / "docs/research/fotograaf-scene-metrology-v0.1/TRUTHRAW_FOTOGRAAF_CALIBRATION_DATASET_MANIFEST_CONTRACT_V0_1.json"


def sha_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def scope():
    return {
        "deviceMake": "HONOR",
        "deviceModel": "BKQ-N49",
        "cameraSystemId": "5",
        "physicalCameraId": "fixture-physical",
        "lensRole": "TELE",
        "captureApiDomain": "FIXTURE_DIRECT_DNG",
        "captureMode": "4080x3072_RAW",
        "rawWidth": 4080,
        "rawHeight": 3072,
        "cfaPattern": "BGGR",
        "sampleRepresentation": "RAW10_IN_UINT16_DNG",
        "captureSampleDomainId": "fixture-domain-a",
        "firmwareBuildId": "fixture-build",
        "focusStateClass": "infinity",
        "stabilizationState": "ois-fixed",
        "protocolVersion": "0.1",
    }


def capture(capture_id: str, module: str, role: str, split: str, file_name: str, data: bytes):
    capture_scope = scope()
    return {
        "captureId": capture_id,
        "module": module,
        "role": role,
        "split": split,
        "fileName": file_name,
        "sha256": sha_bytes(data),
        "byteLength": len(data),
        "scopeKeySha256": canonical_scope_sha256(capture_scope),
        "metadataSnapshotSha256": "a" * 64,
        "exposureTimeNs": 10_000_000,
        "isoMetadata": 400,
        "blackLevelIdentity": "phase:64,64,64,64",
        "whiteLevelIdentity": "1023",
        "gainMapOpcodeIdentity": "none-fixture",
        "focusState": "infinity",
        "stabilizationState": "ois-fixed",
        "temperatureObservation": {
            "status": "MEASURED",
            "sensorId": "fixture-probe",
            "valueC": 25.0,
            "uncertaintyC": 0.5,
        },
    }


def manifest(d1=b"dark-fit", d2=b"dark-val"):
    manifest_scope = scope()
    return {
        "schema": "truthraw.fotograaf-calibration-dataset-manifest.v0.1",
        "manifestId": "fixture-manifest",
        "scope": manifest_scope,
        "scopeKeySha256": canonical_scope_sha256(manifest_scope),
        "physicalFrameCountForLaterScene": 1,
        "independentEvidenceCountForLaterScene": 1,
        "captures": [
            capture("dark-fit", "C1_DARK_NOISE", "DARK", "FIT", "dark-fit.dng", d1),
            capture("dark-val", "C1_DARK_NOISE", "DARK", "VALIDATION", "dark-val.dng", d2),
        ],
        "externalReferences": [],
    }


class CalibrationDatasetManifestV01Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.contract = load_json(CONTRACT_PATH)

    def test_contract_valid(self):
        validate_contract(self.contract)

    def test_manifest_valid_and_digest_stable(self):
        value = manifest()
        result1 = validate_manifest(value, self.contract)
        result2 = validate_manifest(copy.deepcopy(value), self.contract)
        self.assertTrue(result1["valid"])
        self.assertEqual(result1["datasetManifestSha256"], result2["datasetManifestSha256"])
        self.assertEqual(result1["captureCount"], 2)

    def test_declared_manifest_digest_checked(self):
        value = manifest()
        value["manifestSha256"] = canonical_manifest_sha256(value)
        result = validate_manifest(value, self.contract)
        self.assertEqual(result["datasetManifestSha256"], value["manifestSha256"])

    def test_duplicate_capture_hash_fails(self):
        same = b"same"
        value = manifest(same, same)
        with self.assertRaises(ManifestError):
            validate_manifest(value, self.contract)

    def test_role_module_mismatch_fails(self):
        value = manifest()
        value["captures"][0]["role"] = "FLAT"
        with self.assertRaises(ManifestError):
            validate_manifest(value, self.contract)

    def test_scope_mismatch_fails(self):
        value = manifest()
        value["captures"][0]["scopeKeySha256"] = "f" * 64
        with self.assertRaises(ManifestError):
            validate_manifest(value, self.contract)

    def test_scene_evidence_count_cannot_increase(self):
        value = manifest()
        value["physicalFrameCountForLaterScene"] = 2
        with self.assertRaises(ManifestError):
            validate_manifest(value, self.contract)

    def test_actual_files_are_rehashed(self):
        d1 = b"dark-fit"
        d2 = b"dark-val"
        value = manifest(d1, d2)
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            (root / "dark-fit.dng").write_bytes(d1)
            (root / "dark-val.dng").write_bytes(d2)
            result = validate_manifest(value, self.contract, root)
            self.assertTrue(result["actualFilesRehashed"])
            (root / "dark-val.dng").write_bytes(b"tampered")
            with self.assertRaises(ManifestError):
                validate_manifest(value, self.contract, root)


if __name__ == "__main__":
    unittest.main()
