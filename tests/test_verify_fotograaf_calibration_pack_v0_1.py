#!/usr/bin/env python3
import unittest
from pathlib import Path

from tools.verify_fotograaf_calibration_pack_v0_1 import (
    ValidationError,
    load_json,
    validate_contract,
    validate_pack,
)

ROOT = Path(__file__).resolve().parents[1]
CONTRACT_PATH = ROOT / "docs/research/fotograaf-scene-metrology-v0.1/TRUTHRAW_FOTOGRAAF_CALIBRATION_CONTRACT_V0_1.json"


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


def calibrated_claim(quantity: str, **extra):
    claim = {
        "quantity": quantity,
        "authority": "CALIBRATED_PHYSICAL",
        "validated": True,
        "modelId": f"fixture-{quantity}-model-v1",
        "uncertaintyModelId": f"fixture-{quantity}-uncertainty-v1",
        "validDomain": {"scopeBound": True, "description": "fixture bounded domain"},
        "acceptanceProtocolId": "truthraw-fotograaf-calibration-v0.1-fixture",
        "validationReportSha256": sha("d"),
        "uncertaintyReportSha256": sha("e"),
    }
    claim.update(extra)
    return claim


def valid_relative_pack():
    return {
        "schema": "truthraw.fotograaf-calibration-pack.v0.1",
        "packId": "honor-tele-fixture-v0.1",
        "status": "VALIDATED_RELATIVE",
        "scope": {
            "deviceMake": "HONOR",
            "deviceModel": "BKQ-N49",
            "cameraSystemId": "5",
            "physicalCameraId": "fixture-physical-id",
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
                "gainReadoutStatesCovered": 7,
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
            "C3_FLAT_SHADING": module({
                "signalLevels": 3,
                "minRepeatsPerSignalLevel": 8,
                "darkReferenceUsed": True,
                "fieldUniformityCharacterized": True,
                "allClaimedFocusStatesCovered": True,
            }),
            "C5_RELATIVE_RADIOMETRY": module({
                "referenceLevels": 8,
                "minRepeatsPerLevel": 5,
                "heldOutValidationLevels": 2,
                "referenceStabilityMeasured": True,
                "geometryFixedAndRecorded": True,
                "exposureAndApertureProvenance": True,
            }),
        },
        "externalReferences": [],
        "traceability": {"absolute": False},
        "claims": [
            calibrated_claim(
                "capture_dynamic_range",
                dynamicRange={
                    "snrThreshold": 1.0,
                    "saturationModelId": "fixture-saturation-v1",
                    "noiseFloorModelId": "fixture-noise-v1",
                },
            ),
            calibrated_claim("relative_scene_radiance"),
        ],
    }


