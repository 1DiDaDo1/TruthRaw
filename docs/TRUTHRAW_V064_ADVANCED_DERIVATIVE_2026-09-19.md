# TruthRaw v0.64 — Advanced derivative route

Date: 2026-09-19

Status: **IMPLEMENTED / HOST GCC + CLANG + ANDROID CI GREEN / APK VERIFIED**

Active branch:

`integration/truthraw-suite-v0-64-advanced-derivative`

App version:

`0.29-v0.64-advanced-derivative`

## Scientific boundary

v0.64 does **not** modify the v0.63 TRUTHRAW PURE writer or its Scientific Master.

PURE remains:

`sealed source -> measured-preserving reconstruction -> Scientific Master -> Zero-Line / scene-scale / Technical Backplane -> 32-bit IEEE Float DNG`

with private contract:

`TRUTHRAW_PURE_SELF_BINDING_V0_63`

TRUTHRAW ADVANCED is downstream of that same scientific root. It is an appearance/restoration derivative and may not write its choices back into measured evidence, Scientific Master, Zero-Line, scene-scale or the frozen 180-byte Technical Backplane.

## Advanced controls now active

### Natural Light Balance

A conservative shadow/light appearance correction is applied only downstream of the scientific route. Its strength is bounded by the existing exposure-plan `evidenceConfidence` and protects near-black values.

It is **not** claimed as a physical reconstruction of scene illumination, geometry, material BRDF or a counterfactual relight.

### Natural HDR

Advanced consumes the existing single-frame scene-aware half-resolution HDR gain state from the streaming pipeline.

The underlying HDR gain generation already suppresses gain in censored highlight neighbourhoods. Advanced applies a bounded fraction of that gain to the derivative preview and uses a soft highlight shoulder for SDR display.

This is presentation/appearance HDR. It does not create additional measured dynamic range and does not change evidence count.

### Detail / Structure

When enabled, Advanced uses the existing:

`SkinSafeDetailedCrispAppearance`

backend downstream of the same `ResearchEdgeAwareMeasuredPreservingReconstruction`.

The Scientific Master identity remains based on the measured-preserving reconstruction. Detail appearance is not written back into it.

### Evidence-bound Restoration

The first production-wired restoration mode is deliberately narrow.

After the scientific/appearance stream completes, Advanced re-reads only the RAW sample spans needed for its bounded preview and builds a censor mask from measured CFA samples at or above source `WhiteLevel`.

Only those preview locations are eligible for restoration.

A censored preview pixel is compensated only when at least three non-censored neighbouring samples are available within radius 1–2. Otherwise it remains unchanged.

The compensation is a derivative reconstruction only. The underlying scientific state remains `CENSORED`; it is never relabelled `MEASURED`.

Valid non-censored preview pixels are not restoration targets.

## Provenance gates

The Advanced native route still requires:

- sealed source SHA verification before computation;
- source-bound DNG colour admission;
- the existing edge-aware measured-preserving reconstruction;
- Scientific Master Streaming Binding v0.2;
- Zero-Line / TruthRange self gauge;
- Technical Backplane Phase 2;
- physical frame count = 1;
- independent evidence count = 1;
- `scientificMasterModifiedByAppearance=false`;
- `counterfactualObservationCreated=false`;
- no adapter-owned full RAW/SDR/half-gain/diagnostic frame;
- source SHA re-verification after Advanced processing.

## Current output scope

v0.64 makes Advanced **functionally selectable and testable in the app**.

The current Advanced visual/export surface remains the bounded preview path:

- maximum preview edge: 384 px;
- Advanced JPEG export is encoded from that bounded sRGB derivative preview.

Therefore v0.64 is **not yet a full-resolution Advanced export engine**. A later transport/export step must stream the same authority-bound Advanced transforms to a full-resolution output format without materializing a prohibited full-frame scientific workspace.

PURE full-resolution Float32 DNG remains unchanged and fully separate.

## UI

The `TRUTHRAW ADVANCED` card is now enabled.

Selecting it opens a control screen with:

- Natural Light Balance;
- Natural HDR;
- Detail / Structure;
- Evidence-bound Restoration.

Using Advanced stores it as the preferred output route and opens the existing RAW/DNG processor. Camera captures that converge on the same RAW ingress also inherit the selected output preference.

The processor reports Advanced derivative metrics including:

- light-adjusted preview pixels;
- HDR-gain preview pixels;
- censored preview pixels;
- restored preview pixels;
- detail state.

## CI and APK

Workflow run `35471926706`: **SUCCESS**

- host GCC: SUCCESS;
- host Clang: SUCCESS;
- Android arm64: SUCCESS;
- artifact ID: `10593425008`;
- artifact name: `truthraw-suite-v0-64-advanced-derivative-debug-arm64`;
- artifact ZIP SHA-256: `fa2343ed75784752056ded903657818076d0a70f19a551f381cb37954f78fa1d`;
- extracted APK bytes: `5961945`;
- extracted APK SHA-256: `8d5b0e54517fe29c7e26eade989652aba5292fd498308d41d11dd6691d662441`.

The v0.63 PURE self-binding marker remains present in the v0.64 native library, confirming that Advanced was added beside PURE rather than replacing its writer contract.
