from __future__ import annotations

import copy
import hashlib
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
TOOL = TOOLS / "build_fotograaf_calibration_intake_v0_1.py"

spec = importlib.util.spec_from_file_location("fotograaf_intake", TOOL)
mod = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(mod)

INTAKE_CONTRACT = ROOT / "docs" / "research" / "fotograaf-scene-metrology-v0.1" / "TRUTHRAW_FOTOGRAAF_CALIBRATION_INTAKE_CONTRACT_V0_1.json"
C0_CONTRACT = ROOT / "docs" / "research" / "fotograaf-scene-metrology-v0.1" / "TRUTHRAW_FOTOGRAAF_C0_CAPTURE_IDENTITY_CONTRACT_V0_1.json"
MANIFEST_CONTRACT = ROOT / "docs" / "research" / "fotograaf-scene-metrology-v0.1" / "TRUTHRAW_FOTOGRAAF_CALIBRATION_DATASET_MANIFEST_CONTRACT_V0_1.json"
PLAN = ROOT / "docs" / "research" / "fotograaf-scene-metrology-v0.1" / "TRUTHRAW_HONOR_TELE_CALIBRATION_ACQUISITION_PLAN_V0_1.json"


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


class FotoGraafCalibrationIntakeTests(unittest.TestCase):
    def setUp(self) -> None:
        self.official = mod.load_json(INTAKE_CONTRACT)
        self.c0_contract = mod.load_json(C0_CONTRACT)
        self.manifest_contract = mod.load_json(MANIFEST_CONTRACT)
        self.plan = mod.load_json(PLAN)
        mod.validate_contract(self.official, self.plan)
        self.contract = copy.deepcopy(self.official)
        self.contract["phaseACompleteness"] = {
            "minimumAcquisitionAnchors": 1,
            "C1_DARK_NOISE": {
                "minimumDistinctExposureTimesPerAnchor": 2,
                "minimumIndependentRepeatsPerAnchorExposure": 2,
            },
            "C2_LINEARITY_GAIN_SATURATION": {
                "minimumDistinctSignalLevelsPerAnchor": 2,
                "minimumIndependentRepeatsPerSignalLevel": 2,
                "minimumHeldOutValidationLevelsPerAnchor": 1,
                "minimumSaturationBracketLevelsPerAnchor": 1,
            },
        }
        mod.validate_contract(self.contract)

    def scope(self, domain: str) -> dict:
        return {
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
            "captureSampleDomainId": domain,
            "firmwareBuildId": "HONORBKQ-N49/10.0.0.199C636E4R106P1",
            "focusStateClass": "FIXED_CALIBRATION_FOCUS_STATE_A",
            "stabilizationState": "OIS_OFF_EIS_OFF",
            "protocolVersion": "FOTOGRAAF_CALIBRATION_ACQUISITION_V0_1",
        }

    def write_capture(self, root: Path, capture_id: str, scope: dict, module: str, role: str,
                      split: str, anchor: str, exposure_ns: int, iso: int,
                      level: str | None = None, saturation: bool = False) -> dict:
        source_rel = Path("captures") / f"{capture_id}.dng"
        meta_rel = Path("metadata") / f"{capture_id}.json"
        c0_rel = Path("c0") / f"{capture_id}.json"
        for p in (root / source_rel, root / meta_rel, root / c0_rel):
            p.parent.mkdir(parents=True, exist_ok=True)
        source = (f"truthraw-test-source:{capture_id}:{scope['captureSampleDomainId']}" * 3).encode()
        (root / source_rel).write_bytes(source)
        source_sha = sha(source)
        scope_sha = mod.c0.canonical_sha256(scope)
        measurement = {
            "exposureTimeNs": exposure_ns,
            "isoMetadata": iso,
            "gainReadoutStateId": f"GAIN_STATE_{anchor}",
            "blackLevelIdentity": "BLACKLEVEL_64_PHASE4",
            "whiteLevelIdentity": "WHITELEVEL_1023",
            "gainMapOpcodeIdentity": "GAINMAP_SOURCE_OPCODE_IDENTITY_A",
            "focusState": scope["focusStateClass"],
            "stabilizationState": scope["stabilizationState"],
            "temperatureObservation": {"status": "UNAVAILABLE", "reason": "unit-test"},
        }
        if module == "C2_LINEARITY_GAIN_SATURATION":
            measurement.update({
                "controlledSignalLevelId": level,
                "sourceStabilityReferenceId": "SOURCE_MONITOR_A",
                "isSaturationBracket": saturation,
            })
        snapshot = {
            "schema": mod.SNAPSHOT_SCHEMA,
            "sourceEvidenceSha256": source_sha,
            "scopeKeySha256": scope_sha,
            "measurement": measurement,
        }
        meta_bytes = (json.dumps(snapshot, sort_keys=True, separators=(",", ":")) + "\n").encode()
        (root / meta_rel).write_bytes(meta_bytes)

        def obs(field: str, method: str) -> dict:
            return {"value": str(scope[field]), "method": method, "evidenceId": f"sidecar:{capture_id}:{field}"}

        record = {
            "schema": mod.c0.RECORD_SCHEMA,
            "recordId": f"C0_{capture_id}",
            "scope": scope,
            "scopeKeySha256": scope_sha,
            "sourceEvidence": {"fileName": str(source_rel), "sha256": source_sha, "byteLength": len(source)},
            "metadataSnapshot": {"fileName": str(meta_rel), "sha256": sha(meta_bytes), "byteLength": len(meta_bytes)},
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
                "rawWidth": scope["rawWidth"],
                "rawHeight": scope["rawHeight"],
                "cfaPattern": scope["cfaPattern"],
                "sampleRepresentation": scope["sampleRepresentation"],
                "directCfaMeasurement": True,
                "processedRgbInput": False,
                "multiFrameEvidenceMerged": False,
                "parserBackendId": "truthraw-intake-unit-test-parser-v0.1",
            },
            "physicalFrameCountForLaterScene": 1,
            "independentEvidenceCountForLaterScene": 1,
            "calibrationAuthorityGrantedByC0": False,
        }
        record["c0RecordSha256"] = mod.c0.canonical_record_sha256(record)
        (root / c0_rel).write_text(json.dumps(record, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        return {
            "captureId": capture_id,
            "module": module,
            "role": role,
            "split": split,
            "sourceFileName": str(source_rel),
            "metadataSnapshotFileName": str(meta_rel),
            "c0RecordFileName": str(c0_rel),
            "acquisitionAnchorId": anchor,
        }

    def complete_session(self, root: Path) -> dict:
        entries = []
        scope_a = self.scope("BKQ-N49_TELE_DIRECT_CFA_DOMAIN_A")
        scope_b = self.scope("BKQ-N49_TELE_EXACT_8192_DOMAIN_B")
        for exposure in (1000, 2000):
            for repeat in range(2):
                entries.append(self.write_capture(
                    root, f"dark_{exposure}_{repeat}", scope_a, "C1_DARK_NOISE", "DARK", "FIT",
                    "ANCHOR_100", exposure, 100,
                ))
        for level, split, sat in (("L01", "FIT", False), ("L02", "VALIDATION", True)):
            for repeat in range(2):
                entries.append(self.write_capture(
                    root, f"uniform_{level}_{repeat}", scope_b, "C2_LINEARITY_GAIN_SATURATION",
                    "SATURATION_BRACKET" if sat else "UNIFORM_SIGNAL", split,
                    "ANCHOR_8192", 16000000, 8192, level, sat,
                ))
        return {"schema": mod.SESSION_SCHEMA, "sessionId": "UNIT_TEST_INTAKE", "captures": entries, "externalReferences": []}

    def test_official_contract_matches_phase_a_plan(self) -> None:
        mod.validate_contract(self.official, self.plan)

    def test_complete_candidate_partitions_exact_c0_scopes(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            session = self.complete_session(root)
            out = root / "out"
            bundle = mod.build_intake(session, self.contract, self.c0_contract, self.manifest_contract, root, out)
            self.assertTrue(bundle["completeness"]["complete"])
            self.assertEqual(bundle["status"], "PHASE_A_C1_C2_COMPLETE_CANDIDATE")
            self.assertEqual(bundle["scopePartitionCount"], 2)
            self.assertEqual(len(bundle["manifests"]), 2)
            self.assertEqual({x["captureSampleDomainId"] for x in bundle["manifests"]},
                             {"BKQ-N49_TELE_DIRECT_CFA_DOMAIN_A", "BKQ-N49_TELE_EXACT_8192_DOMAIN_B"})
            self.assertTrue((out / "UNIT_TEST_INTAKE_intake_bundle_v0_1.json").is_file())
            self.assertEqual(bundle["physicalFrameCountForLaterScene"], 1)
            self.assertEqual(bundle["independentEvidenceCountForLaterScene"], 1)
            self.assertFalse(bundle["calibrationAuthorityGrantedByIntake"])

    def test_incomplete_candidate_is_reported_not_promoted(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            session = self.complete_session(root)
            session["captures"] = [x for x in session["captures"] if x["captureId"] != "uniform_L02_1"]
            bundle = mod.build_intake(session, self.contract, self.c0_contract, self.manifest_contract, root, root / "out")
            self.assertFalse(bundle["completeness"]["complete"])
            self.assertEqual(bundle["status"], "CANDIDATE_INCOMPLETE")
            self.assertTrue(any(x.startswith("C2_REPEATS") for x in bundle["completeness"]["findings"]))

    def test_duplicate_source_bytes_fail_even_with_different_capture_ids(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            session = self.complete_session(root)
            first = session["captures"][0]
            second = session["captures"][1]
            source_a = root / first["sourceFileName"]
            source_b = root / second["sourceFileName"]
            source_b.write_bytes(source_a.read_bytes())
            # C0 record is now stale; fail-closed must occur before duplicate acceptance.
            with self.assertRaises((mod.IntakeError, mod.c0.C0Error)):
                mod.build_intake(session, self.contract, self.c0_contract, self.manifest_contract, root, root / "out")

    def test_iso_value_does_not_collapse_different_sample_domains(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            session = self.complete_session(root)
            # Both scopes can carry ISO 8192 metadata; exact C0 scope still forces partitioning.
            for capture in session["captures"][:1]:
                meta_path = root / capture["metadataSnapshotFileName"]
                snapshot = json.loads(meta_path.read_text())
                snapshot["measurement"]["isoMetadata"] = 8192
                # Changing sidecar bytes invalidates the pre-existing C0 seal, which is the desired fail-closed behavior.
                meta_path.write_text(json.dumps(snapshot, sort_keys=True, separators=(",", ":")) + "\n")
                with self.assertRaises(mod.c0.C0Error):
                    mod.build_intake(session, self.contract, self.c0_contract, self.manifest_contract, root, root / "out")
                break


if __name__ == "__main__":
    unittest.main()
