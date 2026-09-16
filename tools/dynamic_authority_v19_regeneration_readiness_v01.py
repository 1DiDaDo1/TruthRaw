#!/usr/bin/env python3
"""TruthRaw Dynamic Authority v1.9 regeneration-readiness gate v0.1.

This gate answers a deliberately narrower question than regeneration itself:

    Is the frozen v1.9 scientific identity still bound to an exact repository
    lineage from which an independent streaming regenerator may be attempted?

It MUST NOT regenerate the 37,601,280 authority records from aggregate counts.
It MUST NOT treat the absence of the historical local audit-probe source as
proof that v1.9 was scientifically unreproducible.  The frozen v1.9 state says
that the payload was not persisted and that the identity was deterministically
recomputable.  The historical local probe is therefore a provenance/artifact
question, while exact digest reproduction remains the future falsification gate.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from copy import deepcopy
from pathlib import Path
import sys
from typing import Dict, Mapping

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / "tools"
# Historical open-world modules use a mixture of package-style `tools.x` imports
# and same-directory top-level imports such as `open_world_foundations_v01`.
# Both paths are required to exercise the frozen v1.9 binding unchanged.
for import_root in (ROOT, TOOLS):
    if str(import_root) not in sys.path:
        sys.path.insert(0, str(import_root))

from tools.truthraw_hdr_dynamic_authority_binding_v19 import (  # noqa: E402
    DYNAMIC_AUTHORITY_CHANNEL_SHA256_V19,
    DYNAMIC_AUTHORITY_FIELD_SHA256_V19,
    FROZEN_REAL_RUN_V19,
    HEIGHT_V19,
    PIXELS_V19,
    RECOMPUTATION_IMPLEMENTATION_SHA256_V19,
    RGB_SAMPLES_V19,
    SCIENTIFIC_MASTER_SHA256_V18,
    SOURCE_CFA_SHA256_V18,
    SOURCE_DNG_SHA256_V18,
    WIDTH_V19,
)

SCHEMA = "TruthRawDynamicAuthorityV19RegenerationReadiness/0.1"
PASS_WITH_PROBE = "PASS_FULL_FROZEN_RECOMPUTATION_IMPLEMENTATION_AVAILABLE"
PASS_PROBE_GAP = "PASS_FROZEN_LINEAGE_EXACT_LOCAL_PROBE_SOURCE_NOT_PERSISTED"
BLOCKED = "BLOCKED_DYNAMIC_AUTHORITY_V19_REGENERATION_READINESS"
LOCAL_PROBE = "probe_dynamic_authority_v19.cpp"
STATE_PATH = Path("state/TRUTHRAW_HDR_DYNAMIC_AUTHORITY_BINDING_V19_STATE.json")


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def _candidate_paths(root: Path, basename: str) -> list[Path]:
    ignored = {".git", "__pycache__", ".pytest_cache", ".mypy_cache"}
    out: list[Path] = []
    for p in root.rglob(basename):
        if not p.is_file():
            continue
        try:
            rel = p.relative_to(root)
        except ValueError:
            continue
        if any(part in ignored for part in rel.parts):
            continue
        out.append(p)
    return sorted(out)


def inspect_implementation_lineage(
    root: Path,
    expected: Mapping[str, str] | None = None,
) -> dict:
    expected = dict(expected or RECOMPUTATION_IMPLEMENTATION_SHA256_V19)
    records: Dict[str, dict] = {}
    required_exact = True
    probe_status = "UNKNOWN"

    for basename, expected_sha in expected.items():
        paths = _candidate_paths(root, basename)
        matches = []
        for p in paths:
            actual = sha256_file(p)
            matches.append(
                {
                    "path": p.relative_to(root).as_posix(),
                    "sha256": actual,
                    "expected_sha256": expected_sha,
                    "exact": actual == expected_sha,
                }
            )

        is_probe = basename == LOCAL_PROBE
        if not paths:
            status = "ABSENT"
            if is_probe:
                probe_status = "NOT_PERSISTED_IN_REPOSITORY"
            else:
                required_exact = False
        elif len(paths) != 1:
            status = "AMBIGUOUS_MULTIPLE_CANDIDATES"
            if is_probe:
                probe_status = status
            else:
                required_exact = False
        elif matches[0]["exact"]:
            status = "EXACT"
            if is_probe:
                probe_status = "EXACT_SOURCE_PRESENT"
        else:
            status = "HASH_MISMATCH"
            if is_probe:
                probe_status = status
            else:
                required_exact = False

        records[basename] = {
            "is_historical_local_probe": is_probe,
            "status": status,
            "candidates": matches,
        }

    probe = records.get(LOCAL_PROBE, {})
    probe_exact_or_absent = probe.get("status") in {"EXACT", "ABSENT"}
    return {
        "records": records,
        "required_repository_dependencies_exact": required_exact,
        "historical_local_probe_status": probe_status,
        "historical_local_probe_exact_or_documented_absent": probe_exact_or_absent,
        "dependency_entry_count": len(expected),
        "non_probe_dependency_count": len(expected) - (1 if LOCAL_PROBE in expected else 0),
    }


def validate_frozen_state(state: dict) -> dict:
    errors: list[str] = []

    if state.get("schema") != "TruthRawHdrDynamicAuthorityBindingState/1.9":
        errors.append("schema_mismatch")

    source = state.get("source") or {}
    master = state.get("scientific_master") or {}
    da = state.get("dynamic_authority") or {}
    validation = state.get("validation") or {}

    expected_counts = {
        "CALIBRATED_ESTIMATE": FROZEN_REAL_RUN_V19.counts.calibrated_estimate,
        "RECONSTRUCTED": FROZEN_REAL_RUN_V19.counts.reconstructed,
        "CENSORED": FROZEN_REAL_RUN_V19.counts.censored,
        "UNKNOWN": FROZEN_REAL_RUN_V19.counts.unknown,
    }

    checks = {
        "source_dng_sha256": source.get("sha256") == SOURCE_DNG_SHA256_V18,
        "source_cfa_sha256": source.get("decoded_cfa_sha256") == SOURCE_CFA_SHA256_V18,
        "scientific_master_sha256": master.get("sha256") == SCIENTIFIC_MASTER_SHA256_V18,
        "dynamic_authority_field_sha256": da.get("field_sha256") == DYNAMIC_AUTHORITY_FIELD_SHA256_V19,
        "dynamic_authority_channel_sha256": tuple(da.get("channel_sha256") or ())
        == tuple(DYNAMIC_AUTHORITY_CHANNEL_SHA256_V19),
        "authority_counts": da.get("counts") == expected_counts,
        "payload_not_persisted": da.get("payload_persisted_by_v19") is False,
        "identity_deterministically_recomputable": da.get("identity_deterministically_recomputable") is True,
        "partition_192_pass": validation.get("local_execution_band_192") == "PASS",
        "partition_257_pass": validation.get("local_execution_band_257") == "PASS",
        "partition_invariance_pass": validation.get("partition_invariance") == "PASS_IDENTICAL",
    }
    errors.extend(k for k, ok in checks.items() if not ok)

    total = sum(int(v) for v in expected_counts.values())
    geometry_checks = {
        "width": WIDTH_V19,
        "height": HEIGHT_V19,
        "pixels": PIXELS_V19,
        "rgb_records": RGB_SAMPLES_V19,
        "count_total_matches_rgb_records": total == RGB_SAMPLES_V19,
    }
    if not geometry_checks["count_total_matches_rgb_records"]:
        errors.append("frozen_count_total_geometry_mismatch")

    return {
        "pass": not errors,
        "checks": checks,
        "geometry": geometry_checks,
        "errors": errors,
    }


def evaluate_readiness(
    root: Path = ROOT,
    *,
    state: dict | None = None,
    expected_implementation: Mapping[str, str] | None = None,
) -> dict:
    if state is None:
        state = json.loads((root / STATE_PATH).read_text(encoding="utf-8"))
    else:
        state = deepcopy(state)

    lineage = inspect_implementation_lineage(root, expected_implementation)
    frozen = validate_frozen_state(state)

    probe_status = lineage["historical_local_probe_status"]
    hard_pass = (
        frozen["pass"]
        and lineage["required_repository_dependencies_exact"]
        and lineage["historical_local_probe_exact_or_documented_absent"]
    )

    if hard_pass and probe_status == "EXACT_SOURCE_PRESENT":
        classification = PASS_WITH_PROBE
    elif hard_pass and probe_status == "NOT_PERSISTED_IN_REPOSITORY":
        classification = PASS_PROBE_GAP
    else:
        classification = BLOCKED

    independent_regenerator_status = (
        "READY_TO_IMPLEMENT_AND_FALSIFY_AGAINST_FROZEN_DIGESTS"
        if hard_pass
        else "BLOCKED_UNTIL_FROZEN_LINEAGE_OR_STATE_MISMATCH_IS_RESOLVED"
    )

    return {
        "schema": SCHEMA,
        "classification": classification,
        "pass": hard_pass,
        "frozen_v19_state": frozen,
        "implementation_lineage": lineage,
        "scientific_interpretation": {
            "historical_v19_identity_documented_deterministically_recomputable": frozen["checks"].get(
                "identity_deterministically_recomputable", False
            ),
            "historical_bulk_payload_persisted": not frozen["checks"].get("payload_not_persisted", False),
            "missing_local_probe_source_equals_scientific_failure": False,
            "aggregate_counts_sufficient_to_regenerate_field": False,
            "aggregate_counts_may_be_used_as_output_falsification": True,
            "exact_field_and_channel_digests_are_required_falsification_targets": True,
            "independent_regenerator_is_equivalent_before_digest_match": False,
        },
        "independent_regenerator_status": independent_regenerator_status,
        "next_gate": (
            "IMPLEMENT_INDEPENDENT_STREAMING_V19_REGENERATOR_THEN_REQUIRE_EXACT_FIELD_CHANNEL_COUNTS_AND_PARTITION_INVARIANCE"
            if hard_pass
            else "REPAIR_LINEAGE_OR_FROZEN_STATE_BEFORE_REGENERATOR_WORK"
        ),
    }


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo-root", default=str(ROOT))
    ap.add_argument("--out")
    ns = ap.parse_args()
    result = evaluate_readiness(Path(ns.repo_root).resolve())
    text = json.dumps(result, indent=2, sort_keys=True)
    if ns.out:
        Path(ns.out).write_text(text + "\n", encoding="utf-8")
    print(text)
    if not result["pass"]:
        raise SystemExit(2)


if __name__ == "__main__":
    main()
