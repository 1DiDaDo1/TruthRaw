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

`85e67ca3090aa113edda7133d656f28087e3ad22`

Current development branch:

`fix/android17-research-resilience-v02`

Latest merged performance PR:

`#115 — authority direct-byte v0.2.8`

## Current status

v0.2.8 is host exact-parity validated, real-device validated and **37/37 post-merge green**.

The source authority route now:
- keeps the historical materialized/general route available as parity oracle and fallback;
- keeps the fused pass and no-replay property from v0.2.6;
- keeps zero tile-wide `ChannelRecord` scratch from v0.2.7;
- emits the canonical 25 authority bytes directly for the current proven source semantics;
- processes `37,601,280` direct-byte records per RAW with `0` generic fallbacks in the validated device run;
- leaves source values, Scientific Master authority, candidate application and writeback unchanged.

The validated two-RAW aggregate profiler time is about `32.968 s`, roughly `18.96%` faster than v0.2.7.

The next isolated performance frontier is **SHA-256 direct-block transport v0.2.9**. It may eliminate redundant copies of complete 64-byte SHA blocks, but it may not alter a digest bit or any scientific semantics. The later per-pixel/three-channel authority encoder stays separate.

## Historical state

Do not delete or rewrite the dated 2026-10-01 and earlier state/handoff files. They remain provenance and include controlled-rotation, measured-support/topography, cable-recovery, Universal Physical Capture, TruthNegative, TruthRange, Free World and older acquisition/reconstruction research.

When current and historical text conflict, the 2026-10-02 state/handoff controls only the global-current pointer; historical documents still control what was actually known at their own checkpoint.
