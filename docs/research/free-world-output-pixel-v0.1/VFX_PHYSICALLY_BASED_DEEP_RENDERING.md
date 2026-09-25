# VFX Physically Based / Deep Rendering

Status: **RESEARCH MODEL**

## 1. Why VFX is relevant to D.RAW

Modern VFX does not need to treat a pixel as a single flat RGB triplet.

A physically based renderer starts from a scene description and light transport. Deep compositing can preserve multiple depth-associated samples along one image-space pixel.

This is useful to D.RAW because the Open Scene can become richer than a flattened photograph while the original evidence stays sealed.

## 2. Physical light transport is not RGB enhancement

The physically meaningful quantity arriving at the camera is radiance.

For a surface, outgoing radiance can be modelled through the rendering equation:

```text
L_o(x, wo, lambda) =
  L_e(x, wo, lambda)
  + integral_hemisphere
      f_r(x, wi, wo, lambda)
      L_i(x, wi, lambda)
      cos(theta_i)
    dwi
```

A useful Open Scene representation therefore needs conceptual room for:

- geometry / depth;
- normals;
- visibility;
- material response / BSDF;
- emission;
- participating media;
- illumination;
- wavelength dependence;
- uncertainty and authority for every one of those properties.

Multiplying RGB cannot substitute for this model.

## 3. Spectral-first lesson

PBRT 4e performs its lighting calculations with wavelength-dependent spectral samples rather than RGB lighting values.

D.RAW should not immediately conclude that a three-channel camera can recover a full physical spectrum. It generally cannot.

Instead, the architectural lesson is:

- physical transport is spectral;
- camera evidence may constrain only a lower-dimensional projection;
- a spectral scene hypothesis may exist, but its authority must reflect metameric ambiguity and any priors used.

A spectral hypothesis can improve a renderer without becoming spectral measurement.

## 4. Deep representation

OpenEXR deep images permit each 2D pixel to contain a variable number of depth-associated samples.

This suggests a D.RAW **Deep Scene Contribution** abstraction:

```text
RayContribution
  z_front
  z_back
  radiance_or_colour
  transmittance_or_alpha
  object_or_region_id
  material_id_optional
  authority
  uncertainty
  provenance
```

Multiple contributions may exist for:

- foreground edge + background;
- glass/transparency;
- hair/fur;
- fog/smoke;
- reflections represented as inferred paths;
- restoration layers;
- counterfactual inserted scene content.

## 5. Important boundary: deep image is not full physics

OpenEXR deep data is a storage/compositing model, not a complete path tracer or scene ontology.

A deep sample can help preserve occlusion/transmission structure, but it does not automatically encode:

- BRDF;
- incoming illumination;
- multiple scattering;
- complete geometry;
- spectral reflectance;
- causal light paths.

D.RAW should borrow the **multi-contribution pixel model**, not confuse it with complete physical truth.

## 6. Path contribution model

For a future physically based Open Scene renderer, a resolved camera ray may conceptually be:

```text
R(ray, lambda) =
  sum_over_admitted_paths
    throughput(path, lambda)
    * emitted_or_reflected_radiance(path, lambda)
```

Each path or aggregate contribution should be authority-typed.

Examples:

- source-constrained visible-surface contribution -> calibrated estimate / reconstructed;
- inferred hidden geometry -> inferred;
- restored missing object -> hypothetical restoration;
- user relighting -> counterfactual;
- display flare/bloom -> appearance-only.

## 7. Inverse rendering

Reconstructing geometry, material and lighting from photographs is an inverse problem and is generally non-unique.

Differentiable rendering can optimize scene parameters against image observations, but convergence to a visually matching scene is not proof that the recovered hidden scene parameters are unique or measured.

This maps naturally to D.RAW:

```text
measured image agreement != measured hidden scene
```

The optimization residual is evidence about image agreement, not automatic authority for geometry/material/light.

## 8. Neural radiance fields and Gaussian splats

NeRF represents a scene as a continuous volumetric function queried by spatial position and view direction, returning density and view-dependent radiance.

3D Gaussian Splatting represents a scene using anisotropic 3D Gaussian primitives and a visibility-aware renderer.

Both demonstrate that a scene representation can be **continuous and non-raster-first**.

For D.RAW they are architectural references, not immediate scientific authorities.

A single-frame D.RAW source does not have the multi-view constraints typically used to construct such scene representations. A single-image learned scene field is therefore highly prior-dependent and must remain inferred/hypothetical unless independently constrained.

## 9. Proposed Open Scene separation

Future state should separate:

```text
Evidence-Constrained Visible Field
Inferred Scene Geometry
Inferred Material Field
Illumination Field
Deep/Ray Contributions
Counterfactual Scene Layer
Appearance Layer
```

The renderer can combine them, but the combined beauty image must remain decomposable back to its contribution classes.

## 10. The future output pixel

The final pixel should result from:

1. selecting a camera/view ray bundle for the target pixel footprint;
2. evaluating admitted surface/volume/path contributions;
3. integrating spectral/tristimulus radiance over that footprint;
4. retaining authority/provenance summaries;
5. only then performing appearance/display rendering.

This is fundamentally different from “take RGB pixel and enhance it.”

## References

- PBRT 4e Introduction: https://www.pbr-book.org/4ed/Introduction
- PBRT 4e Radiometry, Spectra and Color: https://pbr-book.org/4ed/Radiometry%2C_Spectra%2C_and_Color
- OpenEXR deep pixel interpretation: https://openexr.com/en/latest/InterpretingDeepPixels.html
- OpenEXR deep sample theory: https://openexr.com/en/latest/TheoryDeepPixels.html
- NeRF: https://arxiv.org/abs/2003.08934
- 3D Gaussian Splatting: https://arxiv.org/abs/2308.04079
- Differentiable Rendering survey: https://arxiv.org/abs/2006.12057
