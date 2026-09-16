import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from open_world_foundations_v01 import ContractError  # noqa: E402
from open_world_dynamic_authority_v05 import (  # noqa: E402
    CensoringKind,
    DynamicAuthority,
    DynamicAuthoritySample,
)
from open_world_structure_runtime_v04 import (  # noqa: E402
    RuntimeStructureDiagnostics,
    RuntimeStructureResult,
    RuntimeStructureStatus,
)
from open_scene_region_runtime_v07 import (  # noqa: E402
    ColourCalibrationAuthority,
    ColourCalibrationBindingV07,
    HDRChannelStatus,
    IlluminationAuthority,
    IlluminationBindingV07,
    RestorationRequestV07,
    SceneRegionBindingsV07,
    compose_open_scene_region_v07,
)

H1 = "1" * 64
H2 = "2" * 64
H3 = "3" * 64
H4 = "4" * 64
H5 = "5" * 64


def structure(status=RuntimeStructureStatus.ADMITTED_APPEARANCE_ONLY, *, optics=True):
    d = RuntimeStructureDiagnostics(
        exact_measured_reinjection=True,
        measured_pair_count=8,
        significant_measured_pair_count=4,
        measured_cfa_structure_support=0.5,
        reconstructed_topology_proxy_support=0.4,
        uncertainty_confidence=0.8,
        censoring_risk=0.0,
        local_measured_contrast=0.2,
        missing_channel_p95_median=0.01,
        topology_certified=False,
        uncertainty_domain_certified=True,
        optics_calibrated=optics,
    )
    return RuntimeStructureResult(status, None, None, d, H4, "fixture boundary")


def bindings():
    return SceneRegionBindingsV07("r0", H1, H2, H3, 4080, 3072)


def measured(i, value=0.25):
    return DynamicAuthoritySample(
        DynamicAuthority.MEASURED,
        value,
        uncertainty_p95=0.01,
        support=1.0,
        source_sample_index=i,
    )


def reconstructed(value=0.2):
    return DynamicAuthoritySample(
        DynamicAuthority.RECONSTRUCTED,
        value,
        uncertainty_p95=0.03,
        support=0.7,
    )


def unknown():
    return DynamicAuthoritySample(DynamicAuthority.UNKNOWN, None)


