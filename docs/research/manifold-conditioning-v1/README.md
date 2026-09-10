# TruthRaw Manifold Conditioning v1

Research module that turns the historical **Best Observation / Virtual EV** idea into a fail-closed, evidence-neutral numerical-conditioning contract.

## Meaning of “Best Observation”

It means **best conditioning gauge**, not best evidence and not a second exposure. One deterministic power-of-two gauge is selected for a sample/tile so the same scene estimate and its uncertainty are represented in a numerically convenient range. The result is always deconditioned before it can re-enter the scientific Scene Master coordinate system.

For `s = 2^e`:

- `mu' = s mu`
- `sigma' = |s| sigma`
- `variance' = s^2 variance`
- `|mu'|/sigma' = |mu|/sigma`
- standardized residuals are invariant apart from bounded floating-point rounding.

No virtual-EV multiplicity is fused as independent evidence. `physicalFrameCount` and `independentEvidenceCount` remain one upstream.

## Zero-line rule

The global TruthRange zero-line / `L0` is not rewritten. A conditioning EV is a temporary local computational gauge. TruthRange interval bounds may be shifted for computation, including infinite censor bounds, but the shift is reversed before scientific output.

## Model-selection rule

Any algorithm that uses the conditioning gauge to produce a genuinely different reconstructed value is no longer `EXACT_REPARAMETERIZATION`. Such a candidate is `CANDIDATE_REJECTED` unless parameters were frozen before independent held-out evidence and both scalar-error and topology gates pass. Otherwise the canonical v4.7i baseline is returned unchanged.

## Scope boundary

This v1 does not claim physical ISO simulation, extra photons, extra SNR, PTC/electron calibration, or denoising from EV multiplicity. Variance-stabilizing transforms for Poisson–Gaussian RAW noise remain a separate research candidate because denoising between nonlinear forward/inverse transforms can change the estimator and requires independent validation.

## Spatial interaction rule

A per-sample Best Conditioning Gauge is valid only for operations that are local to that sample. Any neighborhood, interpolation, gradient, covariance, topology, or regularization operation must place all interacting operands in one **common conditioning gauge** first. v1 provides a common-gauge selector for an interaction domain and uses its maximum finite magnitude to avoid overflow. A mixed-gauge neighborhood is an invalid scientific comparison space.
