#!/usr/bin/env python3
from pathlib import Path
import json
import re
import sys

repo = Path(__file__).resolve().parents[1]
errors = []


def need(path: str) -> str:
    p = repo / path
    if not p.exists():
        errors.append(f"missing:{path}")
        return ""
    try:
        return p.read_text(encoding="utf-8")
    except Exception as exc:
        errors.append(f"read_error:{path}:{exc}")
        return ""


def dated(pattern: str, directory: Path):
    rx = re.compile(pattern)
    found = []
    if not directory.exists():
        return found
    for p in directory.iterdir():
        if not p.is_file():
            continue
        m = rx.fullmatch(p.name)
        if m:
            found.append((m.group(1), p))
    return sorted(found)


state_versions = dated(r"CURRENT_CANONICAL_STATE_(\d{4}-\d{2}-\d{2})\.json", repo / "state")
house_versions = dated(r"CURRENT_HOUSE_ARCHITECTURE_(\d{4}-\d{2}-\d{2})\.md", repo / "docs")
index_versions = dated(r"DOCUMENT_STATUS_INDEX_(\d{4}-\d{2}-\d{2})\.md", repo / "docs")

if not state_versions:
    errors.append("no_current_state_files")
if not house_versions:
    errors.append("no_current_house_files")
if not index_versions:
    errors.append("no_document_status_index_files")

latest_state_date = state_versions[-1][0] if state_versions else ""
latest_house_date = house_versions[-1][0] if house_versions else ""
latest_index_date = index_versions[-1][0] if index_versions else ""

if len({d for d in (latest_state_date, latest_house_date, latest_index_date) if d}) > 1:
    errors.append(
        "latest_global_dates_mismatch:"
        f"state={latest_state_date},house={latest_house_date},index={latest_index_date}"
    )

current_date = latest_state_date
state_rel = f"state/CURRENT_CANONICAL_STATE_{current_date}.json" if current_date else ""
house_rel = f"docs/CURRENT_HOUSE_ARCHITECTURE_{current_date}.md" if current_date else ""
index_rel = f"docs/DOCUMENT_STATUS_INDEX_{current_date}.md" if current_date else ""
project_map_rel = f"docs/TRUTHRAW_PROJECT_MAP_{current_date}.md" if current_date else ""
audit_rel = f"docs/audit/PROJECT_FACT_CHECK_{current_date}.md" if current_date else ""

root_readme = need("README.md")
bootstrap = need("START_HERE_NEW_CHAT.md")
state_text = need(state_rel) if state_rel else ""
house = need(house_rel) if house_rel else ""
index = need(index_rel) if index_rel else ""
need("state/README.md")

for required in (state_rel, house_rel, index_rel, project_map_rel, audit_rel):
    if required:
        need(required)

for required in (state_rel, house_rel, index_rel, project_map_rel, audit_rel):
    if required and required not in root_readme:
        errors.append(f"root_readme_missing_pointer:{required}")

for required in (state_rel, index_rel, project_map_rel, audit_rel):
    if required and required not in bootstrap:
        errors.append(f"bootstrap_missing_pointer:{required}")

m = re.search(r"## Mandatory reading order\n([\s\S]*?)(?=\n## )", bootstrap)
mandatory = m.group(1) if m else ""
if not m:
    errors.append("mandatory_reading_order_section_missing")

# No older state may appear as a numbered bootstrap authority.
for date, p in state_versions[:-1]:
    rel = p.relative_to(repo).as_posix()
    if re.search(rf"^\s*\d+\..*{re.escape(rel)}", mandatory, re.MULTILINE):
        errors.append(f"old_state_in_mandatory_reading_order:{rel}")

try:
    state = json.loads(state_text)
except Exception as exc:
    errors.append(f"current_state_invalid_json:{exc}")
    state = {}

