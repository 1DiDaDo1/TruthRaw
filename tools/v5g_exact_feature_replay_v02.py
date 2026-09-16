#!/usr/bin/env python3
"""TruthRaw exact v5.0g historical feature-semantics replay gate v0.2.

v0.2 preserves the v0.1 failed harness as provenance and corrects only the
probe shape: the recovered historical predictor is vectorized and therefore is
called with one-element coordinate arrays, matching its historical execution
contract. The 10,023-byte extractor itself remains byte-unchanged.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

from tools.v5g_exact_feature_replay_v01 import (
    EXPECTED_CORE_CPP_SHA256,
    EXPECTED_CORE_H_SHA256,
    EXPECTED_EXTRACTOR_SHA256,
    EXPECTED_FEATURE_NAMES,
    EXPECTED_FEATURE_SCHEMA_SHA256,
    _synthetic_scene,
    load_exact_extractor,
    sha256_bytes,
    sha256_file,
)


def evaluate(repo_root: Path) -> dict:
    import numpy as np

    extractor_path = repo_root / "canonical/uncertainty/v5.0g/source/uncertainty_core_v5_0g.py"
    p1_path = repo_root / "canonical/uncertainty/v5.0g-p1/PROSPECTIVE_TELE_HOLDOUT_RESULT_v5_0g_p1.json"
    m = load_exact_extractor(extractor_path)

    feature_schema = sha256_bytes(
        json.dumps(m.FEATURE_NAMES, separators=(",", ":")).encode("utf-8")
    )
    schema_ok = (
        list(m.FEATURE_NAMES) == EXPECTED_FEATURE_NAMES
        and feature_schema == EXPECTED_FEATURE_SCHEMA_SHA256
    )
    role_constants_ok = (
        tuple(m.ROLES) == ("B", "G1", "G2", "R")
        and dict(m.ROLE_CODE) == {"R": 0, "G1": 1, "G2": 2, "B": 3}
    )

    # The exact historical predictor is vectorized. Probe one hidden location at
    # a time using 1-element coordinate arrays, then alter only its central
    # measured Stage-2 value. All predictor outputs must remain bit-identical.
    _, stage2, _, _, _, _, _, _ = _synthetic_scene(np)
    anti_leak_cases = {}
    coords = {"B": (16, 16), "G1": (16, 17), "G2": (17, 16), "R": (17, 17)}
    anti_leak_ok = True
    for role, (ys, xs) in coords.items():
        y = np.array([ys], dtype=np.int32)
        x = np.array([xs], dtype=np.int32)
        before = m.predict_hidden(stage2, role, y, x)
        changed = stage2.copy()
        changed[ys, xs] = np.float32(changed[ys, xs] + 50.0)
        after = m.predict_hidden(changed, role, y, x)
        same = all(
            np.array_equal(np.asarray(a), np.asarray(b))
            for a, b in zip(before, after)
        )
        anti_leak_cases[role] = same
        anti_leak_ok = anti_leak_ok and same

    base_scene = _synthetic_scene(np)
    m.load_scene = lambda _path: tuple(
        x.copy() if hasattr(x, "copy") else x for x in base_scene
    )
    X, yerr, role_codes, snr, coord, report = m.extract_dataset(
        Path("synthetic_semantic_probe.dng")
    )

    # Historical quirk: extractor iteration order B,G1,G2,R, but emitted labels
    # are role_R,G1,G2,B. Preserve the resulting B/R label-slot reversal.
    expected_onehot_col = {3: 14, 1: 15, 2: 16, 0: 17}
    onehot_ok = True
    onehot_observed = {}
    for role_code, col in expected_onehot_col.items():
        idx = np.flatnonzero(role_codes == role_code)
        if len(idx) == 0:
            onehot_ok = False
            onehot_observed[str(role_code)] = "MISSING"
            continue
        row = X[int(idx[0]), 14:18]
        expected = np.zeros(4, dtype=np.float32)
        expected[col - 14] = 1.0
        same = np.array_equal(row, expected)
        onehot_observed[str(role_code)] = row.tolist()
        onehot_ok = onehot_ok and same

    baseline_uncensored = int(report["targets_uncensored"])
    raw, s2, gain, bfull, denom, white, noise, meta = _synthetic_scene(np)
    by, bx = m.target_coords(raw.shape[0], raw.shape[1], "B", 0)
    if len(by) == 0:
        raise RuntimeError("synthetic scene produced no B targets")
    raw[int(by[0]), int(bx[0])] = np.float32(white)
    censored_scene = (raw, s2, gain, bfull, denom, white, noise, meta)
    m.load_scene = lambda _path: tuple(
        x.copy() if hasattr(x, "copy") else x for x in censored_scene
    )
    _, _, _, _, _, censored_report = m.extract_dataset(
        Path("synthetic_semantic_probe.dng")
    )
    source_white_exclusion_ok = (
        int(censored_report["targets_source_white_censored"]) == 1
        and int(censored_report["targets_uncensored"]) == baseline_uncensored - 1
        and int(censored_report["role_counts"]["B"]["source_white_censored"]) == 1
    )

    p1 = json.loads(p1_path.read_text(encoding="utf-8"))
    ai = p1.get("asset_integrity") or {}
    holdout_binding_ok = (
        (ai.get("feature_extractor") or {}).get("pass") is True
        and (ai.get("feature_extractor") or {}).get("got") == EXPECTED_EXTRACTOR_SHA256
        and (ai.get("production_core_cpp") or {}).get("got") == EXPECTED_CORE_CPP_SHA256
        and (ai.get("production_core_h") or {}).get("got") == EXPECTED_CORE_H_SHA256
        and p1.get("decision") == "BACKEND_BOUND_UNCERTAINTY_PROSPECTIVE_PASS"
        and (p1.get("prospective_checks") or {}).get(
            "source_white_excluded_from_exact_value_scoring"
        )
        is True
    )

    checks = {
        "extractor_exact_sha256_and_10023_bytes": (
            extractor_path.stat().st_size == 10023
            and sha256_file(extractor_path) == EXPECTED_EXTRACTOR_SHA256
        ),
        "feature_schema_exact": schema_ok,
        "historical_role_constants_exact": role_constants_ok,
        "hidden_target_predictor_independence_all_roles": anti_leak_ok,
        "historical_role_onehot_quirk_replayed": onehot_ok,
        "source_white_hidden_target_excluded_from_exact_scoring": source_white_exclusion_ok,
        "prospective_holdout_bound_to_same_extractor_and_v47i_core": holdout_binding_ok,
    }
    passed = all(checks.values())
    return {
        "schema": "TruthRawV5GExactFeatureReplay/0.2",
        "classification": (
            "PASS_EXACT_HISTORICAL_FEATURE_SEMANTICS_REPLAY"
            if passed
            else "BLOCKED_EXACT_HISTORICAL_FEATURE_SEMANTICS_REPLAY"
        ),
        "pass": passed,
        "v01_harness_result": "FAILED_SCALAR_CALL_TO_VECTORIZED_HISTORICAL_PREDICTOR",
        "v02_correction": "ONE_ELEMENT_COORDINATE_ARRAYS_ONLY_NO_EXTRACTOR_CHANGE",
        "extractor": {
            "path": str(extractor_path.relative_to(repo_root)),
            "bytes": extractor_path.stat().st_size,
            "sha256": sha256_file(extractor_path),
            "feature_schema_sha256": feature_schema,
        },
        "checks": checks,
        "anti_leak_cases": anti_leak_cases,
        "onehot_observed_by_role_code": onehot_observed,
        "synthetic_dataset": {
            "baseline_uncensored_targets": baseline_uncensored,
            "feature_width": int(X.shape[1]),
            "error_samples": int(len(yerr)),
            "snr_samples": int(len(snr)),
            "coord_samples": int(len(coord)),
        },
        "historical_prospective_binding": {
            "file_sha256": p1.get("sha256"),
            "decision": p1.get("decision"),
            "extractor_sha256": (ai.get("feature_extractor") or {}).get("got"),
            "production_core_cpp_sha256": (ai.get("production_core_cpp") or {}).get("got"),
            "production_core_h_sha256": (ai.get("production_core_h") or {}).get("got"),
            "holdout_reexecuted_in_this_gate": False,
            "reason_not_reexecuted": "original prospective DNG payload is not repository-resident evidence in this integration branch",
        },
        "scientific_boundary": {
            "synthetic_probes_create_sensor_evidence": False,
            "source_recovery_or_semantic_replay_upgrades_authority": False,
            "current_f64_trace_bound_to_exact_features": False,
            "prospective_holdout_reexecuted": False,
            "next_gate": "BIND_EXACT_V5G_FEATURE_SEMANTICS_TO_CURRENT_F64_RECONSTRUCTION_TRACE",
        },
    }


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo-root", default=".")
    ap.add_argument("--out")
    ns = ap.parse_args()
    r = evaluate(Path(ns.repo_root).resolve())
    text = json.dumps(r, indent=2, sort_keys=True)
    if ns.out:
        Path(ns.out).write_text(text + "\n", encoding="utf-8")
    print(text)
    if not r["pass"]:
        raise SystemExit(2)


if __name__ == "__main__":
    main()
