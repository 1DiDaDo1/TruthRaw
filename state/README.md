# D.RAW state directory

## Current active state — 2026-10-13 — code 44489

Read first:

1. `../START_HERE_NEW_CHAT.md`
2. `../docs/handoff/DRAW_44489_MASTER_CAPSULE_CURRENT.md`
3. `../docs/handoff/DRAW_44489_NEXT_CHAT_2026-10-13.md`
4. resolve the live head of `audit/source-capability-raw-to-linear-contract-v01`
5. `DRAW_PROJECT_STATE_2026-10-13.json`
6. `../docs/DOCUMENT_STATUS_INDEX_2026-10-13.md`

Integrated scientific/runtime anchor:

`96be8a23c0d43f5dec304bdeb635824868cc80a7`

Active candidate:

- branch: `audit/source-capability-raw-to-linear-contract-v01`
- PR: `#136 — Source Capability: raw-to-linear ordering contract v0.1`
- state: draft / open / unmerged
- verified unintegrated code checkpoint: `e6bdbce11c02ccac0e3edf31f39f0eb869105d51`

The candidate reuses the one existing `SourceCapabilityRuntimeEvidenceEnvelope/0.1` cable. It does not create a second parser, source read/hash, Observation Evidence Ledger, Raw-IFD detector or pixel route.

Current represented metadata/specification order:

`Linearization -> Black Subtraction -> Rescaling / Normalization -> Clipping`

The DNG post-normalization clipping-rule is specification authority only. It does not validate observed censored samples, physical full-well/sensor saturation, recoverable scene radiance, TruthRange censor authority or Appearance highlight behavior.

`full_raw_to_linear_mapping_validated=false` remains mandatory. No sample mutation, Scientific Master pixel effect, admission/use promotion, new evidence or scientific writeback is authorized by this state.

Current technical frontier: perform a read-only audit of the existing RAW sample execution path and reuse existing execution where it exists before designing any new pixel-processing candidate.

## Historical state snapshots

All older dated files in `state/` remain provenance. In particular, the former 2026-10-02 / code 44488 state is historical and is no longer the active bootstrap. Older state files must not be rewritten to pretend they knew later results.
