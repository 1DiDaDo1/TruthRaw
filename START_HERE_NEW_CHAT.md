# D.RAW / TruthRaw — CURRENT BOOTSTRAP — 2026-10-06 — CODE 44489

Continuation code: **44489**

A new chat receiving only **44489** must recover the live repository/PR state before changing code. The current active continuation is Workspace / Free Raster v0.2 on PR #131, while PR #130 / `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f` remains the frozen scientific/audit reference.

Documentation-only commits may move the branch beyond a recorded runtime/build checkpoint. Never force-reset a live branch to a remembered SHA merely to make documentation match.

## Mandatory current reading order

1. Resolve live PR **#131** and record its exact current `head_sha`.
2. Read `docs/handoff/DRAW_44489_NEXT_CHAT_2026-10-06.md`.
3. Read `state/DRAW_PROJECT_STATE_2026-10-06.json`.
4. Read `docs/DOCUMENT_STATUS_INDEX_2026-10-06.md`.
5. Read `docs/handoff/DRAW_44489_KNOWLEDGE_CAPSULE.md` as accumulated 44489 context/provenance.
6. Read `docs/handoff/DRAW_44489_WORKSPACE_OUTPUT_FREE_RASTER_V0_2.md`.
7. Read `docs/DRAW_FLEXIBLE_CABLE_INTERNALS_v0_1.md`.
8. Read `docs/DRAW_NON_DESTRUCTIVE_WORKBENCH_v0_1.md`.
9. Read `docs/DRAW_INTERDISCIPLINARY_SCIENTIFIC_DECISION_FOUNDATION_v0_1.md`.
10. Inspect the exact live-head PR #131 code/CI before mutation; consult frozen PR #130 only for the scientific/audit reference.

## Current active repository boundary

Active product/runtime candidate:

- PR #131 — `Android: D.RAW Workspace / Free Raster view v0.1`
- branch `feat/draw-workspace-free-raster-v01`
- open, draft, not merged at the last recorded checkpoint
- base/frozen scientific-audit reference: PR #130 / `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f`

Last runtime/build source checkpoint before the 2026-10-06 handoff documentation series:

`93c8c05343c35c44a0bf7bb46ee3c630cb11787d`

Do not assume this is still the live head; resolve PR #131 first.

## Current v0.2 product cable

`Input -> Universal Intake -> Scientific Core / Scientific Master -> Unified Output State -> PURE / ADVANCED / PRO -> Output / Vrije Raster -> Export`

Free Raster is downstream View/Output/Projection. It must consume an already-rendered output state and may not become a second decoder, Scientific Master, reconstruction path, T5 evaluation or source of scientific truth.

Two safe display inputs:

1. internal D.RAW output from the **existing Unified Output state**;
2. external JPG/PNG/WebP as `EXTERNAL_PRESENTATION_RASTER / PRESENTATION_ONLY`.

RAW/DNG remains on the existing D.RAW / Universal Intake route.

## Current exact implementation frontier

Already present:

- Workspace / Free Raster UI;
- provider-safe `PresentationRasterLoader` for JPG/PNG/WebP;
- `DrawFlexibleCableContractV01` and flexible-inside documentation;
- `UnifiedOutputPresentationBridge` with owned-copy bitmap lifetime contract;
- `NonDestructiveWorkbenchStateV01` plus non-destructive workbench documentation.

Still open / not yet proven complete:

- actual existing `UnifiedOutputPreviewResult.Ready` publisher wired to `UnifiedOutputPresentationBridge.publish(...)`;
- Workspace fail-closed consumption of that bridge snapshot;
- full migration of Workspace/ADVANCED edit controls into `NonDestructiveWorkbenchStateV01`;
- extension of the **existing** Appearance renderer/cable with black point, white point, highlight roll-off, shadows/midtones and colour/detail controls;
- physical device acceptance of internal D.RAW Unified Output -> Free Raster.

Exact next cable:

`UnifiedOutputPreviewResult.Ready -> UnifiedOutputPresentationBridge.publish(...) -> Workspace consume -> Free Raster`

Do **not** solve this by adding a second renderer or second T5 evaluation.

