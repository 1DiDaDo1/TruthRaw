# TruthRaw Uncertainty-Relative Storage Gate v0.7

Date: 2026-09-15  
Status: **OPEN — LOCAL UNCERTAINTY BINDING REQUIRED**  
Authority: numerical/uncertainty research only; no evidence upgrade and no canonical promotion

## Purpose

v0.4 showed that branch-sensitive reconstruction must use Float64 as the scientific numerical reference, while post-compute Float64 -> Float32 storage introduced at most about `5.96e-8` normalized absolute error across eight tested 4080x3072 real sources.

That absolute result is not enough for a scientific storage promotion. TruthRaw now asks the stronger question:

> Is Float32 storage quantization negligible relative to the admitted **local uncertainty of the same Scientific-Master quantity**?

## Binding to existing TruthRaw uncertainty doctrine

This gate follows two existing project authorities:

- `canonical/uncertainty/v5.0g-p1`: reconstructed-channel uncertainty is validated with p50/p95 empirical error anchors for the exact tele source/backend scope. Those values are **quantile-only**.
- `docs/research/camera-rgb-covariance-v0.6`: unknown covariance remains `NaN`; unknown off-diagonals are never set to zero; a quantile band must never be converted into Gaussian variance.

Therefore v0.7 maintains two separate comparison semantics.

### Gaussian-equivalent componentwise comparison

When an upstream layer has an explicitly admitted Gaussian-equivalent marginal sigma, or a known marginal variance, storage may be evaluated as:

`abs(Float32(Float64_value) - Float64_value) / sigma_local`

Only that path may be called an error-over-sigma comparison.

A fully known RGB covariance is **not** required merely to compare one channel with its known marginal sigma. However, missing off-diagonal covariance stays missing. This componentwise check cannot be relabelled as a joint RGB/Mahalanobis covariance result.

### Quantile-relative comparison

When reconstruction has only an empirical p50 or p95 error anchor, storage may be evaluated as:

`abs(storage_error) / quantile_error_anchor`

The result remains quantile-relative. It is not sigma, variance, covariance, probability, or a Gaussian confidence interval.

## New native contract

Added:

- `native/uncertainty_relative_storage_v0_7.h`
- `native/uncertainty_relative_storage_v0_7.cpp`
- `native/test_uncertainty_relative_storage_v0_7.cpp`

The contract is fail-closed:

- unknown anchor -> unresolved comparison;
- unknown covariance diagonal -> unresolved comparison;
- negative/non-finite variance -> unresolved comparison;
- unknown off-diagonal covariance is never filled with zero;
- quantile anchors retain explicit p50/p95 semantics;
- zero uncertainty only passes when Float32 storage is exactly lossless for that value.

## Coarse prospective-tele scale check

The canonical first prospective tele uncertainty PASS reports:

- observed median reconstruction error: `0.008743` Stage-2;
- observed p95 reconstruction error: `0.036181` Stage-2.

Using the current eight-file worst Float64 -> Float32 storage error (`5.960448645758731e-8`) only as a **coarse scale comparison** gives:

- storage error / observed median error: `6.817395225619044e-6`;
- storage error / observed p95 error: `1.6473974311817614e-6`.

This strongly suggests that storage quantization is numerically tiny relative to the validated tele reconstruction-error scale, but it does **not** close the local storage gate. The p50/p95 numbers are population error anchors, not per-output-channel local sigma values.

## v0.6 runtime integration correction

During this step an integration defect was found: the compact-GainMap F64 streaming runtime v0.6 and its test existed in the repository but were not wired into `CMakeLists.txt`/CTest. The build graph has now been corrected so v0.6 must compile and pass before v0.7 can pass.

This matters because the uncertainty gate must sit on the actual current runtime path:

`exact integer CFA evidence`
-> `compact Float32 GainMap knots retained as stored evidence`
-> `Float64 GainMap interpolation / Stage-2`
-> `Float64 branch-sensitive reconstruction`
-> `Float32 storage candidate`
-> `uncertainty-relative storage assessment`

No appearance stage participates in this gate.

## Current decision

- Float32 branch-sensitive compute: **rejected as scientific numerical reference** for tested v4.7i topology.
- Float64 branch-sensitive compute: **required reference**.
- Float64 -> Float32 storage: **absolute numerical gate provisionally passed for tested 4080x3072 sources**.
- Float64 -> Float32 storage relative to local scientific uncertainty: **OPEN**.

## What is needed to close v0.7

For every output class we need uncertainty in the same coordinate/quantity as the stored Scientific Master:

1. where a real Gaussian-equivalent marginal sigma exists, evaluate storage error / sigma;
2. where only p50/p95 reconstructed-error anchors exist, keep a separate quantile-relative result;
3. unresolved uncertainty must remain unresolved and must not count as pass;
4. report coverage fractions separately for Gaussian-comparable, quantile-comparable and unresolved outputs;
5. retain zero measured-channel authority violations;
6. repeat this entire precision/uncertainty gate on the physically proven FotoGraaf `16320x12288 RAW_SENSOR` route before 200MP promotion.

## Scientific boundary

Higher numerical precision can reduce arithmetic error. Lower storage precision can save memory. Neither operation creates evidence, repairs missing color observations, measures unknown covariance, or upgrades provenance.

The Scientific Master remains a free scientific scene space; its storage encoding is an implementation choice that must remain subordinate to evidence and uncertainty authority.
