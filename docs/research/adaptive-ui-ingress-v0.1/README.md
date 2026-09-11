# TruthRaw Adaptive UI + Ingress v0.1

Status: **RESEARCH PROTOTYPE — ANDROID APK CI PASS**

Validated implementation: `bed7c29b5d0773f0ccf499448e4f0c9cee9ea7ae`.

Validation on that implementation: Adaptive UI + Ingress v0.1 push run `34582353230` — **SUCCESS**. The contract verifier, upstream TruthRaw integrity, no-camera-permission gate and Android APK assembly all passed. The generated debug APK was `2,380,265` bytes with SHA-256 `95e5326cd39fb1a100a1d198c157f7a5682ce806640a4bf2b178dbf957662d5e`.

## Purpose

This layer makes TruthRaw usable without moving scientific authority into the UI. It provides a universal adaptive Android shell and a lightweight ingress/batch model above the existing Tile-Native DNG Source, Building Runtime and Room ABI work.

The UI is an orchestrator only. It may not:

- reinterpret Direct-CFA evidence;
- create scene evidence from UI state;
- modify the Scientific Scene Master;
- define or move the global TruthRange zero-line;
- introduce an ISO scene axis;
- silently combine separately selected physical captures;
- change scientific output because a device has a different screen or resource tier.

## Source-memory rule

Selecting a RAW does **not** materialize the RAW payload in the UI process model.

Ingress stores only:

- a persisted/caller-granted Android document `Uri`;
- display name;
- provider-declared byte size when available;
- job/route state.

The prototype deliberately contains no RAW payload byte-array/read-all path in ingress. When scientific processing is later attached, the `Uri`/file descriptor is handed to the existing tile-native source path so CFA samples can be requested only for required tiles.

This is not the same as saying processing needs zero memory. Reconstruction, previews, room workspaces and output tiles still require bounded memory under Building Runtime / Room ABI admission. The rule is only that the complete source RAW is not copied into a UI-owned full-frame buffer.

The CI contract reports `source_raw_full_materialization=0_by_ui_contract`.

## Four user routes

v0.1 exposes the requested input/output relations:

1. `1 upload -> 1 output`
2. `1 upload -> multiple outputs`
3. `multiple different photos -> multiple independent processed outputs`
4. `multiple captures -> enhanced photo or HDR candidate`

The fourth route is intentionally **candidate-only** in this UI prototype. Selecting multiple documents does not by itself prove same-scene relation, alignment, exposure bracketing, temporal independence or valid multi-frame evidence. Until a dedicated Multi-Capture Fusion contract exists, the UI may record user intent but may not fuse the scientific evidence. CI reports `multi_capture_fusion=AUTO_DISABLED`.

Selecting the exact same stored file multiple times is deduplicated by URI in the prototype and may never create additional independent evidence.

## Lineage rule

For ordinary batch processing, every selected source remains an independent TruthRaw lineage:

`RawJob 1 -> lineage 1`

`RawJob 2 -> lineage 2`

`RawJob N -> lineage N`

Batch membership is organizational state, not shared scene truth.

## Adaptive UI rule

The interface adapts to the **current application window**, not to phone brand/model.

Prototype breakpoints:

- Compact: `< 600 dp`
- Medium: `600 .. < 840 dp`
- Expanded: `>= 840 dp`

The measurement uses Android `WindowManager.currentWindowMetrics`, so rotation, multi-window and resized windows naturally re-evaluate the layout. Fold/hinge occlusion-aware placement remains open work.

The prototype renders:

- Compact: image/scene focus, route controls below, lightweight job strip;
- Medium: job/input pane + scene pane;
- Expanded: job/input pane + scene pane + tools/rooms pane.

These are UI layout choices only. Screen size may never change scientific authority or reconstruction truth.

## Device-resource independence

UI adaptation and processing adaptation are separate axes:

`window size/posture -> UI composition`

`RAM / CPU / thermal -> Building Runtime execution policy`

A small flagship phone can therefore use the Compact UI while still receiving a high-resource execution policy. A large low-memory tablet can use an Expanded UI while Building Runtime still admits only a conservative processing plan.

## Current prototype implementation

Android source:

`app/android/truthraw-adaptive-ui-v01/`

The first prototype is intentionally dependency-light and uses platform Android views so the ingress/memory/layout contract can be compiled independently before introducing a richer Compose/Material shell. A later presentation revision can move to Material 3 Adaptive without changing the `RawHandle`, `RawJob`, `BatchSession`, route or scientific-boundary contracts.

The first interface layouts are documented in `DESIGN_CONCEPT_v0_1.md`.

## Proof boundary

The current PASS proves the Android project builds and that its static ingress/UI contract is enforced. It does **not** yet prove:

- execution of this UI prototype on physical Android hardware;
- real-device UI frame times or smoothness;
- JNI hand-off of a selected source descriptor into TileNativeDngSource;
- progressive TruthRaw previews;
- persisted queue recovery after process death;
- multi-capture registration/fusion/HDR science;
- production-quality accessibility, fold posture handling or final visual design.

## Open work

- actual JNI hand-off of source file descriptors into the tile-native engine;
- progressive preview transport;
- bounded thumbnail/proxy cache;
- persisted job queue across process death;
- accessibility and keyboard/focus validation;
- fold/hinge occlusion-aware placement;
- Material 3 Adaptive presentation implementation;
- real-device smoothness / frame-time measurements;
- multi-capture grouping/evidence contract;
- HDR/fusion implementation;
- production export UX.
