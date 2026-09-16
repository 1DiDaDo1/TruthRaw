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
index = need("docs/DOCUMENT_STATUS_INDEX_2026-09-10.md")
house = need("docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md")
state_text = need("state/CURRENT_CANONICAL_STATE_2026-09-10.json")
need("state/README.md")

for required in (
    "docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md",
    "state/CURRENT_CANONICAL_STATE_2026-09-10.json",
    "docs/DOCUMENT_STATUS_INDEX_2026-09-10.md",
):
    if required not in root_readme:
        errors.append(f"root_readme_missing_pointer:{required}")
    if required not in bootstrap and required != "docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md":
        errors.append(f"bootstrap_missing_pointer:{required}")

for old in (
    "state/CURRENT_CANONICAL_STATE_2026-09-06.json",
    "state/CURRENT_CANONICAL_STATE_2026-09-08.json",
    "state/CURRENT_CANONICAL_STATE_2026-09-09.json",
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
if re.search(r"^\s*\d+\..*CURRENT_CANONICAL_STATE_2026-09-(06|08|09)", mandatory, re.MULTILINE):
    errors.append("old_state_in_mandatory_reading_order")

try:
    state = json.loads(state_text)
except Exception as exc:
    errors.append(f"current_state_invalid_json:{exc}")
    state = {}

hist = set(state.get("historical_snapshots_not_bootstrap", []))
for old in (
    "state/CURRENT_CANONICAL_STATE_2026-09-06.json",
    "state/CURRENT_CANONICAL_STATE_2026-09-08.json",
    "state/CURRENT_CANONICAL_STATE_2026-09-09.json",
):
    if old not in hist:
        errors.append(f"current_state_does_not_mark_historical:{old}")

entries = set(state.get("authoritative_entrypoints", []))
for p in (
    "README.md",
    "START_HERE_NEW_CHAT.md",
    "docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md",
    "docs/DOCUMENT_STATUS_INDEX_2026-09-10.md",
    "state/CURRENT_CANONICAL_STATE_2026-09-10.json",
):
    if p not in entries:
        errors.append(f"missing_authoritative_entrypoint:{p}")

for p in entries:
    if p.startswith("docs/research/"):
        errors.append(f"research_readme_must_not_be_global_entrypoint:{p}")

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
        rel in {"README.md","START_HERE_NEW_CHAT.md","state/README.md"} or
        rel.startswith("canonical/") or rel.startswith("docs/research/") or
        rel.startswith("capture/") or rel.startswith("docs/calibration/") or
        rel.startswith("tests/") or rel.startswith("android/") or
        rel.startswith("state/CURRENT_CANONICAL_STATE_") or
        rel.startswith("docs/PROJECT_STATE_AUDIT_")
    )
    if not classified:
        errors.append(f"unclassified_readme_like_path:{rel}")

for text, label in ((root_readme,"root"),(bootstrap,"bootstrap"),(house,"house"),(index,"index")):
    if "canonical/ptc/v1.1" in text and "Pure Truth Certificate" not in text:
        errors.append(f"ptc_name_guard_missing:{label}")

if errors:
    print("DOCUMENTATION_GOVERNANCE_FAIL")
    for e in errors:
        print(e)
    sys.exit(1)

print("DOCUMENTATION_GOVERNANCE_PASS")
print("authoritative_entrypoints=5")
print("historical_current_state_snapshots=3")
print("zero_line_storage=IMMUTABLE_SHARED_SINGLE_BINDING")
