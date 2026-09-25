# D.RAW Free-World Light Transport State v0.6

Status: **EXECUTABLE RESEARCH REFERENCE — NOT PRODUCTION-PROMOTED**

v0.6 adds the first explicit light-transport state above the v0.5 deep-scene binding.

## New separation

The state now carries separate identities/authority for:

- incoming and outgoing directions;
- surface normal;
- normal/geometry authority;
- material model and material authority;
- illumination identity and illumination authority;
- material spectral hypothesis identity;
- illumination spectral hypothesis identity;
- visibility;
- object / region / provenance identity.

This is still downstream of the sealed source and never writes back into Scientific Master.

## Spectral boundary

A spectral hypothesis is represented by a digest plus authority.

A three-channel camera does not thereby become a spectrometer.

The state explicitly distinguishes:

- a spectral hypothesis identity;
- whether spectral measurement has actually been admitted;
- whether a full spectrum is claimed recovered.

A full-spectrum recovery claim is rejected unless spectral measurement is admitted. An inferred spectral state cannot set the measurement-admitted flag.

## Material reference

v0.6 implements one deliberately inspectable material reference:

`LAMBERTIAN_DIFFUSE`

with RGB diffuse reflectance in [0,1].

The reference BSDF is:

```text
f_r = rho / pi
```

This is a bounded engineering reference, not a claim that arbitrary real materials are Lambertian.

## First rendering-equation integrand

For one incident direction, v0.6 evaluates:

```text
dL_o =
    L_i
    * f_r
    * visibility
    * max(0, n dot wi)
```

and combines it with optional emitted radiance.

This corresponds to evaluating one integrand contribution. It does **not** solve the hemisphere integral:

```text
L_o =
    L_e
    + integral_hemisphere
        f_r * L_i * cos(theta) d omega
```

The result therefore reports:

- `renderingEquationIntegralSolved = false`
- `multipleScatteringSolved = false`

## Authority compatibility

The contribution class constrains parameter authority.

### Evidence-constrained transport

To classify a light-transport contribution as `EVIDENCE_CONSTRAINED`, v0.6 requires:

- no hypothetical/counterfactual parameter state;
- material authority at least MEASURED/CALIBRATED_ESTIMATE;
- illumination authority at least MEASURED/CALIBRATED_ESTIMATE;
- normal geometry authority IMAGE_PLANE_BOUND or CALIBRATED_3D_ESTIMATE.

This does not prove that current D.RAW has those calibrations. It merely defines the gate.

### Inferred scene

Inferred materials, lighting or normals remain INFERRED/UNKNOWN and do not become scientific evidence because a physically plausible equation is evaluated.

### Counterfactual scene

Any counterfactual material, illumination, spectral hypothesis or normal geometry requires a `COUNTERFACTUAL_SCENE` contribution.

## Identity

The light-transport-state SHA-256 binds:

- parent v0.5 bound deep packet;
- normalized wi/wo/normal vectors;
- object/region/provenance IDs;
- normal authority;
- material model and authority;
- material identity and RGB parameters;
- illumination identity and authority;
- material and illumination spectral hypothesis states;
- visibility;
- contribution class.

The resulting path-segment ancestry also binds the state identity into the v0.5 path result.

## Tests

The executable suite proves:

- direction and normal vectors are normalized deterministically;
- material or illumination identity changes the state hash;
- counterfactual illumination cannot hide inside an inferred contribution;
- inferred spectra cannot claim admitted spectral measurement;
- the Lambertian integrand matches the analytic numeric reference;
- back-facing incident light contributes zero reflected radiance;
- geometry authority and radiometric authority remain separate;
- evidence-constrained transport fails closed without strong material/illumination authority.

## Deliberate non-claims

v0.6 does not yet provide:

- full hemisphere integration;
- Monte Carlo path tracing;
- multiple bounce transport;
- glossy/microfacet BSDFs;
- measured BRDF fitting;
- participating-media integration;
- polarization;
- wavelength-sampled rendering;
- spectral reconstruction from RGB camera data;
- recovered hidden lighting/material truth.

## Next gate

The next milestone should be `FREE_WORLD_APPEARANCE_RESOLVE_v0.7`.

That layer should consume the scene/light-transport result without changing its authority and introduce explicit:

- viewing conditions;
- appearance-space lightness/colorfulness/hue controls;
- target peak/black luminance;
- target primaries/white;
- SDR/HDR display encoding identity;
- deterministic proof that changing display target leaves scene/light-transport state untouched.
