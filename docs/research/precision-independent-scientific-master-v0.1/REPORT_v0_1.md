# Precision-Independent Scientific Master v0.1 — implementation report

**Date:** 2026-09-15  
**Status:** RESEARCH / CI validation in progress at creation time  
**Canonical impact:** none

## What was inspected

Historical canonical reconstruction v4.7i already preserves decoded CFA codes as `std::vector<uint16_t>`, but its metadata, stage-2 normalized mosaic, reconstruction RGB, color transforms and outputs are predominantly `float`.

Camera-RGB covariance v0.6 likewise represents known variances/covariances as `float`, while correctly preserving unknown covariance as NaN rather than silently assuming zero.

Those historical files are intentionally not rewritten. The new architecture is added as a separate research layer.

## What v0.1 implements

### 1. Exact evidence descriptor

`ExactEvidenceDescriptorV01` distinguishes unpacked RAW_SENSOR and packed RAW10/RAW12/RAW14 evidence and records geometry/stride/payload identity fields. Its purpose is to keep the evidence representation conceptually separate from reconstruction scalars.

### 2. Float32 / float64 stage-2 tile entry

`normalize_raw_tile_u16_v0_1<T>()` reads immutable uint16 evidence and can emit either `float` or `double` tile workspaces from the same source codes and the same scientific formula.

CFA black phase uses global x/y coordinates, so a tile boundary cannot silently alter Bayer phase.

The function does not overwrite evidence and does not change evidence authority.

### 3. Float64 scientific reductions

`CompensatedSum64V01` uses Neumaier-style compensated summation. `mean_float64_v0_1()` uses it for scientific reductions.

`RunningMoments64V01` uses Welford running moments for mean/variance calculations suitable as primitives for dark/noise/PTC/calibration work.

### 4. Float64 covariance propagation

`Covariance3dV01` stores a full 3x3 covariance in double precision with a nine-bit knowledge mask.

Unknown entries initialize to NaN. `propagate_full_covariance_v0_1()` refuses exact propagation when the full input covariance is not known. It therefore preserves the older TruthRaw rule that unknown covariance must never be silently interpreted as zero.

Known covariance is propagated as:

`C_out = A * C_in * A^T`

using compensated double matrix dot products.

### 5. Arbitrary-precision oracle

`tools/high_precision_validator_v0_1.py` uses Python `Decimal` at 80 decimal digits for deterministic reference calculations.

It compares:

- RAW-code normalization in float32 vs float64;
- 3x3 covariance propagation in float32 vs float64;
- both against the high-precision oracle.

The oracle has explicit authority:

`NUMERICAL_REFERENCE_ONLY_NO_EVIDENCE_UPGRADE`

It cannot promote measured/reconstructed state, evidence count, calibration authority or capture provenance.

## Scientific interpretation

The numeric pipeline is now defined as:

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
- permit F32 tiles when validated against F64/reference error budgets;
- use F64 for calibration parameters, reductions and covariance where conditioning matters;
- reserve arbitrary precision for offline validation/reference cases.

## Current limitations

v0.1 is a numeric reference layer, not yet a replacement reconstruction backend.

It does not yet:

- run the full v4.7i edge-aware demosaic in double precision;
- integrate with the Android runtime/resource governor;
- fit a complete PTC/noise model;
- fit spectral/color calibration models;
- implement float128/MPFR native production code;
- choose F32 vs F64 dynamically from a measured per-stage error budget;
- benchmark F64 tiles on the HONOR BKQ-N49;
- change canonical covariance v0.6 storage.

## Next implementation step

After CI is green, add a **v4.7i compatibility adapter** that feeds the exact same decoded uint16 CFA evidence through both F32 and F64 stage-2 tile paths, then compares reconstructed scientific outputs tile-by-tile without changing historical canonical files.

After that, define a numerical error-budget gate:

`numeric error << physical/model uncertainty`

Only stages that satisfy that gate may use F32 by default. Stages that fail it remain F64/reference until improved.

## Promotion boundary

No component in this module is promoted merely because it compiles or because float64 is numerically closer to the arbitrary-precision oracle.

Promotion requires equivalence/error-budget evidence in actual TruthRaw operations and real-device resource benchmarks.

**Precision changes arithmetic accuracy, not epistemic authority.**
