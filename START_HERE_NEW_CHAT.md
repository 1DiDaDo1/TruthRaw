# D.RAW / TruthRaw — CURRENT BOOTSTRAP — 2026-10-06 — CODE 44489

Continuation code: **44489**

A new chat receiving only **44489** must first resolve live PR #131 and then recover this state. PR #130 / `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f` remains the frozen scientific/audit reference.

Documentation-only commits may advance the live branch beyond the tested runtime/build checkpoint. Never force-reset to a remembered SHA just to make documentation match.

## Mandatory current reading order

1. Resolve live PR **#131** and record exact current `head_sha`.
2. Read `state/DRAW_PROJECT_STATE_2026-10-06.json`.
3. Read `docs/handoff/DRAW_44489_JPEG_90_DEGREE_DEVICE_PASS_2026-10-06.md`.
4. Read `docs/handoff/DRAW_44489_ROUTE_MISMATCH_FAIL_CLOSED_TEST_PROTOCOL_2026-10-06.md`.
5. Read `docs/handoff/DRAW_44489_PREVIEW_INDEPENDENT_JPEG_DEVICE_PASS_2026-10-06.md`.
6. Read `docs/handoff/DRAW_44489_PREVIEW_INDEPENDENT_JPEG_CHECKPOINT_2026-10-06.md`.
7. Read `docs/handoff/DRAW_44489_FREE_RASTER_V02_DEVICE_PARTIAL_ACCEPTANCE_2026-10-06.md`.
8. Read `docs/handoff/DRAW_44489_NEXT_CHAT_2026-10-06.md`.
9. Read `docs/DOCUMENT_STATUS_INDEX_2026-10-06.md`.
10. Read `docs/handoff/DRAW_44489_KNOWLEDGE_CAPSULE.md`.
11. Read `docs/handoff/DRAW_44489_WORKSPACE_OUTPUT_FREE_RASTER_V0_2.md`.
12. Read `docs/DRAW_FLEXIBLE_CABLE_INTERNALS_v0_1.md`.
13. Read `docs/DRAW_NON_DESTRUCTIVE_WORKBENCH_v0_1.md`.
14. Read `docs/DRAW_INTERDISCIPLINARY_SCIENTIFIC_DECISION_FOUNDATION_v0_1.md` and `docs/DRAW_INTERDISCIPLINARY_DECISION_MATRIX_v0_2.md`.
15. Inspect exact live-head PR #131 code/CI before mutation; consult frozen PR #130 only for the scientific/audit reference.

## Active repository boundary

- Active product/runtime PR: **#131**
- branch: `feat/draw-workspace-free-raster-v01`
- frozen scientific/audit reference: PR #130 / `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f`
- tested preview-independent JPEG runtime source head: `de0d49ef9f9abda777a8126a2a98a028030eafa1`
- latest head before this bootstrap update: `5f74089ebeba2ad54920abf882b5f8a5d702bb7a`
- PR remains draft and unmerged until required physical acceptance is complete.

Always resolve live head first.

## Canonical product cable

`Input -> Universal Intake -> Scientific Core / Scientific Master -> Unified Output State -> PURE / ADVANCED / PRO -> Output / Free Raster -> Export`

Free Raster is downstream View/Output/Projection only. It is not a second RAW decoder, Scientific Master, reconstruction route, T5 evaluation or scientific truth source.

The intended flexible output architecture is:

`same admitted source/scientific/output state -> sibling adapters`

including:

- fast UI preview;
- Free Raster projection;
- full-resolution JPEG;
- later PNG/other presentation outputs.

A denser/larger raster never manufactures additional `MEASURED` sensor evidence.

## Unified Output -> Free Raster status

Existing internal cable:

`UnifiedOutputPreviewResult.Ready -> MainActivity common state boundary -> UnifiedOutputPresentationBridge.publishReady(...) -> Workspace fail-closed consume -> Free Raster`

This preview branch is retained as a fast display branch. It is **not** the full-resolution JPEG source.

