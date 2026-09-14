#!/usr/bin/env python3
import copy
import unittest
from pathlib import Path

from tools.verify_fotograaf_calibration_pack_v0_1 import load_json
from tools.verify_fotograaf_calibration_scene_admission_v0_1 import admit_scene
from tools.verify_fotograaf_physical_promotion_gate_v0_1 import (
    PromotionError,
    canonical_sha256,
    evaluate_promotion,
    validate_contract,
)

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "docs/research/fotograaf-scene-metrology-v0.1"
CALIBRATION_CONTRACT = BASE / "TRUTHRAW_FOTOGRAAF_CALIBRATION_CONTRACT_V0_1.json"
ADMISSION_CONTRACT = BASE / "TRUTHRAW_FOTOGRAAF_CALIBRATION_SCENE_ADMISSION_CONTRACT_V0_1.json"
SHADOW_CONTRACT = BASE / "TRUTHRAW_FOTOGRAAF_CALIBRATION_MODEL_SHADOW_CONTRACT_V0_1.json"
PROMOTION_CONTRACT = BASE / "TRUTHRAW_FOTOGRAAF_PHYSICAL_PROMOTION_GATE_CONTRACT_V0_1.json"


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


def pack_fixture():
    return {
        "schema": "truthraw.fotograaf-calibration-pack.v0.1",
        "packId": "honor-tele-ordinary-domain-v0.1",
        "status": "VALIDATED_RELATIVE",
        "scope": {
            "deviceMake": "HONOR", "deviceModel": "BKQ-N49", "cameraSystemId": "5",
            "physicalCameraId": "5", "lensRole": "TELE", "captureApiDomain": "MOTIONCAM_DIRECT_DNG",
            "captureMode": "4080x3072_RAW10", "rawWidth": 4080, "rawHeight": 3072,
            "cfaPattern": "BGGR", "sampleRepresentation": "RAW10_IN_UINT16_DNG",
            "captureSampleDomainId": "ordinary-domain", "firmwareBuildId": "fixture-build",
            "focusStateClass": "fixed-near-infinity", "stabilizationState": "ois-normal",
            "protocolVersion": "0.1",
        },
        "datasetManifestSha256": sha("a"), "protocolSha256": sha("b"), "modelSha256": sha("c"),
        "physicalFrameCountForLaterScene": 1, "independentEvidenceCountForLaterScene": 1,
        "fitValidationSeparated": True, "thresholdProtocolSealedBeforeFit": True,
        "temperature": {"calibrated": False, "bins": []},
        "modules": {
            "C0_IDENTITY": module({
                "captureFileSha256ForEveryFrame": True, "parserBackendIdentity": True,
                "scopeKeyComplete": True, "metadataSnapshotPerFrame": True, "gainMapOpcodeIdentity": True,
            }),
            "C1_DARK_NOISE": module({
                "exposureTimesPerState": 4, "minRepeatsPerGainExposureTemperatureCell": 16,
                "gainReadoutStatesCovered": 2, "photonBlockedDark": True,
                "allIntendedGainReadoutStatesCovered": True, "temperatureCalibrated": False,
                "temperatureBins": 1,
            }),
            "C2_LINEARITY_GAIN_SATURATION": module({
                "signalLevelsPerGainReadoutState": 12, "minRepeatsPerSignalLevel": 8,
                "levelsBracketingSaturationOnset": 3, "independentValidationLevelsNotUsedForFit": 4,
                "darkReferenceUsed": True, "sourceStabilityMonitored": True,
            }),
        },
        "externalReferences": [], "traceability": {"absolute": False},
        "claims": [{
            "quantity": "capture_dynamic_range", "authority": "CALIBRATED_PHYSICAL", "validated": True,
            "modelId": "honor-tele-dr-model-v1", "uncertaintyModelId": "honor-tele-dr-uncertainty-v1",
            "validDomain": {
                "scopeBound": True, "description": "fixture ordinary sample-domain range",
                "gainReadoutStateIds": ["ordinary-low", "ordinary-high"],
                "exposureTimeSeconds": {"minInclusive": 0.0001, "maxInclusive": 0.25},
                "isoMetadata": {"minInclusive": 100, "maxInclusive": 12800},
                "fNumber": {"minInclusive": 2.6, "maxInclusive": 2.6},
            },
            "acceptanceProtocolId": "truthraw-fotograaf-calibration-v0.1-fixture",
            "validationReportSha256": sha("d"), "uncertaintyReportSha256": sha("e"),
            "dynamicRange": {"snrThreshold": 1.0, "saturationModelId": "sat-v1", "noiseFloorModelId": "noise-v1"},
        }],
    }


