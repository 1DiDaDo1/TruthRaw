# TruthNegative current research architecture — 2026-09-19

Status: **CURRENT TRUTHNEGATIVE RESEARCH-BRANCH ARCHITECTURE / NOT MAIN PROMOTION**

## 1. Branch isolation

TruthNegative development lives on:

`research/truthnegative-v0-1-scientific-negative-foundation`

It branches from the Android-17/HONOR research state after v0.54 was established.

The parallel HONOR route remains on its own integration/research branches. TruthNegative work must not replace, rewrite or close that route.

## 2. Scientific insertion point

Proposed flow:

`Source Evidence`
`-> Measurement / de-ISP`
`-> TruthNegative`
`-> Scientific Master`
`-> Dynamic Authority + uncertainty/support`
`-> Open Scene State`
`-> Appearance / HDR / transport`

TruthNegative is a **scientific reconstruction intermediate**, not Source Evidence.

## 3. Three distinct coordinate domains

TruthNegative v0.1 formally separates:

### Source Evidence Grid

The admitted CFA/sample lattice and exact source identity.

Example Camera-5 source domain:

`4080 x 3072`

### Reconstruction Domain

A continuous or latent camera/scene field in which the best-supported signal estimate is represented.

This domain may exceed the source lattice.

### Projection Grid

A finite requested sampling of the reconstructed field.

Example research projection:

`16320 x 12288`

The projection grid does not imply native sensor geometry.

## 4. v0.1 fail-closed rule

Until a source-to-reconstruction sampling model is explicitly admitted:

- source anchors remain measured in source coordinates;
- target-lattice measured sample count must remain zero;
- denser target samples are reconstruction capacity only;
- `createsNewEvidence=false`;
- `physicalFrameCount=1`;
- `independentEvidenceCount=1`.

This deliberately prevents a 4x-per-axis projection from being misread as “every fourth target pixel was physically measured.”

## 5. Initial software foundation

v0.1 adds:

- `tools/truthnegative_foundation_v01.py`
- `tests/test_truthnegative_foundation_v01.py`
- a machine-readable state manifest;
- CI that fails on evidence/authority inflation.

The v0.1 tool does **not** reconstruct image pixels yet.

It establishes the scientific contract that later reconstruction code must satisfy.

## 6. Planned implementation ladder

### TN-0 — Foundation

Authority, identity, evidence count, domain separation, target projection semantics.

### TN-1 — Source binding

Bind exact source bytes/CFA digest, dimensions, CFA topology, black/white/censor state and capture provenance.

### TN-2 — Baseline reconstruction

Implement a deterministic non-generative baseline reconstruction with exact measured-anchor preservation and explicit reconstructed authority.

### TN-3 — Continuous field

Move from “upsampled raster” thinking to a continuous/latent camera-scene field sampled at arbitrary finite output grids.

### TN-4 — Uncertainty/support

Bind per-location support, uncertainty/covariance and censor/unknown status.

### TN-5 — Optics-aware model

Admit PSF/MTF/CA/shading only after independent evidence. Use deconvolution/restoration without upgrading reconstructed detail to measured.

### TN-6 — Dense 200MP projection experiment

Sample the TruthNegative onto `16320x12288` and compare against source-consistency constraints, down-projection consistency, edge/texture conservation and uncertainty.

### TN-7 — Scientific Master integration

Only after falsification tests prove no authority leakage.

### TN-8 — Appearance/export

DNG/EXR/TIFF/HDR/preview remain downstream projections.

## 7. Required validation gates

Before any dense TruthNegative projection can be called scientifically useful:

- exact source re-projection/downsample consistency;
- no measured-anchor alteration;
- deterministic reconstruction under fixed inputs;
- authority map identity;
- uncertainty map identity;
- no hidden dependence on appearance/rendering;
- no generative semantic fill;
- numerical precision regression;
- held-out synthetic forward-model tests;
- real-source edge/noise/CFA residual tests.

## 8. 200MP language

Allowed:

- `16320x12288 TruthNegative projection`
- `200,540,160-sample reconstructed projection`
- `dense reconstructed negative`

Not allowed without new physical evidence:

- `measured 200MP RAW`
- `native 200MP CFA`
- `recovered 200MP sensor pixels`
- `untouched 200MP ADC`

## 9. Immediate next implementation

The next code after the foundation contract is TN-1:

1. exact source-binding manifest;
2. CFA topology binding;
3. source measurement coordinate object;
4. no-op identity reconstruction at source resolution;
5. exact source round-trip test;
6. only then a deterministic dense reconstruction baseline.
