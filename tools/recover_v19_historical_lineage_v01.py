#!/usr/bin/env python3
"""Recover exact TruthRaw v1.9/v5.0g historical source bytes from Git history.

This tool searches both normal Git history and explicit historical commit:path
locations. Several files used by frozen Dynamic Authority v1.9 / v5.0g were
later removed or detached from advertised branches.

A candidate is recovered only when its raw bytes produce the frozen SHA-256.
Filename, size, Git blob SHA-1 identity, or similar code are never sufficient.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
from typing import Callable

SCHEMA = "TruthRawHistoricalLineageRecovery/0.1"
V47I_STAGING_COMMIT = "36cd2ca16946a412fa28f04b70ae2f423167fb43"

TARGETS = {
    "canonical_v4_7i_core.cpp": {
        "sha256": "68f52c4896d47c4605adc1cf670e55f95d42a687e18c06a167028a64ce37b94c",
        "required": True,
        "direct_locations": [
            (V47I_STAGING_COMMIT, "staging/v47i-byte-exact/core.cpp"),
        ],
        "path_match": lambda p: p.endswith("/canonical_v4_7i_core.cpp")
        or p == "canonical_v4_7i_core.cpp"
        or p.endswith("/v47i-byte-exact/core.cpp"),
    },
    "canonical_v4_7i_core.h": {
        "sha256": "b7f6e2189d6ecccfc5fda80084041990bd12757145c5ff2c40f2ae0d0046b167",
        "required": True,
        "direct_locations": [
            (V47I_STAGING_COMMIT, "staging/v47i-byte-exact/core.h"),
        ],
        "path_match": lambda p: p.endswith("/canonical_v4_7i_core.h")
        or p == "canonical_v4_7i_core.h"
        or p.endswith("/v47i-byte-exact/core.h"),
    },
    "scientific_master_digest_v0_1.cpp": {
        "sha256": "70dfd24b86f9472a98cded66ecc1130da6dd7a838322fadd55ff5152d22bbbdf",
        "required": True,
        "path_match": lambda p: p.endswith("/scientific_master_digest_v0_1.cpp")
        or p == "scientific_master_digest_v0_1.cpp",
    },
    "scientific_master_digest_v0_1.h": {
        "sha256": "89aaac2329375f7ebae1b8682868480844c75f45541bc36d26fb5b9ed618a058",
        "required": True,
        "path_match": lambda p: p.endswith("/scientific_master_digest_v0_1.h")
        or p == "scientific_master_digest_v0_1.h",
    },
    "probe_dynamic_authority_v19.cpp": {
        "sha256": "d82fc539643650fc68e4d307c3080cf0f4f1ada8dc7311ed7d8fb5005e7ce8c2",
        "required": False,
        "path_match": lambda p: p.endswith("/probe_dynamic_authority_v19.cpp")
        or p == "probe_dynamic_authority_v19.cpp",
    },
    "uncertainty_core_v5_0g.py": {
        "sha256": "b1cfbf061a32aa4abccb9d8bb86a9b5b9257f88498081eb9aa659f9402d0919d",
        "required": True,
        "path_match": lambda p: p.endswith("/uncertainty_core_v5_0g.py")
        or p == "uncertainty_core_v5_0g.py",
    },
}


def _git(repo: Path, *args: str, binary: bool = False):
    p = subprocess.run(
        ["git", "-C", str(repo), *args],
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    return p.stdout if binary else p.stdout.decode("utf-8", errors="strict")


def enumerate_historical_objects(repo: Path) -> list[tuple[str, str]]:
    refs = [
        r.strip()
        for r in _git(repo, "for-each-ref", "--format=%(refname)").splitlines()
        if r.strip()
    ]
    rev_args = ["rev-list", "--objects"] + (refs if refs else ["HEAD"])
    text = _git(repo, *rev_args)
    out: list[tuple[str, str]] = []
    seen: set[tuple[str, str]] = set()
    for line in text.splitlines():
        if not line.strip() or " " not in line:
            continue
        oid, path = line.split(" ", 1)
        key = (oid, path)
        if key in seen:
            continue
        seen.add(key)
        out.append(key)
    return out


def blob_bytes(repo: Path, oid: str) -> bytes:
    typ = _git(repo, "cat-file", "-t", oid).strip()
    if typ != "blob":
        raise ValueError(f"object {oid} is {typ}, not blob")
    return _git(repo, "cat-file", "-p", oid, binary=True)


def _candidate_from_direct_location(
    repo: Path, commit: str, path: str, expected: str
) -> dict | None:
    spec = f"{commit}:{path}"
    try:
        oid = _git(repo, "rev-parse", spec).strip()
        raw = _git(repo, "show", spec, binary=True)
    except subprocess.CalledProcessError:
        return None
    actual = hashlib.sha256(raw).hexdigest()
    return {
        "path": path,
        "historical_commit": commit,
        "git_blob_sha1": oid,
        "bytes": len(raw),
        "sha256": actual,
        "expected_sha256": expected,
        "exact": actual == expected,
        "discovery": "DIRECT_COMMIT_PATH",
    }


def _candidate_from_blob(repo: Path, oid: str, path: str, expected: str) -> dict:
    raw = blob_bytes(repo, oid)
    actual = hashlib.sha256(raw).hexdigest()
    return {
        "path": path,
        "git_blob_sha1": oid,
        "bytes": len(raw),
        "sha256": actual,
        "expected_sha256": expected,
        "exact": actual == expected,
        "discovery": "HISTORICAL_OBJECT_SCAN",
    }


def scan_history(repo: Path, targets: dict | None = None) -> dict:
    targets = targets or TARGETS
    objects = enumerate_historical_objects(repo)
    records: dict[str, dict] = {}

    for name, spec in targets.items():
        matcher: Callable[[str], bool] = spec["path_match"]
        expected = spec["sha256"]
        candidates: list[dict] = []
        seen_blobs: set[str] = set()

        # Known detached/renamed historical paths are inspected directly. This
        # avoids relying on rev-list's non-authoritative path alias selection.
        for commit, path in spec.get("direct_locations", []):
            c = _candidate_from_direct_location(repo, commit, path, expected)
            if c is None:
                continue
            candidates.append(c)
            seen_blobs.add(c["git_blob_sha1"])

        for oid, path in objects:
            if not matcher(path) or oid in seen_blobs:
                continue
            try:
                c = _candidate_from_blob(repo, oid, path, expected)
            except ValueError:
                continue
            seen_blobs.add(oid)
            candidates.append(c)

        exact = [c for c in candidates if c["exact"]]
        if len(exact) == 1:
            status = "RECOVERED_EXACT"
        elif len(exact) > 1:
            status = "RECOVERED_EXACT_MULTIPLE_HISTORICAL_OCCURRENCES"
        elif candidates:
            status = "CANDIDATES_FOUND_NO_SHA256_MATCH"
        else:
            status = "NOT_FOUND_IN_RECOVERED_GIT_OBJECTS"

        records[name] = {
            "expected_sha256": expected,
            "required_for_current_recovery_gate": bool(spec["required"]),
            "status": status,
            "exact_recovered": bool(exact),
            "exact_occurrences": exact,
            "all_candidates": candidates,
        }

    required = [n for n, s in targets.items() if s["required"]]
    required_exact = all(records[n]["exact_recovered"] for n in required)
    probe_rec = records.get("probe_dynamic_authority_v19.cpp")
    extractor_rec = records.get("uncertainty_core_v5_0g.py")
    probe_exact = bool(probe_rec and probe_rec["exact_recovered"])
    extractor_exact = bool(extractor_rec and extractor_rec["exact_recovered"])
    extractor_10023 = bool(
        extractor_rec
        and any(c["bytes"] == 10023 for c in extractor_rec["exact_occurrences"])
    )

    return {
        "schema": SCHEMA,
        "classification": (
            "PASS_REQUIRED_HISTORICAL_SOURCE_BYTES_RECOVERED_EXACT"
            if required_exact
            else "BLOCKED_REQUIRED_HISTORICAL_SOURCE_BYTES_NOT_ALL_RECOVERED"
        ),
        "pass": required_exact,
        "required_targets": required,
        "optional_targets": [n for n, s in targets.items() if not s["required"]],
        "records": records,
        "v19_local_probe_recovered_exact": probe_exact,
        "v5g_exact_10023_byte_extractor_recovered": extractor_exact and extractor_10023,
        "scientific_interpretation": {
            "matching_name_or_size_is_sufficient": False,
            "git_blob_sha1_is_substitute_for_frozen_sha256": False,
            "exact_sha256_match_required": True,
            "recovered_source_bytes_upgrade_scientific_authority": False,
            "recovered_source_bytes_close_semantic_source_recovery_only": True,
            "v19_field_equivalence_still_requires_full_digest_falsification": True,
        },
    }


def materialize_exact(repo: Path, report: dict, dest: Path) -> list[str]:
    written: list[str] = []
    dest.mkdir(parents=True, exist_ok=True)
    for name, rec in report["records"].items():
        if not rec["exact_recovered"]:
            continue
        chosen = rec["exact_occurrences"][0]
        if chosen.get("historical_commit"):
            raw = _git(
                repo,
                "show",
                f"{chosen['historical_commit']}:{chosen['path']}",
                binary=True,
            )
        else:
            raw = blob_bytes(repo, chosen["git_blob_sha1"])
        out = dest / name
        out.write_bytes(raw)
        written.append(out.as_posix())
    return written


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo-root", default=".")
    ap.add_argument("--out")
    ap.add_argument("--materialize-dir")
    ns = ap.parse_args()
    repo = Path(ns.repo_root).resolve()
    report = scan_history(repo)
    if ns.materialize_dir:
        report["materialized"] = materialize_exact(
            repo, report, Path(ns.materialize_dir).resolve()
        )
    text = json.dumps(report, indent=2, sort_keys=True)
    if ns.out:
        Path(ns.out).write_text(text + "\n", encoding="utf-8")
    print(text)
    if not report["pass"]:
        raise SystemExit(2)


if __name__ == "__main__":
    main()
