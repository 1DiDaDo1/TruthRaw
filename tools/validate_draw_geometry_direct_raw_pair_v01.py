#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import sys

HEX64 = re.compile(r"^[0-9a-f]{64}$")

def digest(doc):
    x = dict(doc)
    x.pop("pair_state_sha256", None)
    return hashlib.sha256(
        json.dumps(
            x,
            sort_keys=True,
            separators=(",", ":"),
            ensure_ascii=False,
            allow_nan=False,
        ).encode("utf-8")
    ).hexdigest()

def fail(errors):
    print("DRAW_GEOMETRY_DIRECT_RAW_PAIR_V01_FAIL")
    for e in errors:
        print(e)
    raise SystemExit(1)

def main():
    if len(sys.argv) != 2:
        fail(["usage: validate_draw_geometry_direct_raw_pair_v01.py PAIR.json"])
    try:
        d = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
    except Exception as exc:
        fail(["json:" + str(exc)])

    errors = []
    if d.get("schema") != "D.RAW/GeometryDirectRawPair/0.1":
        errors.append("schema")
    if d.get("status") not in {
        "SEALED_PAIR_AWAITING_SOURCE_LOCAL_ADMISSION",
        "ADMISSION_BOUND_PAIR_READY_FOR_FEATURE_EXTRACTION",
    }:
        errors.append("status")
    if d.get("subset") not in {"TRAINING", "HOLDOUT"}:
        errors.append("subset")
    if not isinstance(d.get("pose_id"), str) or not d.get("pose_id"):
        errors.append("pose_id")

    target = d.get("target") or {}
    if HEX64.fullmatch(str(target.get("target_geometry_sha256", ""))) is None:
        errors.append("target.sha256")
    if not isinstance(target.get("target_family"), str) or not target.get("target_family"):
        errors.append("target.family")
    if not isinstance(target.get("metric_spacing_m"), (int, float)) or target.get("metric_spacing_m", 0) <= 0:
        errors.append("target.spacing")

    sources = d.get("sources") or {}
    seen = set()
    for key, role in (("main", "MAIN"), ("ultra_wide", "ULTRA_WIDE")):
        src = sources.get(key) or {}
        if src.get("role") != role:
            errors.append(key + ".role")
        sha = src.get("sha256")
        if HEX64.fullmatch(str(sha or "")) is None:
            errors.append(key + ".sha256")
        elif sha in seen:
            errors.append("source_hash_collision")
        else:
            seen.add(sha)
        if src.get("sealed") is not True:
            errors.append(key + ".sealed")
        if not isinstance(src.get("byte_length"), int) or src.get("byte_length", 0) <= 0:
            errors.append(key + ".bytes")

    att = d.get("capture_pair_attestation") or {}
    if att.get("authority") != "OPERATOR_ATTESTATION_ONLY":
        errors.append("attestation.authority")
    for k, v in {
        "camera_system_rigid_between_pair": True,
        "target_static_between_pair": True,
        "device_pose_changed_between_pair": False,
        "target_pose_changed_between_pair": False,
        "attestation_creates_measured_geometry": False,
    }.items():
        if att.get(k) != v:
            errors.append("attestation." + k)

    ready = d.get("geometry_readiness") or {}
    if ready.get("pair_source_bytes_sealed") is not True:
        errors.append("readiness.sealed")
    if ready.get("geometry_fit_allowed") is not False:
        errors.append("readiness.fit")
    if ready.get("relation_status") != "UNKNOWN":
        errors.append("readiness.relation")
    if ready.get("coordinate_transform_allowed") is not False:
        errors.append("readiness.transform")
    if ready.get("fusion_allowed") is not False:
        errors.append("readiness.fusion")

    admissions = d.get("source_local_admissions") or {}
    if d.get("status") == "SEALED_PAIR_AWAITING_SOURCE_LOCAL_ADMISSION":
        if admissions.get("main") is not None or admissions.get("ultra_wide") is not None:
            errors.append("premature_admission_binding")
        if ready.get("both_sources_admitted_source_local") is not False:
            errors.append("premature_ready")
        if ready.get("feature_extraction_allowed") is not False:
            errors.append("premature_features")
    else:
        for key in ("main", "ultra_wide"):
            adm = admissions.get(key) or {}
            if not isinstance(adm.get("schema"), str) or not adm["schema"].startswith("D.RAW/SourceAdmissionPackage/"):
                errors.append("bound." + key + ".schema")
            if HEX64.fullmatch(str(adm.get("state_sha256", ""))) is None:
                errors.append("bound." + key + ".state")
            if not isinstance(adm.get("physical_observation_id"), str) or not adm.get("physical_observation_id"):
                errors.append("bound." + key + ".obs")
            expected_sha = sources.get(key, {}).get("sha256")
            if adm.get("physical_source_evidence_sha256") != expected_sha:
                errors.append("bound." + key + ".source")
        if ready.get("both_sources_admitted_source_local") is not True:
            errors.append("bound.ready")
        if ready.get("feature_extraction_allowed") is not True:
            errors.append("bound.features")

    evo = d.get("open_world_evolution") or {}
    if evo.get("law") != "SEAL_EVIDENCE_NOT_THINKING":
        errors.append("evolution.law")
    if evo.get("source_bytes_immutable") is not True:
        errors.append("evolution.source")
    if evo.get("interpretation_model_mutable_via_versioned_successor") is not True:
        errors.append("evolution.interpretation")
    if evo.get("future_extensions_allowed") is not True:
        errors.append("evolution.extensions")
    if not isinstance(d.get("extensions"), dict):
        errors.append("extensions")

    inv = d.get("invariants") or {}
    for k, v in {
        "source_bytes_mutated": False,
        "creates_new_evidence": False,
        "scientific_writeback_allowed": False,
        "geometry_relation_granted": False,
        "fusion_granted": False,
        "calibration_transfer_granted": False,
        "appearance_writeback_allowed": False,
    }.items():
        if inv.get(k) != v:
            errors.append("invariant." + k)

    if d.get("pair_state_sha256") != digest(d):
        errors.append("pair_state_sha256:" + digest(d))

    if errors:
        fail(errors)

    print("DRAW_GEOMETRY_DIRECT_RAW_PAIR_V01_PASS")
    print("status=" + d["status"])
    print("relation_status=UNKNOWN")
    print("geometry_fit_allowed=false")
    print("seal_evidence_not_thinking=true")

if __name__ == "__main__":
    main()
