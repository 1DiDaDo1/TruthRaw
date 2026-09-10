# TruthRaw Manifold Conditioning v1 — validation report
**Date:** 2026-09-10

## Decision

`EXACT_REPARAMETERIZATION_CONTRACT_LOCAL_PASS__MODEL_SELECTION_ADMISSION_OPEN`

This closes only the local implementation contract for exact, evidence-neutral binary conditioning. It does not admit a reconstruction-changing Multi-EV candidate and does not claim physical denoising from Virtual EV.

## Why this exists

The historical Best Observation / Multi-EV idea is useful only after separating two meanings:

1. **conditioning gauge** — a temporary coordinate choice that may improve numerical scale but must leave the scientific state unchanged after deconditioning;
2. **model/appearance choice** — may change an estimate or rendering and therefore needs independent evidence or must stay outside the scientific master.

The legacy reconstruction-changing dog Multi-EV candidate remains rejected.

## Exact path implemented

For each Gaussian scalar state `(mu, sigma)`, v1 selects an integer binary EV. The requested EV aims to place `max(|mu|, sigma)` near binary exponent -2, but the selector moves the EV toward zero until both values are demonstrably bit-exact after forward and inverse `scalbn`.

For spatial interactions, v1 forbids mixed per-pixel gauges. It selects one common exact gauge for the full interaction domain. This prevents relative geometry, gradients or neighborhood comparisons from being changed solely by coordinate scaling.

TruthRange interval bounds can be temporarily shifted by the conditioning EV; infinite censor bounds stay infinite. The global stored zero-line / L0 is not redefined.

## Native validation

Always-active checks are used; Release builds do not rely on `assert`.

- GCC Release: PASS
- Clang Release: PASS
- Clang ASan/UBSan: PASS
- broad normal-range randomized states: 250,000 / 250,000 bit-exact round trips
- IEEE-754 fuzz: 992,347 valid tested states; 992,347 accepted; every accepted state bit-exact round trip
- maximum SNR delta on main randomized test: 0
- maximum standardized-residual delta: 0
- high-censored TruthRange `+infinity` preservation: PASS
- fail-closed candidate admission: PASS
- unknown/NaN uncertainty rejection: PASS

### Preserved negative finding during development

An earlier implementation assumed power-of-two scaling would be bit-exact for every successful finite operation. A broad IEEE-754 fuzz test falsified that statement for extreme/subnormal combinations: a value could suffer precision loss before inverse scaling. v1 was corrected so the gauge selector explicitly verifies exact round-trip safety and retreats toward EV=0 if needed. This failed intermediate assumption is retained here because it explains the current gate.

## Real 094423 source stress probe

A measurement-domain probe was run on 3,133,440 sampled CFA values from `IMG_BNC_TRUTHRAW20260907_094423_122.dng`, using normalized source sample magnitude and DNG NoiseProfile sigma. ISO and shutter metadata were not inputs to gauge selection.

- conditioning EV median: +2 in this particular normalization
- 5–95%: 0 to +7
- mean bit-exact round trip: 100%
- sigma bit-exact round trip: 100%
- max SNR delta: 0
- max standardized-residual delta: 0

This numeric EV distribution is **not** a photographic exposure recommendation and is not directly comparable to the historical 094423 Best Observation median near -0.5 EV, because the two use different scene scales, targets and objectives. Only the invariance properties are comparable.

## Fact-check consequences

- Virtual EV cannot itself lower physical sensor noise or create new information when the transformation is invertible and the probability model is handled consistently.
- RAW noise should remain tied to the source noise likelihood. Poisson–Gaussian modeling is a well-established approximation for raw sensor data; clipping/censoring matters.
- Generalized Anscombe / variance-stabilizing transforms are a legitimate *separate* research direction for heteroscedastic denoising, but denoising inside a nonlinear transformed domain changes the estimator and is not exact Virtual-EV reparameterization.
- Spatially varying appearance exposure/curve control needs an image-level edge-aware/conservative regularization gate. The current S-curve pixel API delegates that responsibility to its caller; therefore image-level no-halo closure remains open.

## Admission boundary

Any candidate that changes reconstructed Scene Master values based on Best Observation is `CANDIDATE_REJECTED` unless, before evaluation:

- parameters are frozen;
- genuinely independent held-out evidence exists;
- scalar reconstruction-error gates pass;
- topology gates pass.

Otherwise return canonical v4.7i unchanged.
