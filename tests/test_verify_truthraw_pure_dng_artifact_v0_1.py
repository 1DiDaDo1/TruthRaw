#!/usr/bin/env python3

import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib

ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "verify_truthraw_pure_dng_artifact_v0_1.py"

spec = importlib.util.spec_from_file_location("pure_verify", TOOL)
mod = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = mod
assert spec.loader is not None
spec.loader.exec_module(mod)


def pack_entry(tag, type_id, count, payload, data, cursor):
    unit = mod.TYPE_SIZES[type_id]
    assert len(payload) == count * unit
    if len(payload) <= 4:
        inline = payload + b"\x00" * (4 - len(payload))
        return struct.pack("<HHI", tag, type_id, count) + inline, cursor
    off = cursor
    data.extend(b"\x00" * (off - len(data)))
    data.extend(payload)
    cursor = (len(data) + 3) & ~3
    return struct.pack("<HHII", tag, type_id, count, off), cursor


def make_classic_tiff(entries_spec):
    entries_spec = sorted(entries_spec, key=lambda x: x[0])
    ifd_size = 2 + len(entries_spec) * 12 + 4
    data = bytearray(b"II*\x00" + struct.pack("<I", 8))
    data.extend(struct.pack("<H", len(entries_spec)))
    data.extend(b"\x00" * (len(entries_spec) * 12))
    data.extend(struct.pack("<I", 0))
    cursor = (8 + ifd_size + 3) & ~3
    data.extend(b"\x00" * (cursor - len(data)))
    encoded_entries = []
    for tag, type_id, count, payload in entries_spec:
        entry, cursor = pack_entry(tag, type_id, count, payload, data, cursor)
        encoded_entries.append(entry)
    for i, entry in enumerate(encoded_entries):
        start = 10 + i * 12
        data[start:start + 12] = entry
    return bytes(data)


def short(*values):
    return struct.pack("<" + "H" * len(values), *values)


def longv(*values):
    return struct.pack("<" + "I" * len(values), *values)


def bytev(*values):
    return bytes(values)


def ascii_v(text):
    return text.encode("ascii") + b"\x00"


def rat(*pairs):
    out = bytearray()
    for n, d in pairs:
        out.extend(struct.pack("<II", n, d))
    return bytes(out)


def srat_identity():
    out = bytearray()
    for r in range(3):
        for c in range(3):
            out.extend(struct.pack("<ii", 1 if r == c else 0, 1))
    return bytes(out)


def make_source(path: Path):
    blob = make_classic_tiff([
        (mod.TAG_IMAGE_WIDTH, mod.TIFF_LONG, 1, longv(2)),
        (mod.TAG_IMAGE_LENGTH, mod.TIFF_LONG, 1, longv(2)),
    ])
    path.write_bytes(blob)
    return hashlib.sha256(blob).digest()


def make_cert(source_hash: bytes, master_hash: bytes) -> bytes:
    cert = bytearray(mod.CERT_BYTES)
    cert[0:8] = mod.CERT_MAGIC
    struct.pack_into("<HH", cert, 8, mod.CERT_VERSION, mod.CERT_BYTES)
    cert[12] = mod.CERT_PROJECTION_PURE_FLOAT32
    cert[13] = mod.CERT_CLAIM_RECONSTRUCTED
    cert[14] = mod.CERT_SIGNATURE_UNSIGNED_DEVELOPMENT
    cert[15] = mod.CERT_SIGNATURE_ALGORITHM_NONE
    cert[16] = 1
    cert[20:52] = source_hash
    cert[52:84] = master_hash
    cert[84:116] = bytes(range(1, 33))
    cert[116:148] = bytes(range(33, 65))
    struct.pack_into("<III", cert, 148, 0x12345678, 1, 1)
    cert[160:192] = mod.CERT_BUILD_IDENTITY
    crc = zlib.crc32(cert[:mod.CERT_CRC_OFFSET]) & 0xFFFFFFFF
    struct.pack_into("<I", cert, mod.CERT_CRC_OFFSET, crc)
    return bytes(cert)


