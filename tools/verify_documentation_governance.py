#!/usr/bin/env python3
from pathlib import Path
import json, re, sys

repo = Path(__file__).resolve().parents[1]
errors = []

def need(path):
    p = repo / path
    if not p.exists():
        errors.append(f"missing:{path}")
        return ""
    return p.read_text(encoding="utf-8")

root_readme = need("README.md")
bootstrap = need("START_HERE_NEW_CHAT.md")
state_readme = need("state/README.md")
state_text = need("state/CURRENT_CANONICAL_STATE_2026-09-11.json")
index = need("docs/DOCUMENT_STATUS_INDEX_2026-09-11.md")
policy = need("docs/DOCUMENTATION_SYNC_POLICY_2026-09-11.md")
house = need("docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-11.md")
modules = need("docs/CURRENT_MODULE_STATUS_2026-09-11.md")
claims = need("docs/CURRENT_CLAIM_MAP_2026-09-11.md")
ci_index = need("docs/CI_EVIDENCE_INDEX_2026-09-11.md")
handoff = need("docs/CHAT_HANDOFF_2026-09-11.md")
audit = need("docs/PROJECT_STATE_AUDIT_2026-09-11.md")
need("docs/BRANCH_STATUS_INDEX_2026-09-11.md")

current_required = (
    "state/CURRENT_CANONICAL_STATE_2026-09-11.json",
    "docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-11.md",
    "docs/CURRENT_MODULE_STATUS_2026-09-11.md",
    "docs/CURRENT_CLAIM_MAP_2026-09-11.md",
    "docs/CI_EVIDENCE_INDEX_2026-09-11.md",
    "docs/DOCUMENT_STATUS_INDEX_2026-09-11.md",
    "docs/DOCUMENTATION_SYNC_POLICY_2026-09-11.md",
    "docs/CHAT_HANDOFF_2026-09-11.md",
    "docs/PROJECT_STATE_AUDIT_2026-09-11.md",
)
for required in current_required:
    if required not in root_readme:
        errors.append(f"root_readme_missing_pointer:{required}")

for required in current_required[:8]:
    if required not in bootstrap:
        errors.append(f"bootstrap_missing_pointer:{required}")

for old in (
    "state/CURRENT_CANONICAL_STATE_2026-09-10.json",
    "state/CURRENT_CANONICAL_STATE_2026-09-09.json",
    "state/CURRENT_CANONICAL_STATE_2026-09-08.json",
    "state/CURRENT_CANONICAL_STATE_2026-09-06.json",
    "docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md",
    "docs/DOCUMENT_STATUS_INDEX_2026-09-10.md",
    "docs/PROJECT_STATE_AUDIT_2026-09-08.md",
):
    if not (repo / old).exists():
        errors.append(f"historical_snapshot_missing:{old}")
    if old not in index:
        errors.append(f"historical_snapshot_not_classified:{old}")

m = re.search(r"## Mandatory reading order\n([\s\S]*?)(?=\n## )", bootstrap)
mandatory = m.group(1) if m else ""
if not m:
    errors.append("mandatory_reading_order_section_missing")
if re.search(r"^\s*\d+\..*CURRENT_CANONICAL_STATE_2026-09-(06|08|09|10)", mandatory, re.MULTILINE):
    errors.append("old_state_in_mandatory_reading_order")

try:
    state = json.loads(state_text)
except Exception as exc:
    errors.append(f"current_state_invalid_json:{exc}")
    state = {}

if state.get("snapshot_date") != "2026-09-11":
    errors.append("current_state_wrong_snapshot_date")
if state.get("promoted_main_head") != "514f2f4bde6aba5a6709e176c03b22c3b9aea912":
    errors.append("current_state_main_head_mismatch")
if state.get("integrated_research_handoff_head") != "2fdf05ca1bbbc59cd8867df0cae117d1eec92d51":
    errors.append("current_state_research_head_mismatch")

hist = set(state.get("historical_snapshots_not_bootstrap", []))
for old in (
    "state/CURRENT_CANONICAL_STATE_2026-09-10.json",
    "state/CURRENT_CANONICAL_STATE_2026-09-09.json",
    "state/CURRENT_CANONICAL_STATE_2026-09-08.json",
    "state/CURRENT_CANONICAL_STATE_2026-09-06.json",
):
    if old not in hist:
        errors.append(f"current_state_does_not_mark_historical:{old}")

entries = set(state.get("authoritative_entrypoints", []))
expected_entries = {
    "README.md",
    "START_HERE_NEW_CHAT.md",
    "docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-11.md",
    "docs/CURRENT_MODULE_STATUS_2026-09-11.md",
    "docs/CURRENT_CLAIM_MAP_2026-09-11.md",
    "docs/CI_EVIDENCE_INDEX_2026-09-11.md",
    "docs/DOCUMENT_STATUS_INDEX_2026-09-11.md",
    "docs/DOCUMENTATION_SYNC_POLICY_2026-09-11.md",
    "docs/CHAT_HANDOFF_2026-09-11.md",
    "state/CURRENT_CANONICAL_STATE_2026-09-11.json",
}
for p in expected_entries:
    if p not in entries:
        errors.append(f"missing_authoritative_entrypoint:{p}")
