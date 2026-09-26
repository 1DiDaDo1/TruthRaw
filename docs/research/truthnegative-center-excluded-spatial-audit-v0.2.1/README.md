# TruthNegative N2 Center-Excluded Spatial Audit v0.2.1

Status: **EXECUTABLE RESEARCH SIDECAR — AUDIT ONLY**

## Purpose

This module extends the center-excluded N2 v0.2 research predictor from three
1:1 diagnostic crops to a bounded whole-frame spatial audit.

It does not replace N2 v0.1, does not change the current appearance candidate,
does not modify Direct CFA, Scientific Master or TruthNegative, and does not
create new evidence.

The immediate research question is:

> For v0.1 candidate centers that survive source-site selection, where can the
> surrounding CFA support predict the center without using the observed center
> to select that support, and how does that confidence vary spatially?

## v0.1 parity gate

For every 64x64 source tile, v0.2.1 re-runs the existing v0.1 CFA audit on the
same source region and sampling period.

The tile is admitted to v0.2.1 only if the v0.1 tile audit is bit-identical in
its floating-point accumulators and exact in all counters, CFA phase counts and
border-protection counts to the already produced v0.1 reference audit.

The v0.2.1 JSON is also cryptographically bound to:

- sealed source SHA-256;
- Scientific Master SHA-256;
- authority-field SHA-256;
- TruthNegative-state SHA-256;
- v0.1 candidate SHA-256;
- v0.1 audit SHA-256;
- v0.1 spatial SHA-256.

Thus v0.2.1 cannot silently redefine which samples were v0.1 candidates.

## Per-tile measurements

For each v0.1 candidate center the audit records, aggregated per tile:

- predictor valid / invalid;
- symmetric H/V/diagonal pairs considered / accepted / rejected;
- scales considered / accepted / rejected;
- center-only residual z bins: <=1 sigma, 1-2 sigma, >2 sigma;
- combined diagnostic residual z bins;
- mean and maximum absolute residual;
- mean center variance;
- mean predictor estimate variance;
- mean/max predictor-variance to center-variance ratio;
- maximum directional disagreement;
- maximum cross-scale disagreement;
- v0.1 candidate CFA-phase distribution;
- predictor-valid CFA-phase distribution.

## Two sigma domains

Primary conservative statistic:

    z_center =
        abs(center - estimate) / sqrt(centerVariance)

Secondary diagnostic only:

    z_combined =
        abs(center - estimate) /
        sqrt(centerVariance + estimateVariance)

The combined form is **not** an authority upgrade. It assumes enough
independence between center noise and predictor support for variances to add.
That independence has not been established for the real camera.

Accordingly the schema always states:

    center_only_sigma_primary=true
    combined_sigma_diagnostic_only=true
    noise_independence_admitted=false

## Permanent invariants

- Direct CFA unchanged;
- Scientific Master unchanged;
- TruthNegative unchanged;
- current v0.1 appearance candidate unchanged;
- reconstruction-support closure unchanged;
- exact Scientific-Master baseline gate unchanged;
- candidate_applied=false;
- creates_new_evidence=false;
- scientific_writeback_allowed=false.

## Device motivation

The 2026-09-26 v0.2 device run showed that center-excluded prediction was
nearly ubiquitous in a quiet/noise crop but substantially less coherent in
structure and censor/highlight crops. The next necessary step is therefore
spatial coverage, not stronger denoise.

This sidecar is intended to reveal whether that separation persists across the
whole admitted source rather than only three automatically chosen crops.
