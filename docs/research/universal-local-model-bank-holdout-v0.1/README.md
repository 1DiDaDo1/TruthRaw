# D.RAW Universal Local Model Bank Holdout v0.1

Date: 2026-09-29

Status: research-only successor above Universal Observation Model Selection v0.1 / PR #91.

## Why this successor exists

PR #90 proved that a single affine local plane must not be promoted globally:
on the directly comparable real-device tele hold-outs the affine solver had
higher MAE/RMSE than the existing center-excluded reference, although it had
better coverage.

PR #91 then froze the architectural response:

- universal input stays source-agnostic;
- the source raster is full-resolution MEASURED support, not a noise-only grid;
- the source raster is not world-resolution authority;
- the existing 20-bit fixed-point sample lattice is the free scientific
  coordinate world;
- no lens-specific noise calibration, camera-model route or vendor map may
  determine model selection;
- the already-seen PR #90 tele result cannot serve as independent validation
  for a selector designed after seeing it.

This v0.1 implements the next native audit.

## Broader use of original resolution

This audit is deliberately not limited to Dark-Chroma candidate tiles.

It also does **not** require a Dark-Chroma candidate, N2 Dark-Chroma query, or the PR #91 query-policy to exist before the full-resolution audit can run. Those may be bound as extra context where available, but they are not admission gates for this path.

It selects a deterministic stratified subset of **real measured CFA anchors
across the original full-resolution source raster**.

The original raster therefore supplies exact authority for:

- CFA value;
- CFA phase/channel;
- full-resolution detail/structure relationships;
- source-coordinate geometry;
- censor/saturation paths;
- provenance back to the sealed observation;
- hold-out validation.

The raster does not become the resolution boundary of the reconstructed world.

Permanent law:

> The source raster determines where D.RAW measured. It does not determine the raster on which D.RAW must think.

## Hold-out design

Current deterministic audit constants:

- fixed-point lattice: 2^20 coordinate units/source pixel;
- hold-out period: 64;
- local support radius: ±8 source pixels;
- source processed in bounded 256×256 core tiles with support halo;
- held-out anchors remain immutable MEASURED source evidence;
- only other exact same-CFA-phase measured anchors enter a candidate model;
- censored anchors and support paths crossing censoring are excluded.

The model-bank audit itself does **not** require DNG NoiseProfile for model
selection.

This is important for universality: if source noise metadata is absent, the
new model bank still runs. The historical center-excluded baseline is reported
only where its own variance contract can be satisfied.

## Candidate bank

Target-blind candidates:

1. `ROBUST_MEDIAN_CONSTANT`
2. `DIRECTIONAL_LINE`
3. `AFFINE_PLANE`
4. `QUADRATIC_SURFACE`
5. `NO_RECONSTRUCTION`

The directional candidate evaluates deterministic H/V/diagonal orientations.

The affine and quadratic candidates operate in normalized local coordinates
whose anchors remain mapped into the existing raster-independent lattice.

`NO_RECONSTRUCTION` remains a valid scientific state; in this first audit it
is selected when no numeric model is admissible.

## Target-blind model selection

For each hold-out, every model is fit without reading the target.

The native audit now enforces a stricter boundary than mere data-flow separation:
the held-out Stage-2 value is not read from the workspace at all until the
research selector and the independent reference predictor are both frozen.

Model selection is frozen before the target is revealed.

Current research selection score:

`TARGET_BLIND_RESIDUAL_BIC_LIKE_COMPLEXITY_SCORE`

It combines model residual with an explicit complexity penalty. The score is a
research selector, not a probability and not a truth metric.

Hard prohibitions:

- target value cannot enter model fitting;
- target value cannot enter model selection;
- hold-out error cannot enter model selection;
- post-reveal oracle cannot enter model selection.

## Post-reveal oracle

After the selected model has been frozen, the real held-out CFA value is
revealed for scoring.

The audit then reports:

- selected-model absolute error;
- baseline absolute error where baseline is valid;
- post-reveal best model inside the candidate bank;
- selector regret = selected-model absolute error minus post-reveal oracle
  absolute error.

The oracle is diagnostic only. It is deliberately unavailable to the selector.

This allows a useful distinction:

- **model-bank capacity:** did one candidate contain a good local predictor?
- **selector quality:** did the pre-target selector choose it?
- **baseline comparison:** did the chosen model beat the established reference?

## Universal input boundary

Model selection uses no:

- lens identity;
- lens calibration profile;
- focal-length class;
- camera model;
- physical Camera2 ID;
- vendor map;
- AI/ML/neural/generative model.

The implementation is currently executable through the admitted native DNG
scientific route because that is the available decoder/Stage-2 bridge. This is
an implementation-readiness boundary, not an architectural camera/lens
dependency. Future format adapters can expose the same sealed-source contract.

## Authority and safety

Hard state remains:

- source bytes immutable;
- measured source anchors immutable;
- source-anchor positions immutable;
- unanchored lattice positions are not promoted to MEASURED;
- all private model outputs are reconstructed audit values only;
- candidate applied = false;
- model bank applied to Scientific Master = false;
- creates new evidence = false;
- Scientific Master writeback = false;
- D.RAWnegative writeback = false.

## Machine-readable APK export

New APK action:

`Export Universal Local Model Bank Holdout v0.1 · JSON`

The sidecar contains:

- source/scientific/authority/TruthNegative hashes;
- full-resolution source geometry;
- lattice mapping;
- safety flags;
- aggregate metrics;
- per-model validity/selection/error metrics;
- per-holdout selected model frozen before reveal;
- actual target only in the post-reveal section;
- post-reveal oracle and selector regret;
- `holdout_stream_sha256`;
- full JSON SHA after verified write.

## Scientific validation boundary

The 2026-09-29 PR #90 tele capture was observed before this model bank and
selector were designed.

Therefore it is development evidence only.

**A new independent real-device RAW is required for scientific evaluation of
this successor.**

No promotion rule is predeclared.

Even a successful hold-out result would establish predictive performance of
the observed CFA sample, not direct latent scene truth and not permission to
replace measured anchors.
