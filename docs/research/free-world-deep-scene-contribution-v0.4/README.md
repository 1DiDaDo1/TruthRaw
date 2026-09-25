# D.RAW Free-World Deep Scene Contribution v0.4

Status: **EXECUTABLE RESEARCH REFERENCE — NOT PRODUCTION-PROMOTED**

This is the first deep contribution layer above the v0.3 bound Scientific Master + Open Scene camera plane.

## Purpose

A final image-space location no longer has to collapse immediately to one flat RGB triplet.

v0.4 allows one target pixel to contain an ordered list of scene-linear contributions with:

- front/back depth;
- scene-linear RGB;
- opacity/transmittance role;
- contribution class;
- per-channel authority;
- per-channel uncertainty;
- provenance ID.

The packet is canonicalized front-to-back and bound by SHA-256.

## Contribution classes

v0.4 separates four classes:

- `EVIDENCE_CONSTRAINED`
- `INFERRED_SCENE`
- `RESTORATION_HYPOTHESIS`
- `COUNTERFACTUAL_SCENE`

Only the first class may carry non-UNKNOWN scientific channel authority.

An inferred, restoration or counterfactual sample that tries to claim RECONSTRUCTED or CENSORED scientific authority is rejected.

## Camera-plane binding

`makeCameraPlaneContribution()` converts a v0.2 area-resolved camera-plane pixel into an opaque evidence-constrained deep sample.

It requires:

- zero measured target claims;
- one physical frame;
- one independent evidence item;
- no new evidence creation.

Its channel authority and admitted uncertainty are copied from the v0.2 resolve.

## Four view resolves

The same deep packet can be resolved in four explicit modes:

1. `SCIENTIFIC_VIEW`: evidence-constrained contributions only;
2. `OPEN_SCENE_VIEW`: evidence + inferred scene;
3. `RESTORATION_VIEW`: evidence + inferred + restoration hypothesis;
4. `COUNTERFACTUAL_RENDER`: all of the above + counterfactual scene.

This means one packet can support multiple outputs without relabelling hypothetical content as scientific observation.

## Ordered transmittance reference

The first resolver is intentionally simple and inspectable:

```text
T_0 = 1

for front-to-back sample i:
    visible_i = T_i * opacity_i
    rgb += visible_i * scene_linear_rgb_i
    T_(i+1) = T_i * (1 - opacity_i)
```

This is a deterministic layered transmittance/compositing reference.

It is **not** yet a path tracer, BSDF model, volumetric radiative-transfer solver or spectral renderer.

The physical-light-transport research remains a later gate.

## Authority rule

If any visible non-evidence contribution participates in a channel, that rendered channel authority is UNKNOWN.

A visually convincing inferred foreground therefore cannot inherit the authority of the evidence-constrained background it occludes.

`SCIENTIFIC_VIEW` can remain scientific because it excludes all non-evidence contributions before resolve.

## Uncertainty rule

v0.4 propagates numeric p95 uncertainty only through visible evidence-constrained contributions.

If a visible inferred, restoration or counterfactual contribution participates, output uncertainty is marked unknown rather than assigning a false scientific uncertainty to the hypothesis.

This is deliberately conservative.

## What v0.4 proves

The tests demonstrate:

- a bound v0.2 camera-plane pixel can become a deep evidence sample without new evidence;
- non-evidence deep samples cannot claim scientific authority;
- packet ordering and digest are deterministic;
- Scientific View ignores inferred/restoration/counterfactual layers;
- Open Scene View may render inferred content while authority becomes UNKNOWN;
- Restoration View admits restoration only explicitly;
- Counterfactual Render admits counterfactual content only explicitly;
- opaque foreground correctly occludes farther contributions;
- appearance and display encoding remain absent.

## Non-claims

v0.4 does not claim:

- recovered physical depth from a single image;
- recovered hidden geometry;
- recovered BRDF/material truth;
- spectral reconstruction;
- physically exact transparency;
- participating-media physics;
- multiple scattering;
- path tracing;
- neural-field truth.

Depth and non-evidence layers are representational structures unless independently constrained by future evidence.

## Next step

The next gate is `FREE_WORLD_DEEP_SCENE_BINDING_v0.5`:

- bind each deep packet to the v0.3 camera-plane scene identity;
- add explicit region/object IDs and per-contribution ancestry;
- distinguish geometry authority from radiometric authority;
- add a first physically based ray/path contribution interface without yet claiming full scene recovery.
