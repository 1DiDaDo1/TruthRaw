# TruthRaw Manifold Conditioning v1.0

This layer restores the useful part of the early Multi-EV idea inside the current sealed-house architecture without reintroducing evidence multiplication.

## Two hard modes

### 1. `EXACT_REPARAMETERIZATION`
The same single-evidence likelihood/estimate is evaluated in a numerically convenient EV coordinate and mapped back. For exposure EV `e`:

- scene-linear values scale by `2^e`;
- standard deviations scale by `2^e`;
- covariance scales by `2^(2e)` through the already-closed v0.9/v0.6 path;
- deconditioning must recover the same scene coordinates;
- gain/ISO encoding is rejected as an optimizer coordinate.

This mode is allowed to improve numerical conditioning, not statistical confidence.

### 2. `ROBUST_MODEL_SELECTION`
A candidate missing-channel estimator may differ from the canonical baseline only after frozen-before-evidence, independent held-out scenes pass an explicit promotion policy. Development/tuning evidence is excluded. Duplicate evidence roots are excluded. Scalar error improvement cannot compensate for a configured topology regression.

If eligibility is not proven, `apply_selected_estimate_v1` returns the canonical baseline value.

## Evidence rule

At all times:

- `physicalFrameCount = 1` per admitted source frame;
- `independentEvidenceCount = 1` per source evidence root;
- virtual EV node count is never an evidence multiplier;
- co-sited missing colour remains uncertified unless separately proven.

## Real dog evidence

The full 094423 Scene Master was conditioned/deconditioned across EV -20..+20 on power-of-two nodes: 451,215,360 float32 value round-trip checks, zero changed IEEE-754 values, max absolute error 0.

The earlier robust-asinh triple-EV transplant is preserved as negative evidence. 094423 was the development scene; only 094414 and 094416 are frozen-parameter checks. Those checks contain topology regressions, so the historical candidate is not promotable.
