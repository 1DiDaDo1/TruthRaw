# TruthRaw XYZ(D50) uncertainty v0.7 — research report

## Goal

Roadmap item 151 asks for uncertainty propagation from camera-RGB into the XYZ(D50) colorimetric Scene Master layer. The repository had no separate closed camera->XYZ implementation module at the start of this work, so v0.7 deliberately accepts a matrix as an explicit provenance-bound input rather than introducing a second color-calibration pipeline.

## Implemented

- exact `M Sigma M^T` propagation for fully known PSD-certified v0.6 covariance;
- exact output marginal variances and XY/XZ/YZ covariances;
- output PSD revalidation;
- conservative component variance outer bounds when input off-diagonals are unresolved;
- partial-covariance support: already-certified pair terms are used exactly, only unresolved terms expand the interval;
- per-component exact variance when a matrix row depends only on fully known terms;
- unresolved preservation when required marginal variance is absent;
- dense tile propagation with geometry preservation;
- explicit matrix provenance/declaration contract;
- no p50/p95 -> sigma conversion;
- no independence assumption;
- no matrix calibration or illuminant inference.

## Bound semantics

For output component `y = a^T x`:

`Var(y) = sum_i a_i^2 Var_i + 2 sum_{i<j} a_i a_j Cov_ij`.

Known pair covariance contributes exactly. For an unresolved pair, v0.6 already guarantees the Cauchy-Schwarz outer interval:

`-sqrt(Var_i Var_j) <= Cov_ij <= +sqrt(Var_i Var_j)`.

v0.7 therefore adds an interval radius

`2 |a_i a_j| sqrt(Var_i Var_j)`

for each unresolved pair and clamps the lower variance bound to zero. Treating unresolved pair intervals independently can be wider than the true PSD-coupled feasible set, so the result is explicitly an **outer bound**, not a tight posterior covariance.

## Claim boundary

This work closes a numerical propagation contract only if repository CI passes. It does not establish the real camera-RGB correlation field, reconstructed-channel topology, physical per-unit color calibration, illuminant SPD/CCT, or spectral camera correctness. Those evidence gaps remain open.
