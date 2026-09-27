#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

def canonical_hash(value) -> str:
    return hashlib.sha256(
        json.dumps(
            value,
            sort_keys=True,
            separators=(",", ":"),
            ensure_ascii=False,
            allow_nan=False,
        ).encode("utf-8")
    ).hexdigest()

def validate_pair(path: Path) -> dict:
    validator = Path(__file__).with_name("validate_draw_geometry_direct_raw_pair_v01.py")
    cp = subprocess.run(
        [sys.executable, str(validator), str(path)],
        capture_output=True,
        text=True,
    )
    if cp.returncode != 0:
        raise SystemExit(
            "pair validation failed for "
            + str(path)
            + ": "
            + (cp.stdout.strip().replace("\n", "|") or cp.stderr.strip().replace("\n", "|"))
        )
    doc = json.loads(path.read_text(encoding="utf-8"))
    if doc.get("status") != "ADMISSION_BOUND_PAIR_READY_FOR_FEATURE_EXTRACTION":
        raise SystemExit("pair is not admission-bound: " + str(path))
    return doc

def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("--campaign", required=True)
    p.add_argument("--pair", action="append", required=True)
    p.add_argument("--output", required=True)
    a = p.parse_args()

    campaign = json.loads(Path(a.campaign).read_text(encoding="utf-8"))
    if campaign.get("schema") != "D.RAW/GeometryRelationCampaign/0.1":
        raise SystemExit("campaign schema")
    if campaign.get("status") != "PENDING_MATCHED_SCENE_CAPTURE":
        raise SystemExit("campaign status")

    protocol = campaign.get("capture_protocol") or {}
    training_min = int(protocol.get("training_pose_minimum", 0))
    holdout_min = int(protocol.get("holdout_pose_minimum", 0))
    if training_min < 1 or holdout_min < 1:
        raise SystemExit("campaign minimum counts invalid")

    docs = [validate_pair(Path(x)) for x in a.pair]
    if not docs:
        raise SystemExit("no pairs")

    campaign_id = campaign.get("campaign_id")
    pose_ids = set()
    main_sources = set()
    wide_sources = set()
    training = []
    holdout = []
    target_ref = None

    for d in docs:
        if d.get("campaign_id") != campaign_id:
            raise SystemExit("pair campaign mismatch")
        pose = d.get("pose_id")
        if pose in pose_ids:
            raise SystemExit("duplicate pose_id: " + str(pose))
        pose_ids.add(pose)

        target = d.get("target") or {}
        normalized_target = {
            "target_geometry_sha256": target.get("target_geometry_sha256"),
            "target_family": target.get("target_family"),
            "metric_spacing_m": target.get("metric_spacing_m"),
        }
        if target_ref is None:
            target_ref = normalized_target
        elif normalized_target != target_ref:
            raise SystemExit("target identity/geometry mismatch across pairs")

        sources = d.get("sources") or {}
        main_sha = (sources.get("main") or {}).get("sha256")
        wide_sha = (sources.get("ultra_wide") or {}).get("sha256")
        if main_sha in main_sources:
            raise SystemExit("MAIN source reused: " + str(main_sha))
        if wide_sha in wide_sources:
            raise SystemExit("ULTRA_WIDE source reused: " + str(wide_sha))
        main_sources.add(main_sha)
        wide_sources.add(wide_sha)

        entry = {
            "pose_id": pose,
            "pair_state_sha256": d.get("pair_state_sha256"),
            "main_source_sha256": main_sha,
            "ultrawide_source_sha256": wide_sha,
            "main_physical_observation_id": (d.get("source_local_admissions") or {}).get("main", {}).get("physical_observation_id"),
            "ultrawide_physical_observation_id": (d.get("source_local_admissions") or {}).get("ultra_wide", {}).get("physical_observation_id"),
        }
        subset = d.get("subset")
        if subset == "TRAINING":
            training.append(entry)
        elif subset == "HOLDOUT":
            holdout.append(entry)
        else:
            raise SystemExit("invalid subset")

    if len(training) < training_min:
        raise SystemExit(f"insufficient training pairs: {len(training)} < {training_min}")
    if len(holdout) < holdout_min:
        raise SystemExit(f"insufficient holdout pairs: {len(holdout)} < {holdout_min}")

    training.sort(key=lambda x: x["pose_id"])
    holdout.sort(key=lambda x: x["pose_id"])

    training_identity = {
        "campaign_id": campaign_id,
        "target": target_ref,
        "subset": "TRAINING",
        "members": [x["pair_state_sha256"] for x in training],
    }
    holdout_identity = {
        "campaign_id": campaign_id,
        "target": target_ref,
        "subset": "HOLDOUT",
        "members": [x["pair_state_sha256"] for x in holdout],
    }

    result = {
        "schema": "D.RAW/GeometryRelationCaptureSet/0.2",
        "status": "CAPTURE_SET_COMPLETE_TRAINING_MODEL_SELECTION_OPEN",
        "campaign_id": campaign_id,
        "target": target_ref,
        "requirements": {
            "training_pose_minimum": training_min,
            "holdout_pose_minimum": holdout_min,
        },
        "training": {
            "pair_count": len(training),
            "pairs": training,
            "set_sha256": canonical_hash(training_identity),
        },
        "holdout": {
            "pair_count": len(holdout),
            "pairs": holdout,
            "set_sha256": canonical_hash(holdout_identity),
        },
        "model_selection_gate": {
            "training_feature_extraction_allowed": True,
            "training_model_selection_allowed": True,
            "holdout_feature_extraction_may_be_prepared": True,
            "holdout_model_selection_allowed": False,
            "final_holdout_scoring_allowed": False,
            "reason_final_holdout_closed": "MODEL_AND_THRESHOLDS_NOT_YET_FROZEN_BY_LATER_ARTIFACT",
        },
        "relation_authority": {
            "axis": "GEOMETRY",
            "status": "UNKNOWN",
            "coordinate_transform_allowed": False,
            "equality_allowed": False,
            "fusion_allowed": False,
            "calibration_transfer_allowed": False,
        },
        "open_world_evolution": {
            "law": "SEAL_EVIDENCE_SPLIT_NOT_MODEL_HYPOTHESIS",
            "membership_is_sealed": True,
            "projection_model_selected": False,
            "distortion_model_selected": False,
            "feature_detector_selected": False,
            "future_model_successors_allowed": True,
        },
        "extensions": {},
        "invariants": {
            "source_evidence_mutated": False,
            "pair_membership_rewritten": False,
            "creates_new_evidence": False,
            "scientific_writeback_allowed": False,
            "geometry_relation_granted": False,
            "fusion_granted": False,
            "calibration_transfer_granted": False,
        },
    }
    result["capture_set_state_sha256"] = canonical_hash(result)
    Path(a.output).write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")

    print("DRAW_GEOMETRY_CAPTURE_SET_GATE_V01_PASS")
    print("status=" + result["status"])
    print("training_count=" + str(len(training)))
    print("holdout_count=" + str(len(holdout)))
    print("training_set_sha256=" + result["training"]["set_sha256"])
    print("holdout_set_sha256=" + result["holdout"]["set_sha256"])
    print("capture_set_state_sha256=" + result["capture_set_state_sha256"])
    print("geometry_relation_status=UNKNOWN")

if __name__ == "__main__":
    main()
