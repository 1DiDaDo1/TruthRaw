import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from open_world_foundations_v01 import (  # noqa: E402
    CalibrationDomain,
    CalibrationKind,
    CalibrationRecord,
    ContractError,
    HoldoutStatus,
    RestorationClass,
    RestorationRecord,
    StructureSupportClass,
)
from open_world_integration_v02 import CalibrationRegistry  # noqa: E402
from open_world_integration_v03 import (  # noqa: E402
    NativeTeleRoute,
    RestorationGraph,
    StructureMeasurementInputs,
    bind_registry_to_native_tele,
    derive_structure_evidence,
)
from camera5_200mp_evidence_bundle_v03 import (  # noqa: E402
    TARGET_SAMPLES,
    V03_PASS,
    evaluate_bundle_v03,
)

H = "a" * 64
H2 = "b" * 64
H3 = "c" * 64
H4 = "d" * 64
H5 = "e" * 64


def tele_calibration(calibration_id="tele-mtf-1", holdout=HoldoutStatus.PASS, camera_id="5"):
    return CalibrationRecord(
        calibration_id=calibration_id,
        kind=CalibrationKind.OPTICS_PSF_MTF,
        domain=CalibrationDomain(
            device="HONOR BKQ-N49",
            physical_camera_id=camera_id,
            sensor_pixel_mode="MAXIMUM_RESOLUTION",
            width=16320,
            height=12288,
            cfa="BGGR",
            iso_min=50,
            iso_max=1600,
        ),
        source_evidence_sha256=(H,),
        method="measured slanted-edge/point-source optics campaign",
        uncertainty_description="frequency/support uncertainty retained",
        holdout_status=holdout,
        holdout_report_sha256=H2 if holdout is HoldoutStatus.PASS else None,
    )


def restoration(layer_id, cls, *, confidence=None, hypothesis=None):
    return RestorationRecord(
        layer_id=layer_id,
        restoration_class=cls,
        source_evidence_sha256=(H,),
        mask_sha256=H2,
        method="documented reversible restoration",
        confidence=confidence,
        hypothesis_description=hypothesis,
    )


def proof_fixture():
    manifest = {
        "evidence_class": "DEVICE_RUNTIME_CAPTURE",
        "physical_camera_id": "5",
        "source_identity_sha256": H,
        "camera_characteristics": {"cfa_arrangement": 3},
        "requested": {"sensor_pixel_mode": "MAXIMUM_RESOLUTION"},
        "capture_result": {
            "result_camera_id": "5",
            "sensor_pixel_mode": "MAXIMUM_RESOLUTION",
            "sensor_raw_binning_factor_used": False,
        },
        "raw_output": {
            "format": "RAW_SENSOR",
            "width": 16320,
            "height": 12288,
            "payload_sha256": H,
        },
        "dng_output": {"sha256": H3},
    }
    runtime_gate = {
        "pass": True,
        "classification": "FULL_SENSOR_200MP_APP_VISIBLE_RAW_CAPTURE_PROVEN",
        "observed": {"raw_dimensions": [16320, 12288], "manifest_payload_sha256": H},
    }
    canonical = {
        "classification": "FULL_SENSOR_200MP_CANONICAL_APP_VISIBLE_CFA_MIRROR",
        "width": 16320,
        "height": 12288,
        "samples": TARGET_SAMPLES,
        "source_buffer_sha256": H,
        "canonical_sha256": H2,
    }
    identity = {
        "pass": True,
        "classification": "CAMERA5_200MP_RAW_DNG_CFA_IDENTITY_PROVEN",
        "observed": {
            "manifest_dimensions": [16320, 12288],
            "canonical_raw_sha256": H2,
            "dng_sha256": H3,
            "dng_raw_ifd": {"cfa_pattern_name": "BGGR"},
            "identity": {
                "canonical_raw_sha256_stream": H2,
                "dng_cfa_canonical_sha256": H2,
                "sample_identity": True,
                "samples_compared": TARGET_SAMPLES,
                "first_mismatch_sample": None,
            },
        },
    }
    return manifest, runtime_gate, canonical, identity


