from __future__ import annotations

import copy
import importlib.util
import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / "tools"
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))
TOOL = TOOLS / "seal_fotograaf_calibration_capture_v0_1.py"

spec = importlib.util.spec_from_file_location("fotograaf_capture_seal", TOOL)
mod = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(mod)

CONTRACT = ROOT / "docs" / "research" / "fotograaf-scene-metrology-v0.1" / "TRUTHRAW_FOTOGRAAF_CAPTURE_EVIDENCE_ENVELOPE_CONTRACT_V0_1.json"
C0_CONTRACT = ROOT / "docs" / "research" / "fotograaf-scene-metrology-v0.1" / "TRUTHRAW_FOTOGRAAF_C0_CAPTURE_IDENTITY_CONTRACT_V0_1.json"


class FotoGraafCaptureSealerTests(unittest.TestCase):
    def setUp(self) -> None:
        self.contract = mod.load_json(CONTRACT)
        self.c0_contract = mod.load_json(C0_CONTRACT)
        mod.validate_contract(self.contract)

    def scope(self, domain: str = "BKQ-N49_TELE_DIRECT_CFA_DOMAIN_A") -> dict:
        return {
            "deviceMake": "HONOR",
            "deviceModel": "BKQ-N49",
            "cameraSystemId": "5",
            "physicalCameraId": "5-tele-physical-id",
            "lensRole": "TELE",
            "captureApiDomain": "CAMERA2_RAW_SENSOR_CAPTURE_WITH_DNG_WRAPPER",
            "captureMode": "4080x3072_DIRECT_CFA_TARGET",
            "rawWidth": 4080,
            "rawHeight": 3072,
            "cfaPattern": "BGGR",
            "sampleRepresentation": "RAW10_MEASUREMENT_IN_16BIT_DNG_STORAGE",
            "captureSampleDomainId": domain,
            "firmwareBuildId": "HONORBKQ-N49/10.0.0.199C636E4R106P1",
            "focusStateClass": "FIXED_CALIBRATION_FOCUS_STATE_A",
            "stabilizationState": "OIS_OFF_EIS_OFF",
            "protocolVersion": "FOTOGRAAF_CALIBRATION_ACQUISITION_V0_1",
        }

    def envelope(self, capture_id: str = "DARK_001", domain: str = "BKQ-N49_TELE_DIRECT_CFA_DOMAIN_A",
                 module: str = "C1_DARK_NOISE", role: str = "DARK") -> dict:
        scope = self.scope(domain)
        observations = {}
        methods = {
            "cameraSystemId": "CAMERA2_LOGICAL_CAMERA_ID_QUERY",
            "physicalCameraId": "CAMERA2_PHYSICAL_CAMERA_RESULT",
            "captureApiDomain": "CAPTURE_APPLICATION_ROUTE",
            "captureSampleDomainId": "RAW_PAYLOAD_AND_METADATA_CLASSIFIER",
            "firmwareBuildId": "ANDROID_BUILD_FINGERPRINT",
            "focusStateClass": "CAPTURE_CONTROL_AND_RESULT",
            "stabilizationState": "CAPTURE_CONTROL_AND_RESULT",
        }
        for field, method in methods.items():
            observations[field] = {"value": str(scope[field]), "method": method, "evidenceId": f"capture-result:{capture_id}:{field}"}
        measurement = {
            "exposureTimeNs": 32734796,
            "isoMetadata": 100,
            "gainReadoutStateObservation": {
                "value": "GAIN_READOUT_STATE_A",
                "method": "CAPTURE_STACK_GAIN_READOUT_CLASSIFIER",
                "evidenceId": f"capture-result:{capture_id}:gain-readout",
            },
            "blackLevelIdentity": "BLACKLEVEL_PHASE4_64",
            "whiteLevelIdentity": "WHITELEVEL_1023",
            "gainMapOpcodeIdentity": "GAINMAP_OPCODE_SOURCE_A",
            "focusState": scope["focusStateClass"],
            "stabilizationState": scope["stabilizationState"],
            "temperatureObservation": {"status": "UNAVAILABLE", "reason": "test-no-sensor-temp"},
        }
        if module == "C2_LINEARITY_GAIN_SATURATION":
            measurement.update({
                "controlledSignalLevelId": "LEVEL_01",
                "sourceStabilityReferenceId": "SOURCE_MONITOR_01",
                "isSaturationBracket": role == "SATURATION_BRACKET",
            })
        return {
            "schema": mod.ENVELOPE_SCHEMA,
            "captureId": capture_id,
            "sourceFileName": f"captures/{capture_id}.dng",
            "scope": scope,
            "identityObservations": observations,
            "topologyEvidence": {
                "rawWidth": 4080,
                "rawHeight": 3072,
                "cfaPattern": "BGGR",
                "sampleRepresentation": scope["sampleRepresentation"],
                "directCfaMeasurement": True,
                "processedRgbInput": False,
                "multiFrameEvidenceMerged": False,
                "parserBackendId": "truthraw-calibration-capture-seal-test-parser-v0.1",
            },
            "measurement": measurement,
            "acquisition": {
                "module": module,
                "role": role,
                "split": "FIT",
                "acquisitionAnchorId": "ISO_ANCHOR_100",
            },
        }

    def write_source(self, root: Path, envelope: dict) -> Path:
        path = root / envelope["sourceFileName"]
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(("sealed-direct-cfa-test:" + envelope["captureId"]).encode() * 8)
        return path

    def test_valid_dark_capture_seals_and_revalidates_c0(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            envelope = self.envelope()
            source = self.write_source(root, envelope)
            result = mod.seal_capture(envelope, self.contract, self.c0_contract, root, root)
            self.assertFalse(result["calibrationAuthorityGrantedBySealing"])
            self.assertEqual(result["physicalFrameCountForLaterScene"], 1)
            self.assertEqual(result["independentEvidenceCountForLaterScene"], 1)
            metadata = root / result["metadataSnapshotFileName"]
            c0_record = root / result["c0RecordFileName"]
            seal = mod.c0.validate_record(mod.load_json(c0_record), self.c0_contract, source, metadata)
            self.assertEqual(seal["decision"], "C0_IDENTITY_SEALED")
            snapshot = mod.load_json(metadata)
            self.assertEqual(snapshot["measurement"]["gainReadoutStateId"], "GAIN_READOUT_STATE_A")

    def test_physical_camera_id_from_dng_metadata_is_rejected(self) -> None:
        envelope = self.envelope()
        envelope["identityObservations"]["physicalCameraId"]["method"] = "DNG_METADATA_ONLY"
        with self.assertRaises(mod.SealError):
            mod.validate_envelope(envelope, self.contract, self.c0_contract)

    def test_iso_derived_sample_domain_is_rejected(self) -> None:
        envelope = self.envelope(domain="EXACT_ISO8192_ASSOCIATED_DOMAIN")
        envelope["measurement"]["isoMetadata"] = 8192
        envelope["identityObservations"]["captureSampleDomainId"]["method"] = "ISO_DERIVED"
        with self.assertRaises(mod.SealError):
            mod.validate_envelope(envelope, self.contract, self.c0_contract)

    def test_iso_derived_gain_readout_state_is_rejected(self) -> None:
        envelope = self.envelope()
        envelope["measurement"]["gainReadoutStateObservation"]["method"] = "ISO_ONLY"
        with self.assertRaises(mod.SealError):
            mod.validate_envelope(envelope, self.contract, self.c0_contract)

    def test_c2_requires_source_stability_and_signal_level(self) -> None:
        envelope = self.envelope(module="C2_LINEARITY_GAIN_SATURATION", role="UNIFORM_SIGNAL")
        del envelope["measurement"]["sourceStabilityReferenceId"]
        with self.assertRaises(mod.SealError):
            mod.validate_envelope(envelope, self.contract, self.c0_contract)

    def test_source_mutation_after_seal_breaks_c0(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            envelope = self.envelope()
            source = self.write_source(root, envelope)
            result = mod.seal_capture(envelope, self.contract, self.c0_contract, root, root)
            metadata = root / result["metadataSnapshotFileName"]
            c0_record = root / result["c0RecordFileName"]
            source.write_bytes(source.read_bytes() + b"mutated")
            with self.assertRaises(mod.c0.C0Error):
                mod.c0.validate_record(mod.load_json(c0_record), self.c0_contract, source, metadata)

    def test_same_iso_can_seal_different_sample_domains(self) -> None:
        a = self.envelope("A", "ORDINARY_DOMAIN")
        b = self.envelope("B", "EXACT_8192_ASSOCIATED_DOMAIN")
        a["measurement"]["isoMetadata"] = 8192
        b["measurement"]["isoMetadata"] = 8192
        na = mod.validate_envelope(a, self.contract, self.c0_contract)
        nb = mod.validate_envelope(b, self.contract, self.c0_contract)
        self.assertNotEqual(mod.c0.canonical_sha256(na["scope"]), mod.c0.canonical_sha256(nb["scope"]))


if __name__ == "__main__":
    unittest.main()