class CalibrationPackV01Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.contract = load_json(CONTRACT_PATH)

    def test_contract_valid(self):
        validate_contract(self.contract)

    def test_relative_pack_admitted(self):
        result = validate_pack(valid_relative_pack(), self.contract)
        self.assertTrue(result["valid"])
        self.assertEqual(result["promotionEligibility"]["capture_dynamic_range"], "CALIBRATED_PHYSICAL_ADMITTED")
        self.assertEqual(result["sceneEvidenceCounts"]["physicalFrameCount"], 1)

    def test_missing_dark_repeat_count_fails(self):
        pack = valid_relative_pack()
        pack["modules"]["C1_DARK_NOISE"]["metrics"]["minRepeatsPerGainExposureTemperatureCell"] = 15
        with self.assertRaises(ValidationError):
            validate_pack(pack, self.contract)

    def test_fit_validation_overlap_fails(self):
        pack = valid_relative_pack()
        pack["modules"]["C2_LINEARITY_GAIN_SATURATION"]["validationDatasetIds"] = ["fit-a"]
        with self.assertRaises(ValidationError):
            validate_pack(pack, self.contract)

    def test_research_only_cannot_self_promote(self):
        pack = valid_relative_pack()
        pack["status"] = "RESEARCH_ONLY"
        with self.assertRaises(ValidationError):
            validate_pack(pack, self.contract)

    def test_calibrated_claim_requires_model_uncertainty_domain_and_reports(self):
        for field in (
            "modelId", "uncertaintyModelId", "validDomain", "acceptanceProtocolId",
            "validationReportSha256", "uncertaintyReportSha256",
        ):
            pack = valid_relative_pack()
            del pack["claims"][0][field]
            with self.subTest(field=field), self.assertRaises(ValidationError):
                validate_pack(pack, self.contract)

    def test_valid_domain_must_be_scope_bound(self):
        pack = valid_relative_pack()
        pack["claims"][0]["validDomain"] = {"scopeBound": False, "description": "bad"}
        with self.assertRaises(ValidationError):
            validate_pack(pack, self.contract)

    def test_duplicate_quantity_claim_fails(self):
        pack = valid_relative_pack()
        pack["claims"].append(dict(pack["claims"][0]))
        with self.assertRaises(ValidationError):
            validate_pack(pack, self.contract)

    def test_absolute_claim_requires_physical_unit(self):
        pack = valid_relative_pack()
        pack["status"] = "VALIDATED_ABSOLUTE_FOR_DECLARED_QUANTITY"
        pack["modules"]["C6_ABSOLUTE_RADIOMETRY"] = module({
            "referenceLevels": 5,
            "minRepeatsPerLevel": 5,
            "traceableReference": True,
            "instrumentModelSerialPresent": True,
            "calibrationCertificateIdentityPresent": True,
            "certificateValidAtAcquisition": True,
            "measurementUncertaintyPresent": True,
            "spectralBandpassPresent": True,
            "geometryAndAngularConditionsPresent": True,
        })
        pack["claims"] = [calibrated_claim("absolute_scene_radiance")]
        pack["traceability"] = {"absolute": True}
        with self.assertRaises(ValidationError):
            validate_pack(pack, self.contract)

    def test_absolute_claim_requires_traceable_reference(self):
        pack = valid_relative_pack()
        pack["status"] = "VALIDATED_ABSOLUTE_FOR_DECLARED_QUANTITY"
        pack["modules"]["C6_ABSOLUTE_RADIOMETRY"] = module({
            "referenceLevels": 5,
            "minRepeatsPerLevel": 5,
            "traceableReference": True,
            "instrumentModelSerialPresent": True,
            "calibrationCertificateIdentityPresent": True,
            "certificateValidAtAcquisition": True,
            "measurementUncertaintyPresent": True,
            "spectralBandpassPresent": True,
            "geometryAndAngularConditionsPresent": True,
        })
        pack["claims"] = [calibrated_claim("absolute_scene_radiance", unit="W sr^-1 m^-2")]
        pack["traceability"] = {"absolute": True}
        with self.assertRaises(ValidationError):
            validate_pack(pack, self.contract)

    def test_incident_light_stays_inferred_without_c7(self):
        pack = valid_relative_pack()
        pack["claims"] = [calibrated_claim("validated_incident_light_inference")]
        with self.assertRaises(ValidationError):
            validate_pack(pack, self.contract)

    def test_scene_evidence_count_cannot_increase(self):
        pack = valid_relative_pack()
        pack["independentEvidenceCountForLaterScene"] = 2
        with self.assertRaises(ValidationError):
            validate_pack(pack, self.contract)

    def test_temperature_calibrated_pack_requires_three_bins_and_module_coverage(self):
        pack = valid_relative_pack()
        pack["temperature"] = {"calibrated": True, "bins": [20.0, 30.0, 40.0]}
        with self.assertRaises(ValidationError):
            validate_pack(pack, self.contract)

    def test_absolute_incident_irradiance_requires_traceable_irradiance_reference(self):
        pack = valid_relative_pack()
        pack["status"] = "VALIDATED_ABSOLUTE_FOR_DECLARED_QUANTITY"
        pack["modules"]["C6_ABSOLUTE_RADIOMETRY"] = module({
            "referenceLevels": 5,
            "minRepeatsPerLevel": 5,
            "traceableReference": True,
            "instrumentModelSerialPresent": True,
            "calibrationCertificateIdentityPresent": True,
            "certificateValidAtAcquisition": True,
            "measurementUncertaintyPresent": True,
            "spectralBandpassPresent": True,
            "geometryAndAngularConditionsPresent": True,
        })
        pack["modules"]["C7_INCIDENT_LIGHT_GEOMETRY"] = module({
            "distinctLightDirections": 3,
            "distinctLightLevelsOrDistances": 3,
            "knownOrMeasuredGeometry": True,
            "surfaceNormals": True,
            "materialReflectanceOrBrdfReference": True,
            "lightPositionDirection": True,
            "cosineCorrectedIrradianceReference": True,
            "shadowVisibilityGroundTruth": True,
        })
        pack["traceability"] = {"absolute": True}
        pack["claims"] = [calibrated_claim("absolute_incident_irradiance", unit="W m^-2")]
        with self.assertRaises(ValidationError):
            validate_pack(pack, self.contract)


if __name__ == "__main__":
    unittest.main()
