# D.RAW / TruthRaw — CURRENT CHECKPOINT — 2026-10-03 — CODE 44488

Continuation code: **44488**

This file is the current bootstrap. Historical checkpoint detail remains in dated handoffs/state files and preserved snapshots; do not treat an older checkpoint as current merely because it contains more detail.

## Mandatory current reading order

1. `state/CURRENT_PROJECT_STATE_2026-10-03.json`
2. `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-10-03.md`
3. `docs/DOCUMENT_STATUS_INDEX_2026-10-03.md`
4. `docs/research/scientific-master-exact-gauge-retained-artifact-v0.3/README.md`
5. inspect the actual `fix/research-fresh-rerun-v01` head and open/merged PR state before changing code.

The current layer supersedes older statements about branch/build/next task. Historical pointers below remain discoverable for provenance and permanent scientific decisions.

## Current repository/code checkpoint

Repository: `1DiDaDo1/TruthRaw`

Active integration branch: `fix/research-fresh-rerun-v01`

Current scientific/runtime source checkpoint before this documentation-only refresh:

`b82ada319ede5a87b0da0cdfe8732a19a3e74ff6`

This is the merge of PR #127, **Scientific Master Tile-Read Attribution v0.1**.

Current Android lineage:

- application ID `com.truthraw.adaptiveui`
- versionCode `26100124`
- versionName `0.53-v0.84.2-scientific-master-tile-read-attribution-v01`

## What has just been proven

- PR #125 fixed-topology Center-Excluded is merged and real-device validated with exact scientific parity. Combined Center-Excluded time fell about 50%, combined predictor time about 65%, and the complete two-RAW profiler about 18% for the established test pair.
- PR #126 measured the Scientific Master binder and established RAW source reads as a distinct material cost after the Center-Excluded win.
- PR #127 reconciled `6144` RAW reads vs `3072` reconstruction calls per established RAW: the canonical v0.2 binder has two tile passes, reconstruction only in pass 1, and pass 2 rereads Stage-2 support for exact low-16 self-gauge median resolution.
- PR #127 did not itself optimize the binder and did not infer unmeasured per-pass timings.

## Current next frontier

**Scientific Master Exact Gauge Retained Artifact v0.3**.

The design target is to retain only the exact Float32 bits already eligible for the existing self-gauge during pass 1, then resolve the low-16 median from that bounded artifact instead of rereading every Stage-2 tile.

The canonical v0.2 two-pass binder remains intact as the complete fallback.

Do not hard-code today's two-pass schedule as the universal architecture. v0.3 must use an extensible, versioned pass-artifact contract so future passes/intermediate products/topologies can be added without narrowing the system.

General optimization law:

**general semantic route -> versioned specialized route -> exact parity oracle -> fail-closed fallback**

Before implementation, source-inspect the exact v0.2 eligibility/rank/Float32/cardinality/memory contract and verify that pass 2 has no additional scientific consumer that the artifact would omit.

## Historical provenance pointers required for continuity

The following are not the current frontier. They remain discoverable because the documentation governance suite protects their scientific/historical role:

- `docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md`
- `state/CURRENT_PROJECT_STATE_2026-09-16.json`
- `docs/DOCUMENT_STATUS_INDEX_2026-09-16.md`
- `docs/PROJECT_HISTORY_AND_CHANGES_2026-09-16.md`
- `docs/handoff/TRUTHRAW_CONSOLIDATED_HANDOFF_2026-09-16.md`
- `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-19.md`
- `state/CURRENT_PROJECT_STATE_2026-09-19.json`
- `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-20.md`
- `state/CURRENT_PROJECT_STATE_2026-09-20.json`
- `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-21.md`
- `state/CURRENT_PROJECT_STATE_2026-09-21.json`
- `state/CURRENT_PROJECT_STATE_2026-09-24.json`
- `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-24.md`
- `state/CURRENT_PROJECT_STATE_2026-09-25.json`
- `docs/DRAW_MAIN_PROJECT_STATE_2026-09-25.md`
- `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-09-25.md`
- `state/CURRENT_PROJECT_STATE_2026-09-27.json`
- `docs/DRAW_KNOWLEDGE_GROWTH_INTEGRATION_2026-09-27.md`
- `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-09-27.md`
- `docs/DOCUMENT_STATUS_INDEX_2026-09-27.md`
- `docs/research/free-world-observation-graph-v0.1/README.md`
- `state/CURRENT_PROJECT_STATE_2026-09-28.json`
- `docs/DRAW_CORE_VISION_REALIGNMENT_2026-09-28.md`
- `state/CURRENT_PROJECT_STATE_2026-10-01.json`
- `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-10-01.md`
- `docs/DOCUMENT_STATUS_INDEX_2026-10-01.md`
- `docs/research/measured-field-support-coordinate-bridge-v0.1/README.md`
- `docs/research/optical-field-topography-v0.1/README.md`
- `state/CURRENT_PROJECT_STATE_2026-10-02.json`
- `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-10-02.md`
- `docs/DOCUMENT_STATUS_INDEX_2026-10-02.md`

The former long bootstrap is preserved verbatim at `docs/history/START_HERE_SNAPSHOT_2026-10-02.md`; the former long root README is preserved at `docs/history/ROOT_README_SNAPSHOT_2026-10-02.md`.

## Permanent scientific laws

- `MEASURED != RECONSTRUCTED != APPEARANCE`.
- Seal the evidence, not the thinking.
- Representation can exceed the source; knowledge claims cannot exceed the evidence.
- Source/Direct-CFA evidence remains immutable.
- Scientific Master is separate from export/presentation.
- Performance caches/artifacts cannot create authority.
- Camera/lens/vendor/RAW identity may route parsing but may not select scientific truth.
- No AI/ML/neural/generative runtime in the scientific path.
- No diagnostic/performance candidate may create new evidence or perform scientific writeback.
- SOURCE/SENSOR SPACE, WORLD/SCENE SPACE and VIEW/OUTPUT SPACE remain distinct.
- UNKNOWN remains valid when evidence/support is absent.

A new chat receiving only **44488** should restore this checkpoint, verify whether the repository has advanced, and continue from the newest proven state without asking the user to reconstruct the project manually.
