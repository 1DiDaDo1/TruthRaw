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
PAIR_TOOL = REPO / "tools" / "draw_geometry_direct_raw_pair_intake_v01.py"
SET_TOOL = REPO / "tools" / "draw_geometry_capture_set_gate_v01.py"
FREEZE_TOOL = REPO / "tools" / "draw_geometry_model_freeze_gate_v01.py"
FREEZE_VALIDATOR = REPO / "tools" / "validate_draw_geometry_model_freeze_v01.py"
CAMPAIGN = (
    REPO
    / "docs"
    / "research"
    / "draw-geometry-relation-campaign-v0.1"
    / "MAIN_ULTRAWIDE_GEOMETRY_CAMPAIGN_v0_1.json"
)


def run(args: list[str], expect: int = 0) -> subprocess.CompletedProcess[str]:
    cp = subprocess.run(args, capture_output=True, text=True)
    if cp.returncode != expect:
        raise AssertionError(
            f"return code {cp.returncode} != {expect}\n"
            f"cmd={args}\nstdout={cp.stdout}\nstderr={cp.stderr}"
        )
    return cp


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


def write(path: Path, doc: dict) -> None:
    path.write_text(json.dumps(doc, indent=2) + "\n", encoding="utf-8")


def fake_hash(label: str) -> str:
    return hashlib.sha256(label.encode("utf-8")).hexdigest()


def future_admission(source_sha: str) -> dict:
    d = {
        "schema": "D.RAW/SourceAdmissionPackage/99.0-TEST",
        "status": "ADMITTED_SOURCE_LOCAL",
        "physical_observation_id": "DRAW_PHYSICAL_OBS_" + source_sha,
        "physical_source_evidence_sha256": source_sha,
        "evidence_counts": {
            "physical_frame_count": 1,
            "independent_evidence_count": 1,
        },
        "graph_relations": [],
        "fusion_admissions": [],
        "invariants": {
            "scientific_writeback_allowed": False,
            "cross_observation_relation_granted": False,
            "cross_observation_fusion_granted": False,
        },
        "state_sha256": "0" * 64,
    }
    d["state_sha256"] = hash_without(d, "state_sha256")
    return d


def build_capture_set(root: Path) -> Path:
    target_sha = fake_hash("target")
    pairs = []
    for i in range(16):
        subset = "TRAINING" if i < 12 else "HOLDOUT"
        pose = (
            f"TRAIN_{i + 1:02d}"
            if i < 12
            else f"HOLD_{i - 11:02d}"
        )
        main = root / f"main-{i:02d}.raw"
        wide = root / f"wide-{i:02d}.raw"
        main.write_text("main-" + pose, encoding="utf-8")
        wide.write_text("wide-" + pose, encoding="utf-8")
        sealed = root / f"sealed-{i:02d}.json"
        run(
            [
                sys.executable,
                str(PAIR_TOOL),
                "seal",
                "--pose-id",
                pose,
                "--subset",
                subset,
                "--main-file",
                str(main),
                "--ultrawide-file",
                str(wide),
                "--target-geometry-sha256",
                target_sha,
                "--target-family",
                "TEST_TARGET",
                "--metric-spacing-m",
                "0.02",
                "--attest-camera-system-rigid",
                "--attest-target-static-between-pair",
                "--output",
                str(sealed),
            ]
        )
        sd = json.loads(sealed.read_text(encoding="utf-8"))
        ma = root / f"ma-{i:02d}.json"
        wa = root / f"wa-{i:02d}.json"
        write(ma, future_admission(sd["sources"]["main"]["sha256"]))
        write(wa, future_admission(sd["sources"]["ultra_wide"]["sha256"]))
        bound = root / f"bound-{i:02d}.json"
        run(
            [
                sys.executable,
                str(PAIR_TOOL),
                "bind",
                "--pair",
                str(sealed),
                "--main-admission",
                str(ma),
                "--ultrawide-admission",
                str(wa),
                "--output",
                str(bound),
            ]
        )
        pairs.append(bound)

    out = root / "capture-set.json"
    args = [
        sys.executable,
        str(SET_TOOL),
        "--campaign",
        str(CAMPAIGN),
    ]
    for pair in pairs:
        args.extend(["--pair", str(pair)])
    args.extend(["--output", str(out)])
    run(args)
    return out


