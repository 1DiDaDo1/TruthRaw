# TruthRaw camera-RGB covariance v0.6 — research report

## Result

The local contract implementation passes the covariance unit test and the v0.3 adapter unit test under strict `-Wall -Wextra -Wpedantic -Werror` builds. The covariance core was additionally exercised under GCC, Clang, and AddressSanitizer/UndefinedBehaviorSanitizer before repository staging.

## Why this layer exists

TruthRange v0.3 carries per-channel marginal p50/p95 bands and, for directly measured NoiseProfile Gaussian-equivalent entries, `sigmaEquivalent`. It explicitly leaves covariance unresolved. The v5.0g reconstructed-channel transport path carries p50/p95 numeric error proxies but intentionally stores no sigma-equivalent quantity for those transported channels.

The v0.6 contract prevents a common but scientifically invalid shortcut: treating unknown correlations as zero, or converting arbitrary empirical p95 bands into Gaussian variance.

## Implemented invariants

- Unknown numeric terms must be NaN when their known-mask bit is clear.
- Known marginal variance must be finite and nonnegative.
- Known covariance requires both associated variances.
- Every known covariance obeys `|cov_ij| <= sqrt(var_i var_j)`.
- A complete 3x3 matrix must be PSD; pairwise-valid correlations are not enough.
- Failed insertion of a final covariance term rolls back atomically.
- Linear scalar rescaling uses the square of the scale for both variance and covariance.
- Quantile-only v5.0g entries are rejected if a finite sigma accidentally appears.
- The adapter never creates any off-diagonal covariance from v0.3.

## Claim boundary

This is a covariance **representation and validation contract**, not evidence that the current TruthRaw pipeline has measured the full camera-RGB covariance. Off-diagonal physical/statistical estimation remains open. Exact propagation to XYZ(D50) should therefore be enabled only when a future source supplies a full PSD-certified camera-RGB covariance; otherwise downstream uncertainty must remain partial/bounded/unresolved.
