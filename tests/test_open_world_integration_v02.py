import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from open_world_foundations_v01 import (  # noqa: E402
    Applicability,
    CalibrationDomain,
    CalibrationKind,
    CalibrationRecord,
    CaptureConditions,
    ContractError,
    HoldoutStatus,
    IlluminationAuthority,
    IlluminationRecord,
    RestorationClass,
    RestorationRecord,
    StructureEvidenceRecord,
)
from open_world_integration_v02 import (  # noqa: E402
    CalibrationRegistry,
    CalibrationResolutionStatus,
    DetailExecutionMode,
    IlluminationConsumer,
    OutputAuthority,
    RestorationStack,
    canonical_detail_profile,
    canonical_output_acutance_profile,
    dispatch_illumination,
    gate_detail_and_acutance,
)
from camera5_200mp_evidence_bundle_v02 import (  # noqa: E402
    BUNDLE_PASS,
    evaluate_bundle,
)

H = "a" * 64
H2 = "b" * 64
H3 = "c" * 64
H4 = "d" * 64


def calibration(cal_id="tele-color-1", iso_min=50, iso_max=800, holdout=HoldoutStatus.PASS):
    return CalibrationRecord(
        calibration_id=cal_id,
        kind=CalibrationKind.COLOR,
        domain=CalibrationDomain(
            device="HONOR BKQ-N49",
            physical_camera_id="5",
            sensor_pixel_mode="MAXIMUM_RESOLUTION",
            width=16320,
            height=12288,
            cfa="BGGR",
            iso_min=iso_min,
            iso_max=iso_max,
        ),
        source_evidence_sha256=(H,),
        method="spectrally measured target",
        uncertainty_description="matrix and residual uncertainty",
        holdout_status=holdout,
        holdout_report_sha256=H2 if holdout is HoldoutStatus.PASS else None,
    )


def capture(iso=100):
    return CaptureConditions(
        device="HONOR BKQ-N49",
        physical_camera_id="5",
        sensor_pixel_mode="MAXIMUM_RESOLUTION",
        width=16320,
        height=12288,
        cfa="BGGR",
        iso=iso,
    )


