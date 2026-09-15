# TruthRaw Precision-Independent Scientific Master v0.1

**Status:** RESEARCH ONLY / NO CANONICAL PROMOTION

This module begins the numeric implementation of the Free Scientific Space vision:

`exact RAW integer evidence -> precision-selected tiled scientific work -> float64 calibration/optimization/covariance -> arbitrary/high-precision reference validation`

It does **not** rewrite historical canonical v4.7i or camera-rgb-covariance-v0.6 bytes. Those remain provenance. This module adds a new precision contract above them.

## Scientific invariants

1. Original packed/integer RAW evidence remains exact and authoritative.
2. Converting an evidence sample to floating point creates a working representation, not new evidence.
3. Float32 and float64 execution must represent the same scientific quantity and authority class.
4. Float64 is the default reference precision for calibration, optimization, matrix fitting, reductions and covariance propagation in this module.
5. Higher precision is used as an oracle to measure numerical error, never to claim additional photons, spatial detail or color evidence.
6. Unknown covariance remains unknown/NaN; higher precision must never turn an unknown into zero.
7. Precision policy is an execution/numerics decision and is independent of `physicalFrameCount`, `independentEvidenceCount`, calibration authority and measured/reconstructed status.
8. A small upstream numeric error is not sufficient evidence for Float32 equivalence when a later branch-sensitive algorithm can amplify that error by selecting a different reconstruction hypothesis.

## Why a new research module

Historical v4.7i stores source CFA codes as `uint16_t` but uses `float` throughout most reconstruction/color work. Camera RGB covariance v0.6 also stores variances/covariances as `float`. Those choices remain valid frozen historical modules, but the Free Scientific Space architecture must not define scientific truth as one primitive type.

The module now provides:

- exact unsigned-integer evidence sample handling;
- templated float32/float64 Stage-2 normalization helpers;
- compensated float64 accumulation for scientific reductions;
- float64 running moments for dark/noise/PTC work;
- float64 3x3 linear transform and covariance propagation;
- fail-closed unknown-covariance semantics;
- an arbitrary-precision Python oracle based on `decimal.Decimal`;
- an SDK-structured Float32 and Float64 DNG GainMap reference;
- a precision-instrumented reproduction of frozen v4.7i edge-aware measured-preserving reconstruction;
- branch and support-clamp tracing;
- real HONOR and MotionCam precision fixtures;
- a real-DNG Stage-2 tile exporter and a C++ reconstruction precision CLI for multi-file sweeps.

## What has been learned so far

### Stage-2 black/white normalization

On tested real HONOR/MotionCam files, Float32 error is far below the physical/model noise budget. Float32 therefore remains a valid hot-path candidate **for this stage**, with Float64 retained as the scientific reference.

### HONOR vendor DNG GainMap

A six-file audit covering 75,202,560 GainMap applications, including highlight stress cases, keeps Float32-vs-Float64 error far below the modeled physical uncertainty. Float32 therefore remains a validated hot-path candidate for the tested GainMap family, again with Float64 as reference.

### Branch-sensitive reconstruction

The conclusion changes at reconstruction.

Frozen v4.7i contains a hard `0.72` directional threshold. A tiny F32/F64 Stage-2 difference can cross that threshold and select another reconstruction branch. Two real-source fixtures now demonstrate this class of behaviour:

1. HONOR vendor DNG `IMG_BNC_TRUTHRAW20260906_151129_256.dng`: Float32 selects `Vertical`, Float64 selects `WeightedBlend`, and the reconstructed red-channel difference is about `1.95e-2` normalized while the measured CFA channel remains exact.
2. MotionCam Direct-CFA `IMG_260816_134122_304_005.dng` with unity GainMaps: Float32 selects `WeightedBlend`, Float64 selects `Horizontal` at a real source neighbourhood. This demonstrates that the precision hazard is not caused by the vendor non-unity GainMap path.

Promoting already-rounded Float32 Stage-2 samples to double does not recover the Float64 decision. Precision must therefore be preserved before the branch-sensitive operation if branch equivalence to the Float64 scientific reference is required.

## Current precision profiles

### Evidence

`EXACT_INTEGER_EVIDENCE`

