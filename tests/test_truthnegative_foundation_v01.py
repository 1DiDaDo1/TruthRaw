import copy
import importlib.util
from pathlib import Path
import unittest
import sys


MODULE_PATH = Path(__file__).resolve().parents[1] / "tools" / "truthnegative_foundation_v01.py"
SPEC = importlib.util.spec_from_file_location("truthnegative_foundation_v01", MODULE_PATH)
tn = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = tn
assert SPEC.loader is not None
SPEC.loader.exec_module(tn)


class TruthNegativeFoundationTests(unittest.TestCase):
    def make(self):
        return tn.build_manifest(
            source_sha256="a" * 64,
            source_width=4080,
            source_height=3072,
            target_width=16320,
            target_height=12288,
        )

    def test_camera5_geometry_is_representation_only(self):
        m = self.make()
        self.assertEqual(m["sourceEvidence"]["sampleCount"], 12_533_760)
        self.assertEqual(m["projectionGrid"]["sampleCount"], 200_540_160)
        self.assertEqual(
            m["projectionGrid"]["sampleCount"] // m["sourceEvidence"]["sampleCount"],
            16,
        )
        self.assertFalse(m["projectionGrid"]["impliesPhysicalSensorGeometry"])
        self.assertEqual(m["projectionGrid"]["measuredClaimCount"], 0)

    def test_evidence_count_cannot_increase(self):
        m = self.make()
        m["evidenceCounts"]["independentEvidenceCount"] = 2
        with self.assertRaises(ValueError):
            tn.validate_manifest(m)

    def test_truthnegative_cannot_create_evidence(self):
        m = self.make()
        m["reconstructionDomain"]["createsNewEvidence"] = True
        with self.assertRaises(ValueError):
            tn.validate_manifest(m)

    def test_unresolved_sampling_model_forbids_target_measured_claim(self):
        m = self.make()
        m["projectionGrid"]["measuredClaimCount"] = 1
        with self.assertRaises(ValueError):
            tn.validate_manifest(m)

    def test_target_resolution_cannot_upgrade_authority(self):
        m = self.make()
        m["laws"]["targetResolutionMayUpgradeEvidenceAuthority"] = True
        with self.assertRaises(ValueError):
            tn.validate_manifest(m)

    def test_source_is_immutable(self):
        m = self.make()
        m["sourceEvidence"]["immutable"] = False
        with self.assertRaises(ValueError):
            tn.validate_manifest(m)

    def test_digest_is_deterministic(self):
        a = self.make()
        b = self.make()
        self.assertEqual(a["manifestSha256"], b["manifestSha256"])


if __name__ == "__main__":
    unittest.main()