class OpenWorldIntegrationTests(unittest.TestCase):
    def test_counterfactual_light_cannot_enter_scientific_scene(self):
        light = IlluminationRecord(
            "virtual-sun",
            IlluminationAuthority.COUNTERFACTUAL,
            "open landscape + sky dome",
            counterfactual_parent_id="scene-master-1",
        )
        d = dispatch_illumination(light, IlluminationConsumer.SCIENTIFIC_SCENE)
        self.assertFalse(d.allowed)
        self.assertFalse(d.may_modify_scientific_master)
        self.assertEqual(d.output_authority, OutputAuthority.COUNTERFACTUAL)

    def test_same_open_world_light_is_allowed_for_cicm_but_stays_counterfactual(self):
        light = IlluminationRecord(
            "virtual-sun",
            IlluminationAuthority.COUNTERFACTUAL,
            "street -> horizon -> sky dome",
            counterfactual_parent_id="scene-master-1",
        )
        d = dispatch_illumination(light, IlluminationConsumer.CICM_RELATIVE_WORLD)
        self.assertTrue(d.allowed)
        self.assertEqual(d.output_authority, OutputAuthority.COUNTERFACTUAL)
        self.assertFalse(d.may_modify_scientific_master)

    def test_calibrated_cicm_forward_requires_applicable_calibration(self):
        light = IlluminationRecord(
            "measured-lamp",
            IlluminationAuthority.MEASURED,
            "calibration target plane",
            evidence_sha256=(H3,),
        )
        denied = dispatch_illumination(
            light,
            IlluminationConsumer.CICM_CALIBRATED_FORWARD,
            calibration_applicability=Applicability.UNCERTIFIED,
        )
        allowed = dispatch_illumination(
            light,
            IlluminationConsumer.CICM_CALIBRATED_FORWARD,
            calibration_applicability=Applicability.APPLICABLE,
        )
        self.assertFalse(denied.allowed)
        self.assertTrue(allowed.allowed)
        self.assertEqual(allowed.output_authority, OutputAuthority.COUNTERFACTUAL)

    def test_calibration_registry_refuses_overlap_without_explicit_choice(self):
        r1 = calibration("tele-color-a")
        r2 = calibration("tele-color-b")
        registry = CalibrationRegistry((r1, r2))
        ambiguous = registry.resolve(CalibrationKind.COLOR, capture())
        self.assertEqual(ambiguous.status, CalibrationResolutionStatus.AMBIGUOUS)
        selected = registry.resolve(
            CalibrationKind.COLOR,
            capture(),
            preferred_calibration_id="tele-color-b",
        )
        self.assertEqual(selected.status, CalibrationResolutionStatus.RESOLVED)
        self.assertEqual(selected.record.calibration_id, "tele-color-b")
        self.assertEqual(len(registry.fingerprint()), 64)

    def test_calibration_registry_fails_closed_uncertified(self):
        registry = CalibrationRegistry((calibration("tele-color-unverified", holdout=HoldoutStatus.UNTESTED),))
        result = registry.resolve(CalibrationKind.COLOR, capture())
        self.assertEqual(result.status, CalibrationResolutionStatus.UNCERTIFIED)

    def test_structure_bridge_maps_supported_detail_to_appearance_only(self):
        s = StructureEvidenceRecord(
            region_id="tile-01",
            measured_support=0.8,
            reconstructed_support=0.1,
            mtf_support=0.75,
            censoring_risk=0.0,
            uncertainty=0.01,
            evidence_sha256=(H,),
        )
        d = gate_detail_and_acutance(s)
        self.assertEqual(d.detail_v47j_mode, DetailExecutionMode.ADAPTIVE_APPEARANCE)
        self.assertEqual(canonical_detail_profile(d), "AdaptiveDetailedCrisp")
        self.assertEqual(canonical_output_acutance_profile(d), "AdaptiveDetail")
        self.assertFalse(d.scientific_master_write_allowed)
        self.assertFalse(d.measured_detail_claim_allowed_from_output)

    def test_weak_structure_forces_neutral_detail_path(self):
        s = StructureEvidenceRecord(
            region_id="tile-dark-censored",
            measured_support=0.0,
            reconstructed_support=0.0,
            mtf_support=0.0,
            censoring_risk=1.0,
            uncertainty=1.0,
            evidence_sha256=(H,),
        )
        d = gate_detail_and_acutance(s)
        self.assertEqual(d.detail_v47j_mode, DetailExecutionMode.NEUTRAL_ONLY)
        self.assertEqual(canonical_detail_profile(d), "NeutralReference")
        self.assertEqual(canonical_output_acutance_profile(d), "Neutral")

    def test_restoration_stack_is_hash_chained_and_cannot_upgrade_after_visual_hypothesis(self):
        stack = RestorationStack(H)
        repair = RestorationRecord(
            layer_id="scratch-repair",
            restoration_class=RestorationClass.EVIDENCE_SUPPORTED_RECONSTRUCTION,
            source_evidence_sha256=(H,),
            mask_sha256=H2,
            method="local continuity reconstruction",
            confidence=0.88,
        )
        a = stack.append(repair, H3)
        visual = RestorationRecord(
            layer_id="historic-colour-hypothesis",
            restoration_class=RestorationClass.HYPOTHETICAL_VISUAL_RESTORATION,
            source_evidence_sha256=(H,),
            mask_sha256=H2,
            method="counterfactual recolouring",
            hypothesis_description="possible pre-fade appearance",
        )
        b = stack.append(visual, H4)
        self.assertEqual(b.parent_layer_sha256, a.layer_sha256)
        self.assertTrue(stack.verify())
        self.assertEqual(stack.manifest()["head_layer_sha256"], b.layer_sha256)

        illegal_upgrade = RestorationRecord(
            layer_id="pretend-evidence-after-hypothesis",
            restoration_class=RestorationClass.EVIDENCE_SUPPORTED_RECONSTRUCTION,
            source_evidence_sha256=(H,),
            mask_sha256=H2,
            method="invalid authority upgrade",
            confidence=0.9,
        )
        with self.assertRaises(ContractError):
            stack.append(illegal_upgrade, H3)

    def test_200mp_bundle_passes_only_when_whole_chain_matches(self):
        samples = 16320 * 12288
        manifest = {
            "evidence_class": "DEVICE_RUNTIME_CAPTURE",
            "physical_camera_id": "5",
            "source_identity_sha256": H,
            "requested": {"sensor_pixel_mode": "MAXIMUM_RESOLUTION"},
            "capture_result": {"sensor_pixel_mode": "MAXIMUM_RESOLUTION"},
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
                "identity": {
                    "canonical_raw_sha256_stream": H2,
                    "dng_cfa_canonical_sha256": H2,
                    "sample_identity": True,
                    "samples_compared": samples,
                    "first_mismatch_sample": None,
                },
            },
        }
        result = evaluate_bundle(manifest, runtime_gate, canonical, identity)
        self.assertTrue(result["pass"])
        self.assertEqual(result["classification"], BUNDLE_PASS)
        self.assertEqual(result["physicalFrameCount"], 1)
        self.assertEqual(result["independentEvidenceCount"], 1)

        identity["observed"]["dng_sha256"] = H4
        bad = evaluate_bundle(manifest, runtime_gate, canonical, identity)
        self.assertFalse(bad["pass"])


if __name__ == "__main__":
    unittest.main()
