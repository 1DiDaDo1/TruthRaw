# TruthRaw Virtual Observation Manifold v0.9

Status: **IMPLEMENTED RESEARCH CONTRACT — repository promotion requires CI**

This module restores the early TruthRaw Multi-EV / virtual-camera idea as a first-class layer in the current sealed-house / Latent Scene / TruthRange architecture.

## Position in the architecture

`sealed RAW evidence -> Measurement Domain -> Latent Camera Scene + TruthRange -> Virtual Observation Manifold -> Virtual Ideal Camera / compatibility / appearance outputs`

The manifold is not a second evidence source. It is a family of deterministic views/projections of one admitted scene estimate.

## Core invariant

For an exposure node `e`:

`L_view = L_scene * 2^e`

For positive-light TruthRange:

`T_view = T_scene + e`

For a linear covariance in the encoded view:

`Sigma_view = 2^(2*(e+g)) * Sigma_scene`

where `g` is an optional virtual gain/encoding EV.

Exposure EV moves the virtual light/exposure coordinate. Gain EV changes only the encoded observation scale; it does **not** move the underlying TruthRange scene coordinate.

## No-double-counting contract

Any number of nodes still has:

- `physicalFrameCount = 1`
- `independentEvidenceCount = 1`
- `viewsAreIndependentMeasurements = false`
- `evidenceConfidenceMultiplier = 1.0`

The implementation never derives those values from `views.size()`.

Multiple virtual views may improve numerical conditioning, visualization, robustness tests, local exposure selection, or forward-consistency scoring. They do not create photons, increase source SNR, or justify `sigma/sqrt(N)` confidence gains.

## Virtual ISO semantics

v0.9 distinguishes two meanings:

1. **GAIN_ENCODING_ONLY** — a virtual ISO label/gain changes output encoding only. This is usable now and carries no physical sensor-noise claim.
2. **CALIBRATED_FORWARD_MODEL** — predicts a virtual sensor observation only when an explicit source-bound sensor forward model is supplied.

The real HONOR tele physical electron/PTC calibration is still open. Therefore v0.9 does not invent ISO-dependent shot/read-noise physics from an ISO number alone.

## Sensor forward model gate

The optional forward-model API requires:

- non-`UNRESOLVED` authority;
- model ID and source-class ID;
- reference ISO;
- scene-to-pregain scale;
- shot-variance slope;
- read-variance offset;
- saturation threshold.

Without these, physical/noise simulation fails closed. Negative signed reconstruction values are also rejected by the physical-light forward simulator; they remain valid in the signed numerical reconstruction domain.

## Relationship to old Multi-EV work

This is the architectural successor of the earlier continuous virtual-EV manifold / Multi-EV Solver. The useful part is retained: arbitrary EV nodes and adaptive normalized weighting. The invalid interpretation is prohibited: duplicate EV views are never independent measurements.

Recommended diagnostic nodes remain `[-6,-4,-2,0,+2,+4,+6,+8,+10] EV`, but the API accepts any finite node whose scaling remains representable.
