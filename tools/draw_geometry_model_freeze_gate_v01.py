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
        raise SystemExit(name + " must be a lowercase SHA-256 hex string")
    return s


def validate_capture_set(path: Path) -> dict:
    validator = Path(__file__).with_name(
        "validate_draw_geometry_capture_set_v01.py"
    )
    cp = subprocess.run(
        [sys.executable, str(validator), str(path)],
        capture_output=True,
        text=True,
    )
    if cp.returncode != 0:
        raise SystemExit(
            "capture set validation failed: "
            + (cp.stdout.strip().replace("\n", "|")
               or cp.stderr.strip().replace("\n", "|"))
        )
    d = json.loads(path.read_text(encoding="utf-8"))
    if d.get("status") != (
        "CAPTURE_SET_COMPLETE_TRAINING_MODEL_SELECTION_OPEN"
    ):
        raise SystemExit("capture set status")
    return d


def recompute_subset_hash(capture_set: dict, key: str, label: str) -> str:
    block = capture_set.get(key) or {}
    pairs = block.get("pairs") or []
    members = [
        item.get("pair_state_sha256")
        for item in sorted(pairs, key=lambda x: x.get("pose_id", ""))
    ]
    for i, member in enumerate(members):
        require_hex64(member, f"{key}.pair[{i}].pair_state_sha256")
    identity = {
        "campaign_id": capture_set.get("campaign_id"),
        "target": capture_set.get("target"),
        "subset": label,
        "members": members,
    }
    return canonical_hash(identity)


