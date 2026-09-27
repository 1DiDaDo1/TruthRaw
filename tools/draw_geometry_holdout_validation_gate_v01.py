#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
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


def hash_without(doc: dict, field: str) -> str:
    x = dict(doc)
    x.pop(field, None)
    return canonical_hash(x)


def require_hex64(value, name: str) -> str:
    s = str(value or "")
    if HEX64.fullmatch(s) is None:
        raise SystemExit(name + " must be lowercase SHA-256 hex")
    return s


def validate_with(tool_name: str, path: Path, label: str) -> dict:
    validator = Path(__file__).with_name(tool_name)
    cp = subprocess.run(
        [sys.executable, str(validator), str(path)],
        capture_output=True,
        text=True,
    )
    if cp.returncode != 0:
        raise SystemExit(
            label
            + " validation failed: "
            + (
                cp.stdout.strip().replace("\n", "|")
                or cp.stderr.strip().replace("\n", "|")
            )
        )
    return json.loads(path.read_text(encoding="utf-8"))


def validate_evaluation(
    evaluation: dict,
    capture_set: dict,
    freeze: dict,
) -> None:
    errors = []

    if evaluation.get("schema") != "D.RAW/GeometryHoldoutEvaluation/0.1":
        errors.append("evaluation.schema")
    if evaluation.get("status") != "HOLDOUT_EVALUATION_COMPLETE":
        errors.append("evaluation.status")
    if evaluation.get("campaign_id") != capture_set.get("campaign_id"):
        errors.append("evaluation.campaign_id")
    if evaluation.get("capture_set_state_sha256") != capture_set.get(
        "capture_set_state_sha256"
    ):
        errors.append("evaluation.capture_set_state_sha256")
    if evaluation.get("freeze_state_sha256") != freeze.get(
        "freeze_state_sha256"
    ):
        errors.append("evaluation.freeze_state_sha256")
    if evaluation.get("holdout_set_sha256") != freeze.get(
        "holdout_set_sha256"
    ):
        errors.append("evaluation.holdout_set_sha256")

    selected = freeze.get("selected_candidate") or {}
    if evaluation.get("selected_candidate_id") != selected.get(
        "candidate_id"
    ):
        errors.append("evaluation.selected_candidate_id")

    frozen_policy = freeze.get("predeclared_holdout_policy") or {}
    if evaluation.get("holdout_scoring_policy_sha256") != frozen_policy.get(
        "holdout_scoring_policy_sha256"
    ):
        errors.append("evaluation.holdout_scoring_policy_sha256")
    if evaluation.get(
        "holdout_acceptance_thresholds_sha256"
    ) != frozen_policy.get("holdout_acceptance_thresholds_sha256"):
        errors.append("evaluation.holdout_acceptance_thresholds_sha256")

    expected_members = sorted(
        p.get("pair_state_sha256")
        for p in (capture_set.get("holdout") or {}).get("pairs", [])
    )
    actual_members = evaluation.get(
        "evaluated_holdout_pair_state_sha256"
    )
    if not isinstance(actual_members, list):
        errors.append("evaluation.evaluated_holdout_pair_state_sha256")
        actual_members = []
    else:
        if len(set(actual_members)) != len(actual_members):
            errors.append("evaluation.duplicate_holdout_member")
        if sorted(actual_members) != expected_members:
            errors.append("evaluation.exact_holdout_coverage")

    for i, value in enumerate(actual_members):
        try:
            require_hex64(
                value,
                f"evaluation.evaluated_holdout_pair_state_sha256[{i}]",
            )
        except SystemExit:
            errors.append(f"evaluation.holdout_member[{i}]")

    for field in (
        "holdout_metrics_bundle_sha256",
        "holdout_residual_report_sha256",
        "decision_basis_sha256",
    ):
        try:
            require_hex64(evaluation.get(field), "evaluation." + field)
        except SystemExit:
            errors.append("evaluation." + field)

    if evaluation.get("decision") not in {"PASS", "FAIL"}:
        errors.append("evaluation.decision")

    execution = evaluation.get("holdout_execution") or {}
    for key in (
        "model_refit_performed",
        "model_family_changed",
        "hyperparameters_changed",
        "selected_candidate_changed",
        "acceptance_thresholds_changed",
    ):
        if execution.get(key) is not False:
            errors.append("evaluation.holdout_execution." + key)

    if not isinstance(evaluation.get("extensions"), dict):
        errors.append("evaluation.extensions")

    inv = evaluation.get("invariants") or {}
    expected_inv = {
        "source_evidence_mutated": False,
        "capture_set_membership_rewritten": False,
        "creates_new_evidence": False,
        "scientific_writeback_allowed": False,
        "geometry_relation_granted": False,
        "fusion_granted": False,
    }
    for key, value in expected_inv.items():
        if inv.get(key) != value:
            errors.append("evaluation.invariants." + key)

    expected_state = hash_without(
        evaluation, "evaluation_state_sha256"
    )
    if evaluation.get("evaluation_state_sha256") != expected_state:
        errors.append(
            "evaluation.evaluation_state_sha256:" + expected_state
        )

    if errors:
        raise SystemExit("\n".join(errors))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--capture-set", required=True)
    parser.add_argument("--freeze", required=True)
    parser.add_argument("--evaluation", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    capture_set = validate_with(
        "validate_draw_geometry_capture_set_v01.py",
        Path(args.capture_set),
        "capture-set",
    )
    freeze = validate_with(
        "validate_draw_geometry_model_freeze_v01.py",
        Path(args.freeze),
        "freeze",
    )

    if freeze.get("campaign_id") != capture_set.get("campaign_id"):
        raise SystemExit("freeze/capture-set campaign mismatch")
    if freeze.get("capture_set_state_sha256") != capture_set.get(
        "capture_set_state_sha256"
    ):
        raise SystemExit("freeze/capture-set state mismatch")
    if freeze.get("holdout_set_sha256") != (
        capture_set.get("holdout") or {}
    ).get("set_sha256"):
        raise SystemExit("freeze/capture-set holdout mismatch")

    evaluation = json.loads(
        Path(args.evaluation).read_text(encoding="utf-8")
    )
    validate_evaluation(evaluation, capture_set, freeze)

    decision = evaluation["decision"]
    status = (
        "HOLDOUT_VALIDATED_CANDIDATE_AWAITING_ENDPOINT_APPLICABILITY"
        if decision == "PASS"
        else "HOLDOUT_REJECTED_CANDIDATE"
    )

    result = {
        "schema": "D.RAW/GeometryHoldoutValidationResult/0.1",
        "status": status,
        "decision": decision,
        "campaign_id": capture_set["campaign_id"],
        "capture_set_state_sha256": capture_set[
            "capture_set_state_sha256"
        ],
        "freeze_state_sha256": freeze["freeze_state_sha256"],
        "evaluation_state_sha256": evaluation[
            "evaluation_state_sha256"
        ],
        "holdout_set_sha256": freeze["holdout_set_sha256"],
        "selected_candidate_id": freeze["selected_candidate"][
            "candidate_id"
        ],
        "holdout_pair_count": len(
            (capture_set.get("holdout") or {}).get("pairs", [])
        ),
        "holdout_metrics_bundle_sha256": evaluation[
            "holdout_metrics_bundle_sha256"
        ],
        "holdout_residual_report_sha256": evaluation[
            "holdout_residual_report_sha256"
        ],
        "decision_basis_sha256": evaluation[
            "decision_basis_sha256"
        ],
        "frozen_policy": {
            "holdout_scoring_policy_sha256": evaluation[
                "holdout_scoring_policy_sha256"
            ],
            "holdout_acceptance_thresholds_sha256": evaluation[
                "holdout_acceptance_thresholds_sha256"
            ],
        },
        "relation_authority": {
            "axis": "GEOMETRY",
            "status": "UNKNOWN",
            "coordinate_transform_allowed": False,
            "equality_allowed": False,
            "fusion_allowed": False,
            "calibration_transfer_allowed": False,
        },
        "endpoint_applicability": {
            "required_before_relation_admission": True,
            "proven_by_this_gate": False,
        },
        "open_world_evolution": {
            "outcome_scope": (
                "ONE_FROZEN_CANDIDATE_ON_ONE_SEALED_HOLDOUT_SET"
            ),
            "candidate_test_outcome_immutable": True,
            "outcome_is_final_scientific_model_truth": False,
            "future_successor_candidates_allowed": True,
            "rejected_candidate_preserved_as_knowledge": True,
        },
        "extensions": {},
        "invariants": {
            "source_evidence_mutated": False,
            "capture_set_membership_rewritten": False,
            "candidate_mutated_during_holdout": False,
            "creates_new_evidence": False,
            "scientific_writeback_allowed": False,
            "geometry_relation_granted": False,
            "fusion_granted": False,
            "calibration_transfer_granted": False,
        },
    }
    result["result_state_sha256"] = canonical_hash(result)

    Path(args.output).write_text(
        json.dumps(result, indent=2) + "\n", encoding="utf-8"
    )

    print("DRAW_GEOMETRY_HOLDOUT_VALIDATION_GATE_V01_PASS")
    print("decision=" + decision)
    print("status=" + status)
    print("holdout_pair_count=" + str(result["holdout_pair_count"]))
    print("geometry_relation_status=UNKNOWN")
    print("endpoint_applicability_proven=false")
    print("result_state_sha256=" + result["result_state_sha256"])


if __name__ == "__main__":
    main()
