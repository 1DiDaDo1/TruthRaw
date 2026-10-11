# D.RAW / TruthRaw

> **CURRENT PROJECT STATE — 2026-10-06 — continuation code `44489`**
>
> Canonical project/product name: **D.RAW**  
> Historical/repository name: `TruthRaw`  
> Repository: `1DiDaDo1/TruthRaw`  
> Active candidate branch: `feat/draw-workspace-free-raster-v01`  
> Active product/runtime PR: **#131**  
> Frozen scientific/audit reference: **PR #130 / `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f`**

D.RAW is deterministic, provenance-bound and non-destructive:

> **Seal the evidence, not the thinking.**  
> **MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE.**  
> **Representation may become richer than the source; knowledge claims may not exceed the evidence.**  
> **Stable outside. Flexible inside. Evidence law unchanged.**  
> **One Free World. Many sealed observations. One evidence law.**

## Start here — 44489

For the current continuation, read:

1. `START_HERE_NEW_CHAT.md`
2. `docs/handoff/DRAW_44489_NEXT_CHAT_2026-10-06.md`
3. `state/DRAW_PROJECT_STATE_2026-10-06.json`
4. `docs/DOCUMENT_STATUS_INDEX_2026-10-06.md`
5. `docs/handoff/DRAW_44489_KNOWLEDGE_CAPSULE.md`
6. `docs/handoff/DRAW_44489_WORKSPACE_OUTPUT_FREE_RASTER_V0_2.md`
7. `docs/DRAW_FLEXIBLE_CABLE_INTERNALS_v0_1.md`
8. `docs/DRAW_NON_DESTRUCTIVE_WORKBENCH_v0_1.md`
9. `docs/DRAW_INTERDISCIPLINARY_SCIENTIFIC_DECISION_FOUNDATION_v0_1.md`
10. live PR #131 / exact current head

Documentation-only commits may advance the branch beyond the last runtime/build checkpoint. Always resolve the live head before mutation; never force-reset to a remembered SHA.

## Current Workspace / Free Raster v0.2 architecture

Canonical product cable:

`Input -> Universal Intake -> Scientific Core / Scientific Master -> Unified Output State -> PURE / ADVANCED / PRO -> Output / Vrije Raster -> Export`

Free Raster is downstream View/Output/Projection. It is not a second RAW/DNG decoder, Scientific Master, reconstruction path, T5 evaluation or output truth.

External JPG/PNG/WebP remains:

`EXTERNAL_PRESENTATION_RASTER / PRESENTATION_ONLY`

RAW/DNG remains routed through existing D.RAW / Universal Intake.

The new `UnifiedOutputPresentationBridge` is intended only to transport an already-rendered `UnifiedOutputPreviewResult.Ready` bitmap safely into Workspace / Free Raster. MainActivity retains ownership of its Ready bitmap, so the bridge uses owned copies instead of leaking a recycled bitmap reference.

**Open runtime boundary:** the bridge class exists, but the full `Ready -> publish -> Workspace consume -> Free Raster` publisher/consumer wiring is not yet proven complete.

## Non-destructive workbench

Canonical model:

`IMMUTABLE_SOURCE + REVERSIBLE_EDIT_STATE -> PRESENTATION / OUTPUT`

`EXPORT -> NEW_DERIVED_OUTPUT`

Never:

`SOURCE -> EDIT -> OVERWRITE SOURCE`

This applies to RAW/DNG scientific sources and imported JPG/PNG/WebP base images.

Current anchors:

- `suite_android/app/src/main/java/com/truthraw/adaptiveui/NonDestructiveWorkbenchStateV01.kt`
- `docs/DRAW_NON_DESTRUCTIVE_WORKBENCH_v0_1.md`

The contract exists, but every current Workspace/ADVANCED control has not yet been proven integrated into it.

## Current build checkpoint

Exact branch source head used for the current v0.2 compile/build checkpoint:

`93c8c05343c35c44a0bf7bb46ee3c630cb11787d`

`D.RAW Suite Universal Intake v0.1` run `37395957742` is **SUCCESS**.

- artifact ID: `11382743163`
- artifact ZIP SHA-256: `1bd705259502a1c2b68cbcb183ff5057654b05eefa91f95cda071f0ff0f5692b`
- APK bytes: `8,686,419`
- APK SHA-256: `3279aab03463bb528e24c6386b68a63e07b401bffc034d735c97f7c239baaad9`
- stable signer SHA-256: `a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`
- versionCode: `26100127`

The exact-head PR-triggered workflow query for `93c8c053...` returned success for the listed scientific-context, TruthRange, canonical, covariance, tile-native Android, XYZ uncertainty, Documentation Governance, Universal Intake, DngCreator compatibility and Universal Physical Capture workflows recorded in the 2026-10-06 handoff/state.

This is build/compile evidence only. Physical acceptance of the unfinished internal Unified Output -> Free Raster path remains open, and no scientific candidate is promoted.

