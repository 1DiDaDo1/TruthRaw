#!/usr/bin/env python3
"""TruthRaw Dynamic Authority v1.9 regeneration-readiness gate v0.2.

v0.2 preserves the negative evidence from v0.1 and fixes its scientific model.
The historical v1.9 audit froze thirteen source hashes, but five of those source
files were local recomputation/audit sources that were not committed as normal
repository files: the audit probe plus four Scientific-Master support sources.
Their absence today is therefore a provenance/source-recovery gap, not proof
that the frozen v1.9 observation is invalid.

This gate separates three questions:

1. Is the frozen v1.9 observation internally exact and unchanged?
2. Are the repository-persisted native dependencies still exact?
3. Are all historical local sources available for byte-identical replay?

Only (3) controls ``regeneration_ready``.  A coherent state with documented
local-source gaps is a valid PASS classification of the *historical state*, but
it is explicitly NOT a PASS for a new regenerator.
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

SCHEMA = "TruthRawDynamicAuthorityV19RegenerationReadiness/0.2"
PASS_FULL = "PASS_FULL_HISTORICAL_V19_SOURCE_LINEAGE_AVAILABLE"
PASS_LOCAL_GAPS = "PASS_FROZEN_V19_IDENTITY_COHERENT_HISTORICAL_LOCAL_SOURCES_NOT_PERSISTED"
BLOCKED = "BLOCKED_FROZEN_V19_IDENTITY_OR_PERSISTED_LINEAGE_MISMATCH"
STATE_PATH = Path("state/TRUTHRAW_HDR_DYNAMIC_AUTHORITY_BINDING_V19_STATE.json")

# Repository evidence shows these names were SHA-bound by the local empirical
# v1.9 recomputation but were not normal files in the original v1.9 Git tree.
# The first is explicitly described by the v1.9 README as a local audit probe.
# The other four are distinct from the earlier v1.8 archive variants and are
# therefore retained as unrecovered v1.9-local recomputation sources.
HISTORICAL_LOCAL_SOURCES = frozenset(
    {
        "probe_dynamic_authority_v19.cpp",
        "canonical_v4_7i_core.cpp",
        "canonical_v4_7i_core.h",
        "scientific_master_digest_v0_1.cpp",
        "scientific_master_digest_v0_1.h",
    }
)


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


def inspect_lineage(
    root: Path,
    expected: Mapping[str, str] | None = None,
) -> dict:
    expected = dict(expected or RECOMPUTATION_IMPLEMENTATION_SHA256_V19)
    records: Dict[str, dict] = {}
    persisted_exact = True
    local_sources_recovered_exact = True
    local_missing: list[str] = []
    local_mismatch: list[str] = []

    for basename, expected_sha in expected.items():
        is_local = basename in HISTORICAL_LOCAL_SOURCES
        paths = _candidate_paths(root, basename)
        candidates = []
        for p in paths:
            actual = sha256_file(p)
            candidates.append(
                {
                    "path": p.relative_to(root).as_posix(),
                    "sha256": actual,
                    "expected_sha256": expected_sha,
                    "exact": actual == expected_sha,
                }
            )

        if not paths:
            status = "HISTORICAL_LOCAL_SOURCE_NOT_PERSISTED" if is_local else "MISSING_REQUIRED_PERSISTED_SOURCE"
            if is_local:
                local_sources_recovered_exact = False
                local_missing.append(basename)
            else:
                persisted_exact = False
        elif len(paths) != 1:
            status = "AMBIGUOUS_MULTIPLE_CANDIDATES"
            if is_local:
                local_sources_recovered_exact = False
                local_mismatch.append(basename)
            else:
                persisted_exact = False
        elif candidates[0]["exact"]:
            status = "EXACT"
        else:
            status = "HASH_MISMATCH"
            if is_local:
                local_sources_recovered_exact = False
                local_mismatch.append(basename)
            else:
                persisted_exact = False

        records[basename] = {
            "historical_local_source": is_local,
            "status": status,
            "candidates": candidates,
        }

    persisted_names = sorted(set(expected) - HISTORICAL_LOCAL_SOURCES)
    local_names = sorted(set(expected) & HISTORICAL_LOCAL_SOURCES)
    return {
        "records": records,
        "dependency_entry_count": len(expected),
        "persisted_dependency_count": len(persisted_names),
        "historical_local_source_count": len(local_names),
        "persisted_dependency_names": persisted_names,
        "historical_local_source_names": local_names,
        "persisted_repository_dependencies_exact": persisted_exact,
        "historical_local_sources_all_recovered_exact": local_sources_recovered_exact,
        "historical_local_sources_missing": sorted(local_missing),
        "historical_local_sources_mismatched_or_ambiguous": sorted(local_mismatch),
    }


def validate_frozen_state(state: dict) -> dict:
    errors: list[str] = []
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
        "schema": state.get("schema") == "TruthRawHdrDynamicAuthorityBindingState/1.9",
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
    geometry = {
        "width": WIDTH_V19,
        "height": HEIGHT_V19,
        "pixels": PIXELS_V19,
        "rgb_records": RGB_SAMPLES_V19,
        "count_total_matches_rgb_records": total == RGB_SAMPLES_V19,
    }
    if not geometry["count_total_matches_rgb_records"]:
        errors.append("frozen_count_total_geometry_mismatch")
    return {"pass": not errors, "checks": checks, "geometry": geometry, "errors": errors}


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

    frozen = validate_frozen_state(state)
    lineage = inspect_lineage(root, expected_implementation)

    state_classification_pass = frozen["pass"] and lineage["persisted_repository_dependencies_exact"]
    regeneration_ready = state_classification_pass and lineage["historical_local_sources_all_recovered_exact"]

    if regeneration_ready:
        classification = PASS_FULL
    elif state_classification_pass and not lineage["historical_local_sources_mismatched_or_ambiguous"]:
        classification = PASS_LOCAL_GAPS
    else:
        classification = BLOCKED

    independent_reimplementation_permitted = (
        state_classification_pass
        and not lineage["historical_local_sources_mismatched_or_ambiguous"]
        and frozen["checks"].get("identity_deterministically_recomputable", False)
    )

    return {
        "schema": SCHEMA,
        "classification": classification,
        # `pass` means the current historical state is correctly classified and
        # contains no contradiction in frozen identity or persisted dependencies.
        "pass": classification != BLOCKED,
        "regeneration_ready": regeneration_ready,
        "independent_reimplementation_permitted": independent_reimplementation_permitted,
        "frozen_v19_state": frozen,
        "implementation_lineage": lineage,
        "scientific_interpretation": {
            "frozen_v19_observation_invalidated_by_local_source_absence": False,
            "historical_bulk_payload_persisted": False,
            "aggregate_counts_sufficient_to_regenerate_field": False,
            "aggregate_counts_may_be_used_only_as_falsification_checks": True,
            "exact_field_and_channel_digests_required_for_equivalence": True,
            "independent_regenerator_equivalent_before_digest_match": False,
            "source_recovery_or_independent_reimplementation_required_before_regeneration": not regeneration_ready,
        },
        "next_gate": (
            "RUN_EXACT_STREAMING_REGENERATOR_AND_FALSIFY_ALL_V19_DIGESTS_COUNTS_AND_PARTITION_INVARIANCE"
            if regeneration_ready
            else "RECOVER_FIVE_HISTORICAL_LOCAL_SOURCES_OR_INDEPENDENTLY_REIMPLEMENT_THEN_REQUIRE_EXACT_V19_DIGEST_FALSIFICATION"
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
