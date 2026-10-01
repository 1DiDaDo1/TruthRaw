# D.RAW document status index — 2026-10-01

This index defines the current reading order for the active controlled-rotation / measured-support / optical-field-topography research wave.

## Current global state

**Current**

- `state/CURRENT_PROJECT_STATE_2026-10-01.json`
- `docs/DRAW_MAIN_PROJECT_STATE_2026-10-01.md`
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

`a7cefe7c568a0218b13abda855ad3dbd8b278cc8`

Current branch:

`research/registration-aware-rotation-audit-v01-2026-10-01`

Current PR:

`#104`

## Current engineering resilience

The device test showed a second failure after the first RAW-selection persistence fix: the four RAW handles survived, but the media-processing service disappeared after about 30:50. A separate Foundation save could leave a 0-byte destination.

The current source head moves the long multi-RAW profiler **into** `TruthRawMediaProcessingForegroundService`, uses `START_REDELIVER_INTENT`, checkpoints each completed derived UniversalSourceProfile to private storage, and resumes from completed sources after restart. MainActivity no longer owns the long batch worker.

Research JSON exports are now frozen to private disk with byte length + SHA-256 **before** `ACTION_CREATE_DOCUMENT`; after the picker, exact bytes are streamed to the destination with no Foundation rebuild. The active relation-record session pointer is also durable and can migrate the previous cache-only session.

Source-code checkpoint:

`a7cefe7c568a0218b13abda855ad3dbd8b278cc8`

Green APK-producing run:

`36853127628`

Artifact:

`11156013820`

APK SHA-256:

`c50c3788864331f9fd71af5a462cbb477b8aad7ecfb2c30fa7e1a1eee462f653`

GPU is not used for this repair because the observed failure is lifecycle/heap/export orchestration. GPU remains a future option only for separately validated kernels, especially APPEARANCE_DERIVED_ONLY work with CPU-reference parity.


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
