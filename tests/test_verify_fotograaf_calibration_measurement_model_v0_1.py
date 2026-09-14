#!/usr/bin/env python3
import copy
import unittest
from pathlib import Path

from tools.verify_fotograaf_calibration_measurement_model_v0_1 import (
    ModelValidationError,
    canonical_sha256,
    load_json,
    validate_contract,
    validate_model,
)

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "docs/research/fotograaf-scene-metrology-v0.1/TRUTHRAW_FOTOGRAAF_CALIBRATION_MEASUREMENT_MODEL_CONTRACT_V0_1.json"


def sha(ch: str) -> str:
    return ch * 64


def valid_model():
    return {
        "schema": "truthraw.fotograaf-calibration-measurement-model.v0.1",
        "modelId": "honor-tele-ordinary-shadow-fixture-v1",
        "protocolSha256": sha("a"),
        "datasetManifestSha256": sha("b"),
        "calibrationScopeSha256": sha("c"),
        "claimQuantity": "capture_dynamic_range",
        "captureSampleDomainId": "ordinary-domain",
        "coordinateContract": {
            "rawCodeUnit": "SOURCE_CODE_VALUE",
            "blackOffsetUnit": "SOURCE_CODE_VALUE",
            "saturationUnit": "SOURCE_CODE_VALUE",
            "noiseCoordinate": "NORMALIZED_PRE_EXISTING_GAINMAP",
            "responseScale": "POSITIVE_SCALAR_ONLY_V0_1",
            "existingSourceGainMapApplication": "EXACTLY_ONCE_AFTER_NORMALIZATION",
            "perChannelResponseScaleAllowed": False,
            "negativePostBlackAllowed": True,
        },
        "parameters": {
            "black": {
                "enabled": True,
                "module": "C1_DARK_NOISE",
                "phaseCodeOffsets": [63.5, 63.75, 63.75, 64.0],
            },
            "noise": {
                "enabled": True,
                "module": "C1_DARK_NOISE",
                "rgbAffineSO": [1.1e-4, 1.1e-6, 1.3e-4, 1.3e-6, 1.6e-4, 1.6e-6],
            },
            "response": {
                "enabled": True,
                "modules": ["C2_LINEARITY_GAIN_SATURATION", "C5_RELATIVE_RADIOMETRY"],
                "scalar": 0.985,
            },
            "saturation": {
                "enabled": True,
                "module": "C2_LINEARITY_GAIN_SATURATION",
                "code": 1018.0,
            },
        },
        "darkSnrThreshold": 1.0,
        "requestsAdditionalGainMapCorrection": False,
    }


class MeasurementModelV01Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.contract = load_json(CONTRACT)

    def test_contract_valid(self):
        validate_contract(self.contract)

    def test_valid_model_computes_stable_sha(self):
        model = valid_model()
        result = validate_model(model, self.contract)
        self.assertTrue(result["valid"])
        self.assertEqual(result["modelSha256"], canonical_sha256(model))
        self.assertEqual(result["enabledBlocks"], ["black", "noise", "response", "saturation"])

    def test_key_order_does_not_change_canonical_sha(self):
        model = valid_model()
        reordered = dict(reversed(list(model.items())))
        self.assertEqual(canonical_sha256(model), canonical_sha256(reordered))

    def test_model_self_hash_is_rejected(self):
        model = valid_model()
        model["modelSha256"] = sha("d")
        with self.assertRaises(ModelValidationError):
            validate_model(model, self.contract)

    def test_second_gainmap_is_rejected(self):
        model = valid_model()
        model["requestsAdditionalGainMapCorrection"] = True
        with self.assertRaises(ModelValidationError):
            validate_model(model, self.contract)

    def test_per_channel_response_is_rejected(self):
        model = valid_model()
        model["parameters"]["response"]["perChannel"] = [1.0, 1.0, 1.0]
        with self.assertRaises(ModelValidationError):
            validate_model(model, self.contract)

    def test_negative_noise_coefficient_is_rejected(self):
        model = valid_model()
        model["parameters"]["noise"]["rgbAffineSO"][1] = -1e-6
        with self.assertRaises(ModelValidationError):
            validate_model(model, self.contract)

    def test_zero_response_scale_is_rejected(self):
        model = valid_model()
        model["parameters"]["response"]["scalar"] = 0.0
        with self.assertRaises(ModelValidationError):
            validate_model(model, self.contract)

    def test_no_parameter_blocks_is_rejected(self):
        model = valid_model()
        model["parameters"] = {}
        with self.assertRaises(ModelValidationError):
            validate_model(model, self.contract)

    def test_partial_noise_only_model_is_valid_and_does_not_require_response(self):
        model = valid_model()
        model["parameters"] = {"noise": copy.deepcopy(model["parameters"]["noise"])}
        result = validate_model(model, self.contract)
        self.assertEqual(result["enabledBlocks"], ["noise"])

    def test_domain_identity_changes_model_sha(self):
        model = valid_model()
        other = copy.deepcopy(model)
        other["captureSampleDomainId"] = "exact-iso8192-associated-domain"
        self.assertNotEqual(canonical_sha256(model), canonical_sha256(other))


if __name__ == "__main__":
    unittest.main()
