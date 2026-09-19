import copy
import importlib.util
from pathlib import Path
import sys
import unittest


PATH = Path(__file__).resolve().parents[1] / "tools" / "truthnegative_existing_house_binding_v02.py"
SPEC = importlib.util.spec_from_file_location("truthnegative_existing_house_binding_v02", PATH)
mod = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = mod
assert SPEC.loader is not None
SPEC.loader.exec_module(mod)


class ExistingHouseBindingTests(unittest.TestCase):
    def make(self, ingress=None):
        return mod.build_binding(
            source_sha256="a" * 64,
            scientific_master_sha256="b" * 64,
            ingress_class=ingress or mod.IngressClass.NATIVE_CERTIFIED_SOURCE,
            source_width=4080,
            source_height=3072,
            projection_width=16320,
            projection_height=12288,
        )

    def test_truthnegative_is_not_second_world(self):
        m = self.make()
        self.assertFalse(m["truthNegativeCore"]["createsSecondScientificWorld"])

    def test_dense_projection_is_reconstructed(self):
        m = self.make()
        p = m["sensorNegativeProjection"]
        self.assertEqual(p["sampleCount"], 200_540_160)
        self.assertEqual(p["newTargetSupportAuthority"], "RECONSTRUCTED")
        self.assertEqual(p["measuredTargetClaimCount"], 0)
        self.assertFalse(p["impliesPhysicalSensorGeometry"])

    def test_gatehouse_ingress_is_brand_independent_contract(self):
        m = self.make(mod.IngressClass.GATEHOUSE_DECODED_MEASUREMENT_HANDOFF)
        self.assertEqual(
            m["truthNegativeCore"]["ingressClass"],
            "GATEHOUSE_DECODED_MEASUREMENT_HANDOFF",
        )
        mod.validate_binding(m)

    def test_sensor_projection_cannot_replace_master(self):
        m = self.make()
        m["sensorNegativeProjection"]["isScientificMasterReplacement"] = True
        with self.assertRaises(ValueError):
            mod.validate_binding(m)

    def test_reconstructed_cfa_cannot_become_measured_cfa(self):
        m = self.make()
        m["separation"]["reconstructedCfaMayBeRelabelledOriginalMeasuredCfa"] = True
        with self.assertRaises(ValueError):
            mod.validate_binding(m)

    def test_precision_does_not_upgrade_authority(self):
        m = self.make()
        m["precision"]["precisionUpgradesEvidenceAuthority"] = True
        with self.assertRaises(ValueError):
            mod.validate_binding(m)

    def test_evidence_count_stays_one(self):
        m = self.make()
        m["evidenceCounts"]["physicalFrameCount"] = 2
        with self.assertRaises(ValueError):
            mod.validate_binding(m)

    def test_truthnegative_cannot_prove_honor_route(self):
        m = self.make()
        m["separation"]["honorRouteMayBeProvenByTruthNegative"] = True
        with self.assertRaises(ValueError):
            mod.validate_binding(m)


if __name__ == "__main__":
    unittest.main()