def make_selection(capture_set: dict) -> dict:
    candidates = []
    for cid, family in (
        ("candidate-A", "PINHOLE_BROWN_CONRADY_LIKE_TEST_FAMILY"),
        ("candidate-B", "FUTURE_SPLINE_RAY_MODEL_TEST_FAMILY"),
    ):
        candidates.append(
            {
                "candidate_id": cid,
                "model_family": family,
                "model_spec_sha256": fake_hash(cid + "-spec"),
                "fitted_parameters_sha256": fake_hash(cid + "-params"),
                "training_result_sha256": fake_hash(cid + "-result"),
                "training_metric_summary_sha256": fake_hash(
                    cid + "-metrics"
                ),
                "validity_domain_sha256": fake_hash(cid + "-validity"),
                "selected": cid == "candidate-B",
                "extensions": {
                    "test_only": True
                },
            }
        )

    d = {
        "schema": "D.RAW/GeometryTrainingModelSelection/0.1",
        "status": "TRAINING_ONLY_SELECTION_COMPLETE",
        "campaign_id": capture_set["campaign_id"],
        "capture_set_state_sha256": capture_set[
            "capture_set_state_sha256"
        ],
        "training_set_sha256": capture_set["training"]["set_sha256"],
        "holdout_set_sha256": capture_set["holdout"]["set_sha256"],
        "training_feature_bundle_sha256": fake_hash("training-features"),
        "selection_policy_sha256": fake_hash("selection-policy"),
        "holdout_scoring_policy_sha256": fake_hash("holdout-policy"),
        "holdout_acceptance_thresholds_sha256": fake_hash(
            "holdout-thresholds"
        ),
        "holdout_used_in_model_selection": False,
        "holdout_metrics_accessed": False,
        "candidates": candidates,
        "selected_candidate_id": "candidate-B",
        "extensions": {},
        "invariants": {
            "selection_uses_training_only": True,
            "source_evidence_mutated": False,
            "capture_set_membership_rewritten": False,
            "geometry_relation_granted": False,
            "fusion_granted": False,
        },
        "selection_state_sha256": "0" * 64,
    }
    d["selection_state_sha256"] = hash_without(
        d, "selection_state_sha256"
    )
    return d


def expect_tool_reject(
    root: Path,
    capture_set: Path,
    selection: dict,
    name: str,
) -> None:
    sel = root / name
    write(sel, selection)
    cp = subprocess.run(
        [
            sys.executable,
            str(FREEZE_TOOL),
            "--capture-set",
            str(capture_set),
            "--selection",
            str(sel),
            "--output",
            str(root / ("out-" + name)),
        ],
        capture_output=True,
        text=True,
    )
    if cp.returncode == 0:
        raise AssertionError(
            f"invalid selection accepted: {name}\n{cp.stdout}"
        )


def expect_freeze_invalid(root: Path, base: dict, name: str, mutate) -> None:
    d = copy.deepcopy(base)
    mutate(d)
    d["freeze_state_sha256"] = hash_without(d, "freeze_state_sha256")
    path = root / name
    write(path, d)
    cp = subprocess.run(
        [sys.executable, str(FREEZE_VALIDATOR), str(path)],
        capture_output=True,
        text=True,
    )
    if cp.returncode == 0:
        raise AssertionError(f"invalid freeze accepted: {name}")