if state:
    if state.get("date") != current_date:
        errors.append(f"current_state_date_mismatch:{state.get('date')}!={current_date}")

    entries = set(state.get("authoritative_entrypoints", []))
    required_entries = {
        "README.md",
        "START_HERE_NEW_CHAT.md",
        state_rel,
        house_rel,
        index_rel,
        project_map_rel,
        audit_rel,
        "docs/CORE_VISION_SEALED_HOUSE_ARCHITECTURE.md",
        "docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md",
    }
    for p in required_entries:
        if p and p not in entries:
            errors.append(f"missing_authoritative_entrypoint:{p}")

    for p in entries:
        if not (repo / p).exists():
            errors.append(f"authoritative_entrypoint_missing_on_disk:{p}")
        if p.startswith("docs/research/"):
            errors.append(f"research_readme_must_not_be_global_entrypoint:{p}")

    historical = set(state.get("historical_snapshots_not_bootstrap", []))
    for _, p in state_versions[:-1]:
        rel = p.relative_to(repo).as_posix()
        if rel not in historical:
            errors.append(f"older_state_not_marked_historical:{rel}")
        if rel not in index:
            errors.append(f"older_state_not_classified_in_index:{rel}")

    evidence = state.get("evidence_invariants", {})
    if evidence.get("physicalFrameCount") != 1:
        errors.append("physical_frame_count_not_one")
    if evidence.get("independentEvidenceCount") != 1:
        errors.append("independent_evidence_count_not_one")
    if evidence.get("virtual_observations_are_independent_evidence") is not False:
        errors.append("virtual_observation_evidence_guard_failed")
    if evidence.get("appearance_can_modify_scientific_master") is not False:
        errors.append("appearance_master_guard_failed")

    truthrange = state.get("truthrange", {})
    if truthrange.get("zero_line_role") != "GAUGE_REFERENCE":
        errors.append("zero_line_role_not_gauge_reference")
    if truthrange.get("sensor_dynamic_range_infinite") is not False:
        errors.append("sensor_dynamic_range_infinite_must_be_false")
    if truthrange.get("clipping_semantics") != "CENSORED_BOUND":
        errors.append("clipping_semantics_not_censored_bound")

    color = state.get("color", {})
    if color.get("full_physical_general_claim_allowed") is not False:
        errors.append("full_physical_general_claim_must_be_blocked")

    readopt = state.get("read_optimization", {})
    if readopt.get("memory_improvement_claim_allowed") is not False:
        errors.append("read_optimization_memory_claim_must_be_blocked")

    outputs = state.get("outputs", {})
    expected_outputs = {
        "DIRECT_CFA": "MEASURED_EVIDENCE",
        "SCIENTIFIC_MASTER": "RECONSTRUCTED_SCIENTIFIC_SCENE_STATE",
        "RECONSTRUCTED_CFA_DNG": "RECONSTRUCTED_CFA_PROJECTION",
        "LINEAR_DNG": "COMPATIBILITY_PROJECTION",
    }
    for k, v in expected_outputs.items():
        if outputs.get(k) != v:
            errors.append(f"output_role_mismatch:{k}:{outputs.get(k)}!={v}")

    house_state = state.get("house", {})
    if house_state.get("room_count") != 12:
        errors.append("house_room_count_not_12")
    if len(house_state.get("rooms", [])) != 12:
        errors.append("house_rooms_list_length_not_12")
    if house_state.get("runtime_science_must_be_device_invariant") is not True:
        errors.append("device_invariant_science_guard_failed")
    if house_state.get("gpu_vulkan_may_create_truth_authority") is not False:
        errors.append("gpu_truth_authority_guard_failed")

# Current high-level docs must retain the core law wording.
for text, label in (
    (root_readme, "root"),
    (bootstrap, "bootstrap"),
    (house, "house"),
    (index, "index"),
):
    if "canonical/ptc/v1.1" in text and "Pure Truth Certificate" not in text:
        errors.append(f"ptc_name_guard_missing:{label}")

if "Representation can exceed the source. Knowledge claims cannot exceed the evidence." not in root_readme:
    errors.append("root_secondary_law_missing")

# Readme-like files outside known/documented scopes should not silently become bootstrap authorities.
for p in repo.rglob("*"):
    if not p.is_file():
        continue
    rel = p.relative_to(repo).as_posix()
    name = p.name
    if not (
        name.startswith("README")
        or name == "START_HERE_NEW_CHAT.md"
        or re.match(r"CURRENT_CANONICAL_STATE_\d{4}-\d{2}-\d{2}\.json$", name)
        or re.match(r"PROJECT_STATE_AUDIT_\d{4}-\d{2}-\d{2}\.md$", name)
    ):
        continue
    classified = (
        rel in {"README.md", "START_HERE_NEW_CHAT.md", "state/README.md"}
        or rel.startswith("canonical/")
        or rel.startswith("docs/research/")
        or rel.startswith("capture/")
        or rel.startswith("docs/calibration/")
        or rel.startswith("tests/")
        or rel.startswith("state/CURRENT_CANONICAL_STATE_")
        or rel.startswith("docs/PROJECT_STATE_AUDIT_")
    )
    if not classified:
        errors.append(f"unclassified_readme_like_path:{rel}")

if errors:
    print("DOCUMENTATION_GOVERNANCE_FAIL")
    for e in errors:
        print(e)
    sys.exit(1)

print("DOCUMENTATION_GOVERNANCE_PASS")
print(f"current_date={current_date}")
print(f"authoritative_entrypoints={len(state.get('authoritative_entrypoints', []))}")
print(f"historical_current_state_snapshots={max(0, len(state_versions) - 1)}")
print("zero_line_role=GAUGE_REFERENCE")
print("sensor_dynamic_range_infinite=false")
print("read_optimization_memory_claim_allowed=false")