def scene_fixture(worker_count=1):
    return {
        "schema": "truthraw.fotograaf-calibration-scene-request.v0.1",
        "sourceEvidenceSha256": sha("f"), "physicalFrameCount": 1, "independentEvidenceCount": 1,
        "executionWorkerCount": worker_count,
        "scope": {
            "deviceMake": "HONOR", "deviceModel": "BKQ-N49", "cameraSystemId": "5",
            "physicalCameraId": "5", "lensRole": "TELE", "captureApiDomain": "MOTIONCAM_DIRECT_DNG",
            "captureMode": "4080x3072_RAW10", "rawWidth": 4080, "rawHeight": 3072,
            "cfaPattern": "BGGR", "sampleRepresentation": "RAW10_IN_UINT16_DNG",
            "captureSampleDomainId": "ordinary-domain", "firmwareBuildId": "fixture-build",
            "focusStateClass": "fixed-near-infinity", "stabilizationState": "ois-normal",
        },
        "measurement": {
            "exposureTimeSeconds": 0.01, "isoMetadata": 800, "fNumber": 2.6,
            "gainReadoutStateId": "ordinary-low",
        },
        "requestedClaims": [{"quantity": "capture_dynamic_range", "requireCalibratedPhysical": True}],
    }


def physical_report(quantity="capture_dynamic_range", model_id="honor-tele-dr-model-v1",
                    protocol="truthraw-fotograaf-calibration-v0.1-fixture"):
    return {
        "schema": "truthraw.fotograaf-physical-validation-report.v0.1", "passed": True,
        "quantity": quantity, "modelId": model_id, "acceptanceProtocolId": protocol,
        "protocolSealedBeforeFinalFit": True, "fitDataReusedForHeldOutValidation": False,
        "fitDatasetIds": ["fit-physical-1"], "heldOutValidationDatasetIds": ["heldout-physical-1"],
        "captureSampleDomainIdsValidated": ["ordinary-domain"],
    }


def uncertainty_report(quantity, uncertainty_model_id, valid_domain_sha):
    return {
        "schema": "truthraw.fotograaf-uncertainty-validation-report.v0.1", "passed": True,
        "quantity": quantity, "uncertaintyModelId": uncertainty_model_id,
        "validDomainBound": True, "coverageAcceptancePassed": True,
        "validDomainSha256": valid_domain_sha,
    }


def shadow_report(binding, quantity="capture_dynamic_range", protocol="truthraw-fotograaf-calibration-v0.1-fixture"):
    master = sha("7")
    worker = sha("8")
    return {
        "schema": "truthraw.fotograaf-shadow-comparison-report.v0.1", "passed": True,
        "shadowOnly": True, "productionRouteChanged": False, "quantity": quantity,
        "sourceEvidenceSha256": binding["sourceEvidenceSha256"], "bindingSha256": binding["bindingSha256"],
        "modelSha256": binding["modelSha256"], "protocolSha256": binding["protocolSha256"],
        "thresholdProtocolId": protocol, "sourceIdentityRegressionPassed": True,
        "sourceCensorPreserved": True, "noDoubleCorrectionPassed": True,
        "captureSampleDomainRegressionPassed": True, "comparisonThresholdsPassed": True,
        "productionScientificMasterBeforeSha256": master,
        "productionScientificMasterAfterSha256": master,
        "workerResultSha256": {"1": worker, "2": worker, "4": worker},
        "noiseOnlySignalCoordinateBitIdentical": True,
    }


