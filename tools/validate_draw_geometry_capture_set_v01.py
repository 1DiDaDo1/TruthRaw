#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import sys

HEX64 = re.compile(r"^[0-9a-f]{64}$")

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

def fail(errors):
    print("DRAW_GEOMETRY_CAPTURE_SET_V01_FAIL")
    for e in errors:
        print(e)
    raise SystemExit(1)

def main():
    if len(sys.argv) != 2:
        fail(["usage: validate_draw_geometry_capture_set_v01.py CAPTURE_SET.json"])
    try:
        d = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
    except Exception as exc:
        fail(["json:" + str(exc)])

    errors = []
    if d.get("schema") != "D.RAW/GeometryRelationCaptureSet/0.2":
        errors.append("schema")
    if d.get("status") != "CAPTURE_SET_COMPLETE_TRAINING_MODEL_SELECTION_OPEN":
        errors.append("status")

    req = d.get("requirements") or {}
    train_min = req.get("training_pose_minimum")
    hold_min = req.get("holdout_pose_minimum")
    train = d.get("training") or {}
    hold = d.get("holdout") or {}
    if not isinstance(train_min, int) or train_min < 1:
        errors.append("training_min")
    if not isinstance(hold_min, int) or hold_min < 1:
        errors.append("holdout_min")
    if train.get("pair_count") != len(train.get("pairs") or []):
        errors.append("training_count")
    if hold.get("pair_count") != len(hold.get("pairs") or []):
        errors.append("holdout_count")
    if isinstance(train_min, int) and train.get("pair_count", 0) < train_min:
        errors.append("training_insufficient")
    if isinstance(hold_min, int) and hold.get("pair_count", 0) < hold_min:
        errors.append("holdout_insufficient")

    all_pose = []
    all_main = []
    all_wide = []
    for subset, block in (("training", train), ("holdout", hold)):
        for item in block.get("pairs") or []:
            if HEX64.fullmatch(str(item.get("pair_state_sha256", ""))) is None:
                errors.append(subset + ".pair_sha")
            if HEX64.fullmatch(str(item.get("main_source_sha256", ""))) is None:
                errors.append(subset + ".main_sha")
            if HEX64.fullmatch(str(item.get("ultrawide_source_sha256", ""))) is None:
                errors.append(subset + ".wide_sha")
            if not isinstance(item.get("pose_id"), str) or not item.get("pose_id"):
                errors.append(subset + ".pose")
            if not isinstance(item.get("main_physical_observation_id"), str) or not item.get("main_physical_observation_id"):
                errors.append(subset + ".main_obs")
            if not isinstance(item.get("ultrawide_physical_observation_id"), str) or not item.get("ultrawide_physical_observation_id"):
                errors.append(subset + ".wide_obs")
            all_pose.append(item.get("pose_id"))
            all_main.append(item.get("main_source_sha256"))
            all_wide.append(item.get("ultrawide_source_sha256"))

    if len(set(all_pose)) != len(all_pose):
        errors.append("duplicate_pose")
    if len(set(all_main)) != len(all_main):
        errors.append("main_source_reuse")
    if len(set(all_wide)) != len(all_wide):
        errors.append("wide_source_reuse")

    target = d.get("target") or {}
    if HEX64.fullmatch(str(target.get("target_geometry_sha256", ""))) is None:
        errors.append("target_sha")
    if not isinstance(target.get("target_family"), str) or not target.get("target_family"):
        errors.append("target_family")
    if not isinstance(target.get("metric_spacing_m"), (int, float)) or target.get("metric_spacing_m", 0) <= 0:
        errors.append("target_spacing")

    if HEX64.fullmatch(str(train.get("set_sha256", ""))) is None:
        errors.append("training_set_sha")
    if HEX64.fullmatch(str(hold.get("set_sha256", ""))) is None:
        errors.append("holdout_set_sha")

    gate = d.get("model_selection_gate") or {}
    expected_gate = {
        "training_feature_extraction_allowed": True,
        "training_model_selection_allowed": True,
        "holdout_feature_extraction_may_be_prepared": True,
        "holdout_model_selection_allowed": False,
        "final_holdout_scoring_allowed": False,
        "reason_final_holdout_closed": "MODEL_AND_THRESHOLDS_NOT_YET_FROZEN_BY_LATER_ARTIFACT",
    }
    for k, v in expected_gate.items():
        if gate.get(k) != v:
            errors.append("gate." + k)

    rel = d.get("relation_authority") or {}
    expected_rel = {
        "axis": "GEOMETRY",
        "status": "UNKNOWN",
        "coordinate_transform_allowed": False,
        "equality_allowed": False,
        "fusion_allowed": False,
        "calibration_transfer_allowed": False,
    }
    for k, v in expected_rel.items():
        if rel.get(k) != v:
            errors.append("relation." + k)

    evo = d.get("open_world_evolution") or {}
    expected_evo = {
        "law": "SEAL_EVIDENCE_SPLIT_NOT_MODEL_HYPOTHESIS",
        "membership_is_sealed": True,
        "projection_model_selected": False,
        "distortion_model_selected": False,
        "feature_detector_selected": False,
        "future_model_successors_allowed": True,
    }
    for k, v in expected_evo.items():
        if evo.get(k) != v:
            errors.append("evolution." + k)
    if not isinstance(d.get("extensions"), dict):
        errors.append("extensions")

    inv = d.get("invariants") or {}
    expected_inv = {
        "source_evidence_mutated": False,
        "pair_membership_rewritten": False,
        "creates_new_evidence": False,
        "scientific_writeback_allowed": False,
        "geometry_relation_granted": False,
        "fusion_granted": False,
        "calibration_transfer_granted": False,
    }
    for k, v in expected_inv.items():
        if inv.get(k) != v:
            errors.append("invariant." + k)

    state = d.get("capture_set_state_sha256")
    x = dict(d)
    x.pop("capture_set_state_sha256", None)
    expected_state = canonical_hash(x)
    if state != expected_state:
        errors.append("capture_set_state_sha256:" + expected_state)

    if errors:
        fail(errors)

    print("DRAW_GEOMETRY_CAPTURE_SET_V01_PASS")
    print("status=" + d["status"])
    print("training_count=" + str(train["pair_count"]))
    print("holdout_count=" + str(hold["pair_count"]))
    print("training_model_selection_allowed=true")
    print("holdout_model_selection_allowed=false")
    print("geometry_relation_status=UNKNOWN")
    print("seal_evidence_split_not_model_hypothesis=true")

if __name__ == "__main__":
    main()
