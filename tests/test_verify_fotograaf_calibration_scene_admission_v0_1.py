#!/usr/bin/env python3
import copy
import unittest
from pathlib import Path

from tools.verify_fotograaf_calibration_pack_v0_1 import load_json
from tools.verify_fotograaf_calibration_scene_admission_v0_1 import (
    AdmissionError,
    admit_scene,
    validate_admission_contract,
)

ROOT = Path(__file__).resolve().parents[1]
CALIBRATION_CONTRACT = ROOT / "docs/research/fotograaf-scene-metrology-v0.1/TRUTHRAW_FOTOGRAAF_CALIBRATION_CONTRACT_V0_1.json"
ADMISSION_CONTRACT = ROOT / "docs/research/fotograaf-scene-metrology-v0.1/TRUTHRAW_FOTOGRAAF_CALIBRATION_SCENE_ADMISSION_CONTRACT_V0_1.json"


def sha(ch: str) -> str:
    return ch * 64


def module(metrics):
    return {
        "present": True,
        "validated": True,
        "fitDatasetIds": ["fit-a"],
        "validationDatasetIds": ["val-a"],
        "metrics": metrics,
    }


def valid_pack():
    return {
        "schema": "truthraw.fotograaf-calibration-pack.v0.1",
        "packId": "honor-tele-ordinary-domain-v0.1",
        "status": "VALIDATED_RELATIVE",
        "scope": {
            "deviceMake": "HONOR",
            "deviceModel": "BKQ-N49",
            "cameraSystemId": "5",
            "physicalCameraId": "5",
            "lensRole": "TELE",
            "captureApiDomain": "MOTIONCAM_DIRECT_DNG",
            "captureMode": "4080x3072_RAW10",
            "rawWidth": 4080,
            "rawHeight": 3072,
            "cfaPattern": "BGGR",
            "sampleRepresentation": "RAW10_IN_UINT16_DNG",
            "captureSampleDomainId": "ordinary-domain",
            "firmwareBuildId": "fixture-build",
            "focusStateClass": "fixed-near-infinity",
            "stabilizationState": "ois-normal",
            "protocolVersion": "0.1",
        },
        "datasetManifestSha256": sha("a"),
        "protocolSha256": sha("b"),
        "modelSha256": sha("c"),
        "physicalFrameCountForLaterScene": 1,
        "independentEvidenceCountForLaterScene": 1,
        "fitValidationSeparated": True,
        "thresholdProtocolSealedBeforeFit": True,
        "temperature": {"calibrated": False, "bins": []},
        "modules": {
            "C0_IDENTITY": module({
                "captureFileSha256ForEveryFrame": True,
                "parserBackendIdentity": True,
                "scopeKeyComplete": True,
                "metadataSnapshotPerFrame": True,
                "gainMapOpcodeIdentity": True,
            }),
            "C1_DARK_NOISE": module({
                "exposureTimesPerState": 4,
                "minRepeatsPerGainExposureTemperatureCell": 16,
                "gainReadoutStatesCovered": 2,
                "photonBlockedDark": True,
                "allIntendedGainReadoutStatesCovered": True,
                "temperatureCalibrated": False,
                "temperatureBins": 1,
            }),
            "C2_LINEARITY_GAIN_SATURATION": module({
                "signalLevelsPerGainReadoutState": 12,
                "minRepeatsPerSignalLevel": 8,
                "levelsBracketingSaturationOnset": 3,
                "independentValidationLevelsNotUsedForFit": 4,
                "darkReferenceUsed": True,
                "sourceStabilityMonitored": True,
            }),
        },
        "externalReferences": [],
        "traceability": {"absolute": False},
        "claims": [{
            "quantity": "capture_dynamic_range",
            "authority": "CALIBRATED_PHYSICAL",
            "validated": True,
            "modelId": "honor-tele-dr-model-v1",
            "uncertaintyModelId": "honor-tele-dr-uncertainty-v1",
            "validDomain": {
                "scopeBound": True,
                "description": "fixture ordinary sample-domain range",
                "gainReadoutStateIds": ["ordinary-low", "ordinary-high"],
                "exposureTimeSeconds": {"minInclusive": 0.0001, "maxInclusive": 0.25},
                "isoMetadata": {"minInclusive": 100, "maxInclusive": 12800},
                "fNumber": {"minInclusive": 2.6, "maxInclusive": 2.6}
            },
            "acceptanceProtocolId": "truthraw-fotograaf-calibration-v0.1-fixture",
            "validationReportSha256": sha("d"),
            "uncertaintyReportSha256": sha("e"),
            "dynamicRange": {
                "snrThreshold": 1.0,
                "saturationModelId": "sat-v1",
                "noiseFloorModelId": "noise-v1"
            }
        }]
    }


def valid_scene(worker_count: int = 1):
    return {
        "schema": "truthraw.fotograaf-calibration-scene-request.v0.1",
        "sourceEvidenceSha256": sha("f"),
        "physicalFrameCount": 1,
        "independentEvidenceCount": 1,
        "executionWorkerCount": worker_count,
        "scope": {
            "deviceMake": "HONOR",
            "deviceModel": "BKQ-N49",
            "cameraSystemId": "5",
            "physicalCameraId": "5",
            "lensRole": "TELE",
            "captureApiDomain": "MOTIONCAM_DIRECT_DNG",
            "captureMode": "4080x3072_RAW10",
            "rawWidth": 4080,
            "rawHeight": 3072,
            "cfaPattern": "BGGR",
            "sampleRepresentation": "RAW10_IN_UINT16_DNG",
            "captureSampleDomainId": "ordinary-domain",
            "firmwareBuildId": "fixture-build",
            "focusStateClass": "fixed-near-infinity",
            "stabilizationState": "ois-normal"
        },
        "measurement": {
            "exposureTimeSeconds": 0.01,
            "isoMetadata": 800,
            "fNumber": 2.6,
            "gainReadoutStateId": "ordinary-low"
        },
        "requestedClaims": [{
            "quantity": "capture_dynamic_range",
            "requireCalibratedPhysical": False
        }]
    }


