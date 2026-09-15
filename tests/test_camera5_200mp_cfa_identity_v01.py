import pathlib
import struct
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from camera5_200mp_cfa_identity_v01 import evaluate_identity, sha256_file  # noqa: E402


def make_tiff_cfa(path: pathlib.Path, width: int, height: int, pixels_le: bytes) -> None:
    entries = []

    def entry(tag, typ, count, value4):
        entries.append(struct.pack("<HHI", tag, typ, count) + value4)

    def short_inline(v):
        return struct.pack("<H", v) + b"\x00\x00"

    def long_inline(v):
        return struct.pack("<I", v)

    ifd_offset = 8
    ifd_size = 2 + 11 * 12 + 4
    pixel_offset = ifd_offset + ifd_size

    entry(256, 4, 1, long_inline(width))
    entry(257, 4, 1, long_inline(height))
    entry(258, 3, 1, short_inline(16))
    entry(259, 3, 1, short_inline(1))
    entry(262, 3, 1, short_inline(32803))
    entry(273, 4, 1, long_inline(pixel_offset))
    entry(277, 3, 1, short_inline(1))
    entry(278, 4, 1, long_inline(height))
    entry(279, 4, 1, long_inline(len(pixels_le)))
    entry(33421, 3, 2, struct.pack("<HH", 2, 2))
    entry(33422, 1, 4, bytes([2, 1, 1, 0]))

    with path.open("wb") as f:
        f.write(b"II")
        f.write(struct.pack("<H", 42))
        f.write(struct.pack("<I", ifd_offset))
        f.write(struct.pack("<H", len(entries)))
        for e in entries:
            f.write(e)
        f.write(struct.pack("<I", 0))
        f.write(pixels_le)


class CfaIdentityTests(unittest.TestCase):
    def build_fixture(self, td: pathlib.Path, *, corrupt_dng_sample=False):
        width, height = 4, 4
        vals = list(range(width * height))
        raw_bytes = b"".join(struct.pack("<H", v) for v in vals)
        raw = td / "canonical.rawsensor"
        raw.write_bytes(raw_bytes)

        dng_bytes = bytearray(raw_bytes)
        if corrupt_dng_sample:
            dng_bytes[6:8] = struct.pack("<H", 999)
        dng = td / "capture.dng"
        make_tiff_cfa(dng, width, height, bytes(dng_bytes))

        source_hash = "1" * 64
        manifest = {
            "evidence_class": "DEVICE_RUNTIME_CAPTURE",
            "physical_camera_id": "5",
            "source_identity_sha256": source_hash,
            "raw_output": {
                "format": "RAW_SENSOR",
                "width": width,
                "height": height,
                "payload_sha256": source_hash,
            },
            "dng_output": {"sha256": sha256_file(dng)},
        }
        report = {
            "width": width,
            "height": height,
            "source_buffer_sha256": source_hash,
            "canonical_sha256": sha256_file(raw),
        }
        return manifest, report, raw, dng

    def test_exact_identity_passes_in_non_target_test_domain(self):
        with tempfile.TemporaryDirectory() as d:
            td = pathlib.Path(d)
            manifest, report, raw, dng = self.build_fixture(td)
            r = evaluate_identity(manifest, report, raw, dng, require_200mp=False)
            self.assertTrue(r["pass"])
            self.assertEqual(r["classification"], "RAW_DNG_CFA_IDENTITY_PROVEN_NON_TARGET_DOMAIN")
            self.assertTrue(r["checks"]["exact_cfa_sample_identity"])
            self.assertEqual(r["observed"]["dng_raw_ifd"]["cfa_pattern_name"], "BGGR")

    def test_one_changed_sample_blocks_identity(self):
        with tempfile.TemporaryDirectory() as d:
            td = pathlib.Path(d)
            manifest, report, raw, dng = self.build_fixture(td, corrupt_dng_sample=True)
            r = evaluate_identity(manifest, report, raw, dng, require_200mp=False)
            self.assertFalse(r["pass"])
            self.assertEqual(r["classification"], "BLOCKED_CFA_IDENTITY_NOT_PROVEN")
            self.assertFalse(r["checks"]["exact_cfa_sample_identity"])
            self.assertIsNotNone(r["observed"]["identity"]["first_mismatch_sample"])

    def test_dng_hash_binding_is_required(self):
        with tempfile.TemporaryDirectory() as d:
            td = pathlib.Path(d)
            manifest, report, raw, dng = self.build_fixture(td)
            manifest["dng_output"]["sha256"] = "0" * 64
            r = evaluate_identity(manifest, report, raw, dng, require_200mp=False)
            self.assertFalse(r["pass"])
            self.assertFalse(r["checks"]["dng_hash_matches_manifest"])


if __name__ == "__main__":
    unittest.main()