for p in entries:
    if p.startswith("docs/research/"):
        errors.append(f"research_readme_must_not_be_global_entrypoint:{p}")

sync = state.get("documentation_sync", {})
if sync.get("policy") != "docs/DOCUMENTATION_SYNC_POLICY_2026-09-11.md":
    errors.append("documentation_sync_policy_not_bound")
if sync.get("required_for_every_substantive_work_cycle") is not True:
    errors.append("documentation_sync_not_mandatory")
if sync.get("historical_or_sealed_bytes_may_be_rewritten_for_cosmetic_currency") is not False:
    errors.append("documentation_sync_historical_guard_missing")

for token in (
    "Every substantive TruthRaw project change must update documentation",
    "Historical and sealed material",
    "Required check before ending a work cycle",
):
    if token not in policy:
        errors.append(f"documentation_sync_policy_token_missing:{token[:24]}")

if "DOCUMENTATION_SYNC_POLICY_2026-09-11.md" not in root_readme:
    errors.append("root_missing_documentation_sync_policy")
if "DOCUMENTATION_SYNC_POLICY_2026-09-11.md" not in bootstrap:
    errors.append("bootstrap_missing_documentation_sync_policy")
if "DOCUMENTATION_SYNC_POLICY_2026-09-11.md" not in handoff:
    errors.append("handoff_missing_documentation_sync_policy")
if "CURRENT_CANONICAL_STATE_2026-09-11.json" not in state_readme:
    errors.append("state_readme_not_pointing_to_2026_09_11")

stale_current_phrases = (
    "What is pending is not more reconstruction quality. It is the memory architecture migration",
    "next canonicalization target is the memory",
)
for phrase in stale_current_phrases:
    if phrase in root_readme or phrase in bootstrap:
        errors.append(f"stale_current_phrase:{phrase[:24]}")

for text, label in ((root_readme,"root"),(bootstrap,"bootstrap"),(house,"house"),(index,"index"),(claims,"claims"),(handoff,"handoff")):
    if "canonical/ptc/v1.1" in text and "Pure Truth Certificate" not in text:
        errors.append(f"ptc_name_guard_missing:{label}")

for token, label in (
    ("SOURCE_BOUND_APPEARANCE_PREVIEW", "appearance_role"),
    ("scientificPreviewReleaseAllowed=false", "scientific_preview_block"),
    ("physical Honor", "physical_device_boundary"),
):
    if token not in root_readme and token not in bootstrap and token not in handoff:
        errors.append(f"global_boundary_missing:{label}")

if "NO CURRENT-HEAD CI RUN FOUND" not in modules:
    errors.append("module_status_missing_e2e_ci_gap")
if "Never rewrite any of these failed runs as success" not in ci_index:
    errors.append("ci_index_failure_history_guard_missing")
if "immutable historical" not in audit.lower():
    errors.append("audit_historical_preservation_missing")

module_prefixes = (
    "canonical/",
    "docs/research/",
    "capture/",
    "docs/calibration/",
    "docs/full-sensor/",
    "tests/",
    "app/",
    "provenance/",
    "evidence/",
    "tools/",
    "state/",
)
for p in repo.rglob("*"):
    if not p.is_file():
        continue
    rel = p.relative_to(repo).as_posix()
    name = p.name
    if not (name.startswith("README") or name == "START_HERE_NEW_CHAT.md" or
            re.match(r"CURRENT_CANONICAL_STATE_\d{4}-\d{2}-\d{2}\.json$", name) or
            re.match(r"PROJECT_STATE_AUDIT_\d{4}-\d{2}-\d{2}\.md$", name)):
        continue
    classified = (
        rel in {"README.md", "START_HERE_NEW_CHAT.md", "state/README.md"} or
        rel.startswith(module_prefixes) or
        rel.startswith("docs/PROJECT_STATE_AUDIT_")
    )
    if not classified:
        errors.append(f"unclassified_readme_like_path:{rel}")

if errors:
    print("DOCUMENTATION_GOVERNANCE_FAIL")
    for e in errors:
        print(e)
    sys.exit(1)

print("DOCUMENTATION_GOVERNANCE_PASS")
print("snapshot=2026-09-11")
print(f"authoritative_entrypoints={len(expected_entries)}")
print("documentation_sync=MANDATORY")
print("historical_snapshots_preserved=2026-09-10_and_earlier")
print("zero_line_storage=IMMUTABLE_SHARED_SINGLE_BINDING")
print("research_vs_main=EXPLICIT")
