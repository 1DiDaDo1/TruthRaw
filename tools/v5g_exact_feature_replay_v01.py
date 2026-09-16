#!/usr/bin/env python3
"""TruthRaw exact v5.0g historical feature-semantics replay gate v0.1.

This gate executes the recovered 10,023-byte historical extractor itself.  It
never substitutes a newly written feature implementation.  Synthetic probes are
used only to falsify semantic invariants that can be tested without the original
prospective DNG: feature schema/order, the historical role one-hot quirk,
hidden-target independence, and source-white target exclusion.

The historical prospective holdout is *bound* and integrity-checked here, not
re-executed: its original DNG is not repository evidence in this branch.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import types

EXPECTED_EXTRACTOR_SHA256 = "b1cfbf061a32aa4abccb9d8bb86a9b5b9257f88498081eb9aa659f9402d0919d"
EXPECTED_EXTRACTOR_BYTES = 10023
EXPECTED_FEATURE_SCHEMA_SHA256 = "8c8e56b762b83a5a846c5201ab3ba43c9a2549d896873cc0ceb66574e74a83e3"
EXPECTED_CORE_CPP_SHA256 = "68f52c4896d47c4605adc1cf670e55f95d42a687e18c06a167028a64ce37b94c"
EXPECTED_CORE_H_SHA256 = "b7f6e2189d6ecccfc5fda80084041990bd12757145c5ff2c40f2ae0d0046b167"
EXPECTED_FEATURE_NAMES = [
    "log1p_abs_prediction_stage2",
    "log1p_sigma_stage2_x1e4",
    "log1p_predicted_snr",
    "log1p_support_std_over_sigma",
    "log1p_support_range_over_sigma",
    "log1p_pair_min_disagreement_over_sigma",
    "log1p_pair_median_disagreement_over_sigma",
    "log1p_green_direction_disagreement_over_sigma",
    "log1p_local_mosaic_range_over_sigma",
    "gain_at_target",
    "neighbor_censor_fraction",
    "radial_position_norm",
    "prediction_negative_flag",
    "prediction_over1_flag",
    "role_R",
    "role_G1",
    "role_G2",
    "role_B",
]


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    return sha256_bytes(path.read_bytes())


def _install_import_stubs() -> None:
    # The exact extractor imports TIFF/GainMap helpers at module load time.  The
    # semantic probes below do not call DNG I/O, so minimal inert modules avoid
    # turning this replay gate into a second decoder implementation.
    if "tifffile" not in sys.modules:
        sys.modules["tifffile"] = types.ModuleType("tifffile")
    if "spatial_color_calibration" not in sys.modules:
        m = types.ModuleType("spatial_color_calibration")
        m.parse_dng_gainmaps = lambda *args, **kwargs: None
        m.bilinear_grid = lambda *args, **kwargs: None
        m.CFA_PATTERNS = {}
        sys.modules["spatial_color_calibration"] = m


def load_exact_extractor(path: Path):
    raw = path.read_bytes()
    if len(raw) != EXPECTED_EXTRACTOR_BYTES:
        raise RuntimeError(f"extractor byte length drift: {len(raw)}")
    got = sha256_bytes(raw)
    if got != EXPECTED_EXTRACTOR_SHA256:
        raise RuntimeError(f"extractor SHA-256 drift: {got}")
    _install_import_stubs()
    spec = importlib.util.spec_from_file_location("truthraw_exact_uncertainty_core_v5_0g", path)
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot load exact extractor module")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _synthetic_scene(np):
    h = w = 64
    yy, xx = np.mgrid[0:h, 0:w]
    stage2 = (
        0.04
        + 0.0031 * yy
        + 0.0023 * xx
        + 0.00017 * ((yy * xx + 3 * yy + xx) % 11)
    ).astype(np.float32)
    raw = (64.0 + stage2 * 700.0).astype(np.float32)
    gain = (1.0 + 0.001 * xx + 0.0007 * yy).astype(np.float32)
    bfull = np.full((h, w), 64.0, dtype=np.float32)
    denom = np.full((h, w), 959.0, dtype=np.float32)
    white = 1023.0
    noise = np.array(
        [0.00076, 1.8e-6, 0.00079, 1.3e-6, 0.00078, 1.7e-6],
        dtype=np.float64,
    )
    meta = {
        "file": "synthetic_semantic_probe.dng",
        "sha256": "0" * 64,
        "make": "SYNTHETIC_TEST_ONLY",
        "model": "SEMANTIC_PROBE",
        "software": "TruthRaw semantic replay",
        "shape": [h, w],
        "cfa": "BGGR",
        "white_level": white,
        "black_levels": [64.0, 64.0, 64.0, 64.0],
        "noise_profile": noise.tolist(),
        "focal_length_mm": 22.48,
        "iso": 100,
        "stage2_min": float(stage2.min()),
        "stage2_max": float(stage2.max()),
        "gain_min": float(gain.min()),
        "gain_max": float(gain.max()),
    }
    return raw, stage2, gain, bfull, denom, white, noise, meta


def evaluate(repo_root: Path) -> dict:
    import numpy as np

    extractor_path = repo_root / "canonical/uncertainty/v5.0g/source/uncertainty_core_v5_0g.py"
    p1_path = repo_root / "canonical/uncertainty/v5.0g-p1/PROSPECTIVE_TELE_HOLDOUT_RESULT_v5_0g_p1.json"
    m = load_exact_extractor(extractor_path)

    feature_schema = sha256_bytes(
        json.dumps(m.FEATURE_NAMES, separators=(",", ":")).encode("utf-8")
    )
    schema_ok = list(m.FEATURE_NAMES) == EXPECTED_FEATURE_NAMES and feature_schema == EXPECTED_FEATURE_SCHEMA_SHA256
    role_constants_ok = (
        tuple(m.ROLES) == ("B", "G1", "G2", "R")
        and dict(m.ROLE_CODE) == {"R": 0, "G1": 1, "G2": 2, "B": 3}
    )

    # Directly execute the historical predictor and change only the hidden
    # central measured target. Every predictor output must remain bit-identical.
    _, stage2, _, _, _, _, _, _ = _synthetic_scene(np)
    anti_leak_cases = {}
    coords = {"B": (16, 16), "G1": (16, 17), "G2": (17, 16), "R": (17, 17)}
    anti_leak_ok = True
    for role, (y, x) in coords.items():
        before = m.predict_hidden(stage2, role, y, x)
        changed = stage2.copy()
        changed[y, x] = np.float32(changed[y, x] + 50.0)
        after = m.predict_hidden(changed, role, y, x)
        same = all(np.array_equal(np.asarray(a), np.asarray(b)) for a, b in zip(before, after))
        anti_leak_cases[role] = same
        anti_leak_ok = anti_leak_ok and same

    base_scene = _synthetic_scene(np)
    m.load_scene = lambda _path: tuple(x.copy() if hasattr(x, "copy") else x for x in base_scene)
    X, yerr, role_codes, snr, coord, report = m.extract_dataset(Path("synthetic_semantic_probe.dng"))

    # Preserve the historical one-hot quirk exactly. Because ROLES is
    # B,G1,G2,R but labels are role_R,G1,G2,B, B activates feature 14 and R 17.
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
    m.load_scene = lambda _path: tuple(x.copy() if hasattr(x, "copy") else x for x in censored_scene)
    _, _, _, _, _, censored_report = m.extract_dataset(Path("synthetic_semantic_probe.dng"))
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
        and (p1.get("prospective_checks") or {}).get("source_white_excluded_from_exact_value_scoring") is True
    )

    checks = {
        "extractor_exact_sha256_and_10023_bytes": True,
        "feature_schema_exact": schema_ok,
        "historical_role_constants_exact": role_constants_ok,
        "hidden_target_predictor_independence_all_roles": anti_leak_ok,
        "historical_role_onehot_quirk_replayed": onehot_ok,
        "source_white_hidden_target_excluded_from_exact_scoring": source_white_exclusion_ok,
        "prospective_holdout_bound_to_same_extractor_and_v47i_core": holdout_binding_ok,
    }
    passed = all(checks.values())
    return {
        "schema": "TruthRawV5GExactFeatureReplay/0.1",
        "classification": (
            "PASS_EXACT_HISTORICAL_FEATURE_SEMANTICS_REPLAY"
            if passed
            else "BLOCKED_EXACT_HISTORICAL_FEATURE_SEMANTICS_REPLAY"
        ),
        "pass": passed,
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
