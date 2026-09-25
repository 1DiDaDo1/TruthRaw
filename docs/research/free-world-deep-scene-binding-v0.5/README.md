# D.RAW Free-World Deep Scene Binding v0.5

Status: **EXECUTABLE RESEARCH REFERENCE — NOT PRODUCTION-PROMOTED**

v0.5 binds the v0.4 deep packet to an explicit camera-plane scene identity and separates **geometry authority** from **radiometric authority**.

## Why this matters

A photograph can constrain visible radiometry more strongly than it constrains hidden 3D geometry.

For example, a surface colour/radiance estimate can remain evidence-constrained while the depth assigned to that surface is only inferred.

D.RAW must not collapse those two claims into one authority label.

v0.5 therefore treats them as independent axes.

## Geometry authority

The research schema introduces:

- `IMAGE_PLANE_BOUND`
- `CALIBRATED_3D_ESTIMATE`
- `INFERRED`
- `HYPOTHETICAL`
- `COUNTERFACTUAL`
- `UNKNOWN`

These labels describe the spatial/depth/scene-placement claim only.

They do not upgrade or downgrade the radiometric channel authority automatically.

An evidence-constrained camera-plane contribution may therefore carry reconstructed radiometry while its assigned 3D depth is still only INFERRED.

## Region and object identity

Every bound contribution now requires:

- non-zero provenance ID;
- non-zero region ID;
- non-zero object ID;
- geometry authority;
- optional parent ancestry digest;
- generated contribution ancestry digest.

The final bound deep packet is hashed from:

- camera-plane scene SHA-256;
- parent v0.4 deep packet SHA-256;
- each contribution ancestry;
- region/object/provenance identity;
- geometry authority.

Changing the object identity or camera-plane scene identity changes the bound packet identity.

## Contribution ancestry

Per-contribution ancestry is a SHA-256 binding over:

```text
camera-plane scene identity
+ parent deep packet identity
+ parent ancestry
+ provenance ID
+ region ID
+ object ID
+ geometry authority
```

It does not claim that the object or depth is measured. It makes the claim lineage explicit and tamper-evident.

## Geometry/class compatibility

v0.5 fails closed when the semantic class and geometry authority contradict one another.

Examples:

- counterfactual geometry requires a COUNTERFACTUAL_SCENE contribution;
- restoration layers may be HYPOTHETICAL/INFERRED/UNKNOWN;
- inferred scene layers may not silently claim counterfactual geometry;
- evidence-constrained radiometry may coexist with inferred geometry.

## First physically based path interface

v0.5 adds a deliberately bounded RGB factorised path contribution reference:

```text
L_path =
    L_emitted
    + L_incident
      * throughput
      * visibility
      * cosine
```

This creates a typed scene-linear deep contribution with:

- path kind;
- geometry authority;
- region/object/provenance identity;
- radiometric authority;
- opacity/depth interval.

Supported path kinds are:

- camera primary;
- surface reflection;
- transmission;
- volume;
- emission.

This is **not** the full rendering equation.

It does not yet model spectral integration, BSDF lobes, probability-density weighting, multiple bounces, participating-media transport, polarization or reciprocity.

Its purpose is to establish the interface through which later physically based transport can enter the Open Scene without destroying authority separation.

## Radiometric authority rule

Only `EVIDENCE_CONSTRAINED` contributions may carry non-UNKNOWN scientific radiometric authority.

Inferred, restoration and counterfactual path contributions must remain radiometrically UNKNOWN in this research reference.

This keeps a physically plausible calculation from turning a hypothesis into evidence.

## Explicit non-claims

Every path result reports:

- `spectralMeasurementClaimed = false`
- `fullPathTracingClaimed = false`
- `createsNewEvidence = false`
- `scientificWritebackAllowed = false`

## Tests

The executable tests prove:

- camera-plane scene identity is bound into the packet;
- region/object identity is mandatory;
- identity changes alter the packet hash;
- geometry and radiometry remain separate authority axes;
- counterfactual geometry cannot hide inside an evidence contribution;
- the factorised path equation is deterministic;
- non-evidence path contributions cannot claim reconstructed radiometric authority;
- zero scene identity and provenance mismatch fail closed.

## Next gate

The next research milestone is `FREE_WORLD_LIGHT_TRANSPORT_STATE_v0.6`.

It should add:

- typed incident/outgoing direction vectors;
- surface normal authority;
- material/BSDF parameter authority;
- illumination-field identity;
- wavelength/spectral-hypothesis identity;
- path-segment ancestry;
- a reference rendering-equation integrand without claiming that hidden materials or spectra were measured from one image.
