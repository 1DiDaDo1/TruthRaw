import hashlib
import os
import struct
import tempfile
import unittest

from tools.open_world_foundations_v01 import ContractError
from tools.truthraw_hdr_legacy_linear_bridge_v17 import (
    CAMERA_TO_P3_D65_V17,
    FROZEN_LEGACY_EXPECTATION_V17,
    LEGACY_DEQUANTIZED_F32_SHA256_V17,
    LEGACY_DNG_SHA256_V17,
    LEGACY_PIXEL_PAYLOAD_SHA256_V17,
    LegacyBridgeManifestV17,
    LegacyLinearRawExpectationV17,
    RECORDED_MAX_QUANTIZATION_ABS_SCENE_ERROR_V17,
    SOURCE_BOUND_P3_TRANSFORM_SHA256_V17,
    THEORETICAL_QUANTIZATION_HALF_STEP_V17,
    build_frozen_bridge_manifest_v17,
    dequantize_triplet_v17,
    frozen_real_observation_v17,
    inspect_legacy_linearraw_v17,
    source_bound_p3_transform_payload_v17,
)


def _make_synthetic_linearraw(path, source_sha="a" * 64, add_master=False):
    width, height, spp = 2, 2, 3
    codes = (1800, 2048, 35000, 2100, 2200, 2300, 2400, 2500, 2600, 2700, 2800, 2900)
    payload = struct.pack("<" + "H" * len(codes), *codes)
    payload_sha = hashlib.sha256(payload).hexdigest()
    master_attr = ' tr:ScientificMasterSHA256="' + ('d' * 64) + '"' if add_master else ""
    xmp = (
        '<rdf:Description xmlns:tr="https://truthraw.example/ns/1.0/" '
        'tr:Classification="DERIVED_RECONSTRUCTED_RAW" '
        f'tr:SourceSHA256="{source_sha}" '
        f'tr:LinearRawPixelPayloadSHA256="{payload_sha}" '
        'tr:MeasuredPhotonClaim="false"' + master_attr + '/>'
    ).encode("utf-8")

    n = 13
    ifd_offset = 8
    data_start = ifd_offset + 2 + n * 12 + 4
    data = bytearray()

    def add_blob(blob):
        off = data_start + len(data)
        data.extend(blob)
        return off

    bits_off = add_blob(struct.pack("<HHH", 16, 16, 16))
    xmp_off = add_blob(xmp)
    black_off = add_blob(struct.pack("<IIIIII", 2048, 1, 2048, 1, 2048, 1))
    white_off = add_blob(struct.pack("<III", 34816, 34816, 34816))
    payload_off = add_blob(payload)

    def entry(tag, typ, count, value4):
        return struct.pack("<HHI", tag, typ, count) + value4

    entries = [
        entry(256, 4, 1, struct.pack("<I", width)),
        entry(257, 4, 1, struct.pack("<I", height)),
        entry(258, 3, 3, struct.pack("<I", bits_off)),
        entry(259, 3, 1, struct.pack("<H", 1) + b"\0\0"),
        entry(262, 3, 1, struct.pack("<H", 34892) + b"\0\0"),
        entry(273, 4, 1, struct.pack("<I", payload_off)),
        entry(274, 3, 1, struct.pack("<H", 3) + b"\0\0"),
        entry(277, 3, 1, struct.pack("<H", 3) + b"\0\0"),
        entry(278, 4, 1, struct.pack("<I", height)),
        entry(279, 4, 1, struct.pack("<I", len(payload))),
        entry(700, 1, len(xmp), struct.pack("<I", xmp_off)),
        entry(50714, 5, 3, struct.pack("<I", black_off)),
        entry(50717, 4, 3, struct.pack("<I", white_off)),
    ]
    body = b"II" + struct.pack("<H", 42) + struct.pack("<I", ifd_offset)
    body += struct.pack("<H", n) + b"".join(entries) + struct.pack("<I", 0) + bytes(data)
    with open(path, "wb") as fh:
        fh.write(body)
    file_sha = hashlib.sha256(body).hexdigest()
    return LegacyLinearRawExpectationV17(
        file_sha256=file_sha,
        pixel_payload_sha256=payload_sha,
        source_dng_sha256=source_sha,
        width=width,
        height=height,
        samples_per_pixel=spp,
        bits_per_sample=(16, 16, 16),
        photometric_interpretation=34892,
        compression=1,
        orientation=3,
        black_level=(2048.0, 2048.0, 2048.0),
        white_level=(34816, 34816, 34816),
    )


