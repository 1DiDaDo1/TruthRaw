# D.RAW / TruthRaw

> **CURRENT PROJECT CHECKPOINT — 2026-10-03 — continuation code `44488`**
>
> Repository integration line: `fix/research-fresh-rerun-v01`  
> Current scientific/runtime source checkpoint: `b82ada319ede5a87b0da0cdfe8732a19a3e74ff6`  
> Latest merged scientific/performance diagnostic PR: **#127 — Scientific Master Tile-Read Attribution v0.1**  
> Android lineage: versionCode `26100124`, versionName `0.53-v0.84.2-scientific-master-tile-read-attribution-v01`

D.RAW is a deterministic, provenance-bound scientific imaging system built around one permanent rule:

> **Measured where measured. Reconstructed where necessary. Never invented.**

> **MEASURED != RECONSTRUCTED != APPEARANCE.**

> **Seal the evidence, not the thinking.**

> **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

## Start here

Continuation code: **44488**

Read the current project layer in this order:

1. [`state/DRAW_PROJECT_STATE_2026-10-03.json`](state/DRAW_PROJECT_STATE_2026-10-03.json)
2. [`docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-10-03.md`](docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-10-03.md)
3. [`START_HERE_NEW_CHAT.md`](START_HERE_NEW_CHAT.md)
4. [`docs/DOCUMENT_STATUS_INDEX_2026-10-03.md`](docs/DOCUMENT_STATUS_INDEX_2026-10-03.md)
5. [`docs/research/scientific-master-exact-gauge-retained-artifact-v0.3/README.md`](docs/research/scientific-master-exact-gauge-retained-artifact-v0.3/README.md)

The previous long-form root README is preserved verbatim at `docs/history/ROOT_README_SNAPSHOT_2026-10-02.md`. The previous long-form bootstrap is preserved at `docs/history/START_HERE_SNAPSHOT_2026-10-02.md`. Older dated handoffs/state files remain provenance for their own checkpoints.

## Current validated engineering state

The recent performance line is deliberately evidence-driven and isolated:

- **v0.2.10 Pixel-Triplet Authority Encoder** remains the accepted authority-performance reference. v0.2.11 template/suffix reuse was safe but did not give meaningful device speedup, so authority micro-optimization hit its diminishing-returns gate.
- **PR #125** added a fixed-topology Center-Excluded fast path with exact parity and complete generic fallback. On the established two-RAW device test, combined Center-Excluded time fell about 50%, combined predictor time about 65%, and total profiler time about 18%, with zero scientific differences outside performance diagnostics.
- **PR #126** measured the Scientific Master binder. Combined bind time was about 15.74 s across the two RAWs; RAW source reads were about 2.64 s, reconstruction about 1.31 s, and authority observation about 9.91 s.
- **PR #127** established why each RAW had 6,144 Stage-2/RAW reads but only 3,072 reconstruction calls: the canonical v0.2 Scientific Master binder has two tile passes. Reconstruction happens in pass 1; pass 2 rereads Stage-2 support to resolve the exact low-16 self-gauge median. No per-pass timing was invented and no optimization was applied by #127.

## Current next frontier

The next isolated code candidate is **Scientific Master Exact Gauge Retained Artifact v0.3**.

The design is to retain only the exact Float32 bits already eligible for the existing self-gauge during pass 1, then resolve the low-16 median from that bounded artifact instead of rereading every Stage-2 tile.

This is **not** allowed to hard-code today's two-pass implementation as the universal architecture. The canonical v0.2 binder remains the complete fallback, and v0.3 must use an extensible versioned pass-artifact contract.

Architecture law:

> **general semantic route -> versioned specialized route -> exact parity oracle -> fail-closed fallback**

Before implementation, the exact current source eligibility/rank/Float32/cardinality/memory semantics and every second-pass consumer must be inspected. Memory budget, unsupported topology or semantic mismatch must fall back to v0.2 rather than narrowing future expansion space.

## Historical provenance pointers retained for governance

These older layers are **not** the current frontier. They remain directly discoverable because they encode permanent scientific decisions and historical checkpoints:

- `docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md`
- `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-19.md`
- `state/CURRENT_PROJECT_STATE_2026-09-19.json`
- `state/CURRENT_PROJECT_STATE_2026-09-28.json`
- `docs/DRAW_CORE_VISION_REALIGNMENT_2026-09-28.md`
- `state/CURRENT_PROJECT_STATE_2026-10-01.json`
- `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-10-01.md`
- `docs/DOCUMENT_STATUS_INDEX_2026-10-01.md`
- `state/CURRENT_PROJECT_STATE_2026-10-02.json`
- `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-10-02.md`
- `docs/DOCUMENT_STATUS_INDEX_2026-10-02.md`

## Scientific boundaries

- Direct-CFA/source evidence is immutable.
- Scientific Master is separate from presentation/export.
- Performance artifacts may remember computation; they cannot become observation or authority.
- Camera/lens/vendor/RAW identity may route parsing but cannot select scientific truth.
- UNKNOWN is valid and must not silently become zero/certainty.
- No AI/ML/neural/generative runtime exists in the scientific path.
- No diagnostic/performance route may create evidence, mutate measured anchors, promote a correction or perform scientific writeback.
- SOURCE/SENSOR SPACE, WORLD/SCENE SPACE and VIEW/OUTPUT SPACE remain distinct.

## Historical name

D.RAW is the canonical project/product name. Historical `TruthRaw` identifiers remain where required for provenance, sealed evidence, schemas, Android compatibility and reproducibility.
