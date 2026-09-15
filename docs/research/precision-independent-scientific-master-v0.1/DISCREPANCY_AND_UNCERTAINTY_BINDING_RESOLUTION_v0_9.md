# TruthRaw Precision Discrepancy + Local-Uncertainty Binding Resolution v0.9

Date: 2026-09-15  
Status: **RESEARCH / FAIL-CLOSED BINDING STEP / NOT CANONICAL PROMOTION**

## 1. Resolution of the 3549/44/2580 versus 3545/0/0 discrepancy

The repository-authoritative v0.4 full-frame sweep is the compiled C++ sweep recorded in `TRUTHRAW_REAL_RECONSTRUCTION_PRECISION_CPP_SWEEP_v0_4.json` and `MIXED_PRECISION_PROMOTION_REPORT_v0_4.md`.

Its method is:

- core: 256;
- halo: 4;
- frozen-v4.7i precision-instrumented C++ reconstruction;
- explicit direction, green-clamp and color-clamp tracing.

That sweep reports:

- green direction divergence: **3549**;
- green clamp divergence: **44**;
- color clamp divergence: **2580**;
- measured-channel violations: **0**.

The later chat-local 3545/0/0 count was produced with a different locator/instrumentation path and an under-specified smaller halo. It is therefore not equivalent to the repository C++ gate and must not replace it. The current v0.8 runtime itself enforces `config.halo >= 4`; a smaller halo is rejected.

Decision: **3549 / 44 / 2580 remains the valid repository research result for v0.4.** The 3545/0/0 result is retained only as a superseded local diagnostic, not as project evidence.

## 2. What exists for uncertainty

`canonical/uncertainty/v5.0g/UNCERTAINTY_MODEL_v5_0g.json` defines a local reconstructed-error model whose outputs are Stage-2 normalized p50/p95 absolute-error bands. These are quantile semantics, not Gaussian sigma and not covariance.

`canonical/uncertainty/v5.0g-p1` preserves the prospective tele holdout/model binding and validated quantile authority for its exact source/backend scope.

The v5.0g manifest lists `uncertainty_runtime_v5_0g.cpp` and `.h`, but those runtime files are **not present in the current repository tree on this branch**. Therefore the mixed-precision layer cannot honestly claim that a live per-output canonical uncertainty runtime is currently linkable.

This is a repository-consistency blocker, not permission to reconstruct or infer the missing runtime.

## 3. v0.9 rule

The mixed-precision runtime may accept local uncertainty only from an external provider that supplies:

1. p50/p95 values already evaluated for the exact Scientific-Master output coordinate/channel;
2. the exact canonical uncertainty-binding SHA-256;
3. the exact feature-schema SHA-256;
4. censoring state;
5. explicit confirmation that the uncertainty field is in the same Stage-2 normalized output quantity being stored.

If any item is absent or mismatched, the sample remains unresolved.

No code in this precision module may:

- derive sigma from p50/p95;
- derive covariance from marginal quantiles;
- fill unknown covariance with zero;
- apply population holdout p50/p95 as if they were per-pixel local fields;
- upgrade evidence or reconstruction authority.

## 4. Current architecture

`exact integer/packed CFA evidence`
-> `F64 Stage-2 / branch-sensitive reconstruction`
-> `F64 Scientific-Master value`
-> `F32 storage candidate`
-> `v0.8 audit callback`
-> **`v0.9 externally supplied local quantile binding`**
-> `v0.7 quantile-relative storage assessment`

The v0.9 binding step does not generate uncertainty; it only validates identity/coordinate semantics and evaluates storage error relative to admitted local p50/p95 fields.

## 5. Gate state

- F32 branch-sensitive compute: **REJECTED as scientific reference for tested v4.7i topology**.
- F64 branch-sensitive compute: **REQUIRED reference**.
- F64 -> F32 storage absolute gate: **PROVISIONAL PASS for tested 4080x3072 scope**.
- Local uncertainty-relative storage gate: **OPEN** until a real per-output uncertainty field is produced and bound.
- 16320x12288 FotoGraaf/200MP precision gate: **OPEN** until a physically delivered, source-bound payload exists.

## 6. Next required evidence

Either:

- restore/reproduce the missing canonical v5.0g runtime from its original validated source and prove parity to the frozen runtime vectors, **without changing its model authority**, or
- provide another explicitly admitted per-output local uncertainty provider with equivalent provenance and coordinate binding.

Only after that provider exists may v0.9 convert the currently open local uncertainty-relative storage gate into a measured result.
