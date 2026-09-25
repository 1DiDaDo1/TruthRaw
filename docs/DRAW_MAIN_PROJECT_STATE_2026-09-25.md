# D.RAW main project state — 2026-09-25

Status: **CURRENT ACTIVE INTEGRATION**

Active integration branch:

`integration/truthraw-suite-v0-84-3-float32-full-colour-scientific-master`

Current code-bearing integration checkpoint:

`420bc1df259ef4db80448c2088bc5c4bf77a4599`

Initial Free-World v0.2-v0.7 promotion checkpoint:

`e1fde59a9edf097fe2ce3fdf996fe181a94ac5e0`

## Current architecture

The Free-World output-pixel research arc v0.2 through v0.7 is now compiled into the Android main native library:

```text
sealed Source Evidence
  -> Scientific Master
  -> Continuous 2D Scene Field        (v0.2)
  -> Scientific/Open Scene binding    (v0.3)
  -> Deep Scene contributions         (v0.4)
  -> object/region + geometry/radiometry authority (v0.5)
  -> Light Transport state            (v0.6)
  -> Appearance / Viewing / Display Resolve (v0.7)
  -> finite visible output
```

Permanent rule:

> **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

The promotion does not grant new evidence. The downstream layers keep:

- `physicalFrameCount = 1`;
- `independentEvidenceCount = 1`;
- `createsNewEvidence = false`;
- `scientificWritebackAllowed = false`.

## v0.7 appearance boundary

The v0.7 layer makes viewing and display state explicit rather than hiding it inside arbitrary RGB curves.

It binds:

- scene colorimetry identity;
- viewing-condition identity;
- display-target identity;
- appearance-policy identity;
- output identity.

Reference display encodings:

- Linear normalized;
- sRGB;
- PQ / ST 2084.

Changing viewing conditions or the display target changes the visible derivative, not the source-scene identity or scientific authority.

## Android route UI

The launcher now uses the architecture directly:

- **D.RAW PURE -> Scientific View**
- **D.RAW ADVANCED -> Appearance / Restoration View**
- **D.RAW PRO -> Open Scene / Light Transport**

File and camera input labels adapt to the selected route.

The main processing screen shows the same route semantics.

ADVANCED is explicitly downstream appearance/restoration. PRO exposes the Open Scene / Deep Scene / light-transport architecture.

## TruthNegative Continuous v0.5

TruthNegative now has a raster-independent scientific-negative state in addition to the existing TN-4 materialized container and historical fixed 4x dense projection.

The v0.5 state binds:

- sealed source SHA-256;
- Scientific Master SHA-256;
- canonical Open Scene Field v0.85 authority-field SHA-256;
- reconstruction backend and colour-binding identity;
- one physical frame and one independent evidence item.

The target raster is deliberately **not** part of this state identity. A 12 MP, 50 MP, 200 MP or preview lattice is a finite query of the same TruthNegative state. Every target site remains derived and creates zero measured target claims.

The raster resolver precomputes X/Y area footprints once per finite target raster. Cached and direct queries are regression-tested to produce the same query SHA, values and source footprint.

## First production bridge

PRO now exposes **TruthNegative Continuous v0.5** as a bounded DNG-only diagnostic preview:

```text
admitted DNG
 -> sealed source + source-bound colour
 -> F64 Scientific Master
 -> Technical Backplane phase 2
 -> Open Scene Field v0.85
 -> canonical TruthNegative authority-field digest
 -> raster-independent TN v0.5 state
 -> Free-World area-integrated query
 -> v0.7 neutral Appearance/Display resolve
 -> sRGB preview
```

The bridge requires Scientific Master/Open Scene numeric bit-identity on every loaded field tile, re-verifies source identity, preserves one-frame/one-evidence, and rejects any scientific writeback.

The preview is capped at 192 pixels on its longest edge for this first production diagnostic. It displays source/target authority counts, footprint-link count and shortened state identity in the PRO UI.

**PURE is unchanged. Existing scientific/export routes are unchanged.** This bridge opts in only the new PRO diagnostic preview.

## Important integration boundary

