# TruthRaw camera-RGB covariance v0.6

Status: **RESEARCH CANDIDATE until repository CI closes the gate.**

This module introduces an explicit camera-RGB covariance representation above the closed TruthRange v0.3 dense marginal uncertainty contract and the closed v0.5 real-latent bridge.

## Binding rules

1. Unknown marginal variance remains `NaN` with the corresponding known-mask bit clear.
2. Unknown RGB off-diagonal covariance remains `NaN`; it is never silently replaced by zero.
3. v0.3 `MeasuredNoiseProfileGaussianEquivalent` may provide `sigmaEquivalent`, and only that explicit Gaussian-equivalent sigma is squared into a marginal variance.
4. v5.0g p50/p95 error anchors and transported reconstructed-channel proxies remain quantile-only. They do **not** define variance and are never divided by a Gaussian quantile to manufacture sigma.
5. High-censored measured evidence does not receive a point variance.
6. A certified off-diagonal requires both corresponding marginal variances and must satisfy the Cauchy-Schwarz bound.
7. A fully numeric 3x3 covariance is accepted only when positive semidefinite (PSD) and explicitly marked PSD-certified.
8. Exact linear covariance propagation is permitted only for a fully known PSD-certified matrix.

## Scope boundary

This version defines the **representation and fail-closed adapter contract**. It does not claim that TruthRaw already knows the real co-sited R/G/B correlation field. Current v0.3/v0.5 evidence does not establish those off-diagonal terms. A later measurement/reconstruction study is required to populate them physically or statistically.

The next layer may propagate a fully certified covariance with `Sigma_out = J Sigma_in J^T`. When off-diagonals remain unresolved, the downstream layer must preserve that unresolved status or use explicitly labelled bounds; it must not assume independence.