def validate_selection(selection: dict, capture_set: dict) -> dict:
    errors = []

    if selection.get("schema") != (
        "D.RAW/GeometryTrainingModelSelection/0.1"
    ):
        errors.append("selection.schema")
    if selection.get("status") != "TRAINING_ONLY_SELECTION_COMPLETE":
        errors.append("selection.status")
    if selection.get("campaign_id") != capture_set.get("campaign_id"):
        errors.append("selection.campaign_id")
    if selection.get("capture_set_state_sha256") != (
        capture_set.get("capture_set_state_sha256")
    ):
        errors.append("selection.capture_set_state_sha256")
    if selection.get("training_set_sha256") != (
        capture_set.get("training", {}).get("set_sha256")
    ):
        errors.append("selection.training_set_sha256")
    if selection.get("holdout_set_sha256") != (
        capture_set.get("holdout", {}).get("set_sha256")
    ):
        errors.append("selection.holdout_set_sha256")

    for field in (
        "training_feature_bundle_sha256",
        "selection_policy_sha256",
        "holdout_scoring_policy_sha256",
        "holdout_acceptance_thresholds_sha256",
    ):
        try:
            require_hex64(selection.get(field), "selection." + field)
        except SystemExit:
            errors.append("selection." + field)

    if selection.get("holdout_used_in_model_selection") is not False:
        errors.append("selection.holdout_used_in_model_selection")
    if selection.get("holdout_metrics_accessed") is not False:
        errors.append("selection.holdout_metrics_accessed")

    candidates = selection.get("candidates")
    if not isinstance(candidates, list) or not candidates:
        errors.append("selection.candidates")
        candidates = []

    ids = set()
    selected = []
    normalized = []
    for index, candidate in enumerate(candidates):
        prefix = f"selection.candidates[{index}]"
        cid = candidate.get("candidate_id")
        family = candidate.get("model_family")
        if not isinstance(cid, str) or not cid:
            errors.append(prefix + ".candidate_id")
        elif cid in ids:
            errors.append(prefix + ".duplicate_candidate_id")
        else:
            ids.add(cid)
        if not isinstance(family, str) or not family:
            errors.append(prefix + ".model_family")
        for field in (
            "model_spec_sha256",
            "fitted_parameters_sha256",
            "training_result_sha256",
            "training_metric_summary_sha256",
            "validity_domain_sha256",
        ):
            try:
                require_hex64(candidate.get(field), prefix + "." + field)
            except SystemExit:
                errors.append(prefix + "." + field)
        if not isinstance(candidate.get("extensions"), dict):
            errors.append(prefix + ".extensions")
        if candidate.get("selected") is True:
            selected.append(cid)
        elif candidate.get("selected") is not False:
            errors.append(prefix + ".selected")

        normalized.append(
            {
                "candidate_id": cid,
                "model_family": family,
                "model_spec_sha256": candidate.get("model_spec_sha256"),
                "fitted_parameters_sha256": candidate.get(
                    "fitted_parameters_sha256"
                ),
                "training_result_sha256": candidate.get(
                    "training_result_sha256"
                ),
                "training_metric_summary_sha256": candidate.get(
                    "training_metric_summary_sha256"
                ),
                "validity_domain_sha256": candidate.get(
                    "validity_domain_sha256"
                ),
                "extensions": candidate.get("extensions", {}),
            }
        )

    if len(selected) != 1:
        errors.append("selection.exactly_one_selected_candidate")
    if selection.get("selected_candidate_id") not in ids:
        errors.append("selection.selected_candidate_id")
    elif selected and selection.get("selected_candidate_id") != selected[0]:
        errors.append("selection.selected_marker_mismatch")

    inv = selection.get("invariants") or {}
    expected_inv = {
        "selection_uses_training_only": True,
        "source_evidence_mutated": False,
        "capture_set_membership_rewritten": False,
        "geometry_relation_granted": False,
        "fusion_granted": False,
    }
    for key, value in expected_inv.items():
        if inv.get(key) != value:
            errors.append("selection.invariants." + key)

    if not isinstance(selection.get("extensions"), dict):
        errors.append("selection.extensions")

    expected_state = hash_without(selection, "selection_state_sha256")
    if selection.get("selection_state_sha256") != expected_state:
        errors.append("selection.selection_state_sha256:" + expected_state)

    if errors:
        raise SystemExit("\n".join(errors))

    selected_index = [
        c.get("candidate_id") for c in candidates
    ].index(selection["selected_candidate_id"])
    return {
        "selected_candidate": normalized[selected_index],
        "selection_state_sha256": selection["selection_state_sha256"],
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--capture-set", required=True)
    parser.add_argument("--selection", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    capture_set = validate_capture_set(Path(args.capture_set))

    expected_training = recompute_subset_hash(
        capture_set, "training", "TRAINING"
    )
    expected_holdout = recompute_subset_hash(
        capture_set, "holdout", "HOLDOUT"
    )
    if capture_set.get("training", {}).get("set_sha256") != expected_training:
        raise SystemExit(
            "capture set training_set_sha256 does not match members"
        )
    if capture_set.get("holdout", {}).get("set_sha256") != expected_holdout:
        raise SystemExit(
            "capture set holdout_set_sha256 does not match members"
        )

    capture_state = capture_set.get("capture_set_state_sha256")
    require_hex64(capture_state, "capture_set_state_sha256")

    selection = json.loads(
        Path(args.selection).read_text(encoding="utf-8")
    )
    checked = validate_selection(selection, capture_set)

    selected = checked["selected_candidate"]

    result = {
        "schema": "D.RAW/GeometryModelFreeze/0.1",
        "status": "MODEL_CANDIDATE_FROZEN_HOLDOUT_SCORING_OPEN",
        "campaign_id": capture_set.get("campaign_id"),
        "capture_set_state_sha256": capture_state,
        "training_set_sha256": expected_training,
        "holdout_set_sha256": expected_holdout,
        "training_selection_state_sha256": checked[
            "selection_state_sha256"
        ],
        "selected_candidate": selected,
        "predeclared_holdout_policy": {
            "holdout_scoring_policy_sha256": selection[
                "holdout_scoring_policy_sha256"
            ],
            "holdout_acceptance_thresholds_sha256": selection[
                "holdout_acceptance_thresholds_sha256"
            ],
        },
        "holdout_gate": {
            "final_holdout_scoring_allowed": True,
            "model_refit_allowed": False,
            "model_family_change_allowed": False,
            "hyperparameter_change_allowed": False,
            "selected_candidate_change_allowed": False,
            "acceptance_threshold_change_allowed": False,
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
            "frozen_scope": "ONE_REPRODUCIBLE_CERTIFICATE_CANDIDATE",
            "frozen_candidate_is_final_scientific_truth": False,
            "future_candidate_successors_allowed": True,
            "future_model_families_allowed": True,
            "competing_models_may_be_preserved": True,
        },
        "extensions": {},
        "invariants": {
            "source_evidence_mutated": False,
            "capture_set_membership_rewritten": False,
            "holdout_used_for_model_selection": False,
            "creates_new_evidence": False,
            "scientific_writeback_allowed": False,
            "geometry_relation_granted": False,
            "fusion_granted": False,
            "calibration_transfer_granted": False,
        },
    }
    result["freeze_state_sha256"] = canonical_hash(result)

    Path(args.output).write_text(
        json.dumps(result, indent=2) + "\n", encoding="utf-8"
    )

    print("DRAW_GEOMETRY_MODEL_FREEZE_GATE_V01_PASS")
    print("status=" + result["status"])
    print("selected_candidate_id=" + selected["candidate_id"])
    print("selected_model_family=" + selected["model_family"])
    print("final_holdout_scoring_allowed=true")
    print("geometry_relation_status=UNKNOWN")
    print("frozen_candidate_is_final_scientific_truth=false")
    print("freeze_state_sha256=" + result["freeze_state_sha256"])


if __name__ == "__main__":
    main()
