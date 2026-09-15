# TruthRaw v5.0g-p1 Native Uncertainty Runtime v0.9

Date: 2026-09-15  
Status: **RESEARCH IMPLEMENTED — OUTPUT-CHANNEL BINDING STILL OPEN**  
Authority: uncertainty/numerical research only; no evidence upgrade and no canonical promotion

## Purpose

The mixed-precision path already exposes, tile by tile:

- the Float64 reconstructed Scientific-Master candidate before storage;
- the Float32 stored candidate after quantization;
- the Float64 reconstruction trace.

v0.7 can compare storage error with a local uncertainty anchor, but the v0.8 runtime intentionally does not manufacture uncertainty. The missing step was a native evaluator for the canonical tele uncertainty model itself.

## Canonical model identity

This implementation is bound to:

- uncertainty binding SHA-256: `61c99b0e29316730ca1323fd3e91fd069ebbc50cba73127cd81b9803b10178a0`
- feature schema SHA-256: `8c8e56b762b83a5a846c5201ab3ba43c9a2549d896873cc0ceb66574e74a83e3`
- model family: `log-error ridge + role/SNR quantile calibration`
- feature count: `18`
- output units: Stage-2 normalized scene-linear absolute-error bands.

The native evaluator reproduces the canonical formula:

`mu = max(exp(intercept + dot(coefficients, X)) - 2e-5, 1e-6)`

followed by role/SNR-bin calibration:

`p50 = mu * q50_factor(role, snr_bin)`

`p95 = max(mu * q95_factor(role, snr_bin), p50)`

## New implementation

Added:

- `native/v5g_p1_uncertainty_runtime_v0_9.h`
- `native/v5g_p1_uncertainty_runtime_v0_9.cpp`
- `native/test_v5g_p1_uncertainty_runtime_v0_9.cpp`

The implementation fails closed when:

- model binding SHA does not match;
- feature schema SHA does not match;
- a feature is non-finite;
- predicted SNR is invalid;
- the sample is censored;
- exponential evaluation overflows;
- resulting quantiles are invalid.

## Semantics retained

The output is deliberately named `p50` / `p95` absolute-error quantiles.

It is **not**:

- Gaussian sigma;
- variance;
- covariance;
- a probability of correctness;
- a joint RGB confidence ellipse;
- additional photographic evidence.

This preserves the existing v0.7 rule that quantile-only uncertainty may not be silently promoted into Gaussian covariance authority.

## What this closes

v0.9 closes one implementation gap:

> given an already-valid canonical 18-feature vector, a target role, predicted SNR and exact model identity, TruthRaw can now evaluate the canonical local p50/p95 model natively and deterministically.

## What remains open

This does **not** yet close the Float64 -> Float32 Scientific-Master storage gate.

The canonical v5.0g-p1 model estimates reconstruction error for its calibrated target/reconstruction scope. Before a p50/p95 result can be used against a stored RGB output sample, TruthRaw must prove that the uncertainty quantity is expressed for the **same output coordinate, channel and reconstruction quantity**.

Therefore the next binding step must provide, for every auditable reconstructed output:

1. the exact canonical feature vector used for that output/target;
2. the target role and predicted SNR used for calibration-bin selection;
3. censor/support state;
4. an explicit mapping from the uncertainty target to the corresponding Scientific-Master RGB sample;
5. unresolved status where that mapping is not valid;
6. separate handling of measured CFA channels and reconstructed channels;
7. preservation of unknown covariance/off-diagonals as unknown.

Only then may v0.8 feed a local p50/p95 anchor into the v0.7 storage comparison.

## Free Scientific Space boundary

The Scientific Master remains precision-independent and is not limited to the source RAW container, RAW bit depth or Float32 storage. Float32 storage is only an implementation candidate. Its acceptability is determined by quantified error relative to admitted local uncertainty, not by the source format.
