#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re

HEX64 = re.compile(r"^[0-9a-f]{64}$")

def sha_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()

def canonical_hash(doc: dict, key: str) -> str:
    x = dict(doc)
    x.pop(key, None)
    return hashlib.sha256(
        json.dumps(
            x,
            sort_keys=True,
            separators=(",", ":"),
            ensure_ascii=False,
            allow_nan=False,
        ).encode("utf-8")
    ).hexdigest()

def write_json(path: str, doc: dict) -> None:
    Path(path).write_text(json.dumps(doc, indent=2) + "\n", encoding="utf-8")

def admission_semantics(adm: dict, expected_source: str, label: str) -> dict:
    errors = []
    schema = adm.get("schema")
    if not isinstance(schema, str) or not schema.startswith("D.RAW/SourceAdmissionPackage/"):
        errors.append(label + ".schema")
    if adm.get("status") != "ADMITTED_SOURCE_LOCAL":
        errors.append(label + ".status")

    source = adm.get("physical_source_evidence_sha256")
    if source != expected_source:
        errors.append(label + ".source_sha256")

    obs = adm.get("physical_observation_id")
    if not isinstance(obs, str) or not obs:
        errors.append(label + ".physical_observation_id")

    counts = adm.get("evidence_counts") or {}
    if counts != {"physical_frame_count": 1, "independent_evidence_count": 1}:
        errors.append(label + ".evidence_counts")

    if adm.get("graph_relations") != []:
        errors.append(label + ".graph_relations")
    if adm.get("fusion_admissions") != []:
        errors.append(label + ".fusion_admissions")

    inv = adm.get("invariants") or {}
    if inv.get("scientific_writeback_allowed") is not False:
        errors.append(label + ".scientific_writeback")
    if inv.get("cross_observation_relation_granted") is not False:
        errors.append(label + ".relation_grant")
    if inv.get("cross_observation_fusion_granted") is not False:
        errors.append(label + ".fusion_grant")

    state = adm.get("state_sha256")
    if HEX64.fullmatch(str(state or "")) is None:
        errors.append(label + ".state_sha256")

    if errors:
        raise SystemExit("\n".join(errors))
    return {
        "schema": schema,
        "state_sha256": state,
        "physical_observation_id": obs,
        "physical_source_evidence_sha256": source,
    }

def seal(args) -> None:
    main = Path(args.main_file)
    wide = Path(args.ultrawide_file)
    if not main.is_file() or not wide.is_file():
        raise SystemExit("both MAIN and ULTRA_WIDE files must exist")
    if main.resolve() == wide.resolve():
        raise SystemExit("MAIN and ULTRA_WIDE must be distinct files")
    if not args.attest_camera_system_rigid:
        raise SystemExit("operator attestation required: camera system rigid")
    if not args.attest_target_static_between_pair:
        raise SystemExit("operator attestation required: target static between pair")

    main_sha = sha_file(main)
    wide_sha = sha_file(wide)
    if main_sha == wide_sha:
        raise SystemExit("MAIN and ULTRA_WIDE source hashes must differ")

    doc = {
        "schema": "D.RAW/GeometryDirectRawPair/0.1",
        "status": "SEALED_PAIR_AWAITING_SOURCE_LOCAL_ADMISSION",
        "campaign_id": args.campaign_id,
        "pose_id": args.pose_id,
        "subset": args.subset,
        "target": {
            "target_geometry_sha256": args.target_geometry_sha256,
            "target_family": args.target_family,
            "metric_spacing_m": args.metric_spacing_m,
        },
        "sources": {
            "main": {
                "role": "MAIN",
                "display_name": main.name,
                "sha256": main_sha,
                "byte_length": main.stat().st_size,
                "sealed": True,
            },
            "ultra_wide": {
                "role": "ULTRA_WIDE",
                "display_name": wide.name,
                "sha256": wide_sha,
                "byte_length": wide.stat().st_size,
                "sealed": True,
            },
        },
        "capture_pair_attestation": {
            "authority": "OPERATOR_ATTESTATION_ONLY",
            "camera_system_rigid_between_pair": True,
            "target_static_between_pair": True,
            "device_pose_changed_between_pair": False,
            "target_pose_changed_between_pair": False,
            "attestation_creates_measured_geometry": False,
        },
        "source_local_admissions": {
            "main": None,
            "ultra_wide": None,
        },
        "geometry_readiness": {
            "pair_source_bytes_sealed": True,
            "both_sources_admitted_source_local": False,
            "feature_extraction_allowed": False,
            "geometry_fit_allowed": False,
            "relation_status": "UNKNOWN",
            "coordinate_transform_allowed": False,
            "fusion_allowed": False,
        },
        "open_world_evolution": {
            "law": "SEAL_EVIDENCE_NOT_THINKING",
            "source_bytes_immutable": True,
            "interpretation_model_mutable_via_versioned_successor": True,
            "future_extensions_allowed": True,
        },
        "extensions": {},
        "invariants": {
            "source_bytes_mutated": False,
            "creates_new_evidence": False,
            "scientific_writeback_allowed": False,
            "geometry_relation_granted": False,
            "fusion_granted": False,
            "calibration_transfer_granted": False,
            "appearance_writeback_allowed": False,
        },
    }
    doc["pair_state_sha256"] = canonical_hash(doc, "pair_state_sha256")
    write_json(args.output, doc)
    print("DRAW_GEOMETRY_DIRECT_RAW_PAIR_SEALED")
    print("status=" + doc["status"])
    print("main_sha256=" + main_sha)
    print("ultrawide_sha256=" + wide_sha)
    print("pair_state_sha256=" + doc["pair_state_sha256"])