## Non-destructive workbench invariant

`IMMUTABLE_SOURCE + REVERSIBLE_EDIT_STATE -> PRESENTATION / OUTPUT`

`EXPORT -> NEW_DERIVED_OUTPUT`

Never:

`SOURCE -> EDIT -> OVERWRITE SOURCE`

This applies to sealed RAW/DNG scientific sources and imported JPEG/PNG/WebP source rasters.

Hard safety state:

- `SOURCE_MUTATION_ALLOWED=false`
- `SCIENTIFIC_MASTER_WRITEBACK_ALLOWED=false`
- `OVERWRITE_SOURCE_ON_EXPORT_ALLOWED=false`

## Current compile/build checkpoint

For source head `93c8c05343c35c44a0bf7bb46ee3c630cb11787d`, the exact-head PR-triggered workflow query returned success for the recorded scientific-context, TruthRange, canonical, covariance, tile-native Android, XYZ uncertainty, Documentation Governance, Universal Intake, DngCreator compatibility and Universal Physical Capture workflows.

Android build:

- workflow: `D.RAW Suite Universal Intake v0.1`
- run: `37395957742`
- result: **SUCCESS**
- artifact: `DRAW_Full_Suite_Universal_Intake_v0.1_debug_arm64`
- artifact ID: `11382743163`
- artifact ZIP SHA-256: `1bd705259502a1c2b68cbcb183ff5057654b05eefa91f95cda071f0ff0f5692b`
- APK bytes: `8,686,419`
- APK SHA-256: `3279aab03463bb528e24c6386b68a63e07b401bffc034d735c97f7c239baaad9`
- stable signing certificate SHA-256: `a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`
- versionCode: `26100127`

This is build/compile evidence only. It does not physically accept the still-open internal Unified Output -> Free Raster path and does not promote a scientific candidate.

## Permanent scientific laws

- **Seal the evidence, not the thinking.**
- **MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE.**
- **Representation may become richer than the source; knowledge claims may not exceed evidence.**
- **Stable outside. Flexible inside. Evidence law unchanged.**
- **One Free World. Many sealed observations. One evidence law.**
- Direct-CFA / RAW_SENSOR evidence is immutable and sealed.
- One physical frame remains one physical frame; derived views do not create captures.
- Scientific Master remains scene/scientific and pre-appearance.
- `UNKNOWN` is valid and may not be converted to zero/certainty for convenience.
- Unknown covariance is not zero.
- Registration/reconstruction/precision/resolution/performance do not create authority.
- Cross-observation radiometric fusion requires an admitted common-gauge relation.
- TruthNegative remains Observation-bound; observations meet only above that boundary.
- Inferred geometry/material/illumination may not silently become `MEASURED`.
- Appearance may not write back into sealed evidence or Scientific Master.
- Workbench edits remain reversible/downstream and do not overwrite the source.
- No AI/ML/neural/generative runtime is admitted as scientific evidence/inference.

## Interdisciplinary decision foundation

For non-trivial project decisions, use `docs/DRAW_INTERDISCIPLINARY_SCIENTIFIC_DECISION_FOUNDATION_v0_1.md` and keep current knowledge synchronized across RAW/CFA sensor metrology, photography, optics/PSF/MTF, radiometry/photometry/light transport, camera/color calibration, black/white/clipping/censoring, human vision/color appearance, 3D/projective geometry, physically based rendering/animation, stop-motion/temporal sampling/motion blur/rolling shutter, artificial illumination/spectra/flicker, architecture/photogrammetry/conservation, and scene-referred/display-referred pipelines.

External scientific knowledge can improve models and falsification tests. It can never become observation-specific evidence by itself.

## Frozen PR #130 audit meaning

Frozen exact-head reference:

`4e4f358ac5d6fb4a8562286257b8edb76ae5d33f`

At that exact head:

- Documentation Governance = proven SUCCESS;
- Research Integrity Guard = no proven exact-head run -> `NOT RUN / UNPROVEN`;
- Lifecycle Contract = no proven exact-head run -> `NOT RUN / UNPROVEN`;
- failures on older SHAs remain attached only to those older SHAs;
- missing workflow runs are not scientific evidence and do not change authority.

