# TruthRaw documentation synchronization policy — 2026-09-11

Status: **CURRENT GLOBAL PROJECT POLICY** on the 2026-09-11 handoff line.

## Rule

Every substantive TruthRaw project change must update documentation in the same work unit.

At minimum, each change must review and, where affected, update:

1. the README/state document of the module being changed;
2. `README.md` when the global current-state summary changes;
3. `START_HERE_NEW_CHAT.md` when handoff order, current heads, proof boundaries or recommended next work changes;
4. `docs/CURRENT_MODULE_STATUS_2026-09-11.md` when a module head/status/proof boundary changes;
5. `docs/CURRENT_CLAIM_MAP_2026-09-11.md` when scientific authority or a claim boundary changes;
6. `docs/CI_EVIDENCE_INDEX_2026-09-11.md` when validation heads/runs change;
7. `docs/CHAT_HANDOFF_2026-09-11.md` when the next-chat continuation point changes;
8. `state/CURRENT_CANONICAL_STATE_2026-09-11.json` when machine-readable current state changes.

A code-only implementation commit may exist temporarily during an isolated proof cycle, but it must not be presented as the new project handoff/current state until its documentation/state overlay is committed and validated. The preferred pattern is implementation proof first, then a documentation/state overlay on the exact validated implementation head.

## Historical and sealed material

This policy does **not** authorize rewriting history.

- sealed/canonical bytes remain unchanged unless their own governance explicitly authorizes a new version;
- dated historical state snapshots remain immutable;
- previous failed runs remain failures;
- versioned research READMEs may remain frozen as exact branch/module records;
- when a historical/versioned README is intentionally immutable, add a current-state overlay or update the global indexes instead of silently editing the old record.

## Scientific discipline

Documentation must preserve the distinction between:

- implemented;
- CI validated;
- physically/device validated;
- scientifically admitted;
- independently calibrated;
- promoted to `main`.

No documentation refresh may promote a claim beyond the evidence.

## Required check before ending a work cycle

Before a TruthRaw work cycle is considered complete:

- identify every changed scientific/runtime/UI/decoder contract;
- check the corresponding README/state overlay;
- update the global handoff if the continuation point changed;
- preserve explicit non-claims and failure history;
- run the relevant documentation governance check.

This policy is itself part of the new-chat handoff and must be read by future sessions.