class OpenSceneRegionRuntimeV07Tests(unittest.TestCase):
    def test_source_metadata_colour_does_not_become_full_physical(self):
        r = compose_open_scene_region_v07(
            bindings=bindings(),
            channels=(measured(0), measured(1), reconstructed()),
            structure=structure(),
            colour=ColourCalibrationBindingV07(
                ColourCalibrationAuthority.SOURCE_METADATA_BOUND,
                transform_sha256=H5,
            ),
            illumination=IlluminationBindingV07(IlluminationAuthority.SOURCE_BOUND_ESTIMATE),
        )
        self.assertTrue(r.pass_contract)
        self.assertFalse(r.full_physical_colour_claim_allowed)
        self.assertEqual(r.detail_status, "APPEARANCE_DETAIL_ALLOWED")
        self.assertFalse(r.detail_scientific_writeback_allowed)
        self.assertFalse(r.creates_new_evidence)
        self.assertEqual(r.physical_frame_count, 1)
        self.assertEqual(r.independent_evidence_count, 1)

    def test_independent_colour_needs_calibration_and_holdout_bindings(self):
        with self.assertRaises(ContractError):
            ColourCalibrationBindingV07(
                ColourCalibrationAuthority.INDEPENDENT_HELDOUT_VALIDATED,
                transform_sha256=H3,
            ).validate()
        good = ColourCalibrationBindingV07(
            ColourCalibrationAuthority.INDEPENDENT_HELDOUT_VALIDATED,
            transform_sha256=H3,
            calibration_evidence_sha256=H4,
            holdout_report_sha256=H5,
        ).validate()
        self.assertTrue(good.full_physical_colour_claim_allowed)

    def test_censored_hdr_is_bound_only_and_unknown_has_no_headroom(self):
        censored = DynamicAuthoritySample(
            DynamicAuthority.CENSORED,
            None,
            support=0.0,
            source_sample_index=12,
            censoring_kind=CensoringKind.HIGHLIGHT_SATURATION,
            censor_bound=1.0,
        )
        r = compose_open_scene_region_v07(
            bindings=bindings(),
            channels=(censored, unknown(), reconstructed()),
            structure=structure(RuntimeStructureStatus.NEUTRAL_WEAK_STRUCTURE, optics=False),
            colour=ColourCalibrationBindingV07(ColourCalibrationAuthority.UNKNOWN),
            illumination=IlluminationBindingV07(IlluminationAuthority.UNKNOWN),
        )
        self.assertEqual(r.hdr_channel_status[0], HDRChannelStatus.CENSORED_BOUND_ONLY.value)
        self.assertEqual(r.hdr_channel_status[1], HDRChannelStatus.NO_SCIENTIFIC_HEADROOM.value)
        self.assertEqual(r.hdr_channel_status[2], HDRChannelStatus.RECONSTRUCTED_FINITE_NOT_MEASURED.value)

    def test_counterfactual_illumination_never_writes_back(self):
        r = compose_open_scene_region_v07(
            bindings=bindings(),
            channels=(measured(0), measured(1), measured(2)),
            structure=structure(),
            colour=ColourCalibrationBindingV07(ColourCalibrationAuthority.UNKNOWN),
            illumination=IlluminationBindingV07(IlluminationAuthority.COUNTERFACTUAL),
        )
        self.assertTrue(r.pass_contract)
        self.assertFalse(r.captured_world_illumination_writeback_allowed)
        self.assertEqual(r.illumination_authority, "COUNTERFACTUAL")

    def test_restoration_cannot_overpaint_valid_measured_support(self):
        r = compose_open_scene_region_v07(
            bindings=bindings(),
            channels=(measured(0), measured(1), measured(2)),
            structure=structure(),
            colour=ColourCalibrationBindingV07(ColourCalibrationAuthority.UNKNOWN),
            illumination=IlluminationBindingV07(IlluminationAuthority.UNKNOWN),
            restoration=RestorationRequestV07(
                requested_role="LOSS_COMPENSATION_RECONSTRUCTED",
                support_present=True,
                provenance_bound=True,
                retreatable=True,
            ),
        )
        self.assertFalse(r.pass_contract)
        self.assertEqual(r.restoration_allowed, (False, False, False))
        self.assertEqual(r.restoration_authority_out, ("MEASURED", "MEASURED", "MEASURED"))

    def test_supported_unknown_loss_may_be_reconstructed_without_new_evidence(self):
        r = compose_open_scene_region_v07(
            bindings=bindings(),
            channels=(unknown(), unknown(), unknown()),
            structure=structure(RuntimeStructureStatus.NEUTRAL_WEAK_STRUCTURE, optics=False),
            colour=ColourCalibrationBindingV07(ColourCalibrationAuthority.UNKNOWN),
            illumination=IlluminationBindingV07(IlluminationAuthority.UNKNOWN),
            restoration=RestorationRequestV07(
                requested_role="LOSS_COMPENSATION_RECONSTRUCTED",
                support_present=True,
                provenance_bound=True,
                retreatable=True,
            ),
        )
        self.assertTrue(r.pass_contract)
        self.assertEqual(r.restoration_authority_out, ("RECONSTRUCTED",) * 3)
        self.assertFalse(r.restoration_scientific_writeback_allowed)
        self.assertFalse(r.creates_new_evidence)

    def test_counterfactual_or_appearance_samples_cannot_enter_captured_scene(self):
        bad = DynamicAuthoritySample(
            DynamicAuthority.COUNTERFACTUAL,
            0.3,
            parent_record_id="cf-1",
        )
        with self.assertRaises(ContractError):
            compose_open_scene_region_v07(
                bindings=bindings(),
                channels=(measured(0), measured(1), bad),
                structure=structure(),
                colour=ColourCalibrationBindingV07(ColourCalibrationAuthority.UNKNOWN),
                illumination=IlluminationBindingV07(IlluminationAuthority.UNKNOWN),
            )

    def test_structure_out_of_domain_stays_out_of_domain(self):
        r = compose_open_scene_region_v07(
            bindings=bindings(),
            channels=(measured(0), reconstructed(), reconstructed()),
            structure=structure(RuntimeStructureStatus.BLOCKED_UNCERTAINTY_OUT_OF_DOMAIN, optics=False),
            colour=ColourCalibrationBindingV07(ColourCalibrationAuthority.UNKNOWN),
            illumination=IlluminationBindingV07(IlluminationAuthority.UNKNOWN),
        )
        self.assertEqual(r.detail_status, "OUT_OF_DOMAIN")
        self.assertFalse(r.detail_scientific_writeback_allowed)


if __name__ == "__main__":
    unittest.main()
