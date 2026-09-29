# D.RAW Universal Observation Model Selection v0.1

Date: 2026-09-29

Status: research-only, prospective successor stacked above Anchor-Constrained Local Reconstruction v0.1.

## Why this exists

The real-device PR #90 hold-out experiment established two useful facts at once:

1. the affine local solver can cover more held-out CFA anchors than the existing center-excluded baseline;
2. on the directly comparable hold-outs it is not globally superior, and its diagnostic uncertainty is too optimistic.

Therefore the next scientific question is no longer:

> Can one stronger universal local predictor replace the baseline?

It is:

> Can D.RAW deterministically choose which local model families are worth testing from the geometry and support of the current sealed observation, without using the hidden target value, lens identity, camera identity or vendor-specific calibration?

This v0.1 implements the **selection policy / candidate-bank layer only**. It does not declare a winning predictor and it does not apply any reconstruction.

## Architectural law clarified here

The original source resolution is not merely a grid for noise analysis.

It remains the exact full-resolution measurement support for:

- source CFA values;
- CFA phase and sample geometry;
- local detail and structure support;
- censor/censor-boundary geometry;
- source-bound authority;
- hold-out validation;
- provenance back to the sealed observation.

At the same time:

> The source raster determines where D.RAW measured. It does not determine the raster on which D.RAW must think.

The existing 20-bit fixed-point Raster-Independent Sample Lattice remains the scientific coordinate world. No second dense upscaled image is created.

Measured source samples remain exact anchors:

`u = x * 2^20`
`v = y * 2^20`

Every unanchored coordinate begins as `UNKNOWN`.

A later reconstruction may assign a value only as explicitly `RECONSTRUCTED` with provenance and uncertainty.

## Universal input law

The model policy is intentionally source-agnostic.

It does **not** require:

- camera model;
- physical camera ID;
- lens role;
- focal-length class;
- vendor profile;
- per-lens noise calibration;
- device lookup table.

Metadata may remain available as provenance/hint elsewhere in D.RAW, but it is not a selector input here.

The selector uses only properties derived from the current sealed observation:

- exact measured source-anchor geometry;
- Raster-Independent Sample Lattice coordinates;
- exact sampled Structure support;
- exact sampled Censored support;
- exact sampled CensorBoundary support;
- frontside candidate geometry already bound to the same source.

## No target leakage

The selector is prospective.

It never reads:

- the held-out target value;
- solver absolute error;
- baseline absolute error;
- solver-vs-baseline winner counts;
- MAE/RMSE;
- hold-out z score.

The existing 2026-09-29 tele hold-out was already observed before this policy was designed. It is therefore development evidence only for this successor and cannot be reused as independent validation.

A new independent capture is required before any conclusion about selector quality.

## Runtime ordering and machine-readable freeze

The Universal Intake computes this policy **before** invoking
`AnchorConstrainedLocalReconstructionAudit.analyze`.

Therefore the runtime order is:

```text
sealed observation
 -> exact source/lattice/support geometry
 -> Universal Observation Model Selection v0.1
 -> freeze/export prospective selector sidecar
 -> anchor hold-out prediction
 -> reveal measured target
 -> score hold-out result
```

The APK exposes:

`Export Universal Observation Model Selection v0.1 · JSON`

That sidecar contains the local evidence regime and eligible model bank, but no
held-out target value, target variance, solver/baseline residual, MAE/RMSE,
winner count or z score.

For a future validation capture, preserve/export this selector sidecar before
interpreting the hold-out outcome. The separately exported hold-out sidecar may
only be joined to it afterwards by source/query identity.

## Current deterministic local regimes

The policy classifies each existing lattice/support query into one of:

- `CENSOR_CONSTRAINED`
- `STRUCTURE_NEAR`
- `STRUCTURE_SPARSE_LOCAL`
- `STRUCTURE_MIXED_OR_SUPPORTED`
- `SUPPORT_UNKNOWN`

These labels do not claim scene semantics. They only describe admitted support geometry.

The policy then exposes an **eligible model bank**, for example:

- center-excluded multiscale reference;
- directional-line research candidate;
- robust local constant candidate;
- existing affine-plane research candidate;
- future low-order curvature research candidate;
- no-reconstruction candidate.

No single winner is predeclared.

The existing affine plane is explicitly **not promoted**.

## Free-lattice meaning

The fixed-point lattice is an internal solution domain, not an upscaler.

A finer coordinate world can represent:

- exact sub-source-pixel distances;
- exact relations between measured anchors and support boundaries;
- future reconstructed coordinates;
- output grids of arbitrary finite size.

It does not create:

- new photons;
- new measured samples;
- extra optical resolution;
- a denser measurement history.

The key rule is:

> A new raster may refine the measurement space without replacing the measurement history.

## Safety invariants

Hard state:

- source bytes immutable;
- measured anchors immutable;
- source-anchor coordinates immutable;
- unanchored positions remain UNKNOWN unless explicitly reconstructed;
- no unanchored value becomes MEASURED;
- held-out target not used for selection;
- hold-out error not used for selection;
- no lens calibration required;
- no camera-model dependency required;
- no vendor mapping required;
- no AI/ML/neural/generative runtime;
- no candidate applied;
- no Scientific Master writeback;
- no D.RAWnegative writeback;
- no new evidence created.

## Next scientific gate

The next native audit should evaluate the eligible candidate bank per held-out CFA anchor while preserving the same source/lattice authority laws.

That audit should:

1. freeze model-selection features before revealing the target;
2. evaluate each eligible deterministic model;
3. record model validity, estimate, residual and uncertainty;
4. compare the preselected model against the baseline and against an oracle-only best-model diagnostic;
5. calibrate uncertainty by CFA phase and support regime;
6. keep every result audit-only;
7. use a **new independent real-device capture** for validation.

The purpose is not to force a reconstruction everywhere.

`UNKNOWN / NO_RECONSTRUCTION` remains a valid scientific result.
