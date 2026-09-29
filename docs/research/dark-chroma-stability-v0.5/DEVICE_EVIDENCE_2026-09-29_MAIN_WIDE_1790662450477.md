# Dark Chroma Stability v0.5 — device evidence — main/wide 1790662450477

Date: 2026-09-29

Status: real-device evidence for PR #87 / Dark Chroma Stability v0.5.

This record is based on the user-supplied device package `DRAWwidemainchroma5.zip`.

Package SHA-256:

`d72028d3add4a429508e0b438de8e1cf39cab439d10ee59c0f1b4e839e2fe6da`

## Observation identity

Selected DNG:

`DRAW_CAPTURE_1790662450477_wide_main_4096x3072.dng`

Visible intake metadata:

- route: wide/main;
- raster: 4096×3072;
- focal length: 6.55 mm;
- ISO: 4091;
- CFA: [2,1,1,0];
- white level: 1023;
- black level: approximately 64;
- frontside inspection: 384×287;
- edge density: 0.0744;
- entropy: 4.13;
- natural geometry candidate: true.

Exact same-observation N2 lineage from the supplied JSON:

- source SHA-256:
  `7a580822af061f290f92dc42082141023d64b6b2596c8c8cc64a591f19702a0e`
- Scientific Master SHA-256:
  `693bba9ef1097fef23db516d152b630eb743452043a99c0226613fa34fd52575`
- authority field SHA-256:
  `1c554a4e40bcae8a96064550398437c91b78606ed6ca0303643567b1ce531cad`
- TruthNegative state SHA-256:
  `8b2e715c508d77ec15dadf4f1455fc46166123a59be28f662a623ccbdb9d5ebc`
- N2 candidate SHA-256:
  `bdd4b6d70b575ff00484162217ce23a9c1d898510882d9663521cfbeb36cec33`
- N2 audit SHA-256:
  `0d76a77c1bca85f13f504733c8c2f44a16e01573510b5304bf276c26b609fe0f`
- N2 spatial SHA-256:
  `af9c5e6f0da4a7d433acd35af8b4212aeeada0db285502b3ccd1910bae224ec3`
- center-excluded audit SHA-256:
  `d72c0439c3b23782f67bbd6b7410148dc519b545442bb17b21505676838bb67c`
- confidence field SHA-256:
  `450a4e50e3c31a3f17bc06cc8dc4222ae1eb8d9216ef12f44c5e3ec8baf3a211`
- factored state SHA-256:
  `b167b6185831517724499fb1b27ea71ba1ec2f30e9595c567f409da93b9c38bb`

Supplied JSON file SHA-256 values:

- N2 spatial audit v0.1:
  `1b30e62ff8c3d328750a99ccdb5794041e03058ff845dc46e1abb37307bab1dd`
- center-excluded spatial audit v0.2.1:
  `5fa39522025988791f02059e64c5e727a235166741b9c93208d7cc8ec3e8bcf6`
- confidence field v0.3:
  `32f087e56d9cf345460ce3899208c659e18803cae7c6cf0a7f70477f075c564b`
- factored confidence state v0.3.1:
  `41117dd327152c3746af99d8d41bb0cb1b367391b7eda70899992798a0132085`

## Dark Chroma frontside/backside result

The device UI reports:

### v0.1

- frontside tiles: 432;
- dark tiles: 118;
- flat-dark tiles: 73;
- visible chroma-instability candidates: 15;
- frontside structure-veto tiles: 77.

### measured backside signal support

- state: `MEASURED_SIGNAL_PRESENT_OR_MIXED`;
- sampled source points: 12,288;
- p50 normalized above black: 0.04171;
- p90: 0.10008;
- p99: 0.16384;
- fraction <= 0.01: 0.100.

### v0.2

- global state: `FRONTSIDE_INFORMATION_PRESENT`;
- visible instability: 15;
- dark-uninformative: 6;
- backside-pending: 15;
- correction-supported: 0.

### v0.3

- global state: `FRONTSIDE_INFORMATION_PRESENT`;
- frontside degenerate: false;
- backside near-black: false;
- visible candidates: 15;
- dark-uninformative: 6;
- backside-pending: 15;
- correction-supported: 0;
- dark fraction: 0.273;
- candidate fraction: 0.035;
- structure fraction: 0.178;
- edge density: 0.0744.

This is therefore a selective, non-degenerate main/wide case rather than the previously validated globally dark-uninformative tele case.

## v0.4 same-observation local N2 binding

Device UI:

