# TruthRaw document status index — 2026-09-11

This is the authority map for documentation. It prevents older snapshots and branch-local research notes from silently becoming current global truth.

## Current global entrypoints

These define the current 2026-09-11 handoff:

- `README.md`
- `START_HERE_NEW_CHAT.md`
- `state/CURRENT_CANONICAL_STATE_2026-09-11.json`
- `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-11.md`
- `docs/CURRENT_MODULE_STATUS_2026-09-11.md`
- `docs/CURRENT_CLAIM_MAP_2026-09-11.md`
- `docs/CI_EVIDENCE_INDEX_2026-09-11.md`
- `docs/CHAT_HANDOFF_2026-09-11.md`
- `docs/PROJECT_STATE_AUDIT_2026-09-11.md`

If these disagree with an older dated dashboard, the 2026-09-11 layer is current for handoff while the older file remains historical evidence.

## Historical immutable global snapshots

Preserve; do not use as current bootstrap:

- `state/CURRENT_CANONICAL_STATE_2026-09-10.json`
- `state/CURRENT_CANONICAL_STATE_2026-09-09.json`
- `state/CURRENT_CANONICAL_STATE_2026-09-08.json`
- `state/CURRENT_CANONICAL_STATE_2026-09-06.json`
- `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md`
- `docs/DOCUMENT_STATUS_INDEX_2026-09-10.md`
- `docs/PROJECT_STATE_AUDIT_2026-09-08.md`
- `state/REPOSITORY_MIGRATION_STATUS.json`

The 2026-09-10 architecture correctly described a then-open memory migration. It is not wrong history; it is simply superseded as the current dashboard by later TileNative/Streaming/Room/Gatehouse/Preview work.

## Canonical documentation

Files under `canonical/**` describe exact canonical module/version semantics. They are not global current-status dashboards. Do not rewrite canonical documentation merely to align terminology with a research branch unless the canonical version itself is intentionally revised under its governance.

`canonical/ptc/v1.1` means **Pure Truth Certificate**, not photon-transfer-curve calibration.

## Core vision documents

`docs/CORE_VISION_*.md` capture durable conceptual doctrine (sealed house, uncertainty-aware appearance, virtual observation manifold, zero-line/TruthRange). They remain conceptual/canonical vision inputs but do not carry current branch/CI status.

## Research documentation

`docs/research/**` is module-local, branch/version-scoped research evidence. A research README may legitimately say “pending” if it is the original contract snapshot; later CI evidence may be recorded in a state/report/PR/current global index instead of mutating sealed or historically meaningful module bytes.

Research module documentation never becomes a global bootstrap entrypoint merely by being newer.

## Failure histories

Any `FAILURE_HISTORY*`, rejected candidate record or failed CI run is historical evidence. Preserve the failure label. A later corrected run may be linked, but the earlier failure must not be rewritten as PASS.

## Cross-branch limitation

The current documentation branch is based on the Android preview research lineage. Some professional-RAW/Gatehouse modules live on separate stacked branches and are therefore indexed cross-branch rather than physically copied into this branch. Their branch head and CI evidence are listed in `CURRENT_MODULE_STATUS_2026-09-11.md` and `CI_EVIDENCE_INDEX_2026-09-11.md`.

## Workflow filename note

The repository workflow file `.github/workflows/documentation-governance-2026-09-10.yml` retains its historical filename/workflow identity, but its contents are updated by this handoff to validate the 2026-09-11 current snapshot plus preservation of prior snapshots. Filename age does not make the validated state 2026-09-10 current.