# D.RAW 44489 — Shared Preview / Full-Resolution Purple Highlight Regression Candidate — 2026-10-09

## Purpose

This handoff records a new real-device regression bundle and the bounded downstream Appearance fix candidate. It does not promote scientific authority and does not alter sealed source/CFA, Scientific Master, reconstruction authority, Natural Light Local Field Tone, Warm Illuminant Retention, or the Q100/4:4:4 codec contract.

Failure bundle supplied by the user:

`DRAWtestpurplehighlightfailagain.zip`

The bundle contains a real PRO full-resolution JPEG plus Android UI screenshots. Purple/magenta contamination is visible in bright ceiling/exterior highlight regions in both the UI preview and the actual saved full-resolution output. Therefore this is not classified as JPEG-only and not classified as UI-only.

## Prior fix status and root cause

The previously accepted near-white highlight guard had not disappeared. The current header and the physically accepted Warm Illuminant head shared the same original blob identity before this candidate.

The original `presentation_highlight_chroma_rolloff_v0_1` was deliberately near-neutral. It returned without correction when the minimum RGB channel fell below 0.65. The earlier regression modeled approximately `(R,G,B)=(1.0,0.84,1.0)`.

The new real-device failure exposes a different class: bright/censored samples can retain R and B near the display-white boundary while G collapses far enough below the old near-neutral gate. This produces severe magenta/purple while bypassing the old guard. Therefore the previous fix was real but too narrow; it was not overwritten.

## Historical UI preview route recovered

The prior chat had already located the correct preview cable. This continuation preserves that architecture rather than adding a second UI colour correction.

Historical commits:

- `67a64167e9a40ef14ae80fdbe6007b650a721219` introduced the exact pre-JPEG RGB24 UI preview route through `PreJpegRgb24PreviewRendererV01` and changed ADVANCED/PRO to use it;
- `b78ed7d8baee799062e9ba51f803a52281291e36` hardened fail-closed route-flag equality for that preview;
- the later preview-fix-labelled `58b8a777...` checkpoint was CI/idempotence support, not the original pixel-route change;
- `16cd5261...` is associated with the later full-resolution highlight-call integration.

Current `PreJpegRgb24PreviewRendererV01` still calls `PhotoExportNativeBridge.renderFullResNv21(...)`, verifies the returned route flags, reads the presentation RGB byte count from packet word 47, and samples the exact pre-JPEG RGB24 sibling after the NV21 region. Thus ADVANCED/PRO UI preview and saved full-resolution output share one native presentation pixel cable.

Permanent rule:

> Do not add a second UI-only highlight correction. Highlight Appearance corrections must live in the shared native pre-JPEG presentation cable so UI preview and saved full-resolution output cannot silently diverge.

## Candidate fix

Native candidate commit:

`c2bac00a...` — `presentation: guard censored white-boundary magenta collapse`

Regression/preview-lock commit and exact validated candidate code head:

`ed3d3eb48c7745c2c624a14464277759c697fbe0`

The extension remains inside the existing `presentation_highlight_chroma_rolloff_v0_1` downstream Appearance helper. It adds a bounded branch only when all relevant conditions indicate the observed white-boundary/censored R/B-high, G-collapse signature:

- sample is marked censored by the existing source-support mask;
- maximum display-linear RGB is above the white-boundary gate;
- R and B are both high;
- R and B remain sufficiently balanced with each other;
- G has a substantial deficit relative to R/B;
- luminance remains above a bounded guard.

The operation contracts display chroma toward the Rec.709 luminance axis while preserving Rec.709 luminance. Maximum severe contraction is bounded below full neutralisation. It explicitly does **not** claim recovered highlight chromaticity.

Protected cases in regression:

- normal tones unchanged;
- censored yellow highlight remains coloured;
- non-censored saturated magenta remains coloured;
- original near-white magenta case remains fixed;
- the new severe censored `(1.0,0.50,1.0)` class contracts strongly with luminance preserved;
- an imbalanced R/B colour that does not match the observed signature remains unchanged;
- PURE remains outside the ADVANCED/PRO highlight branch.

## Shared preview/full-resolution regression lock

`tools/test_presentation_highlight_chroma_rolloff_v0_1.py` now also requires the current Android preview architecture to remain bound to the exact native pre-JPEG output:

- `PhotoExportNativeBridge.renderFullResNv21(...)` present in `PreJpegRgb24PreviewRendererV01`;
- presentation RGB byte count transported through packet word 47;
- preview sampling begins after the NV21 byte region;
- exact returned route flags must equal requested flags;
- ADVANCED/PRO is wired through `PreJpegRgb24PreviewRendererV01.render(...)`.

The test emits both the highlight-regression PASS and a shared-preview/full-resolution RGB24 PASS marker.

## Exact-head CI

Workflow: `D.RAW Free Raster v0.3 Finish APK`

Run: `37923137512`

Exact built head: `ed3d3eb48c7745c2c624a14464277759c697fbe0`

Result: **SUCCESS**

Proven green in this run:

- deterministic Free Raster patch;
- presentation gamut-fit regression;
- ADVANCED/PRO highlight chroma-rolloff regression including severe censored-magenta case and shared UI-preview/full-res RGB24 lock;
- Warm Illuminant Retention v0.1 regression;
- Natural Light Local Field Tone v0.1 regression;
- PURE Float32 headroom regression;
- explicit presentation headroom wiring;
- sealed Full-Frame Streaming integrity;
- strict Q100/4:4:4 High-Fidelity JPEG codec regression;
- Kotlin/unit tests;
- NDK/C++ build;
- APK verification;
- runtime-wiring step;
- artifact upload.

PR #131 remained on the exact same code head after the workflow, so the workflow did not push a hidden runtime rewrite.

## Candidate APK

Artifact ID: `11612464734`

Artifact name: `draw-free-raster-v03-fullres-candidate-apk`

Artifact ZIP digest: `sha256:5d418e0366b9de8be008146daef03dab491ab60c16f938df7b45110f87ba08a1`

Extracted APK:

- bytes: `8,862,571`;
- SHA-256: `e582ff1e75d71921deb8f49c5c2e223dae668dc1aa5e659dddaa1d4ccac34b6d`.

## Scientific / authority boundary

Unchanged:

- `MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`;
- no sealed source/CFA mutation;
- no Scientific Master mutation;
- no scientific writeback;
- no claim of recovered clipped colour;
- no vendor/camera/lens fitting;
- no AI/ML/generative inference;
- no second preview renderer or second output renderer;
- no Local Field Tone or Warm Illuminant parameter change.

The new contraction is strictly `APPEARANCE_ONLY / DERIVED_PRESENTATION_OUTPUT`.

## Physical acceptance still required

This candidate is **CI PASS / REAL-DEVICE NOT YET ACCEPTED**.

Use the same problematic observation/scene and inspect all of the following together:

1. ADVANCED UI preview;
2. PRO UI preview;
3. saved full-resolution PRO JPEG.

Required success conditions:

- previous purple/magenta contamination in bright ceiling/exterior white regions is removed or reduced to a physically credible appearance;
- UI preview and saved full-resolution output agree on the highlight treatment;
- legitimate saturated highlights are not blanket-desaturated;
- Warm Illuminant appearance remains intact;
- Natural Light Local Field Tone behaviour does not regress;
- black regions remain protected;
- no new halos/seams or colour-ratio artifacts appear;
- Q100/4:4:4 saved-output compatibility remains intact;
- PURE remains unaffected.

Only after that physical comparison may this candidate be reclassified as real-device PASS.

PR #131 must remain draft and unmerged until physical/product acceptance is complete.
