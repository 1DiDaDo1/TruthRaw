import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from open_world_foundations_v01 import ContractError  # noqa: E402
from open_scene_region_runtime_v07 import OpenSceneRegionResultV07  # noqa: E402
from open_scene_state_stream_v08 import (  # noqa: E402
    OpenSceneStateBindingV08,
    StreamingOpenSceneStateV08,
)

H1 = "1" * 64
H2 = "2" * 64
H3 = "3" * 64
H4 = "4" * 64
H5 = "5" * 64


def binding(width=4, height=1):
    return OpenSceneStateBindingV08("frame-1", width, height, H1, H2, H3)


def region(
    sha=H4,
    *,
    pass_contract=True,
    source=H1,
    master=H2,
    dynamic=H3,
    channel_authorities=("MEASURED", "RECONSTRUCTED", "CENSORED"),
    hdr=("EVIDENCE_SUPPORTED_FINITE", "RECONSTRUCTED_FINITE_NOT_MEASURED", "CENSORED_BOUND_ONLY"),
    colour="SOURCE_METADATA_BOUND",
    full_colour=False,
    illumination="SOURCE_BOUND_ESTIMATE",
    illumination_writeback=True,
    detail="APPEARANCE_DETAIL_ALLOWED",
    restoration_allowed=(True, True, True),
):
    return OpenSceneRegionResultV07(
        schema="TruthRawOpenSceneRegion/0.7",
        region_id="r",
        pass_contract=pass_contract,
        scene_state_sha256=sha,
        source_evidence_sha256=source,
        scientific_master_sha256=master,
        dynamic_authority_artifact_sha256=dynamic,
        channel_authorities=channel_authorities,
        hdr_channel_status=hdr,
        colour_authority=colour,
        full_physical_colour_claim_allowed=full_colour,
        illumination_authority=illumination,
        captured_world_illumination_writeback_allowed=illumination_writeback,
        detail_status=detail,
        detail_scientific_writeback_allowed=False,
        restoration_allowed=restoration_allowed,
        restoration_authority_out=("MEASURED", "RECONSTRUCTED", "CENSORED"),
        restoration_scientific_writeback_allowed=False,
        creates_new_evidence=False,
        physical_frame_count=1,
        independent_evidence_count=1,
        claim_boundary="fixture",
    )


class OpenSceneStateStreamV08Tests(unittest.TestCase):
    def test_chunk_boundaries_do_not_change_identity(self):
        r = region()
        a = StreamingOpenSceneStateV08(binding())
        a.append_span(0, 4, r)
        sa = a.finalize()

        b = StreamingOpenSceneStateV08(binding())
        b.append_span(0, 1, r)
        b.append_span(1, 1, r)
        b.append_span(2, 2, r)
        sb = b.finalize()

        self.assertEqual(sa.content_sha256, sb.content_sha256)
        self.assertEqual(sa.policy_sha256, sb.policy_sha256)
        self.assertEqual(sa.scientific_region_run_count, 1)
        self.assertEqual(sb.scientific_region_run_count, 1)

    def test_different_scientific_region_state_changes_identity(self):
        a = StreamingOpenSceneStateV08(binding())
        a.append_span(0, 4, region(H4))
        sa = a.finalize()

        b = StreamingOpenSceneStateV08(binding())
        b.append_span(0, 2, region(H4))
        b.append_span(2, 2, region(H5))
        sb = b.finalize()

        self.assertNotEqual(sa.content_sha256, sb.content_sha256)
        self.assertEqual(sb.scientific_region_run_count, 2)

    def test_region_identity_mismatch_fails_closed(self):
        w = StreamingOpenSceneStateV08(binding())
        with self.assertRaises(ContractError):
            w.append_span(0, 4, region(source="9" * 64))

    def test_blocked_region_cannot_enter_full_frame(self):
        w = StreamingOpenSceneStateV08(binding())
        with self.assertRaises(ContractError):
            w.append_span(0, 4, region(pass_contract=False))

    def test_incomplete_frame_fails(self):
        w = StreamingOpenSceneStateV08(binding())
        w.append_span(0, 3, region())
        with self.assertRaises(ContractError):
            w.finalize()

    def test_summary_counts_authority_without_writeback(self):
        r = region(
            channel_authorities=("MEASURED", "UNKNOWN", "RECONSTRUCTED"),
            hdr=("EVIDENCE_SUPPORTED_FINITE", "NO_SCIENTIFIC_HEADROOM", "RECONSTRUCTED_FINITE_NOT_MEASURED"),
            colour="INDEPENDENT_HELDOUT_VALIDATED",
            full_colour=True,
            illumination="COUNTERFACTUAL",
            illumination_writeback=False,
            detail="NEUTRAL_OR_BLOCKED",
        )
        w = StreamingOpenSceneStateV08(binding())
        w.append_span(0, 4, r)
        s = w.finalize()
        self.assertEqual(s.channel_authority_counts["MEASURED"], 4)
        self.assertEqual(s.channel_authority_counts["UNKNOWN"], 4)
        self.assertEqual(s.channel_authority_counts["RECONSTRUCTED"], 4)
        self.assertEqual(s.hdr_status_counts["NO_SCIENTIFIC_HEADROOM"], 4)
        self.assertEqual(s.full_physical_colour_claim_pixel_count, 4)
        self.assertEqual(s.counterfactual_illumination_pixel_count, 4)
        self.assertFalse(s.creates_new_evidence)
        self.assertFalse(s.scientific_master_writeback_allowed)
        self.assertEqual(s.physical_frame_count, 1)
        self.assertEqual(s.independent_evidence_count, 1)

    def test_restoration_blocked_pixels_are_visible_in_summary(self):
        r = region(restoration_allowed=(False, True, True))
        # pass_contract=False is the normal v0.7 outcome for a blocked request;
        # to test summary accounting specifically, construct a provenance state
        # that is still accepted by the full-frame writer only when contract is true.
        r = OpenSceneRegionResultV07(**{**r.__dict__, "pass_contract": True})
        w = StreamingOpenSceneStateV08(binding())
        w.append_span(0, 4, r)
        s = w.finalize()
        self.assertEqual(s.restoration_blocked_pixel_count, 4)


if __name__ == "__main__":
    unittest.main()
