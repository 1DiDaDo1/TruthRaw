#!/usr/bin/env python3
from __future__ import annotations

import copy
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile


REPO = Path(__file__).resolve().parents[1]
GATE = REPO / "tools" / "draw_geometry_holdout_validation_gate_v01.py"
VALIDATOR = (
    REPO / "tools" / "validate_draw_geometry_holdout_validation_result_v01.py"
)


def chash(value) -> str:
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
    return chash(x)


def fake_hash(label: str) -> str:
    return hashlib.sha256(label.encode("utf-8")).hexdigest()


def write(path: Path, doc: dict) -> None:
    path.write_text(json.dumps(doc, indent=2) + "\n", encoding="utf-8")


def run(args: list[str], expect: int = 0) -> subprocess.CompletedProcess[str]:
    cp = subprocess.run(args, capture_output=True, text=True)
    if cp.returncode != expect:
        raise AssertionError(
            f"return code {cp.returncode} != {expect}\n"
            f"cmd={args}\nstdout={cp.stdout}\nstderr={cp.stderr}"
        )
    return cp


def capture_set() -> dict:
    campaign = "DRAW_GEOMETRY_MAIN_ULTRAWIDE_2026_09_27_V0_1"
    target = {
        "target_geometry_sha256": fake_hash("target"),
        "target_family": "TEST_TARGET",
        "metric_spacing_m": 0.02,
    }

    def pair(i: int, prefix: str) -> dict:
        return {
            "pose_id": f"{prefix}_{i:02d}",
            "pair_state_sha256": fake_hash(f"{prefix}-pair-{i}"),
            "main_source_sha256": fake_hash(f"{prefix}-main-{i}"),
            "ultrawide_source_sha256": fake_hash(f"{prefix}-wide-{i}"),
            "main_physical_observation_id": (
                "DRAW_PHYSICAL_OBS_" + fake_hash(f"{prefix}-main-{i}")
            ),
            "ultrawide_physical_observation_id": (
                "DRAW_PHYSICAL_OBS_" + fake_hash(f"{prefix}-wide-{i}")
            ),
        }

    training = [pair(i, "TRAIN") for i in range(1, 13)]
    holdout = [pair(i, "HOLD") for i in range(1, 5)]

    training_identity = {
        "campaign_id": campaign,
        "target": target,
        "subset": "TRAINING",
        "members": [p["pair_state_sha256"] for p in training],
    }
    holdout_identity = {
        "campaign_id": campaign,
        "target": target,
        "subset": "HOLDOUT",
        "members": [p["pair_state_sha256"] for p in holdout],
    }

    d = {
        "schema": "D.RAW/GeometryRelationCaptureSet/0.2",
        "status": "CAPTURE_SET_COMPLETE_TRAINING_MODEL_SELECTION_OPEN",
        "campaign_id": campaign,
        "target": target,
        "requirements": {
            "training_pose_minimum": 12,
            "holdout_pose_minimum": 4,
        },
        "training": {
            "pair_count": 12,
            "pairs": training,
            "set_sha256": chash(training_identity),
        },
        "holdout": {
            "pair_count": 4,
            "pairs": holdout,
            "set_sha256": chash(holdout_identity),
        },
        "model_selection_gate": {
            "training_feature_extraction_allowed": True,
            "training_model_selection_allowed": True,
            "holdout_feature_extraction_may_be_prepared": True,
            "holdout_model_selection_allowed": False,
            "final_holdout_scoring_allowed": False,
            "reason_final_holdout_closed": (
                "MODEL_AND_THRESHOLDS_NOT_YET_FROZEN_BY_LATER_ARTIFACT"
            ),
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
    d["capture_set_state_sha256"] = chash(d)
    return d


def freeze(capture: dict) -> dict:
    d = {
        "schema": "D.RAW/GeometryModelFreeze/0.1",
        "status": "MODEL_CANDIDATE_FROZEN_HOLDOUT_SCORING_OPEN",
        "campaign_id": capture["campaign_id"],
        "capture_set_state_sha256": capture[
            "capture_set_state_sha256"
        ],
        "training_set_sha256": capture["training"]["set_sha256"],
        "holdout_set_sha256": capture["holdout"]["set_sha256"],
        "training_selection_state_sha256": fake_hash("selection"),
        "selected_candidate": {
            "candidate_id": "candidate-test",
            "model_family": "FUTURE_TEST_MODEL",
            "model_spec_sha256": fake_hash("spec"),
            "fitted_parameters_sha256": fake_hash("params"),
            "training_result_sha256": fake_hash("training-result"),
            "training_metric_summary_sha256": fake_hash("training-metrics"),
            "validity_domain_sha256": fake_hash("validity"),
            "extensions": {},
        },
        "predeclared_holdout_policy": {
            "holdout_scoring_policy_sha256": fake_hash("holdout-policy"),
            "holdout_acceptance_thresholds_sha256": fake_hash(
                "holdout-thresholds"
            ),
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
    d["freeze_state_sha256"] = chash(d)
    return d


def evaluation(capture: dict, frozen: dict, decision: str) -> dict:
    d = {
        "schema": "D.RAW/GeometryHoldoutEvaluation/0.1",
        "status": "HOLDOUT_EVALUATION_COMPLETE",
        "campaign_id": capture["campaign_id"],
        "capture_set_state_sha256": capture[
            "capture_set_state_sha256"
        ],
        "freeze_state_sha256": frozen["freeze_state_sha256"],
        "holdout_set_sha256": frozen["holdout_set_sha256"],
        "selected_candidate_id": frozen["selected_candidate"][
            "candidate_id"
        ],
        "holdout_scoring_policy_sha256": frozen[
            "predeclared_holdout_policy"
        ]["holdout_scoring_policy_sha256"],
        "holdout_acceptance_thresholds_sha256": frozen[
            "predeclared_holdout_policy"
        ]["holdout_acceptance_thresholds_sha256"],
        "evaluated_holdout_pair_state_sha256": [
            p["pair_state_sha256"] for p in capture["holdout"]["pairs"]
        ],
        "holdout_metrics_bundle_sha256": fake_hash(
            "metrics-" + decision
        ),
        "holdout_residual_report_sha256": fake_hash(
            "residuals-" + decision
        ),
        "decision": decision,
        "decision_basis_sha256": fake_hash("basis-" + decision),
        "holdout_execution": {
            "model_refit_performed": False,
            "model_family_changed": False,
            "hyperparameters_changed": False,
            "selected_candidate_changed": False,
            "acceptance_thresholds_changed": False,
        },
        "extensions": {},
        "invariants": {
            "source_evidence_mutated": False,
            "capture_set_membership_rewritten": False,
            "creates_new_evidence": False,
            "scientific_writeback_allowed": False,
            "geometry_relation_granted": False,
            "fusion_granted": False,
        },
    }
    d["evaluation_state_sha256"] = hash_without(
        d, "evaluation_state_sha256"
    )
    return d


def expect_reject(
    root: Path,
    capture_path: Path,
    freeze_path: Path,
    evaluation_doc: dict,
    name: str,
) -> None:
    ep = root / name
    write(ep, evaluation_doc)
    cp = subprocess.run(
        [
            sys.executable,
            str(GATE),
            "--capture-set",
            str(capture_path),
            "--freeze",
            str(freeze_path),
            "--evaluation",
            str(ep),
            "--output",
            str(root / ("out-" + name)),
        ],
        capture_output=True,
        text=True,
    )
    if cp.returncode == 0:
        raise AssertionError(f"invalid evaluation accepted: {name}")


def expect_result_invalid(root: Path, base: dict, name: str, mutate) -> None:
    d = copy.deepcopy(base)
    mutate(d)
    d["result_state_sha256"] = hash_without(
        d, "result_state_sha256"
    )
    p = root / name
    write(p, d)
    cp = subprocess.run(
        [sys.executable, str(VALIDATOR), str(p)],
        capture_output=True,
        text=True,
    )
    if cp.returncode == 0:
        raise AssertionError(f"invalid result accepted: {name}")


def main() -> None:
    with tempfile.TemporaryDirectory(
        prefix="draw_geometry_holdout_v01_"
    ) as td:
        root = Path(td)
        c = capture_set()
        f = freeze(c)
        cp = root / "capture.json"
        fp = root / "freeze.json"
        write(cp, c)
        write(fp, f)

        outputs = {}
        for decision in ("PASS", "FAIL"):
            e = evaluation(c, f, decision)
            ep = root / f"evaluation-{decision}.json"
            op = root / f"result-{decision}.json"
            write(ep, e)
            run(
                [
                    sys.executable,
                    str(GATE),
                    "--capture-set",
                    str(cp),
                    "--freeze",
                    str(fp),
                    "--evaluation",
                    str(ep),
                    "--output",
                    str(op),
                ]
            )
            run([sys.executable, str(VALIDATOR), str(op)])
            outputs[decision] = json.loads(
                op.read_text(encoding="utf-8")
            )

        assert outputs["PASS"]["status"] == (
            "HOLDOUT_VALIDATED_CANDIDATE_AWAITING_ENDPOINT_APPLICABILITY"
        )
        assert outputs["FAIL"]["status"] == "HOLDOUT_REJECTED_CANDIDATE"
        for result in outputs.values():
            assert result["relation_authority"]["status"] == "UNKNOWN"
            assert (
                result["relation_authority"][
                    "coordinate_transform_allowed"
                ]
                is False
            )
            assert (
                result["endpoint_applicability"]["proven_by_this_gate"]
                is False
            )

        bad = evaluation(c, f, "PASS")
        bad["evaluated_holdout_pair_state_sha256"] = bad[
            "evaluated_holdout_pair_state_sha256"
        ][:-1]
        bad["evaluation_state_sha256"] = hash_without(
            bad, "evaluation_state_sha256"
        )
        expect_reject(cp.parent, cp, fp, bad, "partial-holdout.json")

        bad = evaluation(c, f, "PASS")
        bad["holdout_execution"]["model_refit_performed"] = True
        bad["evaluation_state_sha256"] = hash_without(
            bad, "evaluation_state_sha256"
        )
        expect_reject(cp.parent, cp, fp, bad, "refit.json")

        bad = evaluation(c, f, "PASS")
        bad["holdout_acceptance_thresholds_sha256"] = fake_hash(
            "changed-thresholds"
        )
        bad["evaluation_state_sha256"] = hash_without(
            bad, "evaluation_state_sha256"
        )
        expect_reject(cp.parent, cp, fp, bad, "threshold-change.json")

        expect_result_invalid(
            root,
            outputs["PASS"],
            "pass-grants-relation.json",
            lambda d: d["relation_authority"].__setitem__(
                "status", "ADMITTED"
            ),
        )
        expect_result_invalid(
            root,
            outputs["PASS"],
            "pass-grants-transform.json",
            lambda d: d["relation_authority"].__setitem__(
                "coordinate_transform_allowed", True
            ),
        )
        expect_result_invalid(
            root,
            outputs["FAIL"],
            "fail-not-preserved.json",
            lambda d: d["open_world_evolution"].__setitem__(
                "rejected_candidate_preserved_as_knowledge", False
            ),
        )

        print("DRAW_GEOMETRY_HOLDOUT_VALIDATION_GATE_V01_INTEGRATION_PASS")
        print("pass_and_fail_paths_validated=true")
        print("partial_holdout_rejected=true")
        print("holdout_refit_rejected=true")
        print("threshold_change_rejected=true")
        print("pass_relation_status=UNKNOWN")
        print("endpoint_applicability_required=true")
        print("rejected_candidate_preserved=true")


if __name__ == "__main__":
    main()
