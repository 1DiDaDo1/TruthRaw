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
    print("DRAW_GEOMETRY_MODEL_FREEZE_V01_FAIL")
    for error in errors:
        print(error)
    raise SystemExit(1)


def main():
    if len(sys.argv) != 2:
        fail(["usage: validate_draw_geometry_model_freeze_v01.py FREEZE.json"])
    try:
        d = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
    except Exception as exc:
        fail(["json:" + str(exc)])

    errors = []
    if d.get("schema") != "D.RAW/GeometryModelFreeze/0.1":
        errors.append("schema")
    if d.get("status") != (
        "MODEL_CANDIDATE_FROZEN_HOLDOUT_SCORING_OPEN"
    ):
        errors.append("status")

    for field in (
        "capture_set_state_sha256",
        "training_set_sha256",
        "holdout_set_sha256",
        "training_selection_state_sha256",
    ):
        if HEX64.fullmatch(str(d.get(field, ""))) is None:
            errors.append(field)

    candidate = d.get("selected_candidate") or {}
    if not isinstance(candidate.get("candidate_id"), str) or not candidate.get(
        "candidate_id"
    ):
        errors.append("candidate.id")
    if not isinstance(candidate.get("model_family"), str) or not candidate.get(
        "model_family"
    ):
        errors.append("candidate.family")
    for field in (
        "model_spec_sha256",
        "fitted_parameters_sha256",
        "training_result_sha256",
        "training_metric_summary_sha256",
        "validity_domain_sha256",
    ):
        if HEX64.fullmatch(str(candidate.get(field, ""))) is None:
            errors.append("candidate." + field)
    if not isinstance(candidate.get("extensions"), dict):
        errors.append("candidate.extensions")

    policy = d.get("predeclared_holdout_policy") or {}
    for field in (
        "holdout_scoring_policy_sha256",
        "holdout_acceptance_thresholds_sha256",
    ):
        if HEX64.fullmatch(str(policy.get(field, ""))) is None:
            errors.append("policy." + field)

    gate = d.get("holdout_gate") or {}
    expected_gate = {
        "final_holdout_scoring_allowed": True,
        "model_refit_allowed": False,
        "model_family_change_allowed": False,
        "hyperparameter_change_allowed": False,
        "selected_candidate_change_allowed": False,
        "acceptance_threshold_change_allowed": False,
    }
    for key, value in expected_gate.items():
        if gate.get(key) != value:
            errors.append("holdout_gate." + key)

    relation = d.get("relation_authority") or {}
    expected_relation = {
        "axis": "GEOMETRY",
        "status": "UNKNOWN",
        "coordinate_transform_allowed": False,
        "equality_allowed": False,
        "fusion_allowed": False,
        "calibration_transfer_allowed": False,
    }
    for key, value in expected_relation.items():
        if relation.get(key) != value:
            errors.append("relation." + key)

    evolution = d.get("open_world_evolution") or {}
    expected_evolution = {
        "frozen_scope": "ONE_REPRODUCIBLE_CERTIFICATE_CANDIDATE",
        "frozen_candidate_is_final_scientific_truth": False,
        "future_candidate_successors_allowed": True,
        "future_model_families_allowed": True,
        "competing_models_may_be_preserved": True,
    }
    for key, value in expected_evolution.items():
        if evolution.get(key) != value:
            errors.append("evolution." + key)

    if not isinstance(d.get("extensions"), dict):
        errors.append("extensions")

    inv = d.get("invariants") or {}
    expected_inv = {
        "source_evidence_mutated": False,
        "capture_set_membership_rewritten": False,
        "holdout_used_for_model_selection": False,
        "creates_new_evidence": False,
        "scientific_writeback_allowed": False,
        "geometry_relation_granted": False,
        "fusion_granted": False,
        "calibration_transfer_granted": False,
    }
    for key, value in expected_inv.items():
        if inv.get(key) != value:
            errors.append("invariant." + key)

    x = dict(d)
    x.pop("freeze_state_sha256", None)
    expected_state = canonical_hash(x)
    if d.get("freeze_state_sha256") != expected_state:
        errors.append("freeze_state_sha256:" + expected_state)

    if errors:
        fail(errors)

    print("DRAW_GEOMETRY_MODEL_FREEZE_V01_PASS")
    print("status=" + d["status"])
    print("final_holdout_scoring_allowed=true")
    print("geometry_relation_status=UNKNOWN")
    print("frozen_candidate_is_final_scientific_truth=false")


if __name__ == "__main__":
    main()