class OpenWorldIntegrationV03Tests(unittest.TestCase):
    def test_structure_evidence_is_lower_envelope_of_real_support_inputs(self):
        inputs = StructureMeasurementInputs(
            region_id="tele-tile-14-edge-2",
            measured_cfa_support=0.90,
            reconstructed_topology_support=0.70,
            optical_mtf_support=0.60,
            uncertainty_confidence=0.80,
            censoring_risk=0.10,
            evidence_sha256=(H,),
            spatial_frequency_cyc_per_px=0.25,
        )
        out = derive_structure_evidence(inputs)
        self.assertAlmostEqual(out.measured_support, 0.60)
        self.assertAlmostEqual(out.reconstructed_support, 0.60)
        self.assertAlmostEqual(out.uncertainty, 0.20)
        self.assertEqual(out.support_class(), StructureSupportClass.MEASURED_SUPPORTED)

    def test_censoring_can_only_remove_structure_authority(self):
        inputs = StructureMeasurementInputs(
            region_id="clipped-highlight",
            measured_cfa_support=1.0,
            reconstructed_topology_support=1.0,
            optical_mtf_support=1.0,
            uncertainty_confidence=1.0,
            censoring_risk=1.0,
            evidence_sha256=(H,),
        )
        out = derive_structure_evidence(inputs)
        self.assertEqual(out.measured_support, 0.0)
        self.assertEqual(out.reconstructed_support, 0.0)
        self.assertEqual(out.support_class(), StructureSupportClass.CENSORED_OR_WEAK)

    def test_native_tele_registry_binding_requires_exact_route_and_holdout(self):
        registry = CalibrationRegistry((tele_calibration(),))
        binding = bind_registry_to_native_tele(registry)
        self.assertEqual(binding.route, NativeTeleRoute())
        self.assertEqual(len(binding.registry_fingerprint), 64)
        self.assertEqual(len(binding.binding_sha256), 64)
        self.assertEqual(binding.calibration_ids, ("tele-mtf-1",))

        wrong_route = CalibrationRegistry((tele_calibration("wrong-camera", camera_id="2"),))
        with self.assertRaises(ContractError):
            bind_registry_to_native_tele(wrong_route)

        uncertified = CalibrationRegistry((tele_calibration("unverified", holdout=HoldoutStatus.UNTESTED),))
        with self.assertRaises(ContractError):
            bind_registry_to_native_tele(uncertified)

    def test_restoration_graph_allows_clean_sibling_branch_not_authority_upgrade(self):
        graph = RestorationGraph(H)
        clean = graph.append(
            restoration("scratch-evidence", RestorationClass.EVIDENCE_SUPPORTED_RECONSTRUCTION, confidence=0.9),
            H3,
        )
        visual = graph.append(
            restoration(
                "possible-original-colour",
                RestorationClass.HYPOTHETICAL_VISUAL_RESTORATION,
                hypothesis="documented pre-fade colour hypothesis",
            ),
            H4,
            parent_layer_sha256=clean.layer_sha256,
        )

        # A clean sibling is allowed because its ancestry returns explicitly to
        # the immutable source rather than descending through the visual branch.
        sibling = graph.append(
            restoration("tear-repair", RestorationClass.EVIDENCE_SUPPORTED_RECONSTRUCTION, confidence=0.8),
            H5,
            parent_layer_sha256=None,
        )
        self.assertIsNone(sibling.parent_layer_sha256)
        self.assertTrue(graph.verify())
        self.assertEqual(graph.manifest()["node_count"], 3)

        with self.assertRaises(ContractError):
            graph.append(
                restoration("illegal-upgrade", RestorationClass.EVIDENCE_SUPPORTED_RECONSTRUCTION, confidence=0.7),
                H5,
                parent_layer_sha256=visual.layer_sha256,
            )

    def test_200mp_v03_binds_camera2_and_dng_cfa_topology(self):
        manifest, runtime_gate, canonical, identity = proof_fixture()
        result = evaluate_bundle_v03(manifest, runtime_gate, canonical, identity)
        self.assertTrue(result["pass"])
        self.assertEqual(result["classification"], V03_PASS)
        self.assertEqual(result["bindings"]["camera2_cfa_pattern"], "BGGR")
        self.assertEqual(result["bindings"]["dng_cfa_pattern"], "BGGR")
        self.assertEqual(result["physicalFrameCount"], 1)
        self.assertEqual(result["independentEvidenceCount"], 1)

        identity["observed"]["dng_raw_ifd"]["cfa_pattern_name"] = "RGGB"
        mismatch = evaluate_bundle_v03(manifest, runtime_gate, canonical, identity)
        self.assertFalse(mismatch["pass"])

    def test_200mp_v03_rejects_explicit_binning_true(self):
        manifest, runtime_gate, canonical, identity = proof_fixture()
        manifest["capture_result"]["sensor_raw_binning_factor_used"] = True
        result = evaluate_bundle_v03(manifest, runtime_gate, canonical, identity)
        self.assertFalse(result["pass"])
        self.assertFalse(result["checks"]["no_explicit_raw_binning_true"])


if __name__ == "__main__":
    unittest.main()
