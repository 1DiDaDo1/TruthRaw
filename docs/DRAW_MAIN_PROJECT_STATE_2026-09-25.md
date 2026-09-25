# D.RAW main project state — 2026-09-25

Status: **CURRENT ACTIVE INTEGRATION**

Active integration branch:

`integration/truthraw-suite-v0-84-3-float32-full-colour-scientific-master`

Current code-bearing integration checkpoint:

`c7d7cef05502aca6f22f0d049987aad3a9f37b0a`

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

Current A-D integration checkpoint `c7d7cef05502aca6f22f0d049987aad3a9f37b0a`:

- Android signed ARM64 build + Camera-5/container/UI bridges run `36115128965`: **SUCCESS**
- Scientific Master F64 + Color Audit run `36112964482`: **SUCCESS**
- Open Scene Field/local authority run `36112964500`: **SUCCESS**
- Unified Output Preview + Android integration run `36115129004`: **SUCCESS**
- TruthNegative Continuous v0.5 GCC/Clang/ASan/UBSan run `36107899950`: **SUCCESS**
- TruthNegative Round-Trip + Optics gates run `36111981454`: **SUCCESS**
- TruthNegative Deep Scene Bridge v0.8 run `36112847260`: **SUCCESS**

Research validation before promotion:

- Free-World v0.2-v0.7 GCC: **PASS**
- Free-World v0.2-v0.7 Clang: **PASS**
- ASan/UBSan: **PASS**

## Current APK artifact

GitHub Actions artifact:

- artifact ID: `10855057536`
- artifact name: `truthraw-v0-84-2-compute-router-debug-arm64`
- artifact ZIP digest: `sha256:8fc4b769ab8b6fcd8c98924e0a1187a50fee65d1a8f6ec349d8766e32e637865`
- APK size: `6457579` bytes
- extracted APK SHA-256: `dd2ca57e0de292dbc06617d9efce39cff7d6b242a1b20de1ebf74808b1ed5273`

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

## Camera-5 Color / Highlight Oracle v0.1

Step A/B is now implemented and integrated into PRO.

The oracle is available only when the app has the exact verified physical-Camera-5 acquisition/envelope lineage. It evaluates the same TruthNegative/Scientific-Master scene through EV 0, -0.5, -1, -2 and -3 appearance resolves and can localize the first defensible problem stage as:

- SOURCE_CENSORING;
- METADATA_NEUTRAL_MISMATCH;
- COLOR_BINDING;
- APPEARANCE_DISPLAY;
- NONE;
- UNRESOLVED.

The known diagnostic Camera-5 neutral near [0.59,1,0.55] is **not** calibration and is never written into the scientific pipeline. DNG metadata alone is not allowed to prove remosaic state; remosaic remains UNKNOWN until separately sealed runtime evidence is supplied.

The PRO UI now exposes **Camera-5 Color/Highlight Oracle** and shows candidate count, censor fraction, AsShotNeutral versus empirical neutral, low-exposure green bias/drift, first failure stage and oracle identity.

## TruthNegative Native Container v0.1

Step C/D is now implemented through the export/import contract and Android bridge.

The new .tnc format binds:

- source-evidence SHA-256;
- Scientific-Master SHA-256;
- Open Scene authority-field SHA-256;
- raster-independent TruthNegative-state SHA-256;
- exact Float32 scientific value plane;
- Open Scene Field v0.85 role/authority/uncertainty/support/bounds;
- per-tile payload SHA-256;
- whole-body SHA-256.

The Open Scene metadata encoding alone is deliberately insufficient because v0.85 does not duplicate numeric Scientific-Master values. The .tnc payload therefore stores the Float32 value plane alongside the canonical Open Scene encoding.

The host test now performs write -> native import -> record-by-record Float32 bit-identity verification and rejects payload corruption. The Android export bridge writes the container, immediately opens it again natively, recomputes the imported authority-field digest and rebuilds the TruthNegative state; both identities must match before success is reported. A Kotlin post-write header verification is an additional gate.

The PRO UI exposes **Export TruthNegative Native · .tnc**.

Host GCC/Clang/ASan/UBSan validation for Camera-5 Oracle + Native Container: run `36114391772` — **SUCCESS**.

Android signed ARM64 build with all A-D bridges/UI: run `36115128965` — **SUCCESS**.
Unified Output Preview Android integration: run `36115129004` — **SUCCESS**.

## Remaining real-device gate

A-D are implemented as far as software/CI can verify them without a new physical run. Still pending:

1. run Camera-5 Oracle on a real verified physical-5 DNG/evidence pair and retain its diagnostic result;
2. export a real .tnc on-device and confirm native + Kotlin post-write round-trip success;
3. feed that imported container into subsequent PRO diagnostics;
4. keep inverse-optics/deconvolution blocked until independently admitted PSF/MTF calibration exists.

PURE and existing validated exports remain unchanged.