## Exact next implementation task

Do not redesign the architecture. Continue the existing cable:

`UnifiedOutputPreviewResult.Ready -> UnifiedOutputPresentationBridge.publish(...) -> Workspace consume -> Free Raster`

Then integrate `NonDestructiveWorkbenchStateV01` into Workspace/ADVANCED and extend the **existing** Appearance renderer/cable with reversible black point, white point, highlight roll-off, shadows/midtones and colour/detail controls. Do not introduce a duplicate renderer.

After runtime changes: rebuild exact head, record exact APK provenance again, then physically test one normal JPEG plus one D.RAW observation through the internal Unified Output -> Free Raster route.

## Current scientific architecture

`readable source -> sealed Observation -> structural inspection -> Source Capability Envelope -> admitted measurement/calibration/reconstruction -> Scientific Master -> Dynamic Authority + uncertainty -> Observation-bound TruthNegative -> Free World Observation Graph -> Deep Scene / Light Transport -> Room Capsule -> View / Appearance -> finite/free raster projection`

Without admitted geometry/material/illumination evidence, Room Capsule remains exact-preserving bypass.

Foundation-wide scientific promotion remains `NOT_PROMOTED_FAIL_CLOSED` unless separately proven and explicitly promoted.

## Permanent scientific boundaries

- Direct-CFA/source evidence is immutable and sealed.
- Source capability is not proof of measured sample domain.
- Scientific Master remains separate from presentation/export/appearance.
- `UNKNOWN`, `CENSORED`, `RECONSTRUCTED`, `MEASURED`, `CALIBRATED_ESTIMATE` and `APPEARANCE` do not collapse into one another.
- Unknown covariance is not zero.
- Cross-observation radiometric fusion requires an admitted common-gauge relation.
- Performance artifacts may remember computation; they cannot create evidence or authority.
- Camera/lens/vendor/RAW identity may route parsing but cannot select scientific truth.
- No diagnostic/performance/UI/appearance path may create evidence, mutate measured anchors or perform scientific writeback.
- SOURCE/SENSOR SPACE, WORLD/SCENE SPACE and VIEW/OUTPUT SPACE remain distinct.
- Derived/virtual observations do not create additional physical captures.
- Workbench edits are downstream, reversible and source-preserving.
- Explicit export creates a new derivative rather than overwriting the source by default.
- No AI/ML/neural/generative runtime is admitted as scientific inference.

## Frozen PR #130 audit meaning

The exact frozen audit reference remains `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f`.

At that exact head, Documentation Governance had proven SUCCESS, while Research Integrity Guard and Lifecycle Contract had **no proven run on that exact SHA**. Their exact-head audit state is therefore `NOT RUN / UNPROVEN`, not red and not green. Older failures from older SHAs may not be projected onto the frozen head.

## Required historical authority/provenance pointers

These dated documents remain preserved and discoverable. They are historical provenance unless the 2026-10-06 reading order explicitly promotes them for current use:

- `docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md`
- `docs/PROJECT_HISTORY_AND_CHANGES_2026-09-16.md`
- `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-19.md`
- `state/CURRENT_PROJECT_STATE_2026-09-19.json`
- `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-20.md`
- `state/CURRENT_PROJECT_STATE_2026-09-20.json`
- `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-21.md`
- `state/CURRENT_PROJECT_STATE_2026-09-21.json`
- `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-24.md`
- `state/CURRENT_PROJECT_STATE_2026-09-24.json`
- `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-09-25.md`
- `state/CURRENT_PROJECT_STATE_2026-09-25.json`
- `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-09-27.md`
- `state/CURRENT_PROJECT_STATE_2026-09-27.json`
- `docs/DOCUMENT_STATUS_INDEX_2026-09-27.md`
- `docs/DRAW_CORE_VISION_REALIGNMENT_2026-09-28.md`
- `state/CURRENT_PROJECT_STATE_2026-09-28.json`
- `state/CURRENT_PROJECT_STATE_2026-10-01.json`
- `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-10-01.md`
- `docs/DOCUMENT_STATUS_INDEX_2026-10-01.md`
- `state/CURRENT_PROJECT_STATE_2026-10-02.json`
- `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-10-02.md`
- `docs/DOCUMENT_STATUS_INDEX_2026-10-02.md`
- `docs/handoff/DRAW_44489_RECOVERY_2026-10-03.md`
- `state/DRAW_PROJECT_STATE_2026-10-04.json`
- `docs/DOCUMENT_STATUS_INDEX_2026-10-04.md`
- `docs/handoff/DRAW_44489_REAL_DEVICE_VALIDATED_2026-10-05.md`
- `state/DRAW_PROJECT_STATE_2026-10-05.json`
- `docs/DOCUMENT_STATUS_INDEX_2026-10-05.md`

Older handoffs and state files remain provenance. The 2026-10-06 44489 layer is the current operational authority.