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
component is compared with the corresponding 1:1 TruthNegative ScientificView
component.

The route fails closed if the reconstruction baseline differs beyond the
configured floating-point tolerance. This prevents an unrelated tile-boundary
or reconstruction-path difference from being misrepresented as an N2 effect.

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
- baseline must match Scientific Master / TruthNegative ScientificView
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
