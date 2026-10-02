# D.RAW state directory

## Current active state — 2026-10-02 — code 44488

Read first:

- `CURRENT_PROJECT_STATE_2026-10-02.json`
- `../docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-10-02.md`
- `../docs/DOCUMENT_STATUS_INDEX_2026-10-02.md`
- `../START_HERE_NEW_CHAT.md`

Current development branch:

`fix/android17-research-resilience-v02`

Current source-code checkpoint:

`85e67ca3090aa113edda7133d656f28087e3ad22`

Latest merged performance line:

PR #109 sparse-reference reuse -> #110 row-band reuse -> #111 phase timing -> #112 subphase timing -> #113 authority fusion -> #114 authority direct stream -> **#115 authority direct-byte v0.2.8**.

v0.2.8 is real-device validated and **37/37 post-merge green**. It encodes the canonical 25 authority bytes directly for all `37,601,280` current source records per RAW, uses zero generic fallbacks on this semantic case, keeps the old general path as the fallback/parity oracle, and reduces the two-RAW aggregate profiler time to about 32.968 s.

The current frontier is **SHA-256 direct-block transport v0.2.9**: remove the redundant copy of complete 64-byte input blocks before the unchanged SHA compression transform. The later per-pixel three-channel encoder remains a separate candidate so performance attribution stays clean.

All older dated state snapshots remain provenance and must not be rewritten to pretend they knew later results.

---

# D.RAW state directory

## Current active state — 2026-10-01 — code 44485

Read first:

- `CURRENT_PROJECT_STATE_2026-10-01.json`
- `../docs/DRAW_MAIN_PROJECT_STATE_2026-10-01.md`
- `../docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-10-01.md`
- `../docs/DOCUMENT_STATUS_INDEX_2026-10-01.md`
- `../docs/research/measured-field-support-coordinate-bridge-v0.1/README.md`
- `../docs/research/optical-field-topography-v0.1/README.md`

Continuation code:

`44485`

Active branch:

`research/registration-aware-rotation-audit-v01-2026-10-01`

Android/scientific source-code checkpoint:

`faac21e2e2fe97477c846609b8dc4e71af29b3c1`

Current frontier:

measured sparse source support -> field-cell centroid/footprint -> optical-field topography -> device validation on the unchanged 0/90/180/270 controlled-rotation set.

All older dated state snapshots below remain provenance and must not be rewritten to pretend they knew later results.

---

# D.RAW state directory

## Current active state — 2026-09-26

Read first:

- `CURRENT_PROJECT_STATE_2026-09-26.json`
- `../docs/DRAW_MAIN_PROJECT_STATE_2026-09-26.md`
- `../docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-09-26.md`
- `../docs/DOCUMENT_STATUS_INDEX_2026-09-26.md`

Current active architecture branch:

`architecture/lens-independent-free-world-observation-v01-2026-09-26`

Latest Android-code branch:

`research/appearance-highlight-headroom-sweep-v02-2026-09-26`

Latest fully Android-validated code checkpoint:

`402c72d6804f6cc393a5eab93c9d94d0687be111`

Default GitHub `main` is historical/lagging and must not be treated as the
current state shortcut.

The 2026-09-26 machine state now also binds the sealed lens-independent Free
World Observation Contract v0.1: one Free World, many separately sealed
observations, TruthNegative per observation lineage, Zero-Line gauge relations
fail-closed across sources, and Float64-compute / validated-Float32-storage
precision separation. The Android pixel route remains the previously validated
Appearance-v0.2 code line.

The first executable derivative is `DRAWObservationRecord v0.1`, host-validated
in run `36258955138`. The Camera-5 example remains source-local and refuses
cross-observation radiometric fusion while the common gauge is unproven.

The state also retains the N2 support-closure / center-excluded /
factored-confidence research line and both real-device Appearance highlight
scenes.

Dated state snapshots below remain provenance and must not be rewritten to
pretend they knew later results.

---

# TruthRaw state directory

## Current active integration — 2026-09-19

For the current multi-vendor app line, read first:

- `CURRENT_PROJECT_STATE_2026-09-19.json`
- `../docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-19.md`
- `TRUTHRAW_V058_NIKON_NEF_SAMPLE_ADAPTER_2026-09-19.json`
- `TRUTHRAW_V059_NIKON_NEF_RADIOMETRIC_ADMISSION_2026-09-19.json`\n- `../docs/TRUTHRAW_V060_PURE_FLOAT32_DNG_2026-09-19.md`

Active branch:

`integration/truthraw-suite-v0-60-pure-float32-dng`

The older TruthNegative and 2026-09-16 state snapshots below remain historical/current-for-their-branch provenance; they are not the first bootstrap for this integration branch.

`state/` contains current and historical state snapshots. Dated snapshots are provenance and must not be rewritten to pretend they knew later results.

## Current on the TruthNegative branch

When checked out on:

`research/truthnegative-v0-2-existing-house-binding`

use:

- `TRUTHNEGATIVE_V0_2_EXISTING_HOUSE_BINDING_STATE_2026-09-19.json` — current TruthNegative research overlay;\n- `TRUTHNEGATIVE_V0_1_STATE_2026-09-19.json` — retained first-draft provenance; its old insertion-point wording is superseded by v0.2;
- `CURRENT_PROJECT_STATE_2026-09-16.json` — retained global scientific architecture snapshot;
- the latest exact HONOR v0.53/v0.54 state files for acquisition context only.

TruthNegative is a reconstruction branch. Its state must not overwrite or authority-promote acquisition evidence.

## Historical / canonical snapshots

Retain:

- `CURRENT_CANONICAL_STATE_2026-09-06.json`
- `CURRENT_CANONICAL_STATE_2026-09-08.json`
- `CURRENT_CANONICAL_STATE_2026-09-09.json`
- `CURRENT_CANONICAL_STATE_2026-09-10.json`
- `REPOSITORY_MIGRATION_STATUS.json`

These are dated provenance, not automatically the active TruthNegative bootstrap.

## Navigation

For this branch use:

`../docs/DOCUMENT_STATUS_INDEX_2026-09-19_TRUTHNEGATIVE_BRANCH.md`

For historical global interpretation also retain the earlier dated document-status indexes.