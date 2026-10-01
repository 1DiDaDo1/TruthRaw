# D.RAW document status index — 2026-10-01

This index defines the current reading order for the active controlled-rotation / measured-support / optical-field-topography research wave.

## Current global state

**Current**

- `state/CURRENT_PROJECT_STATE_2026-10-01.json`
- `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-10-01.md`
- `START_HERE_NEW_CHAT.md`
- `README.md`

Continuation code:

`44485`

## Current research modules

**Current**

- `docs/research/measured-field-support-coordinate-bridge-v0.1/README.md`
- `docs/research/optical-field-topography-v0.1/README.md`
- `docs/research/free-world-observation-geometry-foundation-v0.1/README.md`

Current Android/scientific source-code checkpoint:

`faac21e2e2fe97477c846609b8dc4e71af29b3c1`

Current branch:

`research/registration-aware-rotation-audit-v01-2026-10-01`

Current PR:

`#104`

## Current device evidence

The latest controlled-rotation device export established:

- same four 0/90/180/270 source roots remain bound;
- residual rotation alone is identical to nominal under the present 12-sector discretization;
- translation improves train slightly and worsens held-out;
- scale alone is slightly worse;
- full residual similarity strongly improves train but substantially worsens held-out;
- no model/variant is promoted.

The next device result is expected to test:

- measured sparse source support instead of theoretical field-bin centres;
- optical-field topography layers;
- top/side/oblique diagnostic views encoded in the Foundation export.

## Historical-but-still-authoritative-for-their-own-checkpoints

All dated state, handoff, module and experiment documents before 2026-10-01 remain preserved as provenance.

They remain authoritative for the exact checkpoint or experiment they describe.

They do **not** override the current global state when they conflict with:

- `state/CURRENT_PROJECT_STATE_2026-10-01.json`
- `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-10-01.md`

## Permanent project boundaries

Older documents must never be interpreted as permission to bypass current invariants:

- MEASURED / RECONSTRUCTED / APPEARANCE separation;
- immutable source evidence;
- no camera/lens/vendor identity as scientific truth selector;
- no automatic winner from training residual;
- held-out cannot fit or select;
- no AI/ML/neural/generative runtime;
- no scientific writeback from the research diagnostics;
- topographic height is not physical depth or literal lens-surface sag unless separately proven.

## Repository-head note

Documentation commits can advance beyond the current Android source-code checkpoint without changing the APK-producing source.

When reproducing APK behavior, use the source-code checkpoint recorded in the current state/handoff rather than assuming the latest documentation commit changed the binary.
