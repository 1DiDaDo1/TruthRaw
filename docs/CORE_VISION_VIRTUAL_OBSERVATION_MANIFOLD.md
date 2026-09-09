# TruthRaw Core Vision — Virtual Observation Manifold

**Status: CANONICAL EXTENSION OF THE SEALED-HOUSE / TRUTHRANGE ARCHITECTURE**

This document restores the original TruthRaw Multi-EV / virtual-camera idea inside the current architecture without weakening the evidence boundary.

## 1. Position in the new house

TruthRaw now explicitly distinguishes:

0. **Immutable Evidence** — sealed source RAW/CFA and capture metadata.
1. **Measurement Domain** — black/linearization/gain/readout/noise/censor interpretation.
2. **Latent Camera Scene** — signed scene-linear camera-domain reconstruction, provenance, support, bounds and uncertainty.
3. **Colorimetric Scene Master** — documented scene-linear device-independent coordinates where available.
4. **Virtual Observation Manifold** — an arbitrary family of deterministic exposure/gain/virtual-camera views derived from the same admitted Scene Master.
5. **Compatibility / appearance outputs** — DNG, HDR, SDR, JPEG and other finite projections.

The Virtual Observation Manifold is part of the new house. It never modifies or replaces the sealed original house.

## 2. Continuous virtual exposure

For a virtual exposure coordinate `e` in stops:

`L_view = L_scene * 2^e`

For positive-light TruthRange:

`T_scene = log2(L_scene/L0)`

therefore:

`T_view = T_scene + e`.

The manifold may contain any finite set of nodes for computation, while the conceptual coordinate is continuous. A useful diagnostic set remains:

`[-6, -4, -2, 0, +2, +4, +6, +8, +10] EV`.

These are views/coordinates, not additional exposures that occurred in the sealed source camera.

## 3. Evidence conservation

All views derived from one physical source retain one evidence root.

Adding 2, 9, 100 or 10,000 virtual EV/ISO nodes does not increase:

- physical frame count;
- captured photon count;
- independent measurement count;
- intrinsic source SNR;
- calibration evidence;
- confidence merely because more views agree.

A virtual-view bank therefore must not use `sqrt(N)` or any other multi-frame confidence gain where `N` is merely the number of derived views.

Per-pixel/node weights used for optimization must be normalized across views when they represent repeated parameterizations of the same likelihood.

## 4. Signed scene-linear and TruthRange remain distinct

Signed reconstruction values may be negative for unbiased numerical reasons. They scale linearly under a virtual exposure view.

Negative numerical values are still not negative photons and are not passed through the positive-light `log2` coordinate.

For positive TruthRange values and finite bounds, an exposure view shifts estimate and bounds by the same EV offset. Infinite high/dark censor tails remain infinite.

## 5. Virtual gain and virtual ISO

TruthRaw distinguishes a virtual gain/ISO **encoding projection** from a physical sensor simulation.

### A. Gain/ISO encoding projection

A virtual gain changes the finite output encoding of an existing latent scene estimate. It may be labelled with a nominal virtual ISO for compatibility/inspection purposes, but it does not change the scene TruthRange coordinate and does not create a physical ISO exposure.

Uncertainty/covariance expressed in that encoded coordinate must scale with the same linear transform.

### B. Calibrated virtual sensor forward model

A claim such as "how this scene would be measured by this sensor at ISO X" requires an explicit source-bound sensor forward model. The model must carry provenance/authority and the relevant signal, shot/read variance, saturation and gain behavior.

Without such a model, physical ISO-dependent noise simulation fails closed.

For the current HONOR tele path, physical electron/PTC calibration remains a separate open scientific blocker. A nominal ISO number alone is insufficient to invent shot/read-noise physics.

## 6. What the manifold is allowed to do

The manifold may be used for:

- numerical conditioning of reconstruction/optimization;
- Best Observation / local exposure selection;
- forward-consistency tests at multiple scales;
- shadow/highlight inspection;
- finite HDR/SDR projections;
- virtual camera compatibility outputs;
- deliberately different virtual gain/ISO encodings;
- controlled synthetic sensor experiments when an explicit forward model is bound.

## 7. What the manifold may not claim

It may not claim that deterministic derived views are:

- newly captured frames;
- new photons;
- independent sensor measurements;
- physical exposure bracketing that did not occur;
- evidence for a lower sensor-noise sigma solely because many views exist;
- physical ISO noise realizations without a calibrated sensor model.

## 8. Relationship to the original Multi-EV idea

The original project insight is retained: the new house is not confined to one source-RAW exposure/gain representation and may expose many mathematically useful virtual observation states.

The later scientific correction is also retained: these states share one evidence root and must not be counted as a burst.

This gives the canonical interpretation:

> **TruthRaw may build unlimited virtual observation windows from one reconstructed house. Every window must still disclose which sealed measurement house its evidence came from.**

## 9. Implementation binding

The first implementation is `docs/research/virtual-observation-manifold-v0.9/`.

It binds to the closed Latent Camera Scene / TruthRange v0.2 and camera-RGB covariance v0.6 contracts and preserves their support/censor/covariance semantics.

The implementation is eligible for repository closure only after strict CI against the real dependency chain and the existing canonical regression workflows.

## 10. Permanent boundary

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

Virtual observation multiplicity expands representation and analysis space. It does not multiply the evidence that entered the reconstruction.