def main() -> None:
    with tempfile.TemporaryDirectory(
        prefix="draw_geometry_model_freeze_v01_"
    ) as td:
        root = Path(td)
        capture_set_path = build_capture_set(root)
        capture_set = json.loads(
            capture_set_path.read_text(encoding="utf-8")
        )
        selection = make_selection(capture_set)
        selection_path = root / "selection.json"
        write(selection_path, selection)

        freeze_path = root / "freeze.json"
        run(
            [
                sys.executable,
                str(FREEZE_TOOL),
                "--capture-set",
                str(capture_set_path),
                "--selection",
                str(selection_path),
                "--output",
                str(freeze_path),
            ]
        )
        run(
            [
                sys.executable,
                str(FREEZE_VALIDATOR),
                str(freeze_path),
            ]
        )
        freeze = json.loads(freeze_path.read_text(encoding="utf-8"))

        assert freeze["selected_candidate"]["candidate_id"] == "candidate-B"
        assert freeze["holdout_gate"]["final_holdout_scoring_allowed"] is True
        assert freeze["holdout_gate"]["model_refit_allowed"] is False
        assert freeze["relation_authority"]["status"] == "UNKNOWN"
        assert (
            freeze["open_world_evolution"][
                "frozen_candidate_is_final_scientific_truth"
            ]
            is False
        )

        bad = copy.deepcopy(selection)
        bad["holdout_metrics_accessed"] = True
        bad["selection_state_sha256"] = hash_without(
            bad, "selection_state_sha256"
        )
        expect_tool_reject(
            root, capture_set_path, bad, "bad-holdout-access.json"
        )

        bad = copy.deepcopy(selection)
        bad["holdout_used_in_model_selection"] = True
        bad["selection_state_sha256"] = hash_without(
            bad, "selection_state_sha256"
        )
        expect_tool_reject(
            root, capture_set_path, bad, "bad-holdout-selection.json"
        )

        bad = copy.deepcopy(selection)
        bad["holdout_acceptance_thresholds_sha256"] = None
        bad["selection_state_sha256"] = hash_without(
            bad, "selection_state_sha256"
        )
        expect_tool_reject(
            root, capture_set_path, bad, "bad-no-thresholds.json"
        )

        bad = copy.deepcopy(selection)
        bad["candidates"][0]["selected"] = True
        bad["selection_state_sha256"] = hash_without(
            bad, "selection_state_sha256"
        )
        expect_tool_reject(
            root, capture_set_path, bad, "bad-two-selected.json"
        )

        # Prove candidate vocabulary is not closed to today's model names.
        future = copy.deepcopy(selection)
        future["candidates"][1][
            "model_family"
        ] = "FUTURE_MODEL_FAMILY_NOT_KNOWN_TO_V0_1"
        future["selection_state_sha256"] = hash_without(
            future, "selection_state_sha256"
        )
        future_path = root / "future-selection.json"
        write(future_path, future)
        run(
            [
                sys.executable,
                str(FREEZE_TOOL),
                "--capture-set",
                str(capture_set_path),
                "--selection",
                str(future_path),
                "--output",
                str(root / "future-freeze.json"),
            ]
        )

        expect_freeze_invalid(
            root,
            freeze,
            "bad-refit.json",
            lambda d: d["holdout_gate"].__setitem__(
                "model_refit_allowed", True
            ),
        )
        expect_freeze_invalid(
            root,
            freeze,
            "bad-relation.json",
            lambda d: d["relation_authority"].__setitem__(
                "status", "ADMITTED"
            ),
        )
        expect_freeze_invalid(
            root,
            freeze,
            "bad-final-truth.json",
            lambda d: d["open_world_evolution"].__setitem__(
                "frozen_candidate_is_final_scientific_truth", True
            ),
        )

        print("DRAW_GEOMETRY_MODEL_FREEZE_GATE_V01_INTEGRATION_PASS")
        print("training_only_selection=true")
        print("holdout_access_during_selection=false")
        print("future_model_family_string_accepted=true")
        print("final_holdout_scoring_after_freeze=true")
        print("geometry_relation_status=UNKNOWN")
        print("freeze_one_candidate_not_scientific_thought=true")


if __name__ == "__main__":
    main()