Already physically accepted on device:

- internal D.RAW -> Workspace -> Free Raster;
- Fit;
- internal Preview 1:1;
- external JPG/JPEG presentation-only load;
- external Fit;
- external Preview 1:1;
- pan/zoom;
- same-environment external raster reload.

`Preview 1:1` means one display pixel per decoded preview pixel only. It is never a CFA/sensor/Scientific-Master sampling claim.

## Preview-independent full-resolution JPEG — REAL-DEVICE PASS

Runtime architecture:

`admitted DNG observation -> DrawPhotoOutputCableV01 frozen binding -> FullResJpegExporter -> new derived JPEG`

The binding freezes:

- source job ID;
- source URI;
- PURE / ADVANCED / PRO route;
- route/appearance flags;
- user quarter-turn orientation.

After Android's document picker returns, source/job/URI/route/flags/orientation are revalidated fail-closed.

Hard contract:

- `previewRequired=false`
- `createsNewEvidence=false`
- `scientificWritebackAllowed=false`
- `sourceMutationAllowed=false`
- source authority = `EXISTING_ADMITTED_DNG_OBSERVATION`
- output authority = `DERIVED_PRESENTATION_OUTPUT`

Real-device timing proved the JPEG worker starts before the normal Advanced/PRO preview reaches Ready.

Accepted 0° JPEG:

- `4080x3072`
- `1,991,838` bytes
- SHA-256 `115354213c3fd828bd2708414a423dba33e39c644f8aa6ea8a251b1c042f1bbc`

## +90° full-resolution JPEG — REAL-DEVICE PASS

Accepted +90° output:

- `3072x4080`
- `1,994,803` bytes
- SHA-256 `9ac32855452494ec8b755d187f6c5f99006284a842123cc8fa80426643154b11`
- pixel geometry physically rotated; no EXIF-orientation transform required.

Direct comparison against the accepted 0° JPEG rotated exactly 90° clockwise:

- MAE `0.6985928936`
- RMSE `1.0416217438`
- PSNR `47.7766 dB`
- correlation `0.9997739056`
- max absolute difference `8`
- opposite-direction MAE about `62.82`

Classification:

`PASS — NON_ZERO_ORIENTATION_FULL_RESOLUTION_JPEG_CLOCKWISE_90`

This is downstream presentation/projective rotation only; sealed RAW/CFA and Scientific Master are not rotated in place.

## Route-mismatch fail-closed — CODE AUDIT PASS / PHYSICAL TEST NEXT

The actual `REQUEST_SAVE_JPEG` return path re-reads:

- current route;
- current route flags;
- current quarter-turn orientation;

and calls:

`DrawPhotoOutputCableV01.validateCurrentOutputContext(...)`

The validator explicitly rejects route, flags or orientation mismatch and first revalidates source/job/URI plus the hard output safety contract.

Code-audit classification:

`PASS — FAIL-CLOSED CHECK EXISTS IN ACTUAL JPEG RESULT PATH`

Physical Android lifecycle acceptance remains pending.

Exact protocol:

`docs/handoff/DRAW_44489_ROUTE_MISMATCH_FAIL_CLOSED_TEST_PROTOCOL_2026-10-06.md`

Preferred test:

1. start on PRO with a known-good DNG;
2. press full-resolution JPEG and leave DocumentsUI pending;
3. if the Android task UI permits, bring D.RAW launcher forward and change PRO -> PURE;
4. return to the original picker and confirm a destination;
5. require `JPG-output geblokkeerd: uitvoerroute veranderde tijdens de bestandsdialoog.`;
6. no stale JPEG render/commit may occur.

If the vendor task stack makes D.RAW inaccessible while DocumentsUI is pending, classify that attempt `TEST_NOT_REACHABLE_WITH_NORMAL_DEVICE_UI`, not PASS and not failure.

## Source-switch / reprocess stale-state acceptance — PENDING