class FotoGraafPhysicalPromotionGateV01Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.calibration_contract = load_json(CALIBRATION_CONTRACT)
        cls.admission_contract = load_json(ADMISSION_CONTRACT)
        cls.shadow_contract = load_json(SHADOW_CONTRACT)
        cls.promotion_contract = load_json(PROMOTION_CONTRACT)

    def make_chain(self, quantity="capture_dynamic_range"):
        pack = pack_fixture()
        if quantity == "read_noise":
            pack["claims"][0].update({
                "quantity": "read_noise", "modelId": "honor-tele-read-noise-v1",
                "uncertaintyModelId": "honor-tele-read-noise-unc-v1"
            })
            pack["claims"][0].pop("dynamicRange", None)
        claim = pack["claims"][0]
        phys = physical_report(quantity, claim["modelId"], claim["acceptanceProtocolId"])
        unc = uncertainty_report(quantity, claim["uncertaintyModelId"], canonical_sha256(claim["validDomain"]))
        claim["validationReportSha256"] = canonical_sha256(phys)
        claim["uncertaintyReportSha256"] = canonical_sha256(unc)
        scene = scene_fixture()
        scene["requestedClaims"][0]["quantity"] = quantity
        admission = admit_scene(scene, pack, self.calibration_contract, self.admission_contract)
        binding = admission["results"][0]["binding"]
        self.assertIsNotNone(binding)
        shadow = shadow_report(binding, quantity, claim["acceptanceProtocolId"])
        request = {
            "schema": "truthraw.fotograaf-physical-promotion-request.v0.1",
            "quantity": quantity, "sourceEvidenceSha256": scene["sourceEvidenceSha256"],
            "packId": pack["packId"], "bindingSha256": binding["bindingSha256"],
            "physicalFrameCount": 1, "independentEvidenceCount": 1,
            "requestProductionRouteMutation": False,
        }
        return request, pack, admission, phys, unc, shadow

    def evaluate(self, chain):
        return evaluate_promotion(*chain, self.calibration_contract, self.admission_contract,
                                  self.shadow_contract, self.promotion_contract)

    def test_contract_valid(self):
        validate_contract(self.promotion_contract)

    def test_complete_chain_becomes_eligible_not_activated(self):
        result = self.evaluate(self.make_chain())
        self.assertEqual(result["decision"], "ELIGIBLE_FOR_EXPLICIT_PROMOTION_REVIEW")
        self.assertFalse(result["productionRouteActivated"])
        self.assertTrue(result["requiresSeparateProductionPromotionReview"])
        self.assertEqual(result["physicalFrameCount"], 1)
        self.assertEqual(result["independentEvidenceCount"], 1)

    def test_validation_report_hash_mismatch_rejected(self):
        chain = list(self.make_chain())
        chain[3]["passed"] = False
        with self.assertRaises(PromotionError):
            self.evaluate(tuple(chain))

    def test_worker_result_mismatch_rejected(self):
        chain = list(self.make_chain())
        chain[5]["workerResultSha256"]["4"] = sha("9")
        with self.assertRaises(PromotionError):
            self.evaluate(tuple(chain))

    def test_production_master_identity_change_rejected(self):
        chain = list(self.make_chain())
        chain[5]["productionScientificMasterAfterSha256"] = sha("9")
        with self.assertRaises(PromotionError):
            self.evaluate(tuple(chain))

    def test_lost_source_censor_rejected(self):
        chain = list(self.make_chain())
        chain[5]["sourceCensorPreserved"] = False
        with self.assertRaises(PromotionError):
            self.evaluate(tuple(chain))

    def test_double_correction_rejected(self):
        chain = list(self.make_chain())
        chain[5]["noDoubleCorrectionPassed"] = False
        with self.assertRaises(PromotionError):
            self.evaluate(tuple(chain))

    def test_capture_sample_domain_regression_rejected(self):
        chain = list(self.make_chain())
        chain[5]["captureSampleDomainRegressionPassed"] = False
        with self.assertRaises(PromotionError):
            self.evaluate(tuple(chain))

    def test_threshold_protocol_mismatch_rejected(self):
        chain = list(self.make_chain())
        chain[5]["thresholdProtocolId"] = "other-protocol"
        with self.assertRaises(PromotionError):
            self.evaluate(tuple(chain))

    def test_missing_required_calibration_module_rejected(self):
        chain = list(self.make_chain())
        chain[1]["modules"]["C2_LINEARITY_GAIN_SATURATION"]["validated"] = False
        with self.assertRaises(Exception):
            self.evaluate(tuple(chain))

    def test_exact_model_hash_mismatch_rejected(self):
        chain = list(self.make_chain())
        chain[5]["modelSha256"] = sha("9")
        with self.assertRaises(PromotionError):
            self.evaluate(tuple(chain))

    def test_noise_only_claim_may_not_move_signal_coordinate(self):
        chain = list(self.make_chain("read_noise"))
        chain[5]["noiseOnlySignalCoordinateBitIdentical"] = False
        with self.assertRaises(PromotionError):
            self.evaluate(tuple(chain))

    def test_fit_holdout_overlap_rejected_even_with_hash_rebound(self):
        chain = list(self.make_chain())
        chain[3]["heldOutValidationDatasetIds"] = ["fit-physical-1"]
        new_sha = canonical_sha256(chain[3])
        chain[1]["claims"][0]["validationReportSha256"] = new_sha
        # Rebuild admission because validationReportSha256 is part of the binding.
        scene = scene_fixture()
        chain[2] = admit_scene(scene, chain[1], self.calibration_contract, self.admission_contract)
        binding = chain[2]["results"][0]["binding"]
        chain[0]["bindingSha256"] = binding["bindingSha256"]
        chain[5] = shadow_report(binding)
        with self.assertRaises(PromotionError):
            self.evaluate(tuple(chain))


if __name__ == "__main__":
    unittest.main()
