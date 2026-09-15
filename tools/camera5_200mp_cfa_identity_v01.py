#!/usr/bin/env python3
"""TruthRaw Camera 5 200 MP CFA identity verifier v0.1.

Purpose
-------
Prove, fail-closed, whether the canonicalized app-visible RAW_SENSOR plane and
one RAW IFD inside the Android DngCreator DNG contain the exact same CFA sample
codes in the exact same raster order.

The tool does *not* prove untouched photodiode/ADC output, absence of sensor/HAL
processing, electron calibration, optical truth, or color truth. It only binds
one already-admitted application-visible RAW capture to its DNG representation.

The implementation is streaming and classic-TIFF/DNG aware. v0.1 deliberately
supports only uncompressed, single-sample, 16-bit strip-organized RAW IFDs. Any
other DNG storage mode fails closed instead of being silently decoded through an
unreviewed library path.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import BinaryIO, Dict, List, Optional, Sequence, Tuple

TARGET = (16320, 12288)
TIFF_CFA = 32803

TYPE_SIZES = {
    1: 1,
    2: 1,
    3: 2,
    4: 4,
    5: 8,
    6: 1,
    7: 1,
    8: 2,
    9: 4,
    10: 8,
    11: 4,
    12: 8,
    13: 4,
}

IMAGE_WIDTH = 256
IMAGE_LENGTH = 257
BITS_PER_SAMPLE = 258
COMPRESSION = 259
PHOTOMETRIC = 262
STRIP_OFFSETS = 273
SAMPLES_PER_PIXEL = 277
ROWS_PER_STRIP = 278
STRIP_BYTE_COUNTS = 279
PLANAR_CONFIGURATION = 284
TILE_OFFSETS = 324
TILE_BYTE_COUNTS = 325
SUB_IFDS = 330
CFA_REPEAT_PATTERN_DIM = 33421
CFA_PATTERN = 33422


class IdentityError(ValueError):
    pass


@dataclass(frozen=True)
class TiffIfd:
    offset: int
    tags: Dict[int, Tuple[int, int, bytes]]


def sha256_file(path: Path, chunk: int = 8 << 20) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        while True:
            b = f.read(chunk)
            if not b:
                break
            h.update(b)
    return h.hexdigest()


def _read_exact(f: BinaryIO, n: int) -> bytes:
    b = f.read(n)
    if len(b) != n:
        raise IdentityError(f"truncated TIFF/DNG: wanted {n} bytes, got {len(b)}")
    return b


def _decode_numbers(raw: bytes, field_type: int, count: int, endian: str) -> Tuple[int, ...]:
    if field_type in (1, 6, 7):
        if len(raw) < count:
            raise IdentityError("short TIFF BYTE/UNDEFINED field")
        return tuple(raw[:count])
    fmts = {3: "H", 4: "I", 8: "h", 9: "i", 13: "I"}
    if field_type not in fmts:
        raise IdentityError(f"unsupported numeric TIFF field type {field_type}")
    return tuple(struct.unpack(endian + fmts[field_type] * count, raw))


def _load_ifd(f: BinaryIO, offset: int, endian: str, file_size: int) -> Tuple[TiffIfd, int]:
    if offset <= 0 or offset >= file_size:
        raise IdentityError(f"IFD offset outside file: {offset}")
    f.seek(offset)
    n = struct.unpack(endian + "H", _read_exact(f, 2))[0]
    if n > 4096:
        raise IdentityError(f"unreasonable IFD entry count: {n}")
    tags: Dict[int, Tuple[int, int, bytes]] = {}
    for _ in range(n):
        entry = _read_exact(f, 12)
        tag, field_type, count = struct.unpack(endian + "HHI", entry[:8])
        if field_type not in TYPE_SIZES:
            continue
        total = TYPE_SIZES[field_type] * count
        if total < 0 or total > file_size:
            raise IdentityError(f"unreasonable TIFF field size for tag {tag}: {total}")
        value4 = entry[8:12]
        if total <= 4:
            raw = value4[:total]
        else:
            value_offset = struct.unpack(endian + "I", value4)[0]
            if value_offset + total > file_size:
                raise IdentityError(f"TIFF field {tag} points outside file")
            here = f.tell()
            f.seek(value_offset)
            raw = _read_exact(f, total)
            f.seek(here)
        tags[tag] = (field_type, count, raw)
    next_ifd = struct.unpack(endian + "I", _read_exact(f, 4))[0]
    return TiffIfd(offset=offset, tags=tags), next_ifd


def parse_tiff_ifds(path: Path) -> Tuple[str, List[TiffIfd]]:
    file_size = path.stat().st_size
    with path.open("rb") as f:
        bo = _read_exact(f, 2)
        if bo == b"II":
            endian = "<"
        elif bo == b"MM":
            endian = ">"
        else:
            raise IdentityError("not a classic TIFF/DNG byte-order marker")
        magic = struct.unpack(endian + "H", _read_exact(f, 2))[0]
        if magic != 42:
            raise IdentityError(f"v0.1 supports classic TIFF/DNG magic 42 only, got {magic}")
        root = struct.unpack(endian + "I", _read_exact(f, 4))[0]

        out: List[TiffIfd] = []
        queue: List[int] = [root]
        seen = set()
        while queue:
            off = queue.pop(0)
            if off == 0 or off in seen:
                continue
            seen.add(off)
            ifd, nxt = _load_ifd(f, off, endian, file_size)
            out.append(ifd)
            if nxt:
                queue.append(nxt)
            sub = ifd.tags.get(SUB_IFDS)
            if sub:
                vals = _decode_numbers(sub[2], sub[0], sub[1], endian)
                queue.extend(int(v) for v in vals if int(v) != 0)
        return endian, out


def _tag_numbers(ifd: TiffIfd, tag: int, endian: str, default: Optional[Sequence[int]] = None) -> Tuple[int, ...]:
    rec = ifd.tags.get(tag)
    if rec is None:
        return tuple(default or ())
    return _decode_numbers(rec[2], rec[0], rec[1], endian)


def _one(ifd: TiffIfd, tag: int, endian: str, default: Optional[int] = None) -> Optional[int]:
    vals = _tag_numbers(ifd, tag, endian)
    if not vals:
        return default
    if len(vals) != 1:
        raise IdentityError(f"TIFF tag {tag} expected one value, got {len(vals)}")
    return int(vals[0])


def cfa_pattern_name(ifd: TiffIfd, endian: str) -> Optional[str]:
    dims = _tag_numbers(ifd, CFA_REPEAT_PATTERN_DIM, endian)
    pat = _tag_numbers(ifd, CFA_PATTERN, endian)
    if dims != (2, 2) or len(pat) != 4:
        return None
    names = {0: "R", 1: "G", 2: "B", 3: "C", 4: "M", 5: "Y", 6: "W"}
    try:
        return "".join(names[int(v)] for v in pat)
    except KeyError:
        return None


def select_raw_ifd(ifds: Sequence[TiffIfd], endian: str, width: int, height: int) -> TiffIfd:
    candidates = []
    for ifd in ifds:
        w = _one(ifd, IMAGE_WIDTH, endian)
        h = _one(ifd, IMAGE_LENGTH, endian)
        photo = _one(ifd, PHOTOMETRIC, endian)
        if w == width and h == height and photo == TIFF_CFA:
            candidates.append(ifd)
    if len(candidates) != 1:
        raise IdentityError(f"expected exactly one {width}x{height} CFA RAW IFD, found {len(candidates)}")
    return candidates[0]


def _canonical_stream_compare(
    canonical_raw: Path,
    dng: Path,
    ifd: TiffIfd,
    endian: str,
    width: int,
    height: int,
    chunk_bytes: int = 8 << 20,
) -> dict:
    bits = _one(ifd, BITS_PER_SAMPLE, endian)
    compression = _one(ifd, COMPRESSION, endian, 1)
    samples_per_pixel = _one(ifd, SAMPLES_PER_PIXEL, endian, 1)
    planar = _one(ifd, PLANAR_CONFIGURATION, endian, 1)
    if bits != 16:
        raise IdentityError(f"v0.1 identity path requires BitsPerSample=16, got {bits}")
    if compression != 1:
        raise IdentityError(f"v0.1 identity path requires Compression=1, got {compression}")
    if samples_per_pixel != 1:
        raise IdentityError(f"RAW CFA IFD must have SamplesPerPixel=1, got {samples_per_pixel}")
    if planar != 1:
        raise IdentityError(f"RAW CFA IFD must be chunky/PlanarConfiguration=1, got {planar}")
    if _tag_numbers(ifd, TILE_OFFSETS, endian) or _tag_numbers(ifd, TILE_BYTE_COUNTS, endian):
        raise IdentityError("v0.1 fails closed on tiled RAW IFDs; strip storage is required")

    offsets = _tag_numbers(ifd, STRIP_OFFSETS, endian)
    counts = _tag_numbers(ifd, STRIP_BYTE_COUNTS, endian)
    rows_per_strip = int(_one(ifd, ROWS_PER_STRIP, endian, height) or height)
    if not offsets or not counts or len(offsets) != len(counts):
        raise IdentityError("RAW IFD strip offsets/counts are missing or inconsistent")
    if rows_per_strip <= 0:
        raise IdentityError("RowsPerStrip must be positive")

    expected_bytes = width * height * 2
    if canonical_raw.stat().st_size != expected_bytes:
        raise IdentityError(f"canonical RAW size mismatch: {canonical_raw.stat().st_size} != {expected_bytes}")

    expected_counts = []
    remaining_rows = height
    while remaining_rows > 0:
        rows = min(rows_per_strip, remaining_rows)
        expected_counts.append(rows * width * 2)
        remaining_rows -= rows
    if len(expected_counts) != len(counts):
        raise IdentityError(f"strip count mismatch: expected {len(expected_counts)}, DNG has {len(counts)}")
    for i, (actual, expected) in enumerate(zip(counts, expected_counts)):
        if int(actual) != int(expected):
            raise IdentityError(f"strip {i} byte count {actual} != exact raster bytes {expected}")

    raw_hash = hashlib.sha256()
    dng_cfa_hash = hashlib.sha256()
    bytes_compared = 0
    first_mismatch_byte: Optional[int] = None

    with canonical_raw.open("rb") as rf, dng.open("rb") as df:
        for off, count in zip(offsets, counts):
            df.seek(int(off))
            remaining = int(count)
            while remaining:
                n = min(remaining, chunk_bytes)
                if n % 2:
                    n -= 1
                if n <= 0:
                    raise IdentityError("odd byte count in 16-bit RAW strip")
                db = _read_exact(df, n)
                rb = _read_exact(rf, n)
                if endian == ">":
                    swapped = bytearray(n)
                    swapped[0::2] = db[1::2]
                    swapped[1::2] = db[0::2]
                    db_le = bytes(swapped)
                else:
                    db_le = db
                raw_hash.update(rb)
                dng_cfa_hash.update(db_le)
                if first_mismatch_byte is None and rb != db_le:
                    for i, (a, b) in enumerate(zip(rb, db_le)):
                        if a != b:
                            first_mismatch_byte = bytes_compared + i
                            break
                bytes_compared += n
                remaining -= n
        if rf.read(1):
            raise IdentityError("canonical RAW has trailing bytes after expected raster")

    return {
        "bytes_compared": bytes_compared,
        "samples_compared": bytes_compared // 2,
        "first_mismatch_byte": first_mismatch_byte,
        "first_mismatch_sample": None if first_mismatch_byte is None else first_mismatch_byte // 2,
        "canonical_raw_sha256_stream": raw_hash.hexdigest(),
        "dng_cfa_canonical_sha256": dng_cfa_hash.hexdigest(),
        "sample_identity": first_mismatch_byte is None and bytes_compared == expected_bytes,
        "bits_per_sample": bits,
        "compression": compression,
        "samples_per_pixel": samples_per_pixel,
        "rows_per_strip": rows_per_strip,
        "strip_count": len(offsets),
    }


def evaluate_identity(
    manifest: dict,
    canonical_report: dict,
    canonical_raw: Path,
    dng: Path,
    *,
    require_200mp: bool = True,
) -> dict:
    raw_out = manifest.get("raw_output") or {}
    dng_out = manifest.get("dng_output") or {}
    width = int(raw_out.get("width", 0))
    height = int(raw_out.get("height", 0))

    actual_raw_hash = sha256_file(canonical_raw)
    actual_dng_hash = sha256_file(dng)

    checks = {
        "device_runtime_capture": manifest.get("evidence_class") == "DEVICE_RUNTIME_CAPTURE",
        "camera5_route": str(manifest.get("physical_camera_id")) == "5",
        "raw_sensor_format": raw_out.get("format") == "RAW_SENSOR",
        "manifest_dimensions_positive": width > 0 and height > 0,
        "target_200mp_dimensions": (width, height) == TARGET,
        "canonical_report_dimensions_match": (
            int(canonical_report.get("width", -1)), int(canonical_report.get("height", -1))
        ) == (width, height),
        "canonical_report_source_bound_to_manifest": (
            canonical_report.get("source_buffer_sha256")
            == raw_out.get("payload_sha256")
            == manifest.get("source_identity_sha256")
        ),
        "canonical_raw_hash_matches_report": actual_raw_hash == canonical_report.get("canonical_sha256"),
        "dng_hash_matches_manifest": actual_dng_hash == dng_out.get("sha256"),
    }

    identity = None
    dng_meta = None
    parse_error = None
    try:
        endian, ifds = parse_tiff_ifds(dng)
        raw_ifd = select_raw_ifd(ifds, endian, width, height)
        dng_meta = {
            "byte_order": "little" if endian == "<" else "big",
            "ifd_offset": raw_ifd.offset,
            "image_width": _one(raw_ifd, IMAGE_WIDTH, endian),
            "image_length": _one(raw_ifd, IMAGE_LENGTH, endian),
            "photometric_interpretation": _one(raw_ifd, PHOTOMETRIC, endian),
            "cfa_repeat_pattern_dim": list(_tag_numbers(raw_ifd, CFA_REPEAT_PATTERN_DIM, endian)),
            "cfa_pattern_codes": list(_tag_numbers(raw_ifd, CFA_PATTERN, endian)),
            "cfa_pattern_name": cfa_pattern_name(raw_ifd, endian),
        }
        identity = _canonical_stream_compare(canonical_raw, dng, raw_ifd, endian, width, height)
        checks["dng_unique_matching_cfa_raw_ifd"] = True
        checks["exact_cfa_sample_identity"] = bool(identity["sample_identity"])
        checks["canonical_hash_equals_dng_cfa_hash"] = (
            identity["canonical_raw_sha256_stream"] == identity["dng_cfa_canonical_sha256"]
        )
    except Exception as e:
        parse_error = f"{type(e).__name__}: {e}"
        checks["dng_unique_matching_cfa_raw_ifd"] = False
        checks["exact_cfa_sample_identity"] = False
        checks["canonical_hash_equals_dng_cfa_hash"] = False

    required_names = [
        "device_runtime_capture",
        "camera5_route",
        "raw_sensor_format",
        "manifest_dimensions_positive",
        "canonical_report_dimensions_match",
        "canonical_report_source_bound_to_manifest",
        "canonical_raw_hash_matches_report",
        "dng_hash_matches_manifest",
        "dng_unique_matching_cfa_raw_ifd",
        "exact_cfa_sample_identity",
        "canonical_hash_equals_dng_cfa_hash",
    ]
    if require_200mp:
        required_names.append("target_200mp_dimensions")
    passed = all(checks.get(k) is True for k in required_names)

    if passed and (width, height) == TARGET:
        classification = "CAMERA5_200MP_RAW_DNG_CFA_IDENTITY_PROVEN"
    elif passed:
        classification = "RAW_DNG_CFA_IDENTITY_PROVEN_NON_TARGET_DOMAIN"
    else:
        classification = "BLOCKED_CFA_IDENTITY_NOT_PROVEN"

    return {
        "schema": "TruthRawCamera5CfaIdentity/0.1",
        "classification": classification,
        "pass": passed,
        "checks": checks,
        "observed": {
            "manifest_dimensions": [width, height],
            "canonical_raw_file": canonical_raw.name,
            "canonical_raw_sha256": actual_raw_hash,
            "dng_file": dng.name,
            "dng_sha256": actual_dng_hash,
            "dng_raw_ifd": dng_meta,
            "identity": identity,
            "parse_or_identity_error": parse_error,
        },
        "scientific_boundary": (
            "PASS proves sample-for-sample identity between the canonicalized app-visible RAW_SENSOR "
            "CFA raster and the selected uncompressed 16-bit CFA RAW IFD in the capture-bound DNG. "
            "It does not prove untouched photodiode/ADC output, absence of upstream sensor/HAL processing, "
            "electron calibration, optical truth, spectral/color truth, or additional independent evidence."
        ),
    }


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("manifest")
    ap.add_argument("canonical_report")
    ap.add_argument("canonical_raw")
    ap.add_argument("dng")
    ap.add_argument("--out", required=True)
    ns = ap.parse_args()

    with open(ns.manifest, "r", encoding="utf-8") as f:
        manifest = json.load(f)
    with open(ns.canonical_report, "r", encoding="utf-8") as f:
        canonical_report = json.load(f)

    result = evaluate_identity(
        manifest,
        canonical_report,
        Path(ns.canonical_raw),
        Path(ns.dng),
        require_200mp=True,
    )
    Path(ns.out).write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps(result, indent=2))
    raise SystemExit(0 if result["pass"] else 2)


if __name__ == "__main__":
    main()