class FotoGraafSceneAdmissionV01Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.calibration_contract = load_json(CALIBRATION_CONTRACT)
        cls.admission_contract = load_json(ADMISSION_CONTRACT)

    def run_admission(self, scene=None, pack=None):
        return admit_scene(
            scene or valid_scene(),
            pack or valid_pack(),
            self.calibration_contract,
            self.admission_contract,
        )

    def test_admission_contract_valid(self):
        validate_admission_contract(self.admission_contract)

    def test_exact_scope_and_runtime_domain_admits(self):
        result = self.run_admission()
        item = result["results"][0]
        self.assertEqual(item["decision"], "CALIBRATED_PHYSICAL_ADMITTED")
        self.assertIsNotNone(item["binding"])
        self.assertEqual(item["binding"]["physicalFrameCount"], 1)
        self.assertEqual(item["binding"]["independentEvidenceCount"], 1)
        self.assertEqual(item["binding"]["protocolSha256"], sha("b"))
        self.assertEqual(item["binding"]["modelSha256"], sha("c"))

    def test_worker_count_does_not_change_binding_digest(self):
        one = self.run_admission(valid_scene(1))["results"][0]["binding"]["bindingSha256"]
        four = self.run_admission(valid_scene(4))["results"][0]["binding"]["bindingSha256"]
        self.assertEqual(one, four)

    def test_model_hash_change_changes_binding_digest(self):
        original = self.run_admission()["results"][0]["binding"]["bindingSha256"]
        changed_pack = valid_pack()
        changed_pack["modelSha256"] = sha("9")
        changed = self.run_admission(pack=changed_pack)["results"][0]["binding"]["bindingSha256"]
        self.assertNotEqual(original, changed)

    def test_same_iso_different_capture_sample_domain_does_not_bind(self):
        scene = valid_scene()
        scene["measurement"]["isoMetadata"] = 8192
        scene["scope"]["captureSampleDomainId"] = "exact-iso8192-associated-domain"
        result = self.run_admission(scene)
        self.assertEqual(result["results"][0]["decision"], "NO_CALIBRATED_PHYSICAL_BINDING")
        self.assertIn("captureSampleDomainId", result["scopeMismatches"])

    def test_gain_readout_state_outside_domain_does_not_bind(self):
        scene = valid_scene()
        scene["measurement"]["gainReadoutStateId"] = "unseen-gain-state"
        result = self.run_admission(scene)
        self.assertEqual(result["results"][0]["decision"], "NO_CALIBRATED_PHYSICAL_BINDING")
        self.assertEqual(result["results"][0]["reason"], "GAIN_READOUT_STATE_OUTSIDE_VALID_DOMAIN")

    def test_exposure_extrapolation_is_rejected(self):
        scene = valid_scene()
        scene["measurement"]["exposureTimeSeconds"] = 1.0
        result = self.run_admission(scene)
        self.assertEqual(result["results"][0]["decision"], "NO_CALIBRATED_PHYSICAL_BINDING")
        self.assertEqual(result["results"][0]["reason"], "OUTSIDE_VALID_DOMAIN_exposureTimeSeconds")

    def test_required_calibration_turns_mismatch_into_fail_closed(self):
        scene = valid_scene()
        scene["scope"]["firmwareBuildId"] = "other-build"
        scene["requestedClaims"][0]["requireCalibratedPhysical"] = True
        result = self.run_admission(scene)
        self.assertTrue(result["strictFailure"])
        self.assertEqual(result["results"][0]["decision"], "FAIL_CLOSED_REQUIRED_CALIBRATION_UNAVAILABLE")

    def test_temperature_calibrated_pack_requires_scene_temperature(self):
        pack = valid_pack()
        pack["temperature"] = {"calibrated": True, "bins": [20.0, 30.0, 40.0]}
        for record in pack["modules"].values():
            record["metrics"]["temperatureBinsCovered"] = 3
        pack["claims"][0]["validDomain"]["temperatureC"] = {
            "minInclusive": 18.0, "maxInclusive": 42.0
        }
        result = self.run_admission(valid_scene(), pack)
        self.assertEqual(result["results"][0]["decision"], "NO_CALIBRATED_PHYSICAL_BINDING")
        self.assertEqual(result["results"][0]["reason"], "SCENE_TEMPERATURE_REQUIRED_BUT_MISSING")

    def test_product_hdr_is_not_a_calibration_claim(self):
        scene = valid_scene()
        scene["requestedClaims"] = [{"quantity": "HDR", "requireCalibratedPhysical": False}]
        result = self.run_admission(scene)
        self.assertEqual(result["results"][0]["decision"], "NO_CALIBRATED_PHYSICAL_BINDING")
        self.assertEqual(result["results"][0]["reason"], "REQUESTED_CLAIM_MISSING_FROM_PACK")

    def test_scene_evidence_count_may_not_change(self):
        scene = valid_scene()
        scene["independentEvidenceCount"] = 2
        with self.assertRaises(AdmissionError):
            self.run_admission(scene)


if __name__ == "__main__":
    unittest.main()
