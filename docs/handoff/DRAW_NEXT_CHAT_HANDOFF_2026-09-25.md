# D.RAW next-chat handoff — 2026-09-25

## Start here

Current public name: **D.RAW**

Active integration:

`integration/truthraw-suite-v0-84-3-float32-full-colour-scientific-master`

Current code-bearing integration checkpoint:

`c7d7cef05502aca6f22f0d049987aad3a9f37b0a`

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
11. `docs/research/truthnegative-roundtrip-oracle-v0.6/README.md`
12. `docs/research/truthnegative-optics-support-v0.7/README.md`
13. `docs/research/truthnegative-deep-scene-bridge-v0.8/README.md`
14. `docs/research/truthnegative-camera5-color-highlight-oracle-v0.1/README.md`
15. `docs/research/truthnegative-native-container-v0.1/README.md`

## What just changed

The complete Free-World research line v0.2-v0.7 was promoted into the main Android native build after green research CI.

The launcher/UI route logic was updated to:

```text
PURE     = Scientific View
ADVANCED = Appearance / Restoration View
PRO      = Open Scene / Light Transport
```

The new layers are compiled into main. TruthNegative Continuous v0.5 now defines a raster-independent scientific-negative state and the first admitted bridge is a PRO-only DNG diagnostic preview through v0.7. TruthNegative Round-Trip Oracle v0.6, calibration-bound Optics Support v0.7 and the authority-preserving TruthNegative Deep Scene Bridge v0.8 are green. Camera-5 Color/Highlight Oracle v0.1 and TruthNegative Native Container v0.1 are now also integrated into PRO. Existing exports and PURE remain unchanged.

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

- Android ARM64 signed build + current A-D bridges/UI: run `36115128965` — SUCCESS.
- Scientific Master F64 + Color Audit: run `36112964482` — SUCCESS.
- Open Scene Field/local authority: run `36112964500` — SUCCESS.
- Unified Output Preview + Android integration: run `36115129004` — SUCCESS.
- TruthNegative Continuous v0.5 GCC/Clang/ASan/UBSan: run `36107899950` — SUCCESS.
- TruthNegative Round-Trip + Optics gates: run `36111981454` — SUCCESS.
- TruthNegative Deep Scene Bridge v0.8: run `36112847260` — SUCCESS.
- Camera-5 Color/Highlight Oracle + TruthNegative Native Container: run `36114391772` — GCC/Clang/ASan/UBSan SUCCESS.

## Current installable APK

Artifact ID: `10855057536`

APK SHA-256:

`dd2ca57e0de292dbc06617d9efce39cff7d6b242a1b20de1ebf74808b1ed5273`

Development certificate SHA-256:

`a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`

## Recommended continuation

Steps A-D are now implemented in code and CI. The next useful evidence comes from the physical device: run the PRO Camera-5 Color/Highlight Oracle on an exactly verified physical-5 DNG/evidence pair, then export **TruthNegative Native · .tnc** and retain the success/status text. Remosaic remains UNKNOWN unless separately sealed runtime evidence is supplied. Do not enable inverse-optics/deconvolution until independently admitted PSF/MTF calibration exists. Do not reroute PURE or existing exports without their own validation.
