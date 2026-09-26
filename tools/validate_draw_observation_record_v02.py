#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import struct
import sys

HEX64 = re.compile(r"^[0-9a-f]{64}$")


def fail(*items: str) -> None:
    print("DRAW_OBSERVATION_RECORD_V02_FAIL")
    for item in items:
        print(item)
    raise SystemExit(1)


def hash_string(h: "hashlib._Hash", value: str) -> None:
    data = value.encode("utf-8")
    h.update(struct.pack("<Q", len(data)))
    h.update(data)


def expected_drawnegative_state(record: dict) -> str:
    parent = bytes.fromhex(
        record["legacy_truthnegative_parent"]["state_sha256"]
    )
    gauge = record["gauge"]
    relation = {
        "SOURCE_LOCAL_ONLY": 0,
        "SHARED_RELATIVE": 1,
        "SHARED_ABSOLUTE": 2,
    }[gauge["gauge_relation"]]
    storage = {
        "FLOAT32_VALIDATED": 0,
        "FLOAT64": 1,
    }[record["precision"]["canonical_storage"]]

    h = hashlib.sha256()
    h.update(b"D_RAW_NEGATIVE_OBSERVATION_BOUND_STATE_V0_1")
    h.update(parent)
    hash_string(h, record["observation_id"])
    hash_string(h, gauge["scale_gauge_id"])
    hash_string(h, gauge["shared_free_world_gauge_id"] or "")
    h.update(bytes([relation]))
    h.update(bytes([storage]))
    h.update(bytes([1]))
    h.update(bytes([
        1 if record["drawnegative"]["per_sample_truthrange_materialized"]
        else 0
    ]))
    return h.hexdigest()


def main() -> None:
    if len(sys.argv) != 2:
        fail("usage")

    try:
        r = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
    except Exception as exc:
        fail(f"json:{exc}")

    errors: list[str] = []
    if r.get("schema") != "D.RAW/DRAWObservationRecord/0.2":
        errors.append("schema")

    obs = r.get("observation_id")
    source = (r.get("source_evidence") or {}).get("sha256")
    master = (r.get("scientific_master") or {}).get("sha256")
    authority = (r.get("authority_field") or {}).get("sha256")
    parent = (r.get("legacy_truthnegative_parent") or {}).get("state_sha256")
    dn = r.get("drawnegative") or {}
    gauge = r.get("gauge") or {}

    for name, value in (
        ("source", source), ("master", master),
        ("authority", authority), ("parent", parent),
        ("drawnegative", dn.get("state_sha256")),
    ):
        if not isinstance(value, str) or not HEX64.fullmatch(value):
            errors.append(f"{name}_sha256")

    if dn.get("schema") != "D.RAW/D.RAWnegative/0.1":
        errors.append("drawnegative_schema")
    if dn.get("parent_truthnegative_state_sha256") != parent:
        errors.append("parent_binding")
    if dn.get("observation_id") != obs:
        errors.append("observation_binding")
    if dn.get("per_observation_lineage") is not True:
        errors.append("per_observation")
    if dn.get("raster_independent") is not True:
        errors.append("raster_independent")
    if dn.get("truthrange_coordinate_family_declared") is not True:
        errors.append("truthrange_declared")
    if dn.get("creates_new_evidence") is not False:
        errors.append("new_evidence")
    if dn.get("scientific_writeback_allowed") is not False:
        errors.append("writeback")

    if gauge.get("truthrange_formula") != "T=log2(L/L0)":
        errors.append("gauge_formula")
    relation = gauge.get("gauge_relation")
    shared = gauge.get("shared_free_world_gauge_id")
    equality = gauge.get("cross_observation_radiometric_equality_allowed")
    fusion = gauge.get("cross_observation_radiometric_fusion_allowed")

    if relation == "SOURCE_LOCAL_ONLY":
        if shared is not None or equality is not False or fusion is not False:
            errors.append("source_local_gauge_gate")
    elif relation in ("SHARED_RELATIVE", "SHARED_ABSOLUTE"):
        if not isinstance(shared, str) or not shared or equality is not True or fusion is not True:
            errors.append("shared_gauge_gate")
    else:
        errors.append("gauge_relation")

    if (r.get("precision") or {}).get("branch_sensitive_compute") != "FLOAT64":
        errors.append("float64_compute")
    if (r.get("precision") or {}).get("precision_upgrades_authority") is not False:
        errors.append("precision_authority")

    counts = r.get("evidence_counts") or {}
    if counts.get("physical_frame_count") != 1:
        errors.append("physical_frame_count")
    if counts.get("independent_evidence_count") != 1:
        errors.append("independent_evidence_count")

    try:
        expected = expected_drawnegative_state(r)
        if dn.get("state_sha256") != expected:
            errors.append(f"drawnegative_digest:{expected}")
    except Exception as exc:
        errors.append(f"digest:{exc}")

    if errors:
        fail(*errors)

    print("DRAW_OBSERVATION_RECORD_V02_PASS")
    print(f"observation_id={obs}")
    print(f"drawnegative_state_sha256={dn['state_sha256']}")
    print(f"legacy_parent_sha256={parent}")
    print("current_negative=D.RAWnegative")
    print("cross_observation_radiometric_fusion=false")


if __name__ == "__main__":
    main()
