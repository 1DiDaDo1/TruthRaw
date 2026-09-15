import io
import pathlib
import sys
import unittest
import zlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from open_world_foundations_v01 import ContractError, StructureEvidenceRecord  # noqa: E402
from open_world_dynamic_authority_v05 import CensoringKind, DynamicAuthority  # noqa: E402
from open_world_structure_runtime_v04 import (  # noqa: E402
    RuntimeStructureDiagnostics,
    RuntimeStructureResult,
    RuntimeStructureStatus,
    RuntimeStructureTile,
)
from open_world_dynamic_runtime_v06 import (  # noqa: E402
    CensorConstraint,
    CensorRelation,
    DynamicAuthorityArtifactWriterV06,
    RuntimeDynamicTileV06,
    build_dynamic_authority_rows_v06,
    parse_technical_backplane_v01,
)

H = "a" * 64
H2 = "b" * 64
H3 = "c" * 64
H4 = "d" * 64
H5 = "e" * 64
H6 = "f" * 64
H7 = "1" * 64


def make_backplane(source=H, master=H2, zero=H3, scene=H4, *, forbidden=0):
    b = bytearray(180)
    b[:8] = b"TRBACK01"
    b[8:10] = (1).to_bytes(2, "little")
    b[10:12] = (180).to_bytes(2, "little")
    b[12:16] = int(forbidden).to_bytes(4, "little")
    b[16:48] = bytes.fromhex(source)
    b[48:80] = bytes.fromhex(master)
    b[80:112] = bytes.fromhex(zero)
    b[112:144] = bytes.fromhex(scene)
    b[144:148] = (1).to_bytes(4, "little")
    b[148:152] = (1).to_bytes(4, "little")
    # 12 room-status bytes and claim status remain zero (Available/Open).
    crc = zlib.crc32(bytes(b[:176])) & 0xFFFFFFFF
    b[176:180] = crc.to_bytes(4, "little")
    return bytes(b)


def make_tile(*, censored=(False, False, False, True), source_hash=H, master_hash=H2, reinjection_ok=True):
    stage2 = (0.10, 0.20, 0.30, 0.40)
    # BGGR measured components: B, G, G, R.
    rgb = [
        0.11, 0.12, 0.10,
        0.21, 0.20, 0.22,
        0.31, 0.30, 0.32,
        0.40, 0.41, 0.42,
    ]
    if not reinjection_ok:
        rgb[2] = 0.1001
    return RuntimeStructureTile(
        region_id="canonical-authority-region-0",
        device_make="HONOR",
        device_model="BKQ-N49",
        physical_camera_id="5",
        focal_length_mm=22.48,
        source_width=2,
        source_height=2,
        cfa="BGGR",
        tile_x=0,
        tile_y=0,
        tile_width=2,
        tile_height=2,
        source_evidence_sha256=source_hash,
        scientific_master_sha256=master_hash,
        reconstruction_backend_name="ResearchEdgeAwareMeasuredPreservingReconstruction",
        production_backend_combined_sha256=H3,
        uncertainty_binding_sha256=H4,
        stage2_cfa=stage2,
        source_sigma_cfa=(0.005, 0.005, 0.005, 0.005),
        reconstructed_rgb=tuple(rgb),
        uncertainty_p95_rgb=(0.02,) * 12,
        censored_cfa=tuple(censored),
    )


def make_result(*, in_domain=True, reconstructed_support=0.75):
    if in_domain:
        record = StructureEvidenceRecord(
            region_id="canonical-authority-region-0",
            measured_support=0.5,
            reconstructed_support=reconstructed_support,
            mtf_support=0.0,
            censoring_risk=0.25,
            uncertainty=0.05,
            evidence_sha256=(H,),
        )
        status = RuntimeStructureStatus.BLOCKED_OPTICS_CALIBRATION_MISSING
    else:
        record = None
        status = RuntimeStructureStatus.BLOCKED_UNCERTAINTY_OUT_OF_DOMAIN
    diag = RuntimeStructureDiagnostics(
        exact_measured_reinjection=True,
        measured_pair_count=4,
        significant_measured_pair_count=2,
        measured_cfa_structure_support=0.5,
        reconstructed_topology_proxy_support=0.7 if in_domain else 0.0,
        uncertainty_confidence=0.9 if in_domain else 0.0,
        censoring_risk=0.25,
        local_measured_contrast=0.3,
        missing_channel_p95_median=0.02,
        topology_certified=False,
        uncertainty_domain_certified=in_domain,
        optics_calibrated=False,
    )
    return RuntimeStructureResult(
        status=status,
        record=record,
        detail_decision=None,
        diagnostics=diag,
        binding_sha256=H5,
        claim_boundary="test fixture",
    )


def make_runtime(*, in_domain=True, reinjection_ok=True, source_hash=H, master_hash=H2):
    tile = make_tile(source_hash=source_hash, master_hash=master_hash, reinjection_ok=reinjection_ok)
    constraints = (
        None,
        None,
        None,
        CensorConstraint(
            kind=CensoringKind.HIGHLIGHT_SATURATION,
            relation=CensorRelation.SCENE_VALUE_GE_BOUND,
            bound=0.40,
        ),
    )
    return RuntimeDynamicTileV06(
        structure_tile=tile,
        structure_result=make_result(in_domain=in_domain),
        measured_uncertainty_p95_cfa=(0.01, 0.01, 0.01, 0.01),
        censor_constraints_cfa=constraints,
    )


