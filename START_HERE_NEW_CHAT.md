# D.RAW / TruthRaw — CURRENT BOOTSTRAP — 2026-10-06 — CODE 44489

Continuation code: **44489**

A new chat receiving only **44489** must first resolve live PR #131 and then recover the current Workspace / Free Raster v0.2 state. PR #130 / `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f` remains the frozen scientific/audit reference.

Documentation-only commits may advance the live branch beyond the runtime/build checkpoint. Never force-reset to a remembered SHA just to make documentation match.

## Mandatory current reading order

1. Resolve live PR **#131** and record exact current `head_sha`.
2. Read `docs/handoff/DRAW_44489_NEXT_CHAT_2026-10-06.md`.
3. Read `state/DRAW_PROJECT_STATE_2026-10-06.json`.
4. Read `docs/handoff/DRAW_44489_FREE_RASTER_V02_DEVICE_PARTIAL_ACCEPTANCE_2026-10-06.md`.
5. Read `docs/DOCUMENT_STATUS_INDEX_2026-10-06.md`.
6. Read `docs/handoff/DRAW_44489_KNOWLEDGE_CAPSULE.md`.
7. Read `docs/handoff/DRAW_44489_WORKSPACE_OUTPUT_FREE_RASTER_V0_2.md`.
8. Read `docs/DRAW_FLEXIBLE_CABLE_INTERNALS_v0_1.md`.
9. Read `docs/DRAW_NON_DESTRUCTIVE_WORKBENCH_v0_1.md`.
10. Read `docs/DRAW_INTERDISCIPLINARY_SCIENTIFIC_DECISION_FOUNDATION_v0_1.md`.
11. Inspect exact live-head PR #131 code/CI before mutation; consult frozen PR #130 only for the scientific/audit reference.

## Active repository boundary

- Active product/runtime PR: **#131**
- branch: `feat/draw-workspace-free-raster-v01`
- frozen base/scientific-audit reference: PR #130 / `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f`
- runtime/build head used by the tested APK: `7f7d3f896d5e48d31a5b0ab833b9fbfe53cca390`
- PR remains draft until remaining physical acceptance is complete.

Resolve live head first because the current head may be a later documentation-only commit.

## Current v0.2 product cable

`Input -> Universal Intake -> Scientific Core / Scientific Master -> Unified Output State -> PURE / ADVANCED / PRO -> Output / Vrije Raster -> Export`

Free Raster is downstream View/Output/Projection only. It is not a second decoder, Scientific Master, reconstruction route, T5 evaluation or scientific truth source.

Safe display inputs:

1. internal D.RAW output from the existing Unified Output state;
2. external JPG/PNG/WebP as `EXTERNAL_PRESENTATION_RASTER / PRESENTATION_ONLY`.

RAW/DNG remains on Universal Intake.

## Runtime wiring status

The internal cable is implemented and build-green:

`UnifiedOutputPreviewResult.Ready -> MainActivity common state boundary -> UnifiedOutputPresentationBridge.publishReady(...) -> Workspace fail-closed consume -> Free Raster`

Runtime anchors:

- `suite_android/app/src/main/java/com/truthraw/adaptiveui/MainActivity.kt`
- `suite_android/app/src/main/java/com/truthraw/adaptiveui/UnifiedOutputPresentationBridge.kt`
- `suite_android/app/src/main/java/com/truthraw/adaptiveui/TruthRawWorkspaceActivity.kt`

Properties:

- one existing Unified Output renderer/state only;
- bridge owns independent presentation copies;
- Workspace never retains MainActivity's recyclable Ready bitmap directly;
- source/job/route binding is UI freshness/provenance, not scientific authority;
- known source SHA may be transported; missing source SHA stays `UNKNOWN`;
- stale source/reprocess state clears fail-closed;
- no second renderer, RAW decoder, Scientific Master, reconstruction route or T5 evaluation.

## Current real-device status

Current status:

