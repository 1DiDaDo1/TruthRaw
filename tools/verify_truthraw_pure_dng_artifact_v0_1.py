#!/usr/bin/env python3
"""TruthRaw PURE float32 DNG artifact verifier v0.1.

This verifier is intentionally downstream of the scientific pipeline. It checks
that an already-produced TRUTHRAW PURE DNG matches the current artifact and
provenance contract. It does *not* prove physical truth, calibration quality,
or that Android executed the scientific pipeline correctly; those remain
separate validation gates.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
import sys
import zlib
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Mapping, Optional, Sequence, Tuple

TAG_IMAGE_WIDTH = 256
TAG_IMAGE_LENGTH = 257
TAG_BITS_PER_SAMPLE = 258
TAG_COMPRESSION = 259
TAG_PHOTOMETRIC = 262
TAG_SAMPLES_PER_PIXEL = 277
TAG_PLANAR_CONFIGURATION = 284
TAG_SOFTWARE = 305
TAG_TILE_WIDTH = 322
TAG_TILE_LENGTH = 323
TAG_TILE_OFFSETS = 324
TAG_TILE_BYTE_COUNTS = 325
TAG_SAMPLE_FORMAT = 339
TAG_SUB_IFDS = 330
TAG_DNG_VERSION = 50706
TAG_DNG_BACKWARD_VERSION = 50707
TAG_UNIQUE_CAMERA_MODEL = 50708
TAG_COLOR_MATRIX1 = 50721
TAG_AS_SHOT_NEUTRAL = 50728
TAG_DNG_PRIVATE_DATA = 50740
TAG_CALIBRATION_ILLUMINANT1 = 50778

TIFF_BYTE = 1
TIFF_ASCII = 2
TIFF_SHORT = 3
TIFF_LONG = 4
TIFF_RATIONAL = 5
TIFF_SRATIONAL = 10

TYPE_SIZES = {
    TIFF_BYTE: 1,
    TIFF_ASCII: 1,
    TIFF_SHORT: 2,
    TIFF_LONG: 4,
    TIFF_RATIONAL: 8,
    TIFF_SRATIONAL: 8,
}

PURE_PHOTOMETRIC_LINEAR_RAW = 34892
PURE_TILE_EDGE = 64
PURE_SAMPLES_PER_PIXEL = 3
PURE_BITS_PER_SAMPLE = (32, 32, 32)
PURE_SAMPLE_FORMAT = (3, 3, 3)
PURE_TILE_BYTES = PURE_TILE_EDGE * PURE_TILE_EDGE * PURE_SAMPLES_PER_PIXEL * 4
PURE_DNG_VERSION = (1, 4, 0, 0)
PURE_DNG_BACKWARD_VERSION = (1, 4, 0, 0)
PURE_CALIBRATION_ILLUMINANT_D50 = 23
PURE_UNIQUE_CAMERA_MODEL = "TruthRaw Scientific Master XYZ D50 Projection"
PURE_SOFTWARE = "TruthRaw scientific-master-linear-dng-projection-v0.1"
PURE_PRIVATE_ID = "TruthRaw scientific-master-linear-dng-projection-v0.1"
PURE_PRIVATE_ROLE = "LINEAR_DNG_XYZ_D50_COMPATIBILITY_PROJECTION"

CERT_MAGIC = b"TRCERT01"
CERT_VERSION = 1
CERT_BYTES = 296
CERT_PROJECTION_PURE_FLOAT32 = 4
CERT_CLAIM_RECONSTRUCTED = 3
CERT_SIGNATURE_UNSIGNED_DEVELOPMENT = 0
CERT_SIGNATURE_ALGORITHM_NONE = 0
CERT_CRC_OFFSET = 292
CERT_BUILD_IDENTITY = bytes.fromhex(
    "e8ed38cc9b92378760f60ae17d90dc6b3149f7e4405cc97eb7a9f36166cc8e55"
)


class VerificationError(RuntimeError):
    pass


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def _require(condition: bool, message: str) -> None:
    if not condition:
        raise VerificationError(message)


@dataclass(frozen=True)
class TiffEntry:
    tag: int
    type_id: int
    count: int
    value_or_offset: int
    entry_offset: int
    inline_value: bytes


class ClassicTiff:
    """Minimal classic-TIFF reader sufficient for TruthRaw validation."""

    def __init__(self, path: Path, *, require_little_endian: bool = False) -> None:
        self.path = path
        self.size = path.stat().st_size
        self._f = path.open("rb")
        header = self._pread(0, 8)
        if header[:2] == b"II":
            self.endian = "<"
        elif header[:2] == b"MM":
            self.endian = ">"
        else:
            raise VerificationError(f"{path}: not a TIFF byte-order marker")
        if require_little_endian:
            _require(self.endian == "<", f"{path}: PURE output must be little-endian classic TIFF")
        version = self._unpack("H", header[2:4])[0]
        _require(version == 42, f"{path}: expected classic TIFF version 42, got {version}")
        self.first_ifd = self._unpack("I", header[4:8])[0]
        _require(8 <= self.first_ifd < self.size, f"{path}: invalid first IFD offset {self.first_ifd}")

    def close(self) -> None:
        self._f.close()

    def __enter__(self) -> "ClassicTiff":
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        self.close()

    def _pread(self, offset: int, size: int) -> bytes:
        _require(offset >= 0 and size >= 0 and offset + size <= self.size,
                 f"{self.path}: read outside file bounds ({offset}+{size}>{self.size})")
        self._f.seek(offset)
        data = self._f.read(size)
        _require(len(data) == size, f"{self.path}: short read at {offset}")
        return data

    def _unpack(self, fmt: str, data: bytes) -> Tuple[object, ...]:
        return struct.unpack(self.endian + fmt, data)

    def read_ifd(self, offset: int) -> Tuple[Dict[int, TiffEntry], int]:
        _require(0 < offset < self.size, f"{self.path}: invalid IFD offset {offset}")
        count = self._unpack("H", self._pread(offset, 2))[0]
        entries_start = offset + 2
        entries_bytes = int(count) * 12
        _require(entries_start + entries_bytes + 4 <= self.size,
                 f"{self.path}: IFD entries exceed file bounds")
        out: Dict[int, TiffEntry] = {}
        for i in range(int(count)):
            eo = entries_start + i * 12
            raw = self._pread(eo, 12)
            tag, type_id = self._unpack("HH", raw[:4])
            c = self._unpack("I", raw[4:8])[0]
            voo = self._unpack("I", raw[8:12])[0]
            out[int(tag)] = TiffEntry(int(tag), int(type_id), int(c), int(voo), eo, raw[8:12])
        next_ifd = self._unpack("I", self._pread(entries_start + entries_bytes, 4))[0]
        return out, int(next_ifd)

    def entry_bytes(self, entry: TiffEntry) -> bytes:
        unit = TYPE_SIZES.get(entry.type_id)
        _require(unit is not None, f"{self.path}: unsupported TIFF type {entry.type_id} for tag {entry.tag}")
        total = unit * entry.count
        if total <= 4:
            return entry.inline_value[:total]
        _require(entry.value_or_offset > 0,
                 f"{self.path}: null out-of-line offset for tag {entry.tag}")
        return self._pread(entry.value_or_offset, total)

    def values(self, entry: TiffEntry) -> Tuple[object, ...]:
        b = self.entry_bytes(entry)
        if entry.type_id == TIFF_BYTE:
            return tuple(b)
        if entry.type_id == TIFF_ASCII:
            return (b.rstrip(b"\x00").decode("ascii", errors="strict"),)
        if entry.type_id == TIFF_SHORT:
            return self._unpack(f"{entry.count}H", b)
        if entry.type_id == TIFF_LONG:
            return self._unpack(f"{entry.count}I", b)
        if entry.type_id in (TIFF_RATIONAL, TIFF_SRATIONAL):
            fmt = "I" if entry.type_id == TIFF_RATIONAL else "i"
            raw = self._unpack(f"{entry.count * 2}{fmt}", b)
            vals = []
            for i in range(entry.count):
                n, d = raw[2 * i], raw[2 * i + 1]
                _require(d != 0, f"{self.path}: zero denominator in tag {entry.tag}")
                vals.append(float(n) / float(d))
            return tuple(vals)
        raise VerificationError(f"{self.path}: unsupported TIFF type {entry.type_id}")

    def require_entry(self, entries: Mapping[int, TiffEntry], tag: int) -> TiffEntry:
        _require(tag in entries, f"{self.path}: required TIFF/DNG tag {tag} is missing")
        return entries[tag]

    def scalar_int(self, entries: Mapping[int, TiffEntry], tag: int) -> int:
        vals = self.values(self.require_entry(entries, tag))
        _require(len(vals) == 1 and isinstance(vals[0], (int, float)),
                 f"{self.path}: tag {tag} is not scalar")
        return int(vals[0])

    def ascii(self, entries: Mapping[int, TiffEntry], tag: int) -> str:
        vals = self.values(self.require_entry(entries, tag))
        _require(len(vals) == 1 and isinstance(vals[0], str),
                 f"{self.path}: tag {tag} is not ASCII")
        return vals[0]

    def discover_image_ifds(self) -> List[Tuple[int, Dict[int, TiffEntry]]]:
        queue = [self.first_ifd]
        seen = set()
        found: List[Tuple[int, Dict[int, TiffEntry]]] = []
        while queue:
            off = queue.pop(0)
            if off == 0 or off in seen:
                continue
            seen.add(off)
            entries, next_ifd = self.read_ifd(off)
            found.append((off, entries))
            if next_ifd and next_ifd not in seen:
                queue.append(next_ifd)
            sub = entries.get(TAG_SUB_IFDS)
            if sub is not None:
                for sub_off in self.values(sub):
                    sub_i = int(sub_off)
                    if sub_i and sub_i not in seen:
                        queue.append(sub_i)
        return found


def source_dimensions(path: Path) -> Tuple[int, int]:
    with ClassicTiff(path) as tiff:
        candidates: List[Tuple[int, int, int]] = []
        for _off, entries in tiff.discover_image_ifds():
            if TAG_IMAGE_WIDTH not in entries or TAG_IMAGE_LENGTH not in entries:
                continue
            w = tiff.scalar_int(entries, TAG_IMAGE_WIDTH)
            h = tiff.scalar_int(entries, TAG_IMAGE_LENGTH)
            if w > 0 and h > 0:
                candidates.append((w * h, w, h))
        _require(candidates, f"{path}: no image dimensions found in TIFF/DNG IFDs")
        _, w, h = max(candidates)
        return w, h


def _hex_nonzero(value: bytes, field: str) -> str:
    _require(any(value), f"certificate: {field} must be non-zero")
    return value.hex()


def parse_certificate(cert: bytes) -> Dict[str, object]:
    _require(len(cert) == CERT_BYTES, f"certificate: expected {CERT_BYTES} bytes, got {len(cert)}")
    _require(cert[:8] == CERT_MAGIC, "certificate: bad TRCERT01 magic")
    version, encoded_size = struct.unpack_from("<HH", cert, 8)
    _require(version == CERT_VERSION, f"certificate: unsupported version {version}")
    _require(encoded_size == CERT_BYTES, f"certificate: encoded size {encoded_size} != {CERT_BYTES}")
    projection, claim, sig_state, sig_algo, color_scope = cert[12:17]
    _require(cert[17:20] == b"\x00\x00\x00", "certificate: reserved header bytes are non-zero")
    _require(projection == CERT_PROJECTION_PURE_FLOAT32,
             f"certificate: projection class {projection} is not TRUTHRAW PURE float32")
    _require(claim == CERT_CLAIM_RECONSTRUCTED,
             f"certificate: claim class {claim} is not RECONSTRUCTED")
    _require(sig_state == CERT_SIGNATURE_UNSIGNED_DEVELOPMENT,
             f"certificate: expected UNSIGNED DEVELOPMENT state, got {sig_state}")
    _require(sig_algo == CERT_SIGNATURE_ALGORITHM_NONE,
             "certificate: unsigned development record must use signature algorithm NONE")
    _require(color_scope in (1, 2), f"certificate: unsupported color claim scope {color_scope}")

    source = cert[20:52]
    master = cert[52:84]
    zero_line = cert[84:116]
    scene_scale = cert[116:148]
    backplane_crc, physical_frames, evidence_count = struct.unpack_from("<III", cert, 148)
    build = cert[160:192]
    issuer = cert[192:224]
    signature = cert[224:288]
    _require(cert[288:292] == b"\x00\x00\x00\x00", "certificate: reserved tail is non-zero")
    stored_crc = struct.unpack_from("<I", cert, CERT_CRC_OFFSET)[0]
    computed_crc = zlib.crc32(cert[:CERT_CRC_OFFSET]) & 0xFFFFFFFF
    _require(stored_crc == computed_crc,
             f"certificate: CRC32 mismatch stored={stored_crc:08x} computed={computed_crc:08x}")
    _require(physical_frames == 1, f"certificate: physicalFrameCount={physical_frames}, expected 1")
    _require(evidence_count == 1, f"certificate: independentEvidenceCount={evidence_count}, expected 1")
    _require(build == CERT_BUILD_IDENTITY,
             "certificate: build/pipeline identity does not match current PURE v0.1 pipeline")
    _require(not any(issuer), "certificate: unsigned development issuer key id must be zero")
    _require(not any(signature), "certificate: unsigned development signature must be zero")

    return {
        "version": version,
        "projection_class": projection,
        "claim_class": claim,
        "signature_state": "UNSIGNED DEVELOPMENT",
        "signature_algorithm": "NONE",
        "color_claim_scope": color_scope,
        "source_evidence_sha256": _hex_nonzero(source, "sourceEvidenceSha256"),
        "scientific_master_sha256": _hex_nonzero(master, "scientificMasterSha256"),
        "zero_line_sha256": _hex_nonzero(zero_line, "zeroLineSha256"),
        "scene_scale_sha256": _hex_nonzero(scene_scale, "sceneScaleSha256"),
        "technical_backplane_crc32": f"{backplane_crc:08x}",
        "physical_frame_count": physical_frames,
        "independent_evidence_count": evidence_count,
        "build_identity_sha256": build.hex(),
        "record_crc32": f"{stored_crc:08x}",
    }


def parse_private_text(prefix: bytes) -> Tuple[str, Dict[str, str]]:
    nul = prefix.find(b"\x00")
    _require(nul > 0, "DNGPrivateData: missing NUL-terminated TruthRaw identity")
    identity = prefix[:nul].decode("ascii", errors="strict")
    _require(identity == PURE_PRIVATE_ID, f"DNGPrivateData: unexpected identity {identity!r}")
    body = prefix[nul + 1:].decode("ascii", errors="strict")
    fields: Dict[str, str] = {}
    for line in body.splitlines():
        if not line:
            continue
        _require("=" in line, f"DNGPrivateData: malformed line {line!r}")
        key, value = line.split("=", 1)
        _require(key and key not in fields, f"DNGPrivateData: duplicate/empty key {key!r}")
        fields[key] = value
    required = {
        "role", "representation_only", "physical_frame_count", "independent_evidence_count",
        "sealed_source_sha256", "scientific_master_sha256", "source_evidence_id", "color_binding_id",
    }
    missing = sorted(required - fields.keys())
    _require(not missing, f"DNGPrivateData: missing fields {missing}")
    _require(fields["role"] == PURE_PRIVATE_ROLE, f"DNGPrivateData: unexpected role {fields['role']!r}")
    _require(fields["representation_only"] == "1", "DNGPrivateData: representation_only must be 1")
    _require(fields["physical_frame_count"] == "1", "DNGPrivateData: physical_frame_count must be 1")
    _require(fields["independent_evidence_count"] == "1",
             "DNGPrivateData: independent_evidence_count must be 1")
    _require(len(fields["sealed_source_sha256"]) == 64,
             "DNGPrivateData: sealed source SHA-256 is malformed")
    _require(len(fields["scientific_master_sha256"]) == 64,
             "DNGPrivateData: Scientific Master SHA-256 is malformed")
    _require(bool(fields["source_evidence_id"]), "DNGPrivateData: source_evidence_id is empty")
    _require(bool(fields["color_binding_id"]), "DNGPrivateData: color_binding_id is empty")
    return identity, fields


def _exact_tuple(tiff: ClassicTiff, entries: Mapping[int, TiffEntry], tag: int,
                 expected: Sequence[int], name: str) -> Tuple[int, ...]:
    vals = tuple(int(v) for v in tiff.values(tiff.require_entry(entries, tag)))
    _require(vals == tuple(expected), f"PURE DNG: {name}={vals}, expected {tuple(expected)}")
    return vals


def _verify_identity_color_matrix(tiff: ClassicTiff, entries: Mapping[int, TiffEntry]) -> None:
    vals = tuple(float(v) for v in tiff.values(tiff.require_entry(entries, TAG_COLOR_MATRIX1)))
    expected = (1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0)
    _require(len(vals) == 9 and all(abs(a - b) <= 1e-12 for a, b in zip(vals, expected)),
             f"PURE DNG: ColorMatrix1 is not identity: {vals}")


def _verify_d50_neutral(tiff: ClassicTiff, entries: Mapping[int, TiffEntry]) -> None:
    vals = tuple(float(v) for v in tiff.values(tiff.require_entry(entries, TAG_AS_SHOT_NEUTRAL)))
    expected = (0.9643, 1.0, 0.8251)
    _require(len(vals) == 3 and all(abs(a - b) <= 5e-7 for a, b in zip(vals, expected)),
             f"PURE DNG: AsShotNeutral is not the writer's D50 neutral: {vals}")


def verify_pure_output(source: Path, output: Path,
                       expected_negative_count: Optional[int] = None,
                       expected_over_one_count: Optional[int] = None) -> Dict[str, object]:
    _require(source.is_file(), f"source file not found: {source}")
    _require(output.is_file(), f"output file not found: {output}")

    source_sha = sha256_file(source)
    source_w, source_h = source_dimensions(source)
    output_sha = sha256_file(output)

    with ClassicTiff(output, require_little_endian=True) as tiff:
        entries, next_ifd = tiff.read_ifd(tiff.first_ifd)
        _require(next_ifd == 0, "PURE DNG: unexpected chained IFD; current writer emits one IFD")
        width = tiff.scalar_int(entries, TAG_IMAGE_WIDTH)
        height = tiff.scalar_int(entries, TAG_IMAGE_LENGTH)
        _require((width, height) == (source_w, source_h),
                 f"PURE DNG: dimensions {(width, height)} != source {(source_w, source_h)}")

        _exact_tuple(tiff, entries, TAG_BITS_PER_SAMPLE, PURE_BITS_PER_SAMPLE, "BitsPerSample")
        _require(tiff.scalar_int(entries, TAG_COMPRESSION) == 1, "PURE DNG: Compression must be 1")
        _require(tiff.scalar_int(entries, TAG_PHOTOMETRIC) == PURE_PHOTOMETRIC_LINEAR_RAW,
                 f"PURE DNG: PhotometricInterpretation must be {PURE_PHOTOMETRIC_LINEAR_RAW}")
        _require(tiff.scalar_int(entries, TAG_SAMPLES_PER_PIXEL) == PURE_SAMPLES_PER_PIXEL,
                 "PURE DNG: SamplesPerPixel must be 3")
        _require(tiff.scalar_int(entries, TAG_PLANAR_CONFIGURATION) == 1,
                 "PURE DNG: PlanarConfiguration must be chunky (1)")
        _exact_tuple(tiff, entries, TAG_SAMPLE_FORMAT, PURE_SAMPLE_FORMAT, "SampleFormat")
        _require(tiff.scalar_int(entries, TAG_TILE_WIDTH) == PURE_TILE_EDGE,
                 f"PURE DNG: TileWidth must be {PURE_TILE_EDGE}")
        _require(tiff.scalar_int(entries, TAG_TILE_LENGTH) == PURE_TILE_EDGE,
                 f"PURE DNG: TileLength must be {PURE_TILE_EDGE}")
        _exact_tuple(tiff, entries, TAG_DNG_VERSION, PURE_DNG_VERSION, "DNGVersion")
        _exact_tuple(tiff, entries, TAG_DNG_BACKWARD_VERSION, PURE_DNG_BACKWARD_VERSION,
                     "DNGBackwardVersion")
        _require(tiff.scalar_int(entries, TAG_CALIBRATION_ILLUMINANT1) == PURE_CALIBRATION_ILLUMINANT_D50,
                 "PURE DNG: CalibrationIlluminant1 must be D50 (23)")
        _require(tiff.ascii(entries, TAG_UNIQUE_CAMERA_MODEL) == PURE_UNIQUE_CAMERA_MODEL,
                 "PURE DNG: UniqueCameraModel does not match current PURE writer")
        _require(tiff.ascii(entries, TAG_SOFTWARE) == PURE_SOFTWARE,
                 "PURE DNG: Software does not match current PURE writer")
        _verify_identity_color_matrix(tiff, entries)
        _verify_d50_neutral(tiff, entries)

        tile_offsets = tuple(int(v) for v in tiff.values(tiff.require_entry(entries, TAG_TILE_OFFSETS)))
        tile_counts = tuple(int(v) for v in tiff.values(tiff.require_entry(entries, TAG_TILE_BYTE_COUNTS)))
        across = (width + PURE_TILE_EDGE - 1) // PURE_TILE_EDGE
        down = (height + PURE_TILE_EDGE - 1) // PURE_TILE_EDGE
        expected_tiles = across * down
        _require(len(tile_offsets) == expected_tiles,
                 f"PURE DNG: TileOffsets count {len(tile_offsets)} != {expected_tiles}")
        _require(len(tile_counts) == expected_tiles,
                 f"PURE DNG: TileByteCounts count {len(tile_counts)} != {expected_tiles}")
        _require(all(v == PURE_TILE_BYTES for v in tile_counts),
                 f"PURE DNG: every tile byte count must be {PURE_TILE_BYTES}")

        negative = 0
        over_one = 0
        finite_components = 0
        real_pixels = 0
        for tile_index, (tile_offset, tile_bytes) in enumerate(zip(tile_offsets, tile_counts)):
            _require(tile_offset >= 0 and tile_offset + tile_bytes <= tiff.size,
                     f"PURE DNG: tile {tile_index} exceeds file bounds")
            raw = tiff._pread(tile_offset, tile_bytes)
            tx = tile_index % across
            ty = tile_index // across
            real_w = min(PURE_TILE_EDGE, width - tx * PURE_TILE_EDGE)
            real_h = min(PURE_TILE_EDGE, height - ty * PURE_TILE_EDGE)
            _require(real_w > 0 and real_h > 0, f"PURE DNG: impossible tile geometry at {tile_index}")
            real_pixels += real_w * real_h
            for local_y in range(real_h):
                row_base = local_y * PURE_TILE_EDGE * 12
                for local_x in range(real_w):
                    pixel_base = row_base + local_x * 12
                    xyz = struct.unpack_from("<fff", raw, pixel_base)
                    for value in xyz:
                        _require(math.isfinite(value),
                                 f"PURE DNG: non-finite float component in tile {tile_index}")
                        finite_components += 1
                        if value < 0.0:
                            negative += 1
                        if value > 1.0:
                            over_one += 1

        _require(real_pixels == width * height, "PURE DNG: real-pixel tile coverage mismatch")
        _require(finite_components == width * height * 3,
                 "PURE DNG: float-component coverage mismatch")
        if expected_negative_count is not None:
            _require(negative == expected_negative_count,
                     f"PURE DNG: negative component count {negative} != expected {expected_negative_count}")
        if expected_over_one_count is not None:
            _require(over_one == expected_over_one_count,
                     f"PURE DNG: >1 component count {over_one} != expected {expected_over_one_count}")

        private = tiff.entry_bytes(tiff.require_entry(entries, TAG_DNG_PRIVATE_DATA))
        _require(len(private) > CERT_BYTES,
                 "DNGPrivateData: payload is too short to contain projection metadata + certificate")
        cert_bytes = private[-CERT_BYTES:]
        prefix = private[:-CERT_BYTES]
        certificate = parse_certificate(cert_bytes)
        private_id, private_fields = parse_private_text(prefix)

    _require(private_fields["sealed_source_sha256"].lower() == source_sha,
             "source SHA-256 mismatch between actual source and DNGPrivateData")
    _require(certificate["source_evidence_sha256"] == source_sha,
             "source SHA-256 mismatch between actual source and certificate")
    _require(private_fields["scientific_master_sha256"].lower() == certificate["scientific_master_sha256"],
             "Scientific Master SHA-256 mismatch between DNGPrivateData and certificate")

    return {
        "schema": "truthraw.pure-dng-artifact-verification.v0.1",
        "result": "PASS",
        "scope": "artifact/projection integrity; not physical-truth or calibration proof",
        "source": {"path": str(source), "sha256": source_sha, "width": source_w, "height": source_h},
        "output": {
            "path": str(output), "sha256": output_sha, "bytes": output.stat().st_size,
            "width": width, "height": height,
            "photometric_interpretation": PURE_PHOTOMETRIC_LINEAR_RAW,
            "bits_per_sample": list(PURE_BITS_PER_SAMPLE), "sample_format": list(PURE_SAMPLE_FORMAT),
            "samples_per_pixel": PURE_SAMPLES_PER_PIXEL, "tile_edge": PURE_TILE_EDGE,
            "negative_component_count": negative, "over_one_component_count": over_one,
            "finite_component_count": finite_components,
        },
        "private_data": {
            "identity": private_id, "role": private_fields["role"], "representation_only": True,
            "sealed_source_sha256": private_fields["sealed_source_sha256"].lower(),
            "scientific_master_sha256": private_fields["scientific_master_sha256"].lower(),
            "source_evidence_id": private_fields["source_evidence_id"],
            "color_binding_id": private_fields["color_binding_id"],
            "physical_frame_count": 1, "independent_evidence_count": 1,
        },
        "certificate": certificate,
        "authority_boundary": {
            "projection_is_new_measurement": False,
            "certificate_strengthens_evidence_authority": False,
            "appearance_applied_artifact_field": "NOT_SERIALIZED_IN_V0_1",
            "counterfactual_created_artifact_field": "NOT_SERIALIZED_IN_V0_1",
            "note": (
                "The current exporter enforces appearanceApplied=false and "
                "counterfactualObservationCreated=false before release, but Certificate v0.1 "
                "does not independently serialize those booleans."
            ),
        },
    }


def parse_args(argv: Optional[Sequence[str]] = None) -> argparse.Namespace:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--source", required=True, type=Path,
                   help="Exact sealed source DNG used to produce the PURE artifact")
    p.add_argument("--output", required=True, type=Path,
                   help="Android/host-produced TRUTHRAW PURE float32 DNG")
    p.add_argument("--json-out", type=Path,
                   help="Optional path for a machine-readable verification report")
    p.add_argument("--expected-negative-count", type=int,
                   help="Optional exact negative float-component count from exporter telemetry")
    p.add_argument("--expected-over-one-count", type=int,
                   help="Optional exact >1 float-component count from exporter telemetry")
    return p.parse_args(argv)


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = parse_args(argv)
    try:
        report = verify_pure_output(
            args.source, args.output, args.expected_negative_count, args.expected_over_one_count
        )
    except (VerificationError, OSError, UnicodeError, struct.error, ValueError) as exc:
        failure = {
            "schema": "truthraw.pure-dng-artifact-verification.v0.1",
            "result": "FAIL",
            "error": str(exc),
        }
        if args.json_out:
            args.json_out.parent.mkdir(parents=True, exist_ok=True)
            args.json_out.write_text(json.dumps(failure, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(json.dumps(failure, sort_keys=True), file=sys.stderr)
        return 2

    if args.json_out:
        args.json_out.parent.mkdir(parents=True, exist_ok=True)
        args.json_out.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
