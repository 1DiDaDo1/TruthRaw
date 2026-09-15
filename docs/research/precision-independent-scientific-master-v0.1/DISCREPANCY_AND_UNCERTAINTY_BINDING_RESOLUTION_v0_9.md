# TruthRaw Precision Discrepancy + Local-Uncertainty Binding Resolution v0.9

Date: 2026-09-15  
Status: **RESEARCH / HOST-CI PASS / LOCAL-FEATURE BINDING STILL OPEN / NOT CANONICAL PROMOTION**

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

The later chat-local 3545/0/0 count used a different locator/instrumentation path with an under-specified smaller halo and without equivalent clamp tracing. It is not an equivalent measurement of the repository C++ gate. The current v0.8 runtime itself enforces `config.halo >= 4`; a smaller halo is rejected.

Decision: **3549 / 44 / 2580 remains the valid repository research result for v0.4.** The 3545/0/0 result is superseded local diagnostic output and must not be used as project evidence.

## 2. Canonical uncertainty authority and repository inconsistency

`canonical/uncertainty/v5.0g/UNCERTAINTY_MODEL_v5_0g.json` defines the frozen reconstructed-error model:

- 18 canonical features;
- log-error ridge prediction;
- role/SNR-bin calibration;
- Stage-2 normalized p50/p95 absolute-error bands.

These outputs are empirical error quantiles. They are **not** Gaussian sigma, variance, covariance, probability, or a joint RGB confidence region.

`canonical/uncertainty/v5.0g-p1` preserves the prospective tele holdout/model binding for its exact validated source/backend scope.

The v5.0g manifest lists `uncertainty_runtime_v5_0g.cpp` and `.h`, but those historical runtime files are not present in the current repository tree on this branch. That repository inconsistency remains documented.

## 3. v0.9 research runtime reproduction

A research-side native evaluator now exists in this precision module:

- `native/v5g_p1_uncertainty_runtime_v0_9.h`
- `native/v5g_p1_uncertainty_runtime_v0_9.cpp`
- `native/test_v5g_p1_uncertainty_runtime_v0_9.cpp`

It reproduces the frozen v5.0g/p1 model specification from the canonical JSON:

- exact uncertainty-binding SHA-256 check;
- exact feature-schema SHA-256 check;
- 18 frozen coefficients;
- frozen intercept and epsilon;
- frozen role/SNR calibration factors;
- p50/p95 quantile semantics preserved;
- censoring fails closed;
- non-finite or invalid inputs fail closed.

This is **not** a claim that the missing historical canonical runtime has been recovered byte-for-byte. It is a research reimplementation of the frozen model specification and does not alter canonical uncertainty authority.

## 4. v0.9 local binding contract

The mixed-precision layer now also contains:

- `native/local_uncertainty_binding_v0_9.h`
- `native/local_uncertainty_binding_v0_9.cpp`
- `native/test_local_uncertainty_binding_v0_9.cpp`

This contract accepts only local p50/p95 fields that are explicitly bound to the exact Scientific-Master scalar being stored. It requires:

1. exact canonical uncertainty-binding SHA-256;
2. exact feature-schema SHA-256;
3. p50/p95 already evaluated for the same output coordinate/channel;
4. censoring state;
5. explicit `sameScientificMasterQuantity=true`;
6. exact agreement that the supplied F32 value is the one-step cast of the audited F64 value.

If any requirement fails, that sample remains unresolved.

No v0.9 code may:

- derive sigma from p50/p95;
- derive covariance from marginal quantiles;
- fill unknown covariance with zero;
- treat population holdout p50/p95 as local per-pixel uncertainty;
- change measured/reconstructed/appearance authority.

## 5. Current architecture

`exact integer/packed CFA evidence`
-> `F64 Stage-2 / branch-sensitive reconstruction`
-> `F64 Scientific-Master value`
-> `F32 storage candidate`
-> `v0.8 pre/post-storage audit stream`
-> `v0.9 canonical-model quantile evaluator when canonical features exist`
-> `v0.9 same-quantity local quantile binding`
-> `v0.7 quantile-relative storage assessment`

The precision layer can now evaluate and consume local quantiles, but it still does **not** yet generate the full 18-feature field from each v4.7i reconstructed output inside the mixed-precision runtime.

## 6. Host CI result

Commit `5accdb38c19ed47f3635520e58ab73c77c0945bd` passed:

- documentation governance: **PASS**;
- precision host-reference workflow: **PASS**;
- C++ configure/build: **PASS**;
- all **15/15** C++ tests: **PASS**;
- arbitrary-precision Python regressions: **PASS**;
- v0.4 mixed-precision gate: **PASS**;
- v0.7 uncertainty-relative semantics gate: **PASS**.

## 7. Gate state

- F32 branch-sensitive compute: **REJECTED as scientific reference for tested v4.7i topology**.
- F64 branch-sensitive compute: **REQUIRED reference**.
- F64 -> F32 storage absolute gate: **PROVISIONAL PASS for tested 4080x3072 scope**.
- v5.0g/p1 model evaluator implementation: **HOST-CI PASS, RESEARCH ONLY**.
- local p50/p95 same-quantity binding contract: **HOST-CI PASS, RESEARCH ONLY**.
- end-to-end local uncertainty-relative storage gate: **OPEN** because the 18 canonical features are not yet emitted/bound for every Scientific-Master output sample by the mixed-precision runtime.
- 16320x12288 FotoGraaf/200MP precision gate: **OPEN** until a physically delivered, source-bound payload exists.

## 8. Next required implementation

Implement a feature-generation bridge from the F64 reconstruction/runtime to the frozen 18-feature v5.0g/p1 schema without inventing unavailable inputs. For every output sample the bridge must either:

- produce all required canonical features with provenance and evaluate local p50/p95; or
- mark the uncertainty unresolved.

Coverage must then be reported separately for resolved, censored and unresolved output samples. Only after that can the project measure `F64->F32 storage error / local p50` and `/ local p95` across real files.

The same complete gate must later be repeated on a physically proven FotoGraaf `16320x12288 RAW_SENSOR` payload before any 200MP precision promotion.