def bind(args) -> None:
    pair = json.loads(Path(args.pair).read_text(encoding="utf-8"))
    if pair.get("schema") != "D.RAW/GeometryDirectRawPair/0.1":
        raise SystemExit("pair schema")
    if pair.get("status") != "SEALED_PAIR_AWAITING_SOURCE_LOCAL_ADMISSION":
        raise SystemExit("pair status must be awaiting source-local admission")

    main_adm = json.loads(Path(args.main_admission).read_text(encoding="utf-8"))
    wide_adm = json.loads(Path(args.ultrawide_admission).read_text(encoding="utf-8"))

    main_sha = pair["sources"]["main"]["sha256"]
    wide_sha = pair["sources"]["ultra_wide"]["sha256"]
    main_bound = admission_semantics(main_adm, main_sha, "main")
    wide_bound = admission_semantics(wide_adm, wide_sha, "ultra_wide")

    pair["status"] = "ADMISSION_BOUND_PAIR_READY_FOR_FEATURE_EXTRACTION"
    pair["source_local_admissions"] = {
        "main": main_bound,
        "ultra_wide": wide_bound,
    }
    pair["geometry_readiness"] = {
        "pair_source_bytes_sealed": True,
        "both_sources_admitted_source_local": True,
        "feature_extraction_allowed": True,
        "geometry_fit_allowed": False,
        "relation_status": "UNKNOWN",
        "coordinate_transform_allowed": False,
        "fusion_allowed": False,
    }
    pair["pair_state_sha256"] = canonical_hash(pair, "pair_state_sha256")
    write_json(args.output, pair)
    print("DRAW_GEOMETRY_DIRECT_RAW_PAIR_ADMISSION_BOUND")
    print("status=" + pair["status"])
    print("relation_status=UNKNOWN")
    print("geometry_fit_allowed=false")
    print("pair_state_sha256=" + pair["pair_state_sha256"])

def main() -> None:
    p = argparse.ArgumentParser()
    sub = p.add_subparsers(dest="cmd", required=True)

    s = sub.add_parser("seal")
    s.add_argument("--campaign-id", default="DRAW_GEOMETRY_MAIN_ULTRAWIDE_2026_09_27_V0_1")
    s.add_argument("--pose-id", required=True)
    s.add_argument("--subset", required=True, choices=["TRAINING", "HOLDOUT"])
    s.add_argument("--main-file", required=True)
    s.add_argument("--ultrawide-file", required=True)
    s.add_argument("--target-geometry-sha256", required=True)
    s.add_argument("--target-family", required=True)
    s.add_argument("--metric-spacing-m", required=True, type=float)
    s.add_argument("--attest-camera-system-rigid", action="store_true")
    s.add_argument("--attest-target-static-between-pair", action="store_true")
    s.add_argument("--output", required=True)
    s.set_defaults(func=seal)

    b = sub.add_parser("bind")
    b.add_argument("--pair", required=True)
    b.add_argument("--main-admission", required=True)
    b.add_argument("--ultrawide-admission", required=True)
    b.add_argument("--output", required=True)
    b.set_defaults(func=bind)

    args = p.parse_args()
    if getattr(args, "metric_spacing_m", 1.0) <= 0:
        raise SystemExit("metric spacing must be positive")
    if getattr(args, "target_geometry_sha256", None) is not None and HEX64.fullmatch(args.target_geometry_sha256.lower()) is None:
        raise SystemExit("target geometry SHA-256 must be 64 hex chars")
    args.func(args)

if __name__ == "__main__":
    main()