class TruthRawHdrLegacyLinearBridgeV17Tests(unittest.TestCase):
    def test_frozen_real_observation_keeps_modern_bindings_open(self):
        obs = frozen_real_observation_v17()
        self.assertEqual(obs["legacy_linearraw_sha256"], LEGACY_DNG_SHA256_V17)
        self.assertEqual(obs["pixel_payload_sha256"], LEGACY_PIXEL_PAYLOAD_SHA256_V17)
        self.assertEqual(obs["dequantized_float32_sha256"], LEGACY_DEQUANTIZED_F32_SHA256_V17)
        self.assertIsNone(obs["authority"]["scientific_master_sha256"])
        self.assertIsNone(obs["authority"]["dynamic_authority_sha256"])
        self.assertFalse(obs["authority"]["v16_science_binding_complete"])

    def test_source_bound_p3_transform_is_hash_bound_and_not_physical(self):
        payload = source_bound_p3_transform_payload_v17()
        self.assertEqual(payload["matrix_rgb_to_p3_d65"], CAMERA_TO_P3_D65_V17)
        self.assertEqual(payload["authority"], "SOURCE_METADATA_BOUND_L1_NOT_PHYSICAL_CALIBRATION")
        self.assertEqual(SOURCE_BOUND_P3_TRANSFORM_SHA256_V17, "2105712a1be9950089976ba358afd4b062347680d5e6d8c500462c2e8d541533")

    def test_dequantization_retains_signed_and_over_unity_values(self):
        rgb = dequantize_triplet_v17((1840, 2048, 44573))
        self.assertLess(rgb[0], 0.0)
        self.assertEqual(rgb[1], 0.0)
        self.assertGreater(rgb[2], 1.0)

    def test_parser_verifies_uncompressed_linearraw_and_xmp_lineage(self):
        with tempfile.TemporaryDirectory() as d:
            p = os.path.join(d, "fixture.dng")
            expected = _make_synthetic_linearraw(p)
            obs = inspect_legacy_linearraw_v17(p, expected)
            self.assertEqual(obs.pixel_payload_bytes, 24)
            self.assertEqual(obs.strip_count, 1)
            self.assertEqual(obs.xmp_source_sha256, "a" * 64)
            self.assertFalse(obs.may_be_relabelled_scientific_master)
            self.assertFalse(obs.may_complete_v16_science_binding)

    def test_parser_rejects_payload_hash_mismatch(self):
        with tempfile.TemporaryDirectory() as d:
            p = os.path.join(d, "fixture.dng")
            expected = _make_synthetic_linearraw(p)
            bad = LegacyLinearRawExpectationV17(**{**expected.__dict__, "pixel_payload_sha256": "b" * 64})
            with self.assertRaises(ContractError):
                inspect_legacy_linearraw_v17(p, bad)

    def test_parser_rejects_legacy_file_that_claims_modern_master_identity(self):
        with tempfile.TemporaryDirectory() as d:
            p = os.path.join(d, "fixture.dng")
            expected = _make_synthetic_linearraw(p, add_master=True)
            with self.assertRaises(ContractError):
                inspect_legacy_linearraw_v17(p, expected)

    def test_manifest_cannot_promote_compatibility_payload(self):
        fake_inspection = type("I", (), {
            "file_sha256": LEGACY_DNG_SHA256_V17,
            "pixel_payload_sha256": LEGACY_PIXEL_PAYLOAD_SHA256_V17,
            "xmp_source_sha256": FROZEN_LEGACY_EXPECTATION_V17.source_dng_sha256,
        })()
        m = LegacyBridgeManifestV17(
            inspection=fake_inspection,
            source_bound_color_transform_sha256=SOURCE_BOUND_P3_TRANSFORM_SHA256_V17,
            theoretical_quantization_half_step=THEORETICAL_QUANTIZATION_HALF_STEP_V17,
            recorded_max_quantization_abs_scene_error=RECORDED_MAX_QUANTIZATION_ABS_SCENE_ERROR_V17,
            scientific_master_sha256="c" * 64,
        )
        with self.assertRaises(ContractError):
            m.validate()

    def test_build_frozen_manifest_has_deterministic_non_authoritative_identity(self):
        inspection = type("Inspection", (), {
            "file_sha256": LEGACY_DNG_SHA256_V17,
            "pixel_payload_sha256": LEGACY_PIXEL_PAYLOAD_SHA256_V17,
            "xmp_source_sha256": FROZEN_LEGACY_EXPECTATION_V17.source_dng_sha256,
        })()
        m = build_frozen_bridge_manifest_v17(inspection)
        self.assertFalse(m.binding_complete_for_v16)
        self.assertEqual(m.manifest_sha256, "3376c42215da5fa0d4dfc4682027a59c74b4ba5f770a7adb9ceb5c3ec05ff152")


if __name__ == "__main__":
    unittest.main()