`PARTIAL_DEVICE_ACCEPTANCE_INTERNAL_EXTERNAL_FIT_PREVIEW_1_TO_1_PAN_ZOOM_PASS`

### Internal D.RAW PASS

Real-device screenshots show:

- `D.RAW_UNIFIED_OUTPUT_PRESENTATION` in Free Raster;
- route `PRO`;
- `D.RAWnegative v0.1 · Authority-bound Appearance View`;
- source `IMG_BNC_TRUTHRAW20260907_094414_423.dng`;
- source `4080×3072`;
- decoded preview `192×145`;
- source-SHA prefix `578fad42dad6819b…`;
- `VIEW_ONLY_COPY`;
- `createsNewEvidence=false`;
- `scientificWriteback=false`;
- Fit = PASS;
- Preview 1:1 = PASS.

### External raster PASS

The normal black/white dog JPG was reloaded in the same Workspace/app environment and shows:

- `EXTERNAL_PRESENTATION_RASTER`;
- source `4080×3072`;
- preview `2040×1536`;
- sample `2×`;
- `PRESENTATION_ONLY`;
- Fit scale about `0.502x`;
- Preview 1:1 scale `1.000x`;
- zoom about `3.980x`;
- changed x/y transforms proving pan/positioning;
- same-environment reload = PASS.

**Preview 1:1** means one display pixel per decoded preview pixel only. It is never a sensor/CFA/Scientific-Master sampling claim or optical-resolution proof.

Same-environment reload does **not** prove cold-start/process-death persistence.

Still pending before full v0.2 physical acceptance:

- non-zero orientation;
- route mismatch fail-closed;
- source switch/reprocess stale-state clearing;
- explicit no-source-overwrite / no-Scientific-Master-writeback / no-candidate-application / no-new-evidence interaction check;
- cold-start/process-death raster restoration only if required;
- installed APK hash readback only if byte-exact physical-package provenance is required.

Do not re-test internal/external Fit, Preview 1:1 or pan/zoom unless runtime code changes.

## Current build checkpoint

Runtime/build source head:

`7f7d3f896d5e48d31a5b0ab833b9fbfe53cca390`

Android build:

- workflow: `D.RAW Suite Universal Intake v0.1`
- run: `37439775107`
- result: **SUCCESS**
- artifact: `DRAW_Full_Suite_Universal_Intake_v0.1_debug_arm64`
- artifact ID: `11401290002`
- artifact ZIP SHA-256: `5ae0e7d9d86db230c2d76e5d86cb0521511c07dd7615c695e78782ba56e791a4`
- APK bytes: `8,702,803`
- APK SHA-256: `d8baafc8715f41a5585a9e1687e8ac56c4ddb6d5f900321bb091b7fb16f1c8ee`
- stable signing certificate SHA-256: `a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`
- versionCode: `26100127`
- versionName: `0.54-v0.84.2-open-world-authority-corridor-v01`

The exact-head returned workflow set was green after Android lineage repair, including Research Live Status, Research Fresh Rerun, Android Version Lineage, Documentation Governance, Canonical, TruthRange, Shared Scientific Context, performance provenance, DngCreator compatibility, Universal Intake and Universal Physical Capture.

Build/CI evidence creates no scientific authority. Device screenshots prove functional runtime behavior; the installed APK hash was not independently read back from the device in this chat.

## Non-destructive workbench invariant

`IMMUTABLE_SOURCE + REVERSIBLE_EDIT_STATE -> PRESENTATION / OUTPUT`

`EXPORT -> NEW_DERIVED_OUTPUT`

Never:

`SOURCE -> EDIT -> OVERWRITE SOURCE`

Hard safety state:

- `SOURCE_MUTATION_ALLOWED=false`
- `SCIENTIFIC_MASTER_WRITEBACK_ALLOWED=false`
- `OVERWRITE_SOURCE_ON_EXPORT_ALLOWED=false`

Anchor: `suite_android/app/src/main/java/com/truthraw/adaptiveui/NonDestructiveWorkbenchStateV01.kt`.

