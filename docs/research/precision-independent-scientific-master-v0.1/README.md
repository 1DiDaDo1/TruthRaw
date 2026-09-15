# TruthRaw Precision-Independent Scientific Master v0.1

**Status:** RESEARCH ONLY / NO CANONICAL PROMOTION

This module begins the numeric implementation of the Free Scientific Space vision:

`exact RAW integer evidence -> float32/float64 tiled reconstruction -> float64 calibration/optimization/covariance -> arbitrary/high-precision reference validation`

It does **not** rewrite historical canonical v4.7i or camera-rgb-covariance-v0.6 bytes. Those remain provenance. This module adds a new precision contract above them.

## Scientific invariants

1. Original packed/integer RAW evidence remains exact and authoritative.
2. Converting an evidence sample to floating point creates a working representation, not new evidence.
3. Float32 and float64 reconstruction paths must represent the same scientific quantity and authority class.
4. Float64 is the default reference precision for calibration, optimization, matrix fitting, reductions and covariance propagation in this module.
5. Higher precision is used as an oracle to measure numerical error, never to claim additional photons, spatial detail or color evidence.
6. Unknown covariance remains unknown/NaN; higher precision must never turn an unknown into zero.
7. Precision policy is an execution/numerics decision and is independent of `physicalFrameCount`, `independentEvidenceCount`, calibration authority and measured/reconstructed status.

## Why a new research module

Historical v4.7i stores source CFA codes as `uint16_t` but uses `float` throughout most reconstruction/color work. Camera RGB covariance v0.6 also stores variances/covariances as `float`. Those choices were valid for their frozen modules, but the Free Scientific Space architecture must not define scientific truth as one primitive type.

v0.1 therefore provides:

- exact unsigned-integer evidence sample handling;
- templated float32/float64 stage-2 normalization helpers;
- compensated float64 accumulation for scientific reductions;
- float64 3x3 linear transform and covariance propagation;
- fail-closed unknown-covariance semantics;
- an arbitrary-precision Python oracle based on `decimal.Decimal`;
- tests comparing float32 and float64 to the high-precision reference.

## Precision profiles

### Evidence

`EXACT_INTEGER_EVIDENCE`

Original RAW bytes/codes are preserved. A floating representation is never the only authoritative copy.

### Reconstruction hot path

- `F32_TILED`: default high-throughput candidate where validated.
- `F64_TILED`: reference/high-precision reconstruction candidate for difficult numerics, desktop validation, or selected tiles.

A future runtime may choose between them only when equivalence/error-budget evidence permits. Hardware strength does not change scientific authority.

### Calibration / optimization / covariance

`F64_SCIENTIFIC`: double-precision parameters, accumulators, matrices and covariance propagation by default.

### Reference validator

`ARBITRARY_PRECISION_ORACLE`: offline validation only. It estimates numerical error relative to a much higher-precision computation. It is not a production per-pixel representation.

## Promotion gate

This module may only influence production/canonical paths after it has:

- compiled on host CI;
- passed deterministic numerical tests;
- shown float32-vs-float64 error against the arbitrary-precision oracle;
- defined acceptable tolerances relative to physical/model uncertainty;
- demonstrated no evidence/authority changes;
- been benchmarked in the real tiled runtime before any 200MP default is changed.

## 200MP consequence

At 16320x12288, one scalar full-frame plane costs approximately 0.747 GiB in float32 and 1.494 GiB in float64. Therefore float64 must be introduced through tile-local workspaces, high-precision accumulators and selected scientific stages rather than by blindly duplicating every full-frame plane.

## Files

- `native/precision_policy_v0_1.h` — numeric contract and float64 scientific primitives.
- `native/precision_policy_v0_1.cpp` — implementations.
- `native/test_precision_policy_v0_1.cpp` — host C++ checks.
- `tools/high_precision_validator_v0_1.py` — arbitrary-precision oracle and JSON report.
- `tests/test_high_precision_validator_v0_1.py` — Python regression tests.
- `CMakeLists.txt` — host build.
- `REPORT_v0_1.md` — findings and next integration steps.

**Representation may be freer than the source. Evidence claims may never be stronger than the evidence.**