class OpenWorldDynamicRuntimeV06Tests(unittest.TestCase):
    def test_parses_native_technical_backplane_layout_and_rejects_mutation_flags(self):
        identity = parse_technical_backplane_v01(make_backplane())
        self.assertEqual(identity.source_evidence_sha256, H)
        self.assertEqual(identity.scientific_master_sha256, H2)
        self.assertEqual(identity.zero_line_sha256, H3)
        self.assertEqual(identity.scene_scale_sha256, H4)
        self.assertEqual(identity.physical_frame_count, 1)
        self.assertEqual(identity.independent_evidence_count, 1)
        self.assertEqual(len(identity.serialized_sha256), 64)

        with self.assertRaises(ContractError):
            parse_technical_backplane_v01(make_backplane(forbidden=1))

        corrupt = bytearray(make_backplane())
        corrupt[20] ^= 1
        with self.assertRaises(ContractError):
            parse_technical_backplane_v01(bytes(corrupt))

    def test_stage2_direct_cfa_defaults_to_calibrated_and_missing_channels_reconstructed(self):
        rows = build_dynamic_authority_rows_v06(make_runtime())
        self.assertEqual(len(rows), 2)
        # Pixel (0,0) is B in BGGR: R/G reconstructed, B direct Stage-2.
        p0 = rows[0][2][0]
        self.assertEqual(p0.channels[0].sample.authority, DynamicAuthority.RECONSTRUCTED)
        self.assertEqual(p0.channels[1].sample.authority, DynamicAuthority.RECONSTRUCTED)
        self.assertEqual(p0.channels[2].sample.authority, DynamicAuthority.CALIBRATED_ESTIMATE)
        self.assertEqual(p0.channels[2].sample.source_sample_index, 0)
        self.assertEqual(p0.channels[2].sample.scene_linear_estimate, 0.10)

    def test_censoring_preserves_inequality_and_missing_channels_become_unknown(self):
        rows = build_dynamic_authority_rows_v06(make_runtime())
        # Pixel (1,1) is R and is highlight-censored in the fixture.
        p = rows[1][2][1]
        self.assertEqual(p.channels[0].sample.authority, DynamicAuthority.CENSORED)
        self.assertEqual(p.channels[0].censor_relation, CensorRelation.SCENE_VALUE_GE_BOUND)
        self.assertEqual(p.channels[0].sample.censor_bound, 0.40)
        self.assertEqual(p.channels[1].sample.authority, DynamicAuthority.UNKNOWN)
        self.assertEqual(p.channels[2].sample.authority, DynamicAuthority.UNKNOWN)

    def test_out_of_domain_uncertainty_never_authorizes_missing_channels(self):
        rows = build_dynamic_authority_rows_v06(make_runtime(in_domain=False))
        p0 = rows[0][2][0]
        self.assertEqual(p0.channels[2].sample.authority, DynamicAuthority.CALIBRATED_ESTIMATE)
        self.assertEqual(p0.channels[0].sample.authority, DynamicAuthority.UNKNOWN)
        self.assertEqual(p0.channels[1].sample.authority, DynamicAuthority.UNKNOWN)

    def test_measured_reinjection_is_hard_gate_for_scientific_master_sidecar(self):
        with self.assertRaises(ContractError):
            build_dynamic_authority_rows_v06(make_runtime(reinjection_ok=False))

    def test_writer_binds_backplane_and_is_byte_exact_across_span_sizes(self):
        runtime = make_runtime()
        rows = build_dynamic_authority_rows_v06(runtime)

        sink_a = io.BytesIO()
        a = DynamicAuthorityArtifactWriterV06(
            make_backplane(), frame_id="frame-1", width=2, height=2, reference_l0=1.0, sink=sink_a
        )
        a.validate_runtime_tile_lineage(runtime)
        for y, x, pixels in rows:
            a.append_span(y, x, pixels)
        sa = a.finalize()

        sink_b = io.BytesIO()
        b = DynamicAuthorityArtifactWriterV06(
            make_backplane(), frame_id="frame-1", width=2, height=2, reference_l0=1.0, sink=sink_b
        )
        b.validate_runtime_tile_lineage(runtime)
        for y, x, pixels in rows:
            b.append_span(y, x, pixels[:1])
            b.append_span(y, x + 1, pixels[1:])
        sb = b.finalize()

        self.assertEqual(sink_a.getvalue(), sink_b.getvalue())
        self.assertEqual(sa.artifact_sha256, sb.artifact_sha256)
        self.assertEqual(sa.field_content_sha256, sb.field_content_sha256)
        self.assertEqual(sa.lineage_binding_sha256, sb.lineage_binding_sha256)
        self.assertEqual(sa.technical_backplane_sha256, sb.technical_backplane_sha256)
        self.assertIsNone(sa.fixed_dynamic_range_limit_ev)
        self.assertFalse(sa.scientific_master_writeback_from_sidecar)
        self.assertEqual(sa.pixel_count, 4)

    def test_writer_rejects_runtime_lineage_mismatch(self):
        runtime = make_runtime(source_hash=H7)
        writer = DynamicAuthorityArtifactWriterV06(
            make_backplane(), frame_id="frame-1", width=2, height=2, reference_l0=1.0, sink=io.BytesIO()
        )
        with self.assertRaises(ContractError):
            writer.validate_runtime_tile_lineage(runtime)

    def test_highlight_relation_cannot_be_reversed(self):
        with self.assertRaises(ContractError):
            CensorConstraint(
                kind=CensoringKind.HIGHLIGHT_SATURATION,
                relation=CensorRelation.SCENE_VALUE_LE_BOUND,
                bound=1.0,
            ).validate()


if __name__ == "__main__":
    unittest.main()