Original RAW bytes/codes are preserved. A floating representation is never the only authoritative copy.

### Stage-2 / GainMap hot path

`F32_WITH_F64_REFERENCE_WHERE_VALIDATED`

Float32 is allowed only for source/stage families whose measured error remains safely below physical/model uncertainty and whose downstream branch behaviour is not silently changed.

### Branch-sensitive reconstruction

`F64_SCIENTIFIC_REFERENCE`

Float64 is now the scientific reference for the edge-aware branch-sensitive reconstruction. Float32 may still become a runtime optimization, but only after passing branch-stability and output-error gates against Float64.

### Calibration / optimization / covariance

`F64_SCIENTIFIC`

Double-precision parameters, accumulators, matrices and covariance propagation by default.

### Reference validator

`ARBITRARY_PRECISION_ORACLE`

Offline validation only. It estimates numerical error relative to a much higher-precision computation. It is not a production per-pixel representation and does not raise evidence authority.

## Mixed-precision direction

The current evidence rejects the simplistic strategy:

`F32 Stage-2 -> promote to F64 only inside reconstruction`

because the Stage-2 rounding that can trigger a different branch has already occurred.

The next candidates are:

1. full Float64 Stage-2 + Float64 reconstruction inside each tile;
2. a dual-lane tile with a validated Float64 decision/reference lane and a smaller runtime/storage lane;
3. adaptive escalation of numerically ambiguous neighbourhoods to Float64, but only if it reproduces the Float64 scientific reference within explicit gates.

No mixed-precision candidate is promoted yet.

## 200MP consequence

At 16320x12288, one scalar full-frame plane costs approximately 0.747 GiB in float32 and 1.494 GiB in float64. This does **not** require a full-frame Float64 RAM allocation.

For a 512x512 tile:

- one Float64 scalar plane is 2 MiB;
- one Float64 RGB plane is 6 MiB.

The Free Scientific Space therefore stays compatible with 200MP by using tile-local high-precision workspaces, streaming/chunked scientific storage and device-dependent concurrency. Stronger hardware may make rooms larger/faster; it may not silently change scientific authority.

## Promotion gate

This module may only influence production/canonical paths after it has:

- compiled on host CI;
- passed deterministic numerical tests;
- shown float32-vs-float64 error against the arbitrary-precision oracle;
- defined acceptable tolerances relative to physical/model uncertainty;
- demonstrated no evidence/authority changes;
- measured branch and support-clamp divergence in branch-sensitive stages;
- been benchmarked in the real tiled runtime before any 200MP default is changed.

## Important files

- `native/precision_policy_v0_1.h/.cpp` — numeric contract and float64 scientific primitives.
- `native/gainmap_precision_v0_2.*` — SDK-structured F32/F64 GainMap reference.
- `native/reconstruction_precision_reference_v0_3.*` — F32/F64 frozen-v4.7i reconstruction reference and trace instrumentation.
- `native/test_real_reconstruction_precision_fixture_v0_3.cpp` — real HONOR vendor branch-divergence fixture.
- `native/test_motioncam_direct_reconstruction_precision_fixture_v0_3.cpp` — real MotionCam Direct-CFA branch-divergence fixture.
- `tools/high_precision_validator_v0_1.py` — arbitrary-precision oracle and JSON report.
- `tools/export_real_dng_stage2_tile_v0_3.py` — real DNG -> paired F32/F64 Stage-2 tile exporter.
- `tools/reconstruction_precision_cli_v0_3.cpp` — C++ real-tile F32/F64 reconstruction audit CLI.
- `evidence/REAL_DNG_STAGE2_PRECISION_AUDIT_v0_1.json` — real Stage-2 evidence.
- `evidence/REAL_RECONSTRUCTION_PRECISION_FIXTURE_v0_3.json` — machine-readable real reconstruction fixture.
- `RECONSTRUCTION_PRECISION_REPORT_v0_3.md` — reconstruction finding and policy consequence.
- `STATUS_v0_1.json` — current machine-readable status.

**Representation may be freer than the source. Evidence claims may never be stronger than the evidence. Numerical precision may reduce arithmetic ambiguity; it never creates new photographic evidence.**
