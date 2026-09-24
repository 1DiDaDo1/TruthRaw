# Free-World Output Pixel Contract v0.1

Status: **DRAFT RESEARCH CONTRACT**

This is the first synthesis of the three research tracks.

It is intentionally not a production algorithm.

## 1. Core rule

A D.RAW output pixel is not stored scene truth.

It is a **view-dependent integral/projection of an authority-typed scene representation**.

## 2. Proposed resolution equation

For target pixel `j`:

```text
SceneSample_j(lambda)
  =
  integral_(pixel footprint)
  integral_(ray/depth/path domain)
      R_open_scene(x, y, path, lambda)
      * W_spatial_j(x, y)
      * W_path(path)
  dpath dA
```

Then:

```text
Colorimetric_j
  = spectral_or_camera_bound_projection(SceneSample_j)

Appearance_j
  = ViewTransform(Colorimetric_j, ViewAppearanceState)

EncodedPixel_j
  = DisplayEncode(Appearance_j, DisplayTarget)
```

The exact implementation may use RGB/tristimulus approximations when spectral information is unavailable, but the layer boundaries remain.

## 3. Parallel authority resolve

The numeric output is accompanied by an authority summary:

```text
AuthorityResolve_j
  = combine(
      all scene contributions,
      source footprints,
      reconstruction roles,
      uncertainty,
      censor bounds
    )
```

The display transform is forbidden from increasing this authority.

## 4. Required per-pixel semantic channels

A future native output packet should be able to answer:

- What numeric radiance/color value was rendered?
- Which sealed source samples constrain it?
- Which channels/regions were reconstructed?
- Which content is inferred?
- Which content is counterfactual?
- Was any source support censored?
- What uncertainty class applies?
- Which view/display transform produced the encoded RGB?
- What exact scene-state and output-policy hashes produced it?

## 5. Output classes

At minimum:

### A. SCIENTIFIC_VIEW

Only admitted evidence/calibrated reconstruction contributes to numeric scene content.

Unknown/inferred/hypothetical content is excluded or explicitly masked.

### B. OPEN_SCENE_VIEW

May include admitted reconstructed and inferred scene content.

Every contribution remains authority-typed.

### C. RESTORATION_VIEW

May include reversible restoration hypotheses.

No restoration layer is allowed to replace source evidence.

### D. COUNTERFACTUAL_RENDER

May change illumination, materials, camera/view or scene content.

It is explicitly not a scientific observation.

### E. APPEARANCE_VIEW

May change tone, colourfulness, gamut mapping, local contrast, display adaptation and sharpening/acutance.

It cannot alter scene authority.

## 6. Pixel density independence

The same scene state may be resolved to any target lattice.

```text
scene_hash same
authority state same
target dimensions different
encoded pixels different
```

This is expected and correct.

## 7. Display independence

The same scene state may be rendered to multiple display targets.

```text
scene_hash same
view intent related
display target different
output encoding different
```

Again, this must not modify the source or Scientific Master.

## 8. First implementation prototype

The next prototype should implement only a bounded subset:

1. continuous 2D scene field over the current reconstructed Scientific-Master camera plane;
2. explicit area-integrated target pixel resolve;
3. per-target-pixel provenance footprint;
4. Float64 accumulation;
5. separate view/display policy object;
6. scene-linear Float32/Float64 research output before display encoding;
7. tests at multiple raster sizes proving authority invariance.

No inferred 3D geometry, neural field or physical relighting is required for the first prototype.

That first prototype establishes the pixel contract before the Open Scene becomes fully 3D/deep.

## 9. Non-negotiable invariants

- Direct-CFA source remains immutable.
- Single-frame evidence remains single-frame.
- Interpolation never creates new measured samples.
- Spectral hypotheses never become spectral measurement without calibration/evidence.
- Deep samples never become geometry truth merely because they render correctly.
- Appearance never writes back into Scientific Master.
- Higher raster density never upgrades optical/detail authority.
- A display transfer function never upgrades scene dynamic-range evidence.
- Every counterfactual contribution remains counterfactual.

## 10. Research milestone

The next milestone is:

`FREE_WORLD_PIXEL_RESOLVE_2D_REFERENCE_v0.2`

It should be a deterministic CPU reference that turns the current scientific camera-plane field into arbitrary target raster sizes using declared area integration, while emitting per-pixel authority/provenance summaries.

Only after that reference exists should D.RAW add deep 3D/path contributions.
