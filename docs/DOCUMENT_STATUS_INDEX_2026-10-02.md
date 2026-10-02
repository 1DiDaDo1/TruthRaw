# D.RAW document status index — 2026-10-02

## Global current bootstrap

This index supersedes the 2026-10-01 index for **global current-state navigation only**. Older dated documents remain authoritative provenance for the exact experiments and checkpoints they describe.

Read first:

1. `../state/CURRENT_PROJECT_STATE_2026-10-02.json`
2. `handoff/DRAW_NEXT_CHAT_HANDOFF_2026-10-02.md`
3. `../START_HERE_NEW_CHAT.md`
4. `research/truthnegative-authority-direct-stream-v0.1/README.md`
5. `research/truthnegative-authority-field-fusion-v0.1/README.md`
6. `research/truthnegative-n2-row-band-reuse-v0.1/README.md`
7. `research/shared-scientific-context-v0.1/README.md`
8. `research/free-world-observation-geometry-foundation-v0.1/README.md`

Continuation code: **44488**

Current source-code merge checkpoint:

`33c69b635be5bcc57ec4a947eb8fda3649e492ad`

Current development branch:

`fix/android17-research-resilience-v02`

Latest merged performance PR:

`#114 — authority direct stream v0.2.7`

## Current status

The v0.2.7 direct-stream route is host-parity validated, full-PR CI green and real-device validated.

The temporary tile-wide authority `ChannelRecord` vector is removed from the fused hot path. The authority replay remains removed. Exact authority digest/count semantics remain bound to the same canonical source-record logic.

The next performance frontier is a canonical direct-byte authority encoder that removes remaining per-record object/validation overhead without changing a single canonical authority byte, SHA/count, evidence classification or authority rule.

## Historical state

Do not delete or rewrite the dated 2026-10-01 and earlier state/handoff files. They remain provenance and include controlled-rotation, measured-support/topography, cable-recovery, Universal Physical Capture, TruthNegative, TruthRange, Free World and older acquisition/reconstruction research.

When current and historical text conflict, the 2026-10-02 state/handoff controls only the global-current pointer; historical documents still control what was actually known at their own checkpoint.
