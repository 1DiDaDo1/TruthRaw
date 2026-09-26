#!/usr/bin/env python3
"""Fail-closed validator for D.RAW Observation Record v0.1."""

from __future__ import annotations

import json
from pathlib import Path
import re
import sys

HEX64 = re.compile(r"^[0-9a-f]{64}$")


def fail(errors: list[str]) -> None:
    print("DRAW_OBSERVATION_RECORD_V01_FAIL")
    for error in errors:
        print(error)
    raise SystemExit(1)


def validate(record: dict) -> list[str]:
    e: list[str] = []

    if record.get("schema") != "D.RAW/DRAWObservationRecord/0.1":
        e.append("schema")

    obs_id = record.get("observation_id")
    if not isinstance(obs_id, str) or not obs_id:
        e.append("observation_id")

    source = record.get("source_evidence") or {}
    if not HEX64.fullmatch(str(source.get("sha256", ""))):
        e.append("source_sha256")
    if source.get("role") != "SEALED_SOURCE_EVIDENCE":
        e.append("source_role")
    if source.get("sealed") is not True:
        e.append("source_not_sealed")
    if not source.get("physical_frame_id"):
        e.append("physical_frame_id")

    topology = record.get("source_topology") or {}
    if not isinstance(topology.get("width"), int) or topology.get("width", 0) <= 0:
        e.append("source_width")
    if not isinstance(topology.get("height"), int) or topology.get("height", 0) <= 0:
        e.append("source_height")

    capabilities = record.get("capability_envelope") or {}
    for key in (
        "sampling_geometry", "radiometry", "color", "noise_uncertainty",
        "optics", "geometry_projection", "dynamic_range_bounds",
        "temporal_state", "calibration_scope", "metadata_completeness",
    ):
        if not isinstance(capabilities.get(key), str) or not capabilities.get(key):
            e.append(f"capability:{key}")

    gauge = record.get("gauge") or {}
    if gauge.get("truthrange_formula") != "T=log2(L/L0)":
        e.append("truthrange_formula")
    if not gauge.get("scale_gauge_id"):
        e.append("scale_gauge_id")
    shared = gauge.get("shared_free_world_gauge_id")
    relation = gauge.get("gauge_binding_authority")
    equality = gauge.get("cross_observation_radiometric_equality_allowed")
    fusion = gauge.get("cross_observation_radiometric_fusion_allowed")
    if shared is None:
        if equality is not False:
            e.append("unproven_common_gauge_equality_must_be_false")
        if fusion is not False:
            e.append("unproven_common_gauge_fusion_must_be_false")
    if relation in ("SOURCE_LOCAL_ONLY", "UNKNOWN"):
        if equality is not False or fusion is not False:
            e.append("local_or_unknown_gauge_cannot_enable_cross_observation_radiometry")

    precision = record.get("precision") or {}
    if precision.get("branch_sensitive_compute") != "FLOAT64":
        e.append("branch_sensitive_compute")
    if precision.get("canonical_storage") not in ("FLOAT32_VALIDATED", "FLOAT64"):
        e.append("canonical_storage")
    if precision.get("precision_upgrades_authority") is not False:
        e.append("precision_authority")

    master = record.get("scientific_master") or {}
    if not HEX64.fullmatch(str(master.get("sha256", ""))):
        e.append("scientific_master_sha256")
    if master.get("role") != "SCIENTIFIC_MASTER":
        e.append("scientific_master_role")

    tn = record.get("truthnegative") or {}
    if not HEX64.fullmatch(str(tn.get("state_sha256", ""))):
        e.append("truthnegative_sha256")
    if tn.get("role") != "TRUTHNEGATIVE_CONTINUOUS_PER_OBSERVATION":
        e.append("truthnegative_role")
    if tn.get("source_observation_id") != obs_id:
        e.append("truthnegative_observation_binding")
    if tn.get("raster_independent") is not True:
        e.append("truthnegative_not_raster_independent")
    if tn.get("target_raster_part_of_state_identity") is not False:
        e.append("target_raster_identity")
    if tn.get("measured_target_claim_count") != 0:
        e.append("measured_target_claim_count")

    counts = record.get("evidence_counts") or {}
    if counts.get("physical_frame_count") != 1:
        e.append("physical_frame_count")
    if counts.get("independent_evidence_count") != 1:
        e.append("independent_evidence_count")

    axes = record.get("authority_axes") or {}
    if axes.get("axes_are_separate") is not True:
        e.append("authority_axes_not_separate")
    if not axes.get("geometry_authority"):
        e.append("geometry_authority")
    if not axes.get("radiometric_authority"):
        e.append("radiometric_authority")

    inv = record.get("invariants") or {}
    for key in (
        "creates_new_evidence",
        "scientific_writeback_allowed",
        "appearance_writeback_allowed",
        "lens_identity_upgrades_authority",
        "file_format_upgrades_authority",
    ):
        if inv.get(key) is not False:
            e.append(f"invariant_false:{key}")

    optics = capabilities.get("optics", "")
    if optics.startswith("UNKNOWN") and inv.get("optics_scientific_use_allowed") is not False:
        e.append("unknown_optics_scientific_use")

    return e


def main() -> None:
    if len(sys.argv) != 2:
        print("usage: validate_draw_observation_record_v01.py RECORD.json")
        raise SystemExit(2)

    path = Path(sys.argv[1])
    try:
        record = json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:
        fail([f"json:{exc}"])

    errors = validate(record)
    if errors:
        fail(errors)

    print("DRAW_OBSERVATION_RECORD_V01_PASS")
    print(f"observation_id={record['observation_id']}")
    print(f"source_sha256={record['source_evidence']['sha256']}")
    print(f"lens_role={record['procedure']['lens_role']}")
    print(f"scale_gauge_id={record['gauge']['scale_gauge_id']}")
    print("cross_observation_radiometric_fusion=false")
    print("truthnegative_per_observation=true")


if __name__ == "__main__":
    main()
