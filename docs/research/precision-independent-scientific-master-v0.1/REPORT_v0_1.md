# Precision-Independent Scientific Master v0.1 — implementation report

**Date:** 2026-09-15  
**Status:** RESEARCH — HOST CI PASS  
**Canonical impact:** none

## What was inspected

Historical canonical reconstruction v4.7i already preserves decoded CFA codes as `std::vector<uint16_t>`, but its metadata, stage-2 normalized mosaic, reconstruction RGB, color transforms and outputs are predominantly `float`.

Camera-RGB covariance v0.6 likewise represents known variances/covariances as `float`, while correctly preserving unknown covariance as NaN rather than silently assuming zero.

Those historical files are intentionally not rewritten. The new architecture is added as a separate research layer.

## What v0.1 implements

### 1. Exact evidence descriptor

`ExactEvidenceDescriptorV01` distinguishes unpacked RAW_SENSOR and packed RAW10/RAW12/RAW14 evidence and records geometry/stride/payload identity fields. Its purpose is to keep the evidence representation conceptually separate from reconstruction scalars.

### 2. Genuine float32 / float64 stage-2 tile entry

`normalize_raw_tile_u16_v0_1<T>()` reads immutable uint16 evidence and emits either `float` or `double` tile workspaces from the same source codes and the same scientific formula.

The arithmetic is genuinely type-specific: the F32 path rounds in float arithmetic; the F64 path operates in double arithmetic. This is required for an honest numerical comparison.

CFA black phase uses global x/y coordinates, so a tile boundary cannot silently alter Bayer phase. The source integer buffer is read-only.

### 3. Historical v4.7i compatibility adapter

`adapters/v47i_stage2_precision_adapter_v0_1.*` reads the frozen v4.7i `DecodedDngFrame` without modifying canonical v4.7i.

It provides:

- `v47i_stage2_tile_f32_v0_1()` — reproduces the historical float Stage-2 arithmetic;
- `v47i_stage2_tile_f64_v0_1()` — uses the same uint16 evidence and the same metadata semantics but double arithmetic;
- `compare_stage2_precision_v0_1()` — measures max-absolute and RMS numerical differences.

Important boundary: v4.7i metadata that historically exists only as float is merely promoted numerically to double in the F64 adapter. That promotion does not create higher-precision physical calibration facts.

### 4. Float64 scientific reductions

`CompensatedSum64V01` uses Neumaier-style compensated summation. `mean_float64_v0_1()` uses it for scientific reductions.

`RunningMoments64V01` uses Welford running moments for mean/variance calculations suitable as primitives for dark/noise/PTC/calibration work.

### 5. Float64 covariance propagation

`Covariance3dV01` stores a full 3x3 covariance in double precision with a nine-bit knowledge mask.

Unknown entries initialize to NaN. `propagate_full_covariance_v0_1()` refuses exact propagation when the full input covariance is not known. It therefore preserves the older TruthRaw rule that unknown covariance must never be silently interpreted as zero.

Known covariance is propagated as:

`C_out = A * C_in * A^T`

using compensated double matrix dot products.

### 6. Arbitrary-precision oracle

`tools/high_precision_validator_v0_1.py` uses Python `Decimal` at 80 decimal digits for deterministic reference calculations.

It compares:

- RAW-code normalization in float32 vs float64;
- 3x3 covariance propagation in float32 vs float64;
- both against the high-precision oracle.

The oracle has explicit authority:

`NUMERICAL_REFERENCE_ONLY_NO_EVIDENCE_UPGRADE`

It cannot promote measured/reconstructed state, evidence count, calibration authority or capture provenance.

## Host CI result

Workflow:

`.github/workflows/precision-independent-scientific-master-v01.yml`

Validated on Ubuntu / GCC 13.3 with:

- CMake configure: PASS;
- C++ build: PASS;
- `precision_policy_v0_1`: PASS;
- `v47i_stage2_precision_adapter_v0_1`: PASS;
- Python high-precision regression: PASS;
- oracle report generation: PASS.

The adapter build proves that the new research module can compile directly against the frozen v4.7i public structures without altering the historical canonical source.

## Numeric oracle result

For the deterministic v0.1 reference cases:

| operation | max abs error float32 | max abs error float64 |
|---|---:|---:|
| RAW normalization | `2.21252441e-07` | `2.4472243940578576e-17` |
| 3x3 covariance propagation | `6.800222212714221e-07` | `1.7285779241877742e-15` |

Covariance maximum relative error:

- float32: `1.397631325240096e-07`
- float64: `3.840154148033752e-16`

Interpretation: float64 is substantially closer to the high-precision numerical oracle in these tests. This establishes a **numerical** reason to prefer double precision for covariance/calibration/reference operations. It does not establish that every per-pixel reconstruction operation requires float64, nor does it add scene evidence.

## Scientific interpretation

The numeric pipeline is now implemented in research form as:

`exact RAW bytes/codes`
`-> precision-selected tile workspace (F32 or F64)`
`-> float64 calibration/statistics/covariance reference layer`
`-> optional arbitrary-precision numerical oracle`

This is intentionally not:

`RAW -> float64 -> therefore more photographic truth`.

Higher precision can reduce arithmetic error. It cannot reduce photon/read noise by itself, recover clipped measurements by itself, create missing optical frequencies or add independent evidence.

## 200MP implications

The 16320x12288 Camera-5 lattice contains 200,540,160 samples. Full-frame high-precision duplication is therefore the wrong default architecture.

The intended integration is:

- exact `.rawsensor`/packed source remains authoritative;
- decode/normalize into bounded tiles;
- permit F32 tiles only when validated against F64/reference error budgets;
- use F64 for calibration parameters, reductions and covariance where conditioning matters;
- reserve arbitrary precision for offline validation/reference cases.

## Current limitations

v0.1 is a numeric reference/integration layer, not yet a replacement full reconstruction backend.

It does not yet:

- execute the full v4.7i edge-aware demosaic in double precision;
- integrate the precision profile with the Android Building Runtime/resource governor;
- fit a complete PTC/noise model;
- fit spectral/color calibration models;
- implement float128/MPFR native production code;
- choose F32 vs F64 dynamically from a measured per-stage error budget;
- benchmark F64 tiles on the HONOR BKQ-N49;
- change canonical covariance v0.6 storage.

## Next implementation step

Run the compatibility adapter against real admitted DNG evidence and compare F32 vs F64 Stage-2 tiles over:

- ordinary midtones;
- values close to black;
- values near clipping;
- gain-field regions;
- residual-black corrections;
- high-ISO/noisy samples.

Then define a numerical error-budget gate:

`numeric error << physical/model uncertainty`

Only stages that satisfy that gate may use F32 by default. Stages that fail it remain F64/reference until improved.

After Stage-2, add a separate double-precision reconstruction reference backend rather than silently changing historical v4.7i.

## Promotion boundary

No component in this module is promoted merely because it compiles or because float64 is numerically closer to the arbitrary-precision oracle.

Promotion requires equivalence/error-budget evidence in actual TruthRaw operations and real-device resource benchmarks.

**Precision changes arithmetic accuracy, not epistemic authority.**
