# TruthRaw Document Status Index — 2026-09-13

Purpose: prevent stale dated documents or module-local research notes from being mistaken for current global project state.

## Global current / bootstrap authority

These files are the current project-level entrypoints on the 2026-09-13 audit branch:

| File | Status | Scope |
|---|---|---|
| `README.md` | CURRENT | repository landing page and permanent laws |
| `START_HERE_NEW_CHAT.md` | CURRENT | mandatory bootstrap/reading order |
| `docs/TRUTHRAW_PROJECT_MAP_2026-09-13.md` | CURRENT | project-wide map of useful architecture/state |
| `docs/audit/PROJECT_FACT_CHECK_2026-09-13.md` | CURRENT AUDIT | scientific/standards/architecture fact-check and corrections |
| `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-13.md` | CURRENT | current house/runtime design and optimization direction |
| `state/CURRENT_CANONICAL_STATE_2026-09-13.json` | CURRENT STATE | machine-readable current project-level state |
| `docs/DOCUMENT_STATUS_INDEX_2026-09-13.md` | CURRENT INDEX | this document |

## Permanent core-vision authority

These remain active semantic foundations unless a later document explicitly supersedes a particular rule:

| File | Status | Notes |
|---|---|---|
| `docs/CORE_VISION_SEALED_HOUSE_ARCHITECTURE.md` | ACTIVE CORE | sealed evidence/new-house separation |
| `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md` | ACTIVE CORE | zero-line/TruthRange semantic architecture |
| `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_EVOLUTION_2026-09-13.md` | ACTIVE HISTORY/CLARIFICATION | recovered genealogy; clarifies that v0.2 did not invent the zero-line |

The evolution document adds historical context; it does not silently mutate bytes or erase older research records.

## Module-local authority

Files under `canonical/`, `docs/research/`, `capture/`, `docs/calibration/`, `tests/`, and similar module directories may be authoritative for the exact module/version they accompany. They are **not automatically global current-state authorities**.

When working on a module:

1. bootstrap from the global current files above;
2. read that module's own README/manifest/evidence/tests;
3. preserve its explicit PASS/FAIL/open state;
4. do not promote a module because a newer unrelated global audit exists.

## Current module-level observations relevant to the audit

The following are project-level observations, not replacements for module manifests:

| Area | Project-level status |
|---|---|
| sealed Direct-CFA evidence | ACTIVE / non-negotiable |
| single-frame evidence accounting | ACTIVE / non-negotiable |
| TruthRange/zero-line | ACTIVE; address-space vs evidence distinction clarified |
| Scientific Master | ACTIVE reconstructed scientific scene role |
| finalized source-bound preview | physically exercised/release-capable under source-bound authority; not FULL_PHYSICAL |
| read optimization v0.2 | physically exercised; read/time improvements supported; memory-improvement claim blocked |
| Technical Backplane | active architecture/validated subwork depending on exact module; never image evidence |
| Room/Building Runtime | active architecture; next production optimization is leases/tokens/profiling |
| Linear DNG / raw projection export | ACTIVE VALIDATION / compatibility projection |
| reconstructed CFA DNG | allowed only as explicit reconstructed projection |
| external RAW Gatehouse | active ingress architecture; decoder output must fail closed on ambiguity |
| FULL_PHYSICAL general color/light claims | BLOCKED pending independent calibration |
| Vulkan/GPU scientific backend | NOT AUTHORITY; future optional execution backend only |

## Historical project-level snapshots

These files are intentionally preserved but must **not** be used as the first bootstrap state after 2026-09-13:

| File | Status |
|---|---|
| `state/CURRENT_CANONICAL_STATE_2026-09-06.json` | HISTORICAL SNAPSHOT |
| `state/CURRENT_CANONICAL_STATE_2026-09-08.json` | HISTORICAL SNAPSHOT |
| `state/CURRENT_CANONICAL_STATE_2026-09-09.json` | HISTORICAL SNAPSHOT |
| `state/CURRENT_CANONICAL_STATE_2026-09-10.json` | HISTORICAL SNAPSHOT |
| `docs/PROJECT_STATE_AUDIT_2026-09-08.md` | HISTORICAL AUDIT |
| `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md` | HISTORICAL ARCHITECTURE SNAPSHOT |
| `docs/DOCUMENT_STATUS_INDEX_2026-09-10.md` | HISTORICAL INDEX |

Historical files are not wrong merely because they are old. They describe the state at their recorded point in the project's evolution.

## Conflict resolution rule

If two documents disagree:

1. source evidence/manifests/tests control claims about their own recorded experiment;
2. module-local canonical documentation controls the exact module/version;
3. the newest current global state/index controls which project-level entrypoints are current;
4. a newer project map does **not** turn an old failed experiment into a pass;
5. a branch name, chat summary, screenshot, or README title alone is never sufficient to promote scientific authority.

## Known 2026-09-13 governance correction

The previous bootstrap files still pointed globally at the 2026-09-10 state/house/index even though the active repository lineage had accumulated newer preview, read-optimization and DNG/export work. In addition, the 2026-09-10 state JSON and status index were not fully aligned on some module status.

The 2026-09-13 layer fixes the global-navigation inconsistency while preserving all older files unchanged as history.

## Negative-evidence preservation

The document/status system must retain failures and rejected candidates, including but not limited to:

- non-convergent synthetic neutral/color cases;
- failed preview/color quality gates;
- CI/writer failures that were later repaired;
- branch divergence/integration risk;
- physical metrics that did not improve (for example peak PSS in the read-optimization run).

Do not rewrite those records to match the latest implementation.

## Governance automation

`tools/verify_documentation_governance.py` is updated on the audit branch to discover the newest dated current-state/house/index documents rather than permanently hard-coding 2026-09-10. It also guards important cross-document invariants and historical classification.
