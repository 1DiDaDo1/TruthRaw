# DNG Compatibility Projection v0.2

Status: **RESEARCH CANDIDATE — IMPLEMENTED; HOST/ANDROID VALIDATION BLOCKED UNTIL ACTIONS RUNNERS EXECUTE**

## Purpose

v0.2 corrects the color-profile semantics of the v0.1 DNG compatibility writer without changing the Scientific Master, reconstruction, TruthRange, Backplane, evidence count, or color authority.

The v0.1 writer already streams exact camera-native reconstructed Scientific Master RGB into a LinearRaw DNG and verifies the regenerated Scientific Master digest before success. The problem found during the v0.2 audit is narrower: `ScientificColorBindingRecord::cameraToXyzD50` is already the finalized as-shot camera -> XYZ D50 transform, while v0.1 inverted it into `ColorMatrix1` and also advertised the source's non-D50 resolved white as `AsShotWhiteXY`. A normal no-ForwardMatrix DNG reader can then chromatically adapt that already-D50 transform a second time.

v0.2 prevents that second adaptation by representing the compatibility profile as a fixed as-shot D50 camera transform.

## Fixed-D50 compatibility encoding

The pixel samples remain unchanged.

For a finalized TruthRaw matrix `M = cameraToXyzD50`:

- `ColorMatrix1 = inverse(M)`;
- `CalibrationIlluminant1 = D50`;
- `AsShotWhiteXY = D50 = (0.3457, 0.3585)`;
- no ForwardMatrix is emitted by this compatibility writer.

Under the DNG no-ForwardMatrix path, `inverse(ColorMatrix1)` therefore recovers `M`, and the chromatic-adaptation leg is D50 -> D50 identity.

The real source-resolved white remains part of the sealed source lineage and caller-side audit. v0.2 deliberately does **not** claim that xy/CCT is serialized into DNGPrivateData yet; `sourceResolvedWhiteRetainedAsProvenance=false` remains explicit until a later private-data extension is implemented and tested.

This is a compatibility-profile encoding, not a new physical calibration and not a source-profile roundtrip.

## Projection roles

### `LINEAR_SCIENTIFIC_MASTER_COMPATIBILITY_PROJECTION`

- samples are exact camera-native reconstructed RGB from the Scientific Master reconstruction boundary;
- float32 LinearRaw DNG;
- no sRGB, appearance, tone curve, or preview pixels are used;
- writer recomputes the Scientific Master digest and fails closed on mismatch;
- downstream compatibility representation only.

### `RECONSTRUCTED_CFA_PROJECTION`

- derived CFA projection from reconstructed camera-native RGB;
- v0.2 exposes this route only with `ResearchEdgeAwareMeasuredPreservingReconstruction`, not arbitrary reconstruction backends;
- it is not original sensor-code evidence;
- it does not create a second physical measurement;
- it does not promote source-metadata-bound color to independent physical calibration.

The historical v0.1 generic CFA API remains unchanged for audit history and is not the v0.2 public claim boundary.

## Authority invariants

Every successful export must preserve:

- `physicalFrameCount = 1`;
- `independentEvidenceCount = 1`;
- `projectionIsEvidence = false`;
- `colorAuthorityPromoted = false`;
- exact source seal lineage;
- exact finalized Scientific Master digest;
- no full RAW materialization;
- no modification of Scientific Master, TruthRange zero-line, Backplane, or preview authority.

`SOURCE_METADATA_BOUND` remains source metadata bound. `FULL_PHYSICAL` color is not created by DNG export.

## Android route

The Android bridge:

1. seals/reverifies the exact source;
2. resolves DNG color via Color Binding Producer v0.2;
3. prepares source-bound scientific color authority;
4. opens TileNative from the same sealed byte source;
5. computes Scientific Master Streaming Binding v0.2;
6. finalizes Technical Backplane phase 2;
7. requires admission to match the prepared source/color lineage;
8. streams the requested DNG projection;
9. requires the export-time Scientific Master digest to match the finalized digest;
10. reverifies the source after export;
11. rejects any evidence/authority/materialization/resource invariant violation.

Failed Android exports delete/abandon the partial destination document rather than presenting it as a successful DNG.

Android research build version: `0.5-dngproj` / version code `5`.

## Falsification test

`tests/test_dng_compatibility_projection_v0_2.cpp` deliberately supplies a D65-like source-resolved white while the finalized compatibility matrix is already D50-bound. It requires:

- output `AsShotWhiteXY` to be D50 rather than the source white;
- `ColorMatrix1` to be the inverse of the finalized `cameraToXyzD50` matrix within the writer's SRATIONAL quantization;
- Scientific Master digest match;
- derived CFA to remain non-evidence;
- invalid source-white provenance to fail before any bytes are emitted.

## Validation state

The first v0.2 GitHub Actions run (`34664940316`) and its rerun failed before runner startup: every job reported no executed steps and no logs. A later exact-head run (`34665104802`) showed the same infrastructure/startup condition for GCC, Clang, ASan/UBSan, and Android arm64 jobs. These are **not compiler/test failures and not validation passes**.

Until a workflow run actually executes configure/build/test/assemble steps, this module remains a research candidate and no APK from v0.2 is released as validated.

## Adobe DNG notice

This product includes DNG technology under license by Adobe.

The implementation uses published DNG format semantics; no Adobe SDK source code is copied into this module.

## Canonical rule

**Measured where measured. Reconstructed where necessary. Never invented.**