- frontside-bound: 432/432;
- visible-bound: 15/15;
- structure-blocked: 15/15;
- censor-blocked: 0;
- all-predictable: 0;
- center-outlier-free: 0;
- pair-free: 0;
- scale-free: 0;
- strict-vector: 0;
- correction-supported: 0.

The supplied N2 Factored Confidence v0.3.1 JSON independently reports:

- tile count: 3072;
- structure-protection-present tiles: 3072/3072;
- censor-protection-present tiles: 28;
- censor-boundary-protection-present tiles: 30;
- all-candidates-predictable tiles: 70;
- center-outlier-free tiles: 12;
- predictable-and-center-outlier-free tiles: 1;
- pair-rejection-free tiles: 0;
- scale-rejection-free tiles: 0;
- max-predictor-variance <= center-variance tiles: 3064.

The N2 spatial audit reports:

- sampled: 786,432;
- candidate-corrected in the private audit candidate: 647,499;
- preserved: 138,933;
- structure-protected: 133,491;
- censored-protected: 1,558;
- censor-boundary-protected: 252;
- no-neighborhood-protected: 3,631;
- residual-outlier-protected: 1;
- measured global structure-protected sample fraction:
  `133491 / 786432 = 0.16974258422851562`.

Every historical 64×64 N2 tile contains at least one structure-protected sampled point. Per-tile structure-protection fractions range from 0.01171875 to 0.72265625, with median 0.15625.

## v0.5 fine 32×32 structure-support result

The package does **not** contain the new N2 Structure Support v0.1 JSON sidecar, so the fine-field values below are grounded in the real-device UI screenshot rather than an exported machine-readable sidecar.

Relevant screenshot:

`Screenshot_20260929_081537_com_truthraw_adaptiveui_MainActivity.jpg`

SHA-256:

`f8384de01e04f6b2663466631f2c8562c59ad5e1a441a58d8656f103aee2ed83`

Displayed N2 Structure Support v0.1 result:

- fine source grid: 32×32;
- visible candidates: 15;
- fine-bound candidates: 15;
- visible-candidate overlap structure fraction: 0.1677;
- visible-candidate interior structure fraction: 0.1678;
- counted fine tiles structure/free: 558 / 0;
- maximum fine-tile structure fraction: 0.3750.

Displayed Dark Chroma Stability v0.5 result:

- visible candidates: 15;
- fine-bound: 15;
- legacy v0.4 blocked: 15;
- zero-interior-structure candidates: 0;
- overlap/interior structure fraction: 0.1677 / 0.1678;
- correction-supported: 0.

## Interpretation

The v0.5 refinement is device-validated operationally:

- the new 32×32 fine field runs on a selective main/wide observation;
- all 15 visible candidates bind;
- the source remains one observation;
- the v0.4 safety boundary is preserved;
- correction-supported remains zero;
- no Scientific-Master or D.RAWnegative writeback occurs.

More importantly, the finer field gives a clear scientific result:

**halving reporting tile edge from 64 to 32 does not separate these candidate regions from structure protection.**

The candidate overlap/interior structure fractions (~0.1677 / ~0.1678) are very close to the whole-observation N2 structure-protected sample fraction (~0.16974). In the displayed v0.5 aggregation, every counted overlapping 32×32 fine tile has at least one structure-protected sampled point (558 structure / 0 free), and none of the 15 candidate regions has a zero-structure fully-contained interior.

Therefore the problem is no longer merely "64×64 tiles are too large". The any-presence boolean is intrinsically too coarse for this N2 structure field because structure-protected samples are spatially distributed throughout the image.

## Consequence for the next gate

Do **not** reduce the existing protection and do **not** enable private chroma A/B/Delta from v0.5.

The next scientifically justified refinement is an audit-only **sample-level support-distance geometry** successor that records/uses the exact coordinates of sampled structure-protected points and measures, for each candidate:

- nearest protected-sample distance;
- protected-sample density at explicit radii;
- candidate-center versus boundary support;
- censor/boundary distance separately;
- CFA/sample-grid provenance;
- no interpolation of unsampled pixels;
- no conversion to a scalar probability;
- no promotion/correction permission.

Only device evidence from that distance/support audit can determine whether a later promotion rule is justified.

## Safety result

Still preserved:

- `DARK_UNINFORMATIVE` precedence;
- no hidden-colour reconstruction;
- `chroma_correction_supported=false`;
- `private_ab_delta_allowed=false`;
- `candidate_applied=false`;
- `creates_new_evidence=false`;
- `scientific_writeback_allowed=false`;
- no AI/ML/neural/generative runtime.