Full integration of all Workspace/ADVANCED edit controls remains after Free Raster v0.2 physical acceptance.

## Exact next action

Do not redesign the internal bridge; it is wired and green.

Complete physical acceptance in order:

1. non-zero orientation;
2. route mismatch fail-closed;
3. source switch/reprocess stale-state clearing;
4. confirm no source overwrite, Scientific Master writeback, candidate application or new-evidence claim;
5. optionally test cold-start/process-death raster restoration and installed APK hash readback if those properties are required.

After full v0.2 physical acceptance:

6. bind `NonDestructiveWorkbenchStateV01` into Workspace/ADVANCED;
7. extend the **existing** Appearance renderer/cable with reversible black point, white point, highlight roll-off, shadows/midtones, warmth/tint, saturation/vibrance/colorfulness, neutral protection and detail/appearance sharpening;
8. do not create a duplicate renderer.

## Permanent scientific laws

- **Seal the evidence, not the thinking.**
- **MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE.**
- **Representation may become richer than the source; knowledge claims may not exceed evidence.**
- **Stable outside. Flexible inside. Evidence law unchanged.**
- **One Free World. Many sealed observations. One evidence law.**
- Direct-CFA / RAW_SENSOR evidence is immutable and sealed.
- One physical frame remains one physical frame; derived views do not create captures.
- Scientific Master remains separate from Appearance/export.
- `UNKNOWN` is valid and may not silently become zero/certainty.
- Unknown covariance is not zero.
- Registration/reconstruction/precision/resolution/performance do not create authority.
- Cross-observation radiometric fusion requires an admitted common-gauge relation.
- TruthNegative remains Observation-bound; observations meet only above that boundary.
- Inferred geometry/material/illumination may not silently become `MEASURED`.
- Appearance may not write back to sealed evidence or Scientific Master.
- No AI/ML/neural/generative runtime is admitted as scientific evidence/inference.

## Interdisciplinary decision foundation

Use `docs/DRAW_INTERDISCIPLINARY_SCIENTIFIC_DECISION_FOUNDATION_v0_1.md` for non-trivial decisions across photography/RAW/CFA sensors, optics/PSF/MTF, radiometry/light transport, calibration/color, black/white/clipping, human vision, 3D/projective geometry, PBR/animation, stop-motion/temporal sampling/rolling shutter, illumination/spectra/flicker, architecture/photogrammetry/conservation and scene/display-referred pipelines.

External scientific knowledge may improve models and tests. It never becomes observation-specific evidence by itself.

## Frozen PR #130 audit meaning

Frozen exact head:

`4e4f358ac5d6fb4a8562286257b8edb76ae5d33f`

At that exact head:

- Documentation Governance = proven SUCCESS;
- Research Integrity Guard = no proven exact-head run -> `NOT RUN / UNPROVEN`;
- Lifecycle Contract = no proven exact-head run -> `NOT RUN / UNPROVEN`;
- older failures remain attached to older SHAs only.

PR #131 product work must not rewrite this audit meaning.

## Historical governance provenance

Historical dated architecture/state/handoff documents remain provenance and must remain discoverable. Important current predecessors include the dated September/October state, handoff and document-index files, plus:

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
- `docs/handoff/DRAW_44489_FREE_RASTER_V02_DEVICE_PARTIAL_ACCEPTANCE_2026-10-06.md`
- `state/DRAW_PROJECT_STATE_2026-10-06.json`
- `docs/DOCUMENT_STATUS_INDEX_2026-10-06.md`

## Scientific frontier beyond the product work

Do not add another reconstruction heuristic by default. Highest-value scientific frontier remains:

`multiple real sealed observations -> proven inter-observation registration/gauge/uncertainty -> active Free World Observation Graph -> controlled free raster/world evaluation`

Parallel tracks remain universal source admission, first-class D.RAWnegative, Free Raster productization, user-visible authority/uncertainty, dynamic Appearance/HDR and full-resolution/multi-observation scale proof.