PR #131 product work must not rewrite this audit meaning.

## Required historical governance provenance

The governance verifier intentionally preserves dated snapshots and requires historical pointers to remain discoverable. They are provenance unless the current 2026-10-06 reading order says otherwise.

### 2026-09-16 / 2026-09-19

- `docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md`
- `docs/PROJECT_HISTORY_AND_CHANGES_2026-09-16.md`
- `docs/handoff/TRUTHRAW_CONSOLIDATED_HANDOFF_2026-09-16.md`
- `state/CURRENT_PROJECT_STATE_2026-09-16.json`
- `docs/DOCUMENT_STATUS_INDEX_2026-09-16.md`
- `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-19.md`
- `state/CURRENT_PROJECT_STATE_2026-09-19.json`

### 2026-09-20 / 2026-09-21

- `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-20.md`
- `state/CURRENT_PROJECT_STATE_2026-09-20.json`
- `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-21.md`
- `state/CURRENT_PROJECT_STATE_2026-09-21.json`

### 2026-09-24 / 2026-09-25

- `state/CURRENT_PROJECT_STATE_2026-09-24.json`
- `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-24.md`
- `state/CURRENT_PROJECT_STATE_2026-09-25.json`
- `docs/DRAW_MAIN_PROJECT_STATE_2026-09-25.md`
- `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-09-25.md`

### 2026-09-27 / 2026-09-28

- `state/CURRENT_PROJECT_STATE_2026-09-27.json`
- `docs/DRAW_KNOWLEDGE_GROWTH_INTEGRATION_2026-09-27.md`
- `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-09-27.md`
- `docs/DOCUMENT_STATUS_INDEX_2026-09-27.md`
- `docs/research/free-world-observation-graph-v0.1/README.md`
- `state/CURRENT_PROJECT_STATE_2026-09-28.json`
- `docs/DRAW_CORE_VISION_REALIGNMENT_2026-09-28.md`

### 2026-10-01 / 2026-10-02

- `state/CURRENT_PROJECT_STATE_2026-10-01.json`
- `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-10-01.md`
- `docs/DOCUMENT_STATUS_INDEX_2026-10-01.md`
- `docs/research/measured-field-support-coordinate-bridge-v0.1/README.md`
- `docs/research/optical-field-topography-v0.1/README.md`
- `state/CURRENT_PROJECT_STATE_2026-10-02.json`
- `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-10-02.md`
- `docs/DOCUMENT_STATUS_INDEX_2026-10-02.md`

### 44489 provenance

- `docs/handoff/DRAW_44489_RECOVERY_2026-10-03.md`
- `state/DRAW_PROJECT_STATE_2026-10-04.json`
- `docs/DOCUMENT_STATUS_INDEX_2026-10-04.md`
- `docs/handoff/DRAW_44489_T5_RUNTIME_AUDIT_READY_2026-10-05.md`
- `docs/handoff/DRAW_44489_T5_FOUNDATION_TELEMETRY_READY_2026-10-05.md`
- `docs/handoff/DRAW_44489_REAL_DEVICE_VALIDATED_2026-10-05.md`
- `state/DRAW_PROJECT_STATE_2026-10-05.json`
- `docs/DOCUMENT_STATUS_INDEX_2026-10-05.md`
- `docs/handoff/DRAW_44489_WORKSPACE_DEVICE_ACCEPTANCE_2026-10-05.md`
- `docs/handoff/DRAW_44489_NEXT_CHAT_2026-10-06.md`
- `state/DRAW_PROJECT_STATE_2026-10-06.json`
- `docs/DOCUMENT_STATUS_INDEX_2026-10-06.md`

## Current scientific frontier beyond the product work

Do not add another reconstruction heuristic by default. The highest-value scientific frontier remains:

`multiple real sealed observations -> proven inter-observation registration/gauge/uncertainty -> active Free World Observation Graph -> controlled free raster/world evaluation`

Parallel tracks remain universal source admission, first-class D.RAWnegative, Free Raster productization, user-visible authority/uncertainty, dynamic Appearance/HDR, and full-resolution/multi-observation scale proof.
