# TruthNegative N2 1:1 Full-Colour A/B/Δ Crop Diagnostics v0.2

Status: **DISPLAY-ONLY FULL-COLOUR VALIDATION — NO SCIENTIFIC WRITEBACK**

This diagnostic supersedes the v0.1 1:1 crop display experiment for the
current Android PRO route. The v0.1 experiment modified only the measured CFA
component in a temporary display copy. v0.2 instead propagates the already
audited N2 CFA candidate through the same measured-preserving Float64
reconstruction backend used by the Scientific Master, while keeping the
scientific state immutable.

## Selection

A balanced 1/16 whole-frame N2 audit selects three source regions:

1. a quiet/noise-candidate region;
2. a structure-protected region;
3. a CENSORED/highlight region.

Each selected region is shown as a 192x192 source-native 1:1 crop.

## Core audit

Each 192x192 crop is audited at `samplingPeriod=2`, which covers every CFA
site in the bounded crop. The displayed per-crop N2 statistics therefore remain
the full-lattice core statistics:

- candidate / preserved;
- structure protection;
- CENSORED / censor-boundary protection;
- residual-outlier and neighborhood protection;
- removed residual energy;
- maximum proposed Stage-2 correction.

## Reconstruction halo audit

The full-colour candidate additionally uses the reconstruction backend's exact
required halo. The source-native Stage-2 tile is read with that halo, and the
entire halo tile is separately re-audited at `samplingPeriod=2`.

Only N2-admitted corrections are applied to a private Stage-2 copy. Protected
sites remain numerically unchanged.

The existing
`research_edge_aware_support_limited_measured_preserving_f64_v0_1`
backend then reconstructs:

- a baseline RGB crop from the unmodified Stage-2 tile;
- a candidate RGB crop from the private corrected Stage-2 copy.

The original Stage-2 source is not modified.

## Scientific-Master equivalence gate

Before B can be displayed, every baseline reconstructed camera-native RGB
component is compared with the corresponding **exact Scientific Master source
pixel** from the canonical Open Scene binding.

The earlier v0.2 prototype compared against a same-size TruthNegative
Continuous raster query. That was the wrong equivalence domain: the continuous
resolver intentionally performs area/bilinear footprint integration even when
target dimensions equal source dimensions. It is therefore not an identity
read of the Scientific Master sample.

The corrected gate compares the baseline Float32 value bit-for-bit with the
exact canonical Scientific Master Float32 value. Any mismatch fails closed.
This prevents continuous projection behavior, tile-boundary behavior or a
different reconstruction path from being misrepresented as an N2 effect.

## A / B / Delta

- **A**: unchanged TruthNegative / Deep Scene / Appearance observation.
- **B**: full-colour RGB candidate reconstructed from the private N2-corrected
  Stage-2 copy, then placed into a temporary authority-cleared display scene.
- **Delta**: `abs(B-A)` in encoded display RGB at fixed x32 diagnostic gain.

B receives no measured authority. Its uncertainty/visibility authority is
cleared and it receives a new candidate identity bound to the reconstruction
halo N2 grid and candidate RGB.

## Nonclaims

This does not mean:

- Direct CFA has been denoised;
- Scientific Master has been changed;
- TruthNegative has been changed;
- a second measured capture has been created;
- the candidate is approved for production export;
- a final optimal denoise strength has been established.

It is a controlled appearance-only validation of how the current N2 candidate
behaves after the same full-colour reconstruction used by the Scientific
Master.

## Permanent invariants

- `sourceStage2Modified=false`
- `createsNewEvidence=false`
- `scientificWritebackAllowed=false`
- baseline must be Float32-bit-identical to the exact Scientific Master source pixel
- A remains the primary scientific reference
- B and Delta remain transient diagnostic UI rasters
- no candidate is written into any export primary

The Android PRO button is:

`N2 · 1:1 Full-colour A/B/Δ`


## Android validation

Latest combined Android validation:

- branch: `integration/pro-truthnegative-continuous-primary-route-v075`
- code head: `a0780243b367e253de51c0053650a42b4004dc72`
- GitHub Actions run: `36197020024` — SUCCESS
- signed ARM64 APK bytes: `6638935`
- APK SHA-256: `57bd137ff295845fb7bd78c5ad181d9d9e871b35a7be6367325ad796d1fe0ae4`
- signing certificate SHA-256:
  `a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`
- artifact ID: `10890641200`

The separate N2 full-colour candidate host workflow is also green for GCC,
Clang and ASan/UBSan. The Android route remains fail-closed if the baseline
reconstruction diverges from the existing Scientific Master / TruthNegative
ScientificView.

The provenance parser intentionally bounds candidate Stage-2 site counts
against the admitted source size rather than a hard-coded halo width, so a
future reconstruction backend with a larger declared halo cannot be falsely
rejected by the Android UI while native geometry and digest checks remain
authoritative.


## Corrected exact-baseline Android validation

The corrected baseline-equivalence implementation is validated on Android:

- exact-baseline code head: `bde83be02d810a8f8073c328f010772b36e6bdf8`
- GitHub Actions run: `36198248245` — SUCCESS
- signed ARM64 APK bytes: `6638911`
- APK SHA-256: `fffe03a41f49d346ab3089950d095375356df22e75cde9221ab51eae073c0909`
- signing certificate SHA-256:
  `a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`
- artifact ID: `10890763024`

This build no longer treats a same-size TruthNegative Continuous raster query
as an identity Scientific Master sample. The baseline candidate reconstruction
is checked directly against the exact canonical Scientific Master source
pixel, Float32 bit-for-bit, before any B/Delta diagnostic output is admitted.


## Risk / Quality gate

The next diagnostic layer is implemented in
`docs/research/truthnegative-n2-risk-quality-audit-v0.1`.

It consumes the already-isolated A/B display-encoded crop together with the
full-lattice preserve-reason mask. It measures p50/p95/p99/max delta,
per-display-channel delta, display-luma/chroma separation, the exact maximum
delta source coordinate, distance to Structure and CENSORED/boundary
protection, and A/B display-luma gradient energy.

These metrics are diagnostic only. In particular, the gradient-energy
comparison is not an optical MTF measurement and does not authorize production
denoise.
