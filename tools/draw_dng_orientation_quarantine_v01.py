#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
from pathlib import Path

TAG_PHOTOMETRIC = 262
TAG_ORIENTATION = 274
TAG_STRIP_OFFSETS = 273
TAG_STRIP_BYTE_COUNTS = 279
TAG_TILE_OFFSETS = 324
PHOTO_CFA = 32803
TIFF_SHORT = 3
TIFF_LONG = 4
HEX64 = re.compile(r"^[0-9a-f]{64}$")


class Error(RuntimeError):
    pass


def sha_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def canonical_hash(doc: dict) -> str:
    x = dict(doc)
    x.pop("manifest_sha256", None)
    return hashlib.sha256(
        json.dumps(
            x,
            sort_keys=True,
            separators=(",", ":"),
            ensure_ascii=False,
            allow_nan=False,
        ).encode("utf-8")
    ).hexdigest()


def parse(data: bytes):
    if len(data) < 8:
        raise Error("too small")
    if data[:2] == b"II":
        endian = "<"
    elif data[:2] == b"MM":
        endian = ">"
    else:
        raise Error("not TIFF byte order")

    def u16(offset: int) -> int:
        return struct.unpack_from(endian + "H", data, offset)[0]

    def u32(offset: int) -> int:
        return struct.unpack_from(endian + "I", data, offset)[0]

    if u16(2) != 42:
        raise Error("classic TIFF magic 42 required")

    offset = u32(4)
    ifds = []
    seen = set()
    for _ in range(16):
        if offset == 0:
            break
        if offset in seen or offset + 2 > len(data):
            raise Error("invalid IFD chain")
        seen.add(offset)
        count = u16(offset)
        entries = {}
        for i in range(count):
            entry_offset = offset + 2 + 12 * i
            if entry_offset + 12 > len(data):
                raise Error("truncated IFD")
            tag = u16(entry_offset)
            typ = u16(entry_offset + 2)
            cardinality = u32(entry_offset + 4)
            entries[tag] = (entry_offset, typ, cardinality)

        next_offset_position = offset + 2 + 12 * count
        if next_offset_position + 4 > len(data):
            raise Error("truncated next IFD")
        ifds.append((offset, entries))
        offset = u32(next_offset_position)
    return endian, u16, u32, ifds


def values(data: bytes, endian, u16, u32, entry):
    entry_offset, typ, count = entry
    size = {TIFF_SHORT: 2, TIFF_LONG: 4}.get(typ)
    if size is None:
        raise Error("unsupported tag type")
    total = size * count
    base = entry_offset + 8 if total <= 4 else u32(entry_offset + 8)
    if base + total > len(data):
        raise Error("tag payload out of range")
    if typ == TIFF_SHORT:
        return [u16(base + 2 * i) for i in range(count)], base
    return [u32(base + 4 * i) for i in range(count)], base


def cfa_ifd(data: bytes):
    endian, u16, u32, ifds = parse(data)
    matches = []
    for offset, entries in ifds:
        if TAG_PHOTOMETRIC not in entries:
            continue
        try:
            vals, _ = values(
                data, endian, u16, u32, entries[TAG_PHOTOMETRIC]
            )
        except Error:
            continue
        if vals == [PHOTO_CFA]:
            matches.append((offset, entries))
    if len(matches) != 1:
        raise Error(f"exactly one CFA IFD required, got {len(matches)}")
    return endian, u16, u32, matches[0]


def payload_digest(data: bytes):
    endian, u16, u32, (ifd, entries) = cfa_ifd(data)
    if TAG_TILE_OFFSETS in entries:
        raise Error("v0.1 quarantine supports strip storage only")
    if (
        TAG_STRIP_OFFSETS not in entries
        or TAG_STRIP_BYTE_COUNTS not in entries
    ):
        raise Error("strip tags required")

    offsets, _ = values(
        data, endian, u16, u32, entries[TAG_STRIP_OFFSETS]
    )
    counts, _ = values(
        data, endian, u16, u32, entries[TAG_STRIP_BYTE_COUNTS]
    )
    if len(offsets) != len(counts) or not offsets:
        raise Error("strip cardinality mismatch")

    h = hashlib.sha256()
    total = 0
    ranges = []
    for offset, count in zip(offsets, counts):
        if offset + count > len(data):
            raise Error("strip out of range")
        h.update(data[offset : offset + count])
        total += count
        ranges.append((offset, offset + count))
    return h.hexdigest(), total, ranges, ifd, entries