def make_output(path: Path, source_hash: bytes, *, corrupt_cert=False):
    master_hash = hashlib.sha256(b"synthetic-scientific-master").digest()
    body = (
        f"role={mod.PURE_PRIVATE_ROLE}\n"
        "representation_only=1\n"
        "physical_frame_count=1\n"
        "independent_evidence_count=1\n"
        f"sealed_source_sha256={source_hash.hex()}\n"
        f"scientific_master_sha256={master_hash.hex()}\n"
        "source_evidence_id=synthetic-source\n"
        "color_binding_id=synthetic-color-binding\n"
    ).encode("ascii")
    prefix = mod.PURE_PRIVATE_ID.encode("ascii") + b"\x00" + body
    cert = bytearray(make_cert(source_hash, master_hash))
    if corrupt_cert:
        cert[mod.CERT_CRC_OFFSET] ^= 0x01
    private = prefix + cert

    tile = bytearray(mod.PURE_TILE_BYTES)
    real = [
        (-0.1, 0.2, 1.2),
        (0.0, 0.5, 0.9),
        (0.3, -0.2, 0.7),
        (1.1, 0.4, 1.0),
    ]
    for idx, xyz in enumerate(real):
        y, x = divmod(idx, 2)
        off = (y * mod.PURE_TILE_EDGE + x) * 12
        struct.pack_into("<fff", tile, off, *xyz)

    specs = [
        (mod.TAG_IMAGE_WIDTH, mod.TIFF_LONG, 1, longv(2)),
        (mod.TAG_IMAGE_LENGTH, mod.TIFF_LONG, 1, longv(2)),
        (mod.TAG_BITS_PER_SAMPLE, mod.TIFF_SHORT, 3, short(32, 32, 32)),
        (mod.TAG_COMPRESSION, mod.TIFF_SHORT, 1, short(1)),
        (mod.TAG_PHOTOMETRIC, mod.TIFF_SHORT, 1, short(mod.PURE_PHOTOMETRIC_LINEAR_RAW)),
        (mod.TAG_SAMPLES_PER_PIXEL, mod.TIFF_SHORT, 1, short(3)),
        (mod.TAG_PLANAR_CONFIGURATION, mod.TIFF_SHORT, 1, short(1)),
        (mod.TAG_SOFTWARE, mod.TIFF_ASCII, len(ascii_v(mod.PURE_SOFTWARE)), ascii_v(mod.PURE_SOFTWARE)),
        (mod.TAG_TILE_WIDTH, mod.TIFF_LONG, 1, longv(mod.PURE_TILE_EDGE)),
        (mod.TAG_TILE_LENGTH, mod.TIFF_LONG, 1, longv(mod.PURE_TILE_EDGE)),
        (mod.TAG_TILE_OFFSETS, mod.TIFF_LONG, 1, longv(0)),
        (mod.TAG_TILE_BYTE_COUNTS, mod.TIFF_LONG, 1, longv(mod.PURE_TILE_BYTES)),
        (mod.TAG_SAMPLE_FORMAT, mod.TIFF_SHORT, 3, short(3, 3, 3)),
        (mod.TAG_DNG_VERSION, mod.TIFF_BYTE, 4, bytev(1, 4, 0, 0)),
        (mod.TAG_DNG_BACKWARD_VERSION, mod.TIFF_BYTE, 4, bytev(1, 4, 0, 0)),
        (mod.TAG_UNIQUE_CAMERA_MODEL, mod.TIFF_ASCII,
         len(ascii_v(mod.PURE_UNIQUE_CAMERA_MODEL)), ascii_v(mod.PURE_UNIQUE_CAMERA_MODEL)),
        (mod.TAG_COLOR_MATRIX1, mod.TIFF_SRATIONAL, 9, srat_identity()),
        (mod.TAG_AS_SHOT_NEUTRAL, mod.TIFF_RATIONAL, 3,
         rat((9643, 10000), (1, 1), (8251, 10000))),
        (mod.TAG_DNG_PRIVATE_DATA, mod.TIFF_BYTE, len(private), private),
        (mod.TAG_CALIBRATION_ILLUMINANT1, mod.TIFF_SHORT, 1, short(23)),
    ]
    meta = bytearray(make_classic_tiff(specs))
    tile_offset = (len(meta) + 3) & ~3
    meta.extend(b"\x00" * (tile_offset - len(meta)))
    count = struct.unpack_from("<H", meta, 8)[0]
    for i in range(count):
        eo = 10 + i * 12
        if struct.unpack_from("<H", meta, eo)[0] == mod.TAG_TILE_OFFSETS:
            struct.pack_into("<I", meta, eo + 8, tile_offset)
            break
    else:
        raise AssertionError("TileOffsets not found")
    meta.extend(tile)
    path.write_bytes(meta)


class PureArtifactVerifierTests(unittest.TestCase):
    def test_pass_and_counts(self):
        with tempfile.TemporaryDirectory() as td:
            td = Path(td)
            source = td / "source.dng"
            output = td / "pure.dng"
            source_hash = make_source(source)
            make_output(output, source_hash)
            report = mod.verify_pure_output(source, output, 2, 2)
            self.assertEqual(report["result"], "PASS")
            self.assertEqual(report["output"]["negative_component_count"], 2)
            self.assertEqual(report["output"]["over_one_component_count"], 2)
            self.assertEqual(report["certificate"]["signature_state"], "UNSIGNED DEVELOPMENT")
            self.assertEqual(report["certificate"]["physical_frame_count"], 1)
            self.assertEqual(report["certificate"]["independent_evidence_count"], 1)
            self.assertEqual(
                report["authority_boundary"]["appearance_applied_artifact_field"],
                "NOT_SERIALIZED_IN_V0_1",
            )

    def test_certificate_crc_tamper_fails_closed(self):
        with tempfile.TemporaryDirectory() as td:
            td = Path(td)
            source = td / "source.dng"
            output = td / "pure-tampered.dng"
            source_hash = make_source(source)
            make_output(output, source_hash, corrupt_cert=True)
            with self.assertRaises(mod.VerificationError):
                mod.verify_pure_output(source, output)

    def test_wrong_source_fails_closed(self):
        with tempfile.TemporaryDirectory() as td:
            td = Path(td)
            source = td / "source.dng"
            wrong_source = td / "wrong.dng"
            output = td / "pure.dng"
            source_hash = make_source(source)
            make_output(output, source_hash)
            wrong_source.write_bytes(source.read_bytes() + b"different")
            with self.assertRaises(mod.VerificationError):
                mod.verify_pure_output(wrong_source, output)

    def test_cli_json_report(self):
        with tempfile.TemporaryDirectory() as td:
            td = Path(td)
            source = td / "source.dng"
            output = td / "pure.dng"
            report_path = td / "report.json"
            source_hash = make_source(source)
            make_output(output, source_hash)
            result = subprocess.run(
                [sys.executable, str(TOOL), "--source", str(source), "--output", str(output),
                 "--expected-negative-count", "2", "--expected-over-one-count", "2",
                 "--json-out", str(report_path)],
                text=True, capture_output=True, check=False,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(report_path.read_text(encoding="utf-8"))
            self.assertEqual(report["result"], "PASS")


if __name__ == "__main__":
    unittest.main()
