# D.RAW Free-World Output Pixel Research v0.1

Status: **RESEARCH FOUNDATION — NO PRODUCTION PIXEL POLICY YET**

Date: 2026-09-24

Permanent law:

> **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

## Why this research exists

Traditional RAW pipelines usually begin with a camera raster and ask how to demosaic, white-balance, tone-map and encode that raster for a conventional image editor or display.

D.RAW should ask a different question:

> What is the strongest physically and perceptually coherent image representation we can produce from a sealed evidence source, while keeping every reconstructed, inferred, counterfactual and appearance-only contribution explicitly separate from measured evidence?

The final output pixel must therefore be treated as a **projection of a richer scene state**, not as the primitive object that defines the scene.

This research line develops three subjects together:

1. **continuous/subpixel image reconstruction**;
2. **physically based + deep VFX rendering**;
3. **human/display colour appearance**.

The synthesis target is a future D.RAW pixel-resolve contract that does not inherit the assumptions of Lightroom, a conventional demosaicer, or a fixed SDR/HDR raster pipeline.

## First architectural conclusion

The scientific and open-world model should be split into four domains:

```
SEALED EVIDENCE
    -> CONTINUOUS EVIDENCE-CONSTRAINED FIELD
    -> OPEN SCENE / DEEP LIGHT-TRANSPORT STATE
    -> VIEW + DISPLAY RESOLVE
    -> OUTPUT PIXEL
```

The output pixel is last.

It is not allowed to write authority back into any earlier domain.

## Existing project binding

This research must integrate with, not replace:

- immutable Direct-CFA evidence;
- Scientific Master;
- Open Scene Field v0.85;
- TruthNegative local authority projection;
- measured / calibrated-estimate / censored / reconstructed / unknown separation;
- appearance-only Natural HDR and detail paths;
- reversible restoration layers.

The most important conceptual change is that the Open Scene is no longer required to be a raster-only structure.

## Candidate future internal object

A future resolved pixel may need more than RGB:

```text
FreeWorldPixelSample
  spatial_footprint
  temporal_footprint
  spectral_or_tristimulus_value
  scene_linear_radiometric_state
  ray_or_depth_contributions[]
  alpha_or_transmittance
  source_support
  reconstruction_support
  uncertainty
  censor_bounds
  authority
  creation_role
  scene_state_hash
  viewing_condition_id
  display_target_id
```

Only the final view/display resolve produces encoded RGB values.

## Research gates before production

No production renderer or Scientific-Master mutation is permitted from this branch.

Promotion requires, at minimum:

- a continuous reconstruction operator with explicit sensor/optics footprint;
- deterministic provenance transport from source samples into continuous queries;
- an uncertainty rule that does not confuse smooth interpolation with new information;
- a deep/path contribution model with authority per contribution;
- a view/display transform that consumes explicit viewing conditions;
- reference tests proving raster resolution can change without changing scientific authority;
- reference tests proving display target can change without changing scene state.

See the three topic notes and the synthesis contract in this directory.

## Primary references

- M. Unser, *Sampling—50 Years After Shannon*, Proceedings of the IEEE 88(4), 2000.
- PBRT 4e, chapters on radiometry, spectra, sampling and reconstruction: https://pbr-book.org/4ed/
- OpenEXR deep-image concepts and deep sample theory: https://openexr.com/en/latest/
- CIE 248:2022, CIECAM16: https://www.cie.co.at/publications/cie-2016-colour-appearance-model-colour-management-systems-ciecam16
- ITU-R BT.2100-3: https://www.itu.int/rec/R-REC-BT.2100
- ACES 2 Output Transform documentation: https://docs.acescentral.com/system-components/output-transforms/
