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
    print("DRAW_GEOMETRY_HOLDOUT_VALIDATION_RESULT_V01_FAIL")
    for error in errors:
        print(error)
    raise SystemExit(1)


def main():
    if len(sys.argv) != 2:
        fail([
            "usage: validate_draw_geometry_holdout_validation_result_v01.py RESULT.json"
        ])
    try:
        d = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
    except Exception as exc:
        fail(["json:" + str(exc)])

    errors = []
    if d.get("schema") != "D.RAW/GeometryHoldoutValidationResult/0.1":
        errors.append("schema")

    decision = d.get("decision")
    expected_status = {
        "PASS": "HOLDOUT_VALIDATED_CANDIDATE_AWAITING_ENDPOINT_APPLICABILITY",
        "FAIL": "HOLDOUT_REJECTED_CANDIDATE",
    }.get(decision)
    if expected_status is None:
        errors.append("decision")
    elif d.get("status") != expected_status:
        errors.append("status")

    for field in (
        "capture_set_state_sha256",
        "freeze_state_sha256",
        "evaluation_state_sha256",
        "holdout_set_sha256",
        "holdout_metrics_bundle_sha256",
        "holdout_residual_report_sha256",
        "decision_basis_sha256",
    ):
        if HEX64.fullmatch(str(d.get(field, ""))) is None:
            errors.append(field)

    if not isinstance(d.get("selected_candidate_id"), str) or not d.get(
        "selected_candidate_id"
    ):
        errors.append("selected_candidate_id")
    if not isinstance(d.get("holdout_pair_count"), int) or d.get(
        "holdout_pair_count", 0
    ) < 1:
        errors.append("holdout_pair_count")

    policy = d.get("frozen_policy") or {}
    for field in (
        "holdout_scoring_policy_sha256",
        "holdout_acceptance_thresholds_sha256",
    ):
        if HEX64.fullmatch(str(policy.get(field, ""))) is None:
            errors.append("policy." + field)

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

    applicability = d.get("endpoint_applicability") or {}
    if applicability.get(
        "required_before_relation_admission"
    ) is not True:
        errors.append("endpoint_applicability.required")
    if applicability.get("proven_by_this_gate") is not False:
        errors.append("endpoint_applicability.proven")

    evolution = d.get("open_world_evolution") or {}
    expected_evolution = {
        "outcome_scope": "ONE_FROZEN_CANDIDATE_ON_ONE_SEALED_HOLDOUT_SET",
        "candidate_test_outcome_immutable": True,
        "outcome_is_final_scientific_model_truth": False,
        "future_successor_candidates_allowed": True,
        "rejected_candidate_preserved_as_knowledge": True,
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
        "candidate_mutated_during_holdout": False,
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
    x.pop("result_state_sha256", None)
    expected_state = canonical_hash(x)
    if d.get("result_state_sha256") != expected_state:
        errors.append("result_state_sha256:" + expected_state)

    if errors:
        fail(errors)

    print("DRAW_GEOMETRY_HOLDOUT_VALIDATION_RESULT_V01_PASS")
    print("decision=" + decision)
    print("status=" + d["status"])
    print("geometry_relation_status=UNKNOWN")
    print("endpoint_applicability_proven=false")


if __name__ == "__main__":
    main()