The v0.2-v0.7 and TruthNegative Continuous v0.5 native modules are part of the Android main build. New routes opt in explicitly. Existing validated scientific/export paths are **not silently rerouted**.

## Green validation

Current bridge checkpoint `420bc1df259ef4db80448c2088bc5c4bf77a4599`:

- Android signed ARM64 build + PRO bridge run `36112964452`: **SUCCESS**
- Scientific Master F64 + Color Audit run `36112964482`: **SUCCESS**
- Open Scene Field/local authority run `36112964500`: **SUCCESS**
- Unified Output Preview + Android integration run `36112964468`: **SUCCESS**
- TruthNegative Continuous v0.5 GCC/Clang/ASan/UBSan run `36107899950`: **SUCCESS**
- TruthNegative Round-Trip + Optics gates run `36111981454`: **SUCCESS**
- TruthNegative Deep Scene Bridge v0.8 run `36112847260`: **SUCCESS**

Research validation before promotion:

- Free-World v0.2-v0.7 GCC: **PASS**
- Free-World v0.2-v0.7 Clang: **PASS**
- ASan/UBSan: **PASS**

## Current APK artifact

GitHub Actions artifact:

- artifact ID: `10854211375`
- artifact name: `truthraw-v0-84-2-compute-router-debug-arm64`
- artifact ZIP digest: `sha256:4f41ffd716a1ec61776a0f4721169ec14195f9864f3b80129a74f019f7029686`
- APK size: `6375691` bytes
- extracted APK SHA-256: `8d318ecbb2cc2f3ec758559273b0cfa36eea69f2704997e39492ef4973b4a6e5`

Stable development signing certificate SHA-256:

`a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`

The stable signing identity remains development/test-only and unchanged.

## TruthNegative Round-Trip Oracle v0.6

The round-trip/explainability gate now exists as an executable reference.

Across multiple finite target rasters it requires:

- unchanged TruthNegative Continuous state identity;
- zero measured-target claims;
- normalized positive source footprints;
- one physical frame / one independent evidence item;
- no scientific writeback;
- global scene-linear area-average conservation against the canonical 1x1 whole-frame integral;
- deterministic query-chain SHA-256.

This deliberately tests conservation/explainability rather than claiming arbitrary resize inversion.

## TruthNegative Optics Support v0.7

A calibration-bound optics-support gate now exists.

It accepts a PSF/MTF calibration identity and separates MEASURED, CALIBRATED_ESTIMATE, INFERRED and UNKNOWN calibration authority.

Only MEASURED or CALIBRATED_ESTIMATE optics may affect the **scientific support footprint**. The v0.7 gate does not change scene RGB, does not sharpen, does not deconvolve and does not upgrade authority.

Inferred optics may be retained for research but fail closed for scientific support.

Both v0.6 and v0.7 are compiled into the Android main native library after green GCC/Clang/ASan/UBSan validation.

## TruthNegative Deep Scene Bridge v0.8

TruthNegative Continuous now has a direct authority-preserving bridge into the Free-World Deep Scene.

The bridge binds one finite TruthNegative query into:

- evidence-constrained deep radiometry;
- object / region / provenance identity;
- separate geometry authority;
- deterministic deep-scene ancestry.

A TruthNegative radiometric result can remain evidence-bound while its assigned depth is only INFERRED.

The bridge also creates an explicitly **INFERRED** Lambertian light-transport seed for physically based research. Hidden material, illumination, surface normal and spectral hypotheses remain inferred and do not inherit scientific measurement authority merely because the camera-plane radiometry is bound to TruthNegative.

v0.8 GCC/Clang/ASan/UBSan validation is green.

## Next implementation boundary

Validate the PRO TruthNegative Continuous preview on-device with real admitted DNGs. For inverse-optics reconstruction, first acquire/derive independently admitted lens/sensor PSF/MTF calibration; do not enable deconvolution before that evidence exists. The next software-only step can be the Camera-5 colour/highlight oracle and a native TruthNegative scientific-negative container/export contract. The deep-scene bridge is now available as the boundary for later PRO object/light-transport diagnostics.
