# TruthRaw Adaptive UI + Ingress v0.1

Status: **RESEARCH PROTOTYPE — ANDROID BUILD VALIDATION PENDING**

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

The prototype deliberately contains no `ByteArray` RAW payload, `readBytes()` call or `openInputStream()` payload read in the ingress path. When scientific processing is later attached, the `Uri`/file descriptor is handed to the existing tile-native source path so CFA samples can be requested only for required tiles.

This is not the same as saying processing needs zero memory. Reconstruction, previews, room workspaces and output tiles still require bounded memory under Building Runtime / Room ABI admission. The rule is only that the complete source RAW is not copied into a UI-owned full-frame buffer.

## Four user routes

v0.1 exposes the requested input/output relations:

1. `1 upload -> 1 output`
2. `1 upload -> multiple outputs`
3. `multiple different photos -> multiple independent processed outputs`
4. `multiple captures -> enhanced photo or HDR candidate`

The fourth route is intentionally **candidate-only** in this UI prototype. Selecting multiple documents does not by itself prove same-scene relation, alignment, exposure bracketing, temporal independence or valid multi-frame evidence. Until a dedicated Multi-Capture Fusion contract exists, the UI may record user intent but may not fuse the scientific evidence.

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

The measurement uses Android `WindowManager.currentWindowMetrics`, so rotation, multi-window and resized/foldable windows naturally re-evaluate the layout.

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

The first prototype is intentionally dependency-light and uses platform Android views so the ingress/memory/layout contract can be compiled independently before introducing a richer Compose/Material shell. Current Android guidance supports adaptive layouts based on the current window; a later presentation revision can move to Material 3 Adaptive without changing the ingress/scientific contracts.

## Open work

- build/CI validation of this first Android prototype;
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
