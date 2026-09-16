import copy
import json
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from camera5_200mp_android_contract_v04 import (  # noqa: E402
    BLOCKED,
    EXPECTED_CAPTURE_GATE,
    PASS,
    evaluate_contract,
)


class Camera5200MPAndroidContractV04Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.capability = json.loads(
            (ROOT / "evidence" / "CAMERA5_FULL_SENSOR_CAPABILITY_GATE_v06.json").read_text(
                encoding="utf-8"
            )
        )
        cls.host = json.loads(
            (ROOT / "evidence" / "HOST_VALIDATION_CAMERA5_200MP_v07.json").read_text(
                encoding="utf-8"
            )
        )

    def test_current_static_and_host_state_passes_but_runtime_stays_open(self):
        r = evaluate_contract(self.capability, self.host)
        self.assertTrue(r["pass"])
        self.assertEqual(r["classification"], PASS)
        self.assertEqual(r["runtime_gate"], EXPECTED_CAPTURE_GATE)
        self.assertEqual(r["target"]["samples"], 200_540_160)
        self.assertEqual(
            r["permitted_current_claim"], "STATIC_CAPABILITY_AND_HOST_PREPARATION_ONLY"
        )
        self.assertTrue(r["observations"]["vendor_metadata_tension"])
        self.assertTrue(r["observations"]["android_contract_regular_bayer_expected"])

    def test_closing_capture_gate_without_real_evidence_is_rejected(self):
        bad = copy.deepcopy(self.capability)
        bad["capture_gate"] = "PASS_REAL_200MP_CAPTURE"
        r = evaluate_contract(bad, self.host)
        self.assertFalse(r["pass"])
        self.assertEqual(r["classification"], BLOCKED)
        self.assertFalse(r["checks"]["runtime_capture_gate_still_open"])

    def test_missing_exact_raw_route_is_rejected(self):
        bad = copy.deepcopy(self.capability)
        bad["raw_routes"]["high_resolution"] = []
        r = evaluate_contract(bad, self.host)
        self.assertFalse(r["pass"])
        self.assertFalse(r["checks"]["advertised_raw_sensor_16320x12288"])

    def test_host_must_not_claim_device_execution_completed(self):
        bad_host = copy.deepcopy(self.host)
        bad_host["device_execution"] = "PASS"
        r = evaluate_contract(self.capability, bad_host)
        self.assertFalse(r["pass"])
        self.assertFalse(r["host_checks"]["host_device_execution_pending"])

    def test_metadata_binning_factor_does_not_upgrade_topology_claim(self):
        r = evaluate_contract(self.capability)
        self.assertTrue(r["pass"])
        self.assertEqual(r["observations"]["sensor_info_binning_factor"], [2, 2])
        self.assertTrue(r["observations"]["vendor_metadata_tension"])
        self.assertEqual(
            r["forbidden_without_separate_evidence"], "UNTOUCHED_NATIVE_200MP_ADC"
        )


if __name__ == "__main__":
    unittest.main()
