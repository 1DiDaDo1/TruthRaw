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
PAIR_VALIDATOR = REPO / "tools" / "validate_draw_geometry_direct_raw_pair_v01.py"
SET_TOOL = REPO / "tools" / "draw_geometry_capture_set_gate_v01.py"
SET_VALIDATOR = REPO / "tools" / "validate_draw_geometry_capture_set_v01.py"
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
            f"unexpected return code {cp.returncode} != {expect}\n"
            f"cmd={args}\nstdout={cp.stdout}\nstderr={cp.stderr}"
        )
    return cp


def state_hash(doc: dict, key: str) -> str:
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


def future_admission(source_sha: str) -> dict:
    doc = {
        "schema": "D.RAW/SourceAdmissionPackage/9.99-FUTURE-SEMANTIC-FIXTURE",
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
    doc["state_sha256"] = state_hash(doc, "state_sha256")
    return doc


def write_json(path: Path, doc: dict) -> None:
    path.write_text(json.dumps(doc, indent=2) + "\n", encoding="utf-8")


def build_pair(
    root: Path,
    index: int,
    target_sha: str,
    subset: str,
    pose_id: str,
) -> Path:
    main_source = root / f"main-{index:02d}.raw"
    wide_source = root / f"wide-{index:02d}.raw"
    main_source.write_bytes(f"synthetic-main-{pose_id}\n".encode("utf-8"))
    wide_source.write_bytes(f"synthetic-wide-{pose_id}\n".encode("utf-8"))

    sealed = root / f"sealed-{index:02d}.json"
    run(
        [
            sys.executable,
            str(PAIR_TOOL),
            "seal",
            "--pose-id",
            pose_id,
            "--subset",
            subset,
            "--main-file",
            str(main_source),
            "--ultrawide-file",
            str(wide_source),
            "--target-geometry-sha256",
            target_sha,
            "--target-family",
            "SYNTHETIC_INDEXED_METRIC_TARGET",
            "--metric-spacing-m",
            "0.02",
            "--attest-camera-system-rigid",
            "--attest-target-static-between-pair",
            "--output",
            str(sealed),
        ]
    )
    run([sys.executable, str(PAIR_VALIDATOR), str(sealed)])

    pair = json.loads(sealed.read_text(encoding="utf-8"))
    main_adm = root / f"main-adm-{index:02d}.json"
    wide_adm = root / f"wide-adm-{index:02d}.json"
    write_json(main_adm, future_admission(pair["sources"]["main"]["sha256"]))
    write_json(
        wide_adm,
        future_admission(pair["sources"]["ultra_wide"]["sha256"]),
    )

    bound = root / f"bound-{index:02d}.json"
    run(
        [
            sys.executable,
            str(PAIR_TOOL),
            "bind",
            "--pair",
            str(sealed),
            "--main-admission",
            str(main_adm),
            "--ultrawide-admission",
            str(wide_adm),
            "--output",
            str(bound),
        ]
    )
    run([sys.executable, str(PAIR_VALIDATOR), str(bound)])
    return bound


def build_capture_set(pair_paths: list[Path], output: Path) -> dict:
    args = [
        sys.executable,
        str(SET_TOOL),
        "--campaign",
        str(CAMPAIGN),
    ]
    for path in pair_paths:
        args.extend(["--pair", str(path)])
    args.extend(["--output", str(output)])
    run(args)
    run([sys.executable, str(SET_VALIDATOR), str(output)])
    return json.loads(output.read_text(encoding="utf-8"))


def expect_invalid(root: Path, base: dict, name: str, mutate) -> None:
    doc = copy.deepcopy(base)
    mutate(doc)
    doc["capture_set_state_sha256"] = state_hash(
        doc, "capture_set_state_sha256"
    )
    path = root / name
    write_json(path, doc)
    cp = subprocess.run(
        [sys.executable, str(SET_VALIDATOR), str(path)],
        capture_output=True,
        text=True,
    )
    if cp.returncode == 0:
        raise AssertionError(
            f"invalid capture set accepted: {name}\n{cp.stdout}"
        )


def main() -> None:
    if not CAMPAIGN.is_file():
        raise AssertionError(f"campaign missing: {CAMPAIGN}")

    with tempfile.TemporaryDirectory(prefix="draw_capture_set_v01_") as td:
        root = Path(td)
        target_sha = hashlib.sha256(
            b"synthetic-target-geometry-v01\n"
        ).hexdigest()

        pair_paths: list[Path] = []
        for i in range(1, 17):
            if i <= 12:
                subset = "TRAINING"
                pose = f"TRAIN_{i:02d}"
            else:
                subset = "HOLDOUT"
                pose = f"HOLD_{i - 12:02d}"
            pair_paths.append(
                build_pair(root, i, target_sha, subset, pose)
            )

        out = root / "capture-set.json"
        capture_set = build_capture_set(pair_paths, out)

        assert capture_set["status"] == (
            "CAPTURE_SET_COMPLETE_TRAINING_MODEL_SELECTION_OPEN"
        )
        assert capture_set["training"]["pair_count"] == 12
        assert capture_set["holdout"]["pair_count"] == 4
        assert (
            capture_set["model_selection_gate"][
                "training_model_selection_allowed"
            ]
            is True
        )
        assert (
            capture_set["model_selection_gate"][
                "holdout_model_selection_allowed"
            ]
            is False
        )
        assert (
            capture_set["model_selection_gate"][
                "final_holdout_scoring_allowed"
            ]
            is False
        )
        assert capture_set["relation_authority"]["status"] == "UNKNOWN"
        assert (
            capture_set["relation_authority"][
                "coordinate_transform_allowed"
            ]
            is False
        )
        assert (
            capture_set["open_world_evolution"][
                "projection_model_selected"
            ]
            is False
        )

        # Deterministic membership identity must not depend on CLI input order.
        reversed_out = root / "capture-set-reversed.json"
        reversed_set = build_capture_set(
            list(reversed(pair_paths)), reversed_out
        )
        assert (
            reversed_set["training"]["set_sha256"]
            == capture_set["training"]["set_sha256"]
        )
        assert (
            reversed_set["holdout"]["set_sha256"]
            == capture_set["holdout"]["set_sha256"]
        )
        assert (
            reversed_set["capture_set_state_sha256"]
            == capture_set["capture_set_state_sha256"]
        )

        # 12 training + only 3 holdout must fail closed.
        insufficient_args = [
            sys.executable,
            str(SET_TOOL),
            "--campaign",
            str(CAMPAIGN),
        ]
        for path in pair_paths[:15]:
            insufficient_args.extend(["--pair", str(path)])
        insufficient_args.extend(
            ["--output", str(root / "insufficient.json")]
        )
        cp = subprocess.run(
            insufficient_args, capture_output=True, text=True
        )
        if cp.returncode == 0:
            raise AssertionError("insufficient holdout set accepted")

        expect_invalid(
            root,
            capture_set,
            "bad-holdout-model.json",
            lambda d: d["model_selection_gate"].__setitem__(
                "holdout_model_selection_allowed", True
            ),
        )
        expect_invalid(
            root,
            capture_set,
            "bad-final-score.json",
            lambda d: d["model_selection_gate"].__setitem__(
                "final_holdout_scoring_allowed", True
            ),
        )
        expect_invalid(
            root,
            capture_set,
            "bad-relation.json",
            lambda d: d["relation_authority"].__setitem__(
                "status", "ADMITTED"
            ),
        )
        expect_invalid(
            root,
            capture_set,
            "bad-transform.json",
            lambda d: d["relation_authority"].__setitem__(
                "coordinate_transform_allowed", True
            ),
        )
        expect_invalid(
            root,
            capture_set,
            "bad-model-seal.json",
            lambda d: d["open_world_evolution"].__setitem__(
                "projection_model_selected", True
            ),
        )
        expect_invalid(
            root,
            capture_set,
            "bad-duplicate-pose.json",
            lambda d: d["holdout"]["pairs"][0].__setitem__(
                "pose_id", d["training"]["pairs"][0]["pose_id"]
            ),
        )

        print("DRAW_GEOMETRY_CAPTURE_SET_GATE_V01_INTEGRATION_PASS")
        print("training_count=12")
        print("holdout_count=4")
        print("input_order_invariant=true")
        print("holdout_model_selection_allowed=false")
        print("geometry_relation_status=UNKNOWN")
        print("seal_evidence_split_not_model_hypothesis=true")


if __name__ == "__main__":
    main()
