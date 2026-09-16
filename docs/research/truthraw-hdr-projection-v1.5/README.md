# TruthRaw-owned HDR projection v1.5

Status: **RESEARCH — contract validated in CI, real TruthRaw-owned HDR image render not yet produced**.

## Why this exists

The Adobe field trials established that one unchanged single-frame CFA source can be rendered into very different HDR presentations. The v1.4 counterbalanced trial is especially important: the same P3-D65/PQ source presentation can move the bulk of the image slightly darker while extending only the highlight tail, and can report a 10,000-nit MaxCLL without creating new sensor evidence.

TruthRaw therefore needs its own explicit boundary between:

1. **Scientific scene authority** — immutable source DNG/CFA, Scientific Master, Dynamic Authority Field, censoring and uncertainty; and
2. **HDR presentation** — output gamut, transfer function, display reference white, finite peak luminance, tone mapping and encoder diagnostics.

v1.5 makes that boundary executable.

## Core law

> Representation may exceed the source; knowledge claims may not exceed the evidence.

The Scientific Master remains open-ended in scene-linear / TruthRange representation. A PQ file is only a finite presentation window over that world.

## Scientific input binding

`ScientificProjectionBindingV15` requires four independent SHA-256 bindings:

- immutable source DNG;
- decoded source CFA;
- Scientific Master;
- Dynamic Authority Field artifact.

It also binds the positive reference `L0` used for `T = log2(L/L0)`.

The Adobe v1.4 source remains:

- source DNG SHA-256: `7930ba5db0fe1b7b9dcc09e9337efbca76b8877fb78a6dc4667761bf25159b67`
- decoded CFA SHA-256: `883cbe13fd2a5afe7e9f3f4147df3a8ed3be681340a3c0422933f42d0f4d719c`
- source-white censored samples: `217`

No new Scientific Master or Dynamic Authority artifact hash is invented here; the real render gate remains open until those exact artifacts are bound.

## Headroom semantics

`summarize_scientific_headroom()` keeps three different concepts separate:

- nominal supported peak EV from `MEASURED`, `CALIBRATED_ESTIMATE`, and `RECONSTRUCTED` samples;
- conservative p95 supported peak EV from `(estimate - uncertainty_p95)` where positive;
- censored lower-bound peak EV from known censor bounds.

A censored lower bound is never promoted to exact recovered radiance.

`COUNTERFACTUAL` and `APPEARANCE_ONLY` are rejected from this scientific projection path. They may be used only by a separately identified presentation layer.

## Presentation contract

`HdrPresentationRequestV15` binds:

- primaries (`P3_D65`, `REC709`, or `REC2020`);
- transfer (`SMPTE_ST_2084_PQ`);
- bit depth;
- 4:4:4/RGB transport requirement;
- presentation reference white in nits;
- finite target peak luminance up to the 10,000-nit PQ ceiling;
- a hash-bound monotonic scene-EV -> display-EV curve;
- optional Adobe HDR-limit metadata for interoperability.

The Adobe HDR-limit value is **not** used to derive scientific headroom and is **not** automatically converted into physical nits.

## Scalar PQ utility

`pq_code_from_nits()` implements the ST 2084 scalar OETF for contract/regression purposes. It is intentionally not a complete AVIF/JPEG XL/TIFF encoder and does not imply an RGB/YUV color-management solution by itself.

## Per-sample projection

For a supported scientific sample:

1. preserve original authority class and uncertainty;
2. derive nominal and conservative TruthRange coordinates when positive;
3. apply only the explicitly bound presentation curve;
4. convert requested display luminance to a PQ code;
5. report presentation clipping independently of scientific authority.

For `CENSORED` or `UNKNOWN` samples, the scientific projector does not manufacture an exact PQ value. A future appearance layer may render them, but its output must retain the censored/unknown parent lineage and may not write back to the Scientific Master.

## Relation to the Adobe v1.4 experiment

The v1.4 real trial showed:

- neutral P3/PQ control `(11).avif`: MaxCLL 1391 nits, p99 luma code 697, max 810;
- counterbalanced `(12).avif`: Exposure -5, Highlights -100, Whites -100 plus a steep tone curve, MaxCLL 10,000 nits, p99 684, p99.9 816, max 1023;
- duplicate `(13).avif`: different whole-file bytes but exactly identical decoded Color and Gain Map payload to `(12)`;
- only about `2.0532e-5` of decoded luma samples in `(12)` hit PQ code 1023.

This empirically demonstrates that HDR presentation can redistribute the output tail without changing source CFA authority. v1.5 encodes that separation as a TruthRaw-owned contract.

## What v1.5 does not prove

It does **not** claim that:

- a 10,000-nit MaxCLL is 10,000 nits measured by the sensor;
- an Adobe `HDR Limit +8` is eight stops of new scene evidence;
- presentation clipping or expansion extends physical sensor dynamic range;
- clipped source samples have exact recovered radiance;
- the scalar PQ mapper is already a finished image encoder.

## Next real gate

The next empirical step is to bind an exact real Scientific Master artifact and exact Dynamic Authority Field artifact for the source CFA, generate a TruthRaw-owned P3-D65/PQ render plus manifest, and then round-trip that output through Lightroom. Adobe may alter presentation, but any scientific-authority writeback must remain prohibited.