After route mismatch, test a pending output request against source switch/reprocess. A stale binding must clear or fail closed; it may never silently retarget to a different sealed observation.

## Current tested APK checkpoint

Runtime source head:

`de0d49ef9f9abda777a8126a2a98a028030eafa1`

Build:

- workflow `D.RAW Suite Universal Intake v0.1`
- run `37486202104`
- result **SUCCESS**
- artifact ID `11423262790`
- APK bytes `8,719,187`
- APK SHA-256 `9887085f71a24ceb0057f2503728e886502d11b92f908f23a00f107eab3ec860`
- stable signing certificate SHA-256 `a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`
- versionCode `26100127`
- versionName `0.54-v0.84.2-open-world-authority-corridor-v01`

At documentation head `0ba71c821b1834379dab851cd927c5e634211325`, all returned workflows completed successfully, including Documentation Governance, Canonical, Research Live Status, Research Fresh Rerun, Android Version Lineage, DngCreator Compatibility, Universal Intake and Universal Physical Capture.

Build/CI evidence creates no scientific authority.

## Scientific-master artifact evidence from the tested source

The user-supplied Float32 Scientific Master artifact for the tested source was independently readable as:

- `4080x3072`;
- 3 channels;
- 32-bit IEEE Float per channel;
- SHA-256 `55830c6c780b137c25834d0c2ca64163aea7402c4066f46e404a384d7e7bbeb2`.

This remains separate from JPEG presentation authority.

The uploaded research records retained source SHA-256 `c7356b5ad112e898d720a888410a29705c4aeaa9fcae8b8a6ebf88e2b10c52a6` and reported no source-sample modification, no new measured samples and no scientific writeback permission. Byte-exact source-file immutability is still not independently proven because the original source DNG bytes were not supplied for a before/after hash comparison.

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

After the required Free Raster/JPEG physical acceptance is complete, bind this state more fully into Workspace/ADVANCED and extend the **existing** Appearance renderer/cable. Do not create a duplicate renderer.

## Permanent scientific laws

- **Seal the evidence, not the thinking.**
- **MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE.**
- **Representation may become richer than the source; knowledge claims may not exceed evidence.**
- **Stable outside. Flexible inside. Evidence law unchanged.**
- **One Free World. Many sealed observations. One evidence law.**
- Direct CFA / RAW_SENSOR evidence is immutable and sealed.
- One physical frame remains one physical frame; derived views do not create captures.
- Scientific Master remains separate from Appearance/export.
- `UNKNOWN` is valid and may not silently become zero/certainty.
- Unknown covariance is not zero.
- Registration, reconstruction, precision, resolution and performance do not create authority.
- Cross-observation radiometric fusion requires an admitted common-gauge relation.
- TruthNegative remains observation-bound; observations meet only above that boundary.
- Inferred geometry/material/illumination may not silently become `MEASURED`.
- Appearance may not write back to sealed evidence or Scientific Master.
- No AI/ML/neural/generative runtime is admitted as scientific evidence/inference.

## Interdisciplinary decision foundation

Use the project foundation and matrix for non-trivial decisions across:

- photography / RAW / CFA / sensor metrology;
- optics / lens / PSF / MTF / diffraction / aberration;
- radiometry / photometry / light transport / physically based rendering;
- calibration / colour / black level / white level / clipping / censoring;
- human vision / contrast / adaptation / colour appearance / depth perception;
- 3D/projective geometry and world-space authority;
- stop-motion / temporal sampling / exposure integration / rolling shutter;
- artificial illumination / spectra / flicker / mixed light;
- architecture / photogrammetry / built heritage;
- restoration / conservation / reversible intervention;
- scene-referred vs display-referred pipelines.

External scientific knowledge may improve models, contracts and tests. It never becomes observation-specific evidence by itself.

## Frozen PR #130 audit meaning

Frozen exact head:

`4e4f358ac5d6fb4a8562286257b8edb76ae5d33f`

At that exact head:

- Documentation Governance = proven SUCCESS;
- Research Integrity Guard = no proven exact-head run -> `NOT RUN / UNPROVEN`;
- Lifecycle Contract = no proven exact-head run -> `NOT RUN / UNPROVEN`;
- older failures remain attached to older SHAs only.

PR #131 product work must not rewrite this audit meaning.

## Exact next action

Do **not** redesign the renderer, Scientific Master, T5 or JPEG cable.

1. perform the route-mismatch fail-closed physical protocol;
2. then perform source-switch/reprocess stale-state clearing;
3. optionally perform byte-exact source before/after proof, cold-start/process-death Free Raster restoration and installed APK hash readback if those stronger properties are required;
4. keep PR #131 draft until required acceptance is complete.

## Historical governance provenance / discoverability pointers

The current bootstrap remains authoritative. The following older paths remain explicit only to keep the preserved project history discoverable to repository governance; they are **not** rollback authority:

- `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-19.md`
- `state/CURRENT_PROJECT_STATE_2026-09-19.json`
- `docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md`
- `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-20.md`
- `state/CURRENT_PROJECT_STATE_2026-09-20.json`
- `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-21.md`
- `state/CURRENT_PROJECT_STATE_2026-09-21.json`
- `state/CURRENT_PROJECT_STATE_2026-09-24.json`
- `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-24.md`
- `state/CURRENT_PROJECT_STATE_2026-09-25.json`
- `docs/DRAW_MAIN_PROJECT_STATE_2026-09-25.md`
- `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-09-25.md`
- `state/CURRENT_PROJECT_STATE_2026-09-27.json`
- `docs/DRAW_KNOWLEDGE_GROWTH_INTEGRATION_2026-09-27.md`
- `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-09-27.md`
- `docs/DOCUMENT_STATUS_INDEX_2026-09-27.md`
- `docs/research/free-world-observation-graph-v0.1/README.md`
- `state/CURRENT_PROJECT_STATE_2026-09-28.json`
- `docs/DRAW_CORE_VISION_REALIGNMENT_2026-09-28.md`
- `docs/PROJECT_HISTORY_AND_CHANGES_2026-09-16.md`
- `state/CURRENT_PROJECT_STATE_2026-10-01.json`
- `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-10-01.md`
- `docs/DOCUMENT_STATUS_INDEX_2026-10-01.md`
- `docs/research/measured-field-support-coordinate-bridge-v0.1/README.md`
- `docs/research/optical-field-topography-v0.1/README.md`
- `state/CURRENT_PROJECT_STATE_2026-10-02.json`
- `docs/handoff/DRAW_NEXT_CHAT_HANDOFF_2026-10-02.md`
- `docs/DOCUMENT_STATUS_INDEX_2026-10-02.md`
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
- `docs/handoff/DRAW_44489_PREVIEW_INDEPENDENT_JPEG_CHECKPOINT_2026-10-06.md`
- `docs/handoff/DRAW_44489_PREVIEW_INDEPENDENT_JPEG_DEVICE_PASS_2026-10-06.md`
- `docs/handoff/DRAW_44489_JPEG_90_DEGREE_DEVICE_PASS_2026-10-06.md`
- `docs/handoff/DRAW_44489_ROUTE_MISMATCH_FAIL_CLOSED_TEST_PROTOCOL_2026-10-06.md`
- `state/DRAW_PROJECT_STATE_2026-10-06.json`
- `docs/DOCUMENT_STATUS_INDEX_2026-10-06.md`

## Scientific frontier beyond this product acceptance

Do not add another reconstruction heuristic by default. Highest-value scientific frontier remains:

`multiple real sealed observations -> proven inter-observation registration/gauge/uncertainty -> active Free World Observation Graph -> controlled free-raster/world evaluation`

Parallel tracks remain universal source admission, first-class D.RAWnegative, Free Raster productization, user-visible authority/uncertainty, dynamic Appearance/HDR and full-resolution/multi-observation scale proof.
