# TruthRaw XYZ(D50) uncertainty propagation v0.7

Status: **RESEARCH CANDIDATE until repository CI closes the gate.**

This layer implements roadmap item 151 above the closed camera-RGB covariance v0.6 contract. It propagates camera-RGB uncertainty through an explicitly supplied **linear camera-RGB -> XYZ(D50)** transform without inventing missing correlation or color calibration.

## Binding rules

1. The matrix is caller-supplied. v0.7 does not infer, interpolate, calibrate, or choose a camera color matrix.
2. Matrix provenance must be explicit (`sourceId`, authority, scene-linear declaration, XYZ(D50) declaration).
3. The input covariance must first satisfy the v0.6 fail-closed validator.
4. A fully known PSD-certified camera-RGB covariance may be propagated exactly:
   `Sigma_XYZ = M * Sigma_cameraRGB * M^T`.
5. Exact output is marked full/PSD-certified only after a second numeric PSD check.
6. Unknown input off-diagonals are never replaced by zero and never described as independence.
7. If all variances relevant to an XYZ component are known but one or more needed input pair covariances are unknown, v0.7 emits a conservative **outer variance bound**. For each unknown pair, Cauchy-Schwarz supplies `|Cov_ij| <= sqrt(Var_i Var_j)`. Unknown pair contributions are interval-expanded independently.
8. Those component bounds are intentionally not claimed to be the tight PSD-coupled feasible interval.
9. In non-full mode, XYZ off-diagonal covariance remains `NaN`/unknown. v0.7 does not synthesize an XYZ covariance matrix from marginal bounds.
10. Quantile-only p50/p95 backend error bands do not become variance here. If v0.6 has no valid variance, v0.7 remains unresolved.
11. This layer does not validate missing-channel topology. Roadmap item 152 remains separate.
12. This layer does not promote source-bound metadata color to independent physical color calibration. Per-lens/unit calibration and illuminant characterization remain later evidence gates.

## Matrix authority

`MatrixAuthorityV07` records only the caller's provenance class:

- `SourceBoundMetadataDerived`
- `IndependentTargetCalibration`
- `ExplicitResearchFixture`

The enum does **not** itself prove physical correctness. The `sourceId` must bind the supplied matrix to upstream evidence.

## Scientific boundary

The result is uncertainty propagation in a declared XYZ(D50) coordinate. It is not proof that the transform is a perfect colorimetric model of the scene or that the camera satisfies the Luther condition. Physical color promotion remains blocked until its own calibration evidence closes.

## Mathematical references

- ProbabilityCourse, *Random Vectors*: covariance of `Y = A X + b` is `C_Y = A C_X A^T`.
  https://www.probabilitycourse.com/chapter6/6_1_5_random_vectors.php
- Penn State STAT 505, *Linear Combinations of Random Variables*: `Var(c'X) = c' Sigma c`.
  https://online.stat.psu.edu/stat505/Lesson02
