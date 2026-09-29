# D.RAW Universal Local Model Bank Holdout v0.2

Date: 2026-09-29

Status: research-only successor above PR #92.

## Why v0.2 exists

PR #92 produced the first independent full-resolution real-device result for
the universal local model bank on a new tele capture used only as test
provenance.

The v0.1 selected bank beat the established center-excluded reference on that
capture:

- 11,844 real CFA hold-outs;
- 11,667 directly comparable to the reference;
- selected MAE ≈ 0.02082264 vs reference ≈ 0.02143966;
- selected RMSE ≈ 0.02706725 vs reference ≈ 0.02784103;
- selected/reference wins = 6,227 / 5,440;
- no promotion or writeback was enabled.

The same result exposed two important limitations:

1. the selector still left material oracle regret;
2. DIRECTIONAL_LINE and AFFINE_PLANE produced exactly the same center
   estimate on all 11,844 hold-outs because both were fit on the same symmetric
   80-sample support.

v0.2 is a versioned response to those findings. PR #92 remains unchanged.

## Architectural laws retained

The source raster remains the exact full-resolution MEASURED support for CFA
values, phase, detail, structure, geometry, censoring and provenance.

It is not world-resolution authority.

The existing 20-bit Raster-Independent Sample Lattice remains the scientific
coordinate domain. Unanchored positions do not become MEASURED.

No lens identity, camera model, physical Camera2 ID, focal-length class, vendor
map or per-lens noise calibration participates in model selection.

The prior tele capture is test provenance only. It does not make the algorithm
tele-specific.

## New selector: support cross-fit

v0.1 selected candidates using their in-sample residual plus a complexity
penalty.

v0.2 replaces that selector with:

TARGET_BLIND_SUPPORT_CROSSFIT_PREDICTIVE_SCORE_V0_2

For each held-out CFA anchor, surrounding same-phase measured support is split
deterministically by offset geometry:

- validation when the offset-lattice index sum modulo 3 equals zero;
- all other surrounding admitted anchors form the fit partition.

The target itself does not participate in either partition.

Each model is fit on the fit partition and scored on the separate validation
partition. Only after that target-blind selection score is frozen may the same
candidate be refit on all surrounding admitted support to produce its final
target prediction.

The held-out target numeric Stage-2 value remains unread until both the research
selector and the historical reference predictor are frozen.

## Direction-conditioned support

The old DIRECTIONAL_LINE is replaced in the v0.2 bank by:

DIRECTIONAL_STRIP_LINE

The candidate tests H/V/two diagonal directions, but unlike v0.1 it admits
only same-phase samples lying inside a 2-source-pixel half-width strip around
that direction.

This means the directional predictor sees different observation support from
the affine plane and therefore can form a genuinely different center
prediction.

Direction choice is itself target-blind and based on validation inside the
candidate direction strip.

That strip-specific score chooses only H/V/diagonal orientation inside the
directional family. For the later competition between model families,
ROBUST_MEDIAN_CONSTANT, DIRECTIONAL_STRIP_LINE, AFFINE_PLANE and
QUADRATIC_SURFACE are all scored on the same common target-blind validation
anchors. This avoids giving the directional family a selection advantage merely
because its strip is an easier subset.

## Candidate bank

- ROBUST_MEDIAN_CONSTANT
- DIRECTIONAL_STRIP_LINE
- AFFINE_PLANE
- QUADRATIC_SURFACE
- NO_RECONSTRUCTION

NO_RECONSTRUCTION remains scientifically valid when no numeric candidate is
admissible.

## Scientific interpretation

Cross-fit prediction error is a model-selection diagnostic. It is not latent
scene truth, denoising proof or optical-resolution proof.

A lower real held-out CFA prediction error remains only evidence that the
candidate forecasts the noisy measured target better under the experiment.

No result from v0.2 may automatically alter a MEASURED anchor, Scientific
Master or D.RAWnegative.

## Safety contracts

- target censor state may only gate hold-out admission;
- numeric target Stage-2 value is unread before selector/reference freeze;
- target value not used by models;
- target value not used by selector;
- hold-out error not used by selector;
- post-reveal oracle not used by selector;
- selector uses support cross-fit;
- directional support is observation-conditioned;
- no lens calibration;
- no camera-model route;
- no vendor mapping;
- no AI/ML/neural/generative runtime;
- measured anchors immutable;
- unanchored values not promoted to MEASURED;
- model bank not applied to Scientific Master;
- candidate applied = false;
- creates new evidence = false;
- scientific writeback allowed = false.

## Validation boundary

The PR #92 tele result was already observed before v0.2 was designed and is
therefore development evidence only for v0.2.

A new independent sealed RAW/DNG is required before any scientific conclusion
about v0.2 selector quality.

The first v0.2 device protocol must freeze before the new result is inspected.