def validate_manifest_only(doc: dict) -> list[str]:
    errors: list[str] = []

    if doc.get("schema") != "D.RAW/DngOrientationQuarantine/0.1":
        errors.append("schema")
    if doc.get("status") != "DERIVED_INGRESS_CONTAINER_NO_NEW_EVIDENCE":
        errors.append("status")
    if doc.get("manifest_sha256") != canonical_hash(doc):
        errors.append("manifest_sha256")

    parent = doc.get("parent_source") or {}
    derived = doc.get("derived_ingress") or {}
    issue = doc.get("source_issue") or {}
    norm = doc.get("normalization") or {}
    payload = doc.get("serialized_cfa_payload") or {}
    lineage = doc.get("lineage") or {}

    for name, block, key in (
        ("parent", parent, "sha256"),
        ("derived", derived, "sha256"),
        ("payload.parent", payload, "parent_sha256"),
        ("payload.derived", payload, "derived_sha256"),
    ):
        if HEX64.fullmatch(str(block.get(key, ""))) is None:
            errors.append(name + ".sha256")

    if parent.get("immutable") is not True:
        errors.append("parent.immutable")
    if parent.get("byte_length") != derived.get("byte_length"):
        errors.append("byte_length")
    if issue.get("tag") != "Orientation" or issue.get("tag_code") != 274:
        errors.append("issue.tag")
    if issue.get("source_value") != 9:
        errors.append("issue.source_value")
    if issue.get("tiff_defined_values_min") != 1:
        errors.append("issue.min")
    if issue.get("tiff_defined_values_max") != 8:
        errors.append("issue.max")
    if issue.get("source_value_standard_defined") is not False:
        errors.append("issue.standard_defined")

    if (
        norm.get("method_id")
        != "D_RAW_DNG_INVALID_ORIENTATION_9_TO_STORAGE_COORDINATE_1_V0_1"
    ):
        errors.append("normalization.method")
    if norm.get("derived_value") != 1:
        errors.append("normalization.derived_value")
    if norm.get("meaning") != "STORAGE_COORDINATE_NORMAL_ONLY":
        errors.append("normalization.meaning")
    if norm.get("presentation_orientation_authority") != "UNKNOWN":
        errors.append("normalization.presentation_authority")
    if norm.get("world_orientation_claimed") is not False:
        errors.append("normalization.world_orientation")
    if norm.get("changed_byte_count") != 1:
        errors.append("normalization.changed_byte_count")
    offsets = norm.get("changed_byte_offsets")
    if not isinstance(offsets, list) or len(offsets) != 1:
        errors.append("normalization.changed_byte_offsets")
    if norm.get("source_byte_hex") != "09":
        errors.append("normalization.source_byte")
    if norm.get("derived_byte_hex") != "01":
        errors.append("normalization.derived_byte")

    if payload.get("parent_sha256") != payload.get("derived_sha256"):
        errors.append("payload.hash_changed")
    if payload.get("byte_identical") is not True:
        errors.append("payload.byte_identical")
    if not isinstance(payload.get("byte_length"), int) or payload.get(
        "byte_length", 0
    ) <= 0:
        errors.append("payload.byte_length")

    if (
        derived.get("container_role")
        != "SCIENTIFIC_STORAGE_COORDINATE_INGRESS_ONLY"
    ):
        errors.append("derived.role")

    expected_lineage = {
        "same_physical_observation": True,
        "parent_source_mutated": False,
        "cfa_payload_mutated": False,
        "physical_frame_count_increment": 0,
        "independent_evidence_count_increment": 0,
        "creates_new_evidence": False,
        "scientific_writeback_allowed": False,
        "appearance_orientation_claim_allowed": False,
    }
    for key, value in expected_lineage.items():
        if lineage.get(key) != value:
            errors.append("lineage." + key)

    return errors


