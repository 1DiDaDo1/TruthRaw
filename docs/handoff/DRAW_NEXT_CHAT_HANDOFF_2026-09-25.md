# D.RAW next-chat handoff — 2026-09-25

## Start here

Current public name: **D.RAW**

Active integration:

`integration/truthraw-suite-v0-84-3-float32-full-colour-scientific-master`

Current code-bearing integration checkpoint:

`58e3b6332c828d9b8ea955b0c79b0c5f69342761`

Read first:

1. `START_HERE_NEW_CHAT.md`
2. `state/CURRENT_PROJECT_STATE_2026-09-25.json`
3. `docs/DRAW_MAIN_PROJECT_STATE_2026-09-25.md`
4. `docs/research/free-world-output-pixel-v0.1/README.md`
5. `docs/research/free-world-pixel-resolve-2d-v0.2/README.md`
6. `docs/research/free-world-deep-scene-contribution-v0.4/README.md`
7. `docs/research/free-world-deep-scene-binding-v0.5/README.md`
8. `docs/research/free-world-light-transport-state-v0.6/README.md`
9. `docs/research/free-world-appearance-resolve-v0.7/README.md`
10. `docs/research/truthnegative-continuous-v0.5/README.md`

## What just changed

The complete Free-World research line v0.2-v0.7 was promoted into the main Android native build after green research CI.

The launcher/UI route logic was updated to:

```text
PURE     = Scientific View
ADVANCED = Appearance / Restoration View
PRO      = Open Scene / Light Transport
```

The new layers are compiled into main. TruthNegative Continuous v0.5 now defines a raster-independent scientific-negative state and the first admitted bridge is a PRO-only DNG diagnostic preview through v0.7. Existing exports and PURE remain unchanged.

## Permanent scientific boundaries

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

- Source Evidence remains immutable.
- Scientific Master remains separate from appearance/display derivatives.
- Geometry authority and radiometric authority are separate axes.
- Deep/inferred/restoration/counterfactual contributions never silently become measured evidence.
- Light-transport plausibility does not create evidence.
- Display/viewing changes do not alter scene authority.
- Single-frame remains one physical frame / one independent evidence source.
- Stable Android signing changes package identity continuity only; it changes no scientific authority.

## Current green main validation

- Android ARM64 signed build + TN Continuous PRO bridge: run `36108523949` — SUCCESS.
- Scientific Master F64 + Color Audit: run `36079724798` — SUCCESS.
- Open Scene Field/local authority: run `36079724888` — SUCCESS.
- Unified Output Preview + Android integration: run `36108523910` — SUCCESS.
- TruthNegative Continuous v0.5 GCC/Clang/ASan/UBSan: run `36107899950` — SUCCESS.

## Current installable APK

Artifact ID: `10851937980`

APK SHA-256:

`a22b0316914563ba45d7caad357cc03009f740f09e3a589451c41b8beeacdc1b`

Development certificate SHA-256:

`a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`

## Recommended continuation

Continue from the current main branch. First run the new PRO · TruthNegative Continuous v0.5 preview on real admitted DNGs and retain any failure/status output. Then build the round-trip/explainability oracle and only after that research optics-aware reconstruction with independently measured PSF/MTF/calibration. Do not reroute PURE or existing exports without their own validation.
