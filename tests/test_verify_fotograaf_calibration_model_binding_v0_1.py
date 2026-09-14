#!/usr/bin/env python3
import copy
import unittest
from pathlib import Path

from tools.verify_fotograaf_calibration_measurement_model_v0_1 import (
    canonical_sha256 as model_sha256,
    load_json,
)
from tools.verify_fotograaf_calibration_model_binding_v0_1 import (
    BindingModelError,
    bind_model,
    canonical_sha256 as binding_sha256,
)

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "docs/research/fotograaf-scene-metrology-v0.1/TRUTHRAW_FOTOGRAAF_CALIBRATION_MEASUREMENT_MODEL_CONTRACT_V0_1.json"


def sha(ch: str) -> str:
    return ch * 64


def valid_model():
    return {
        "schema": "truthraw.fotograaf-calibration-measurement-model.v0.1",
        "modelId": "honor-tele-dr-model-v1",
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
            "black": {"enabled": True, "module": "C1_DARK_NOISE", "phaseCodeOffsets": [63.5,63.75,63.75,64.0]},
            "noise": {"enabled": True, "module": "C1_DARK_NOISE", "rgbAffineSO": [1.1e-4,1.1e-6,1.3e-4,1.3e-6,1.6e-4,1.6e-6]},
            "response": {"enabled": True, "modules": ["C2_LINEARITY_GAIN_SATURATION", "C5_RELATIVE_RADIOMETRY"], "scalar": 0.985},
            "saturation": {"enabled": True, "module": "C2_LINEARITY_GAIN_SATURATION", "code": 1018.0},
        },
        "darkSnrThreshold": 1.0,
        "requestsAdditionalGainMapCorrection": False,
    }


def valid_binding(model):
    core = {
        "schema": "truthraw.fotograaf-calibration-binding.v0.1",
        "sourceEvidenceSha256": sha("f"),
        "packId": "honor-tele-pack-v1",
        "datasetManifestSha256": model["datasetManifestSha256"],
        "protocolSha256": model["protocolSha256"],
        "modelSha256": model_sha256(model),
        "calibrationScopeSha256": model["calibrationScopeSha256"],
        "quantity": model["claimQuantity"],
        "authority": "CALIBRATED_PHYSICAL",
        "modelId": model["modelId"],
        "uncertaintyModelId": "honor-tele-uncertainty-v1",
        "validDomainSha256": sha("d"),
        "validationReportSha256": sha("e"),
        "uncertaintyReportSha256": sha("1"),
        "acceptanceProtocolId": "truthraw-fotograaf-calibration-v0.1",
        "captureState": {
            "captureSampleDomainId": model["captureSampleDomainId"],
            "gainReadoutStateId": "ordinary-low",
            "exposureTimeSeconds": 0.01,
            "isoMetadata": 800,
            "fNumber": 2.6,
            "temperatureC": None,
        },
        "physicalFrameCount": 1,
        "independentEvidenceCount": 1,
        "changesScientificMasterByItself": False,
        "changesSourceEvidence": False,
        "workerCountAffectsBinding": False,
    }
    binding = dict(core)
    binding["bindingSha256"] = binding_sha256(core)
    return binding


class BindingModelV01Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.contract = load_json(CONTRACT)

    def test_exact_model_materializes_shadow_packet(self):
        model = valid_model()
        packet = bind_model(valid_binding(model), model, self.contract)
        self.assertTrue(packet["shadowOnly"])
        self.assertFalse(packet["changesScientificMaster"])
        self.assertEqual(packet["physicalFrameCount"], 1)
        self.assertEqual(packet["independentEvidenceCount"], 1)
        self.assertEqual(packet["modelSha256"], model_sha256(model))
        self.assertTrue(packet["parameters"]["useCalibratedNoise"])

    def test_modified_numeric_model_fails_exact_hash_binding(self):
        model = valid_model()
        binding = valid_binding(model)
        modified = copy.deepcopy(model)
        modified["parameters"]["response"]["scalar"] = 0.99
        with self.assertRaises(BindingModelError):
            bind_model(binding, modified, self.contract)

    def test_wrong_capture_sample_domain_fails(self):
        model = valid_model()
        binding = valid_binding(model)
        binding_core = dict(binding)
        binding_core.pop("bindingSha256")
        binding_core["captureState"] = dict(binding_core["captureState"])
        binding_core["captureState"]["captureSampleDomainId"] = "exact-iso8192-associated-domain"
        binding = dict(binding_core)
        binding["bindingSha256"] = binding_sha256(binding_core)
        with self.assertRaises(BindingModelError):
            bind_model(binding, model, self.contract)

    def test_forged_binding_digest_fails(self):
        model = valid_model()
        binding = valid_binding(model)
        binding["bindingSha256"] = sha("9")
        with self.assertRaises(BindingModelError):
            bind_model(binding, model, self.contract)

    def test_dataset_manifest_mismatch_fails(self):
        model = valid_model()
        binding = valid_binding(model)
        binding_core = dict(binding)
        binding_core.pop("bindingSha256")
        binding_core["datasetManifestSha256"] = sha("8")
        binding = dict(binding_core)
        binding["bindingSha256"] = binding_sha256(binding_core)
        with self.assertRaises(BindingModelError):
            bind_model(binding, model, self.contract)

    def test_worker_count_not_in_shadow_packet(self):
        model = valid_model()
        packet = bind_model(valid_binding(model), model, self.contract)
        self.assertNotIn("executionWorkerCount", packet)
        self.assertNotIn("workerCount", packet)


if __name__ == "__main__":
    unittest.main()