def normalize(parent: Path, derived: Path, manifest: Path):
    source = parent.read_bytes()
    endian, u16, u32, (ifd, entries) = cfa_ifd(source)

    if TAG_ORIENTATION not in entries:
        raise Error("Orientation tag required for v0.1")
    _, typ, count = entries[TAG_ORIENTATION]
    if typ != TIFF_SHORT or count != 1:
        raise Error("Orientation must be SHORT count=1")

    vals, value_offset = values(
        source, endian, u16, u32, entries[TAG_ORIENTATION]
    )
    original = vals[0]
    if original != 9:
        raise Error(
            "v0.1 only quarantines observed invalid Orientation=9, "
            f"got {original}"
        )

    payload_sha, payload_bytes, ranges, _, _ = payload_digest(source)
    if any(
        start <= value_offset < end
        or start <= value_offset + 1 < end
        for start, end in ranges
    ):
        raise Error("Orientation storage overlaps CFA payload")

    out = bytearray(source)
    struct.pack_into(endian + "H", out, value_offset, 1)
    out = bytes(out)

    diffs = [
        i for i, (before, after) in enumerate(zip(source, out))
        if before != after
    ]
    if len(source) != len(out) or len(diffs) != 1:
        raise Error(
            f"exactly one changed byte required, got {len(diffs)}"
        )

    out_payload_sha, out_payload_bytes, _, _, _ = payload_digest(out)
    if (
        out_payload_sha != payload_sha
        or out_payload_bytes != payload_bytes
    ):
        raise Error("CFA payload changed")

    derived.write_bytes(out)

    doc = {
        "schema": "D.RAW/DngOrientationQuarantine/0.1",
        "status": "DERIVED_INGRESS_CONTAINER_NO_NEW_EVIDENCE",
        "parent_source": {
            "sha256": hashlib.sha256(source).hexdigest(),
            "byte_length": len(source),
            "immutable": True,
        },
        "source_issue": {
            "tag": "Orientation",
            "tag_code": 274,
            "source_value": 9,
            "tiff_defined_values_min": 1,
            "tiff_defined_values_max": 8,
            "source_value_standard_defined": False,
            "cfa_ifd_offset": ifd,
            "orientation_value_field_offset": value_offset,
        },
        "normalization": {
            "method_id":
                "D_RAW_DNG_INVALID_ORIENTATION_9_TO_STORAGE_COORDINATE_1_V0_1",
            "derived_value": 1,
            "meaning": "STORAGE_COORDINATE_NORMAL_ONLY",
            "presentation_orientation_authority": "UNKNOWN",
            "world_orientation_claimed": False,
            "changed_byte_count": 1,
            "changed_byte_offsets": diffs,
            "source_byte_hex": source[diffs[0] : diffs[0] + 1].hex(),
            "derived_byte_hex": out[diffs[0] : diffs[0] + 1].hex(),
        },
        "serialized_cfa_payload": {
            "parent_sha256": payload_sha,
            "derived_sha256": out_payload_sha,
            "byte_length": payload_bytes,
            "byte_identical": True,
        },
        "derived_ingress": {
            "sha256": hashlib.sha256(out).hexdigest(),
            "byte_length": len(out),
            "container_role":
                "SCIENTIFIC_STORAGE_COORDINATE_INGRESS_ONLY",
        },
        "lineage": {
            "same_physical_observation": True,
            "parent_source_mutated": False,
            "cfa_payload_mutated": False,
            "physical_frame_count_increment": 0,
            "independent_evidence_count_increment": 0,
            "creates_new_evidence": False,
            "scientific_writeback_allowed": False,
            "appearance_orientation_claim_allowed": False,
        },
    }
    doc["manifest_sha256"] = canonical_hash(doc)

    errors = validate_manifest_only(doc)
    if errors:
        raise Error("|".join(errors))

    manifest.write_text(
        json.dumps(doc, indent=2) + "\n", encoding="utf-8"
    )
    return doc


def validate(parent: Path, derived: Path, manifest: Path):
    doc = json.loads(manifest.read_text(encoding="utf-8"))
    source = parent.read_bytes()
    out = derived.read_bytes()
    errors = validate_manifest_only(doc)

    if doc.get("parent_source", {}).get("sha256") != hashlib.sha256(
        source
    ).hexdigest():
        errors.append("parent_sha")
    if doc.get("derived_ingress", {}).get("sha256") != hashlib.sha256(
        out
    ).hexdigest():
        errors.append("derived_sha")
    if len(source) != len(out):
        errors.append("length")

    diffs = [
        i for i, (before, after) in enumerate(zip(source, out))
        if before != after
    ]
    if (
        diffs != doc.get("normalization", {}).get("changed_byte_offsets")
        or len(diffs) != 1
    ):
        errors.append("diffs")

    parent_sha, parent_bytes, _, _, _ = payload_digest(source)
    derived_sha, derived_bytes, _, _, _ = payload_digest(out)
    if parent_sha != derived_sha or parent_bytes != derived_bytes:
        errors.append("payload")
    if (
        doc.get("serialized_cfa_payload", {}).get("parent_sha256")
        != parent_sha
        or doc.get("serialized_cfa_payload", {}).get("derived_sha256")
        != derived_sha
    ):
        errors.append("payload_manifest")

    if errors:
        raise Error("|".join(errors))
    return doc


def main():
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("normalize")
    p.add_argument("parent")
    p.add_argument("derived")
    p.add_argument("manifest")

    p = sub.add_parser("validate")
    p.add_argument("parent")
    p.add_argument("derived")
    p.add_argument("manifest")

    p = sub.add_parser("validate-manifest")
    p.add_argument("manifest")

    args = parser.parse_args()
    try:
        if args.cmd == "normalize":
            doc = normalize(
                Path(args.parent),
                Path(args.derived),
                Path(args.manifest),
            )
            print("DRAW_DNG_ORIENTATION_QUARANTINE_V01_CREATED")
        elif args.cmd == "validate":
            doc = validate(
                Path(args.parent),
                Path(args.derived),
                Path(args.manifest),
            )
            print("DRAW_DNG_ORIENTATION_QUARANTINE_V01_PASS")
        else:
            doc = json.loads(
                Path(args.manifest).read_text(encoding="utf-8")
            )
            errors = validate_manifest_only(doc)
            if errors:
                raise Error("|".join(errors))
            print("DRAW_DNG_ORIENTATION_QUARANTINE_V01_MANIFEST_PASS")

        print("manifest_sha256=" + doc["manifest_sha256"])
        print("parent_sha256=" + doc["parent_source"]["sha256"])
        print("derived_sha256=" + doc["derived_ingress"]["sha256"])
        print(
            "cfa_payload_sha256="
            + doc["serialized_cfa_payload"]["parent_sha256"]
        )
    except Error as exc:
        print("DRAW_DNG_ORIENTATION_QUARANTINE_V01_FAIL")
        print(str(exc))
        raise SystemExit(1)


if __name__ == "__main__":
    main()
