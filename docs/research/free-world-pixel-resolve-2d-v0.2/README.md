# FREE_WORLD_PIXEL_RESOLVE_2D_REFERENCE v0.2

Status: **EXECUTABLE RESEARCH REFERENCE — NOT PRODUCTION-PROMOTED**

This is the first executable step after the D.RAW Free-World Output Pixel v0.1 research contract.

## What it does

The reference treats the current camera-plane scene as samples of a continuous piecewise-bilinear field and resolves arbitrary target raster pixels as **area integrals**, not only centre samples.

For each target pixel it emits:

- scene-linear RGB in Float64;
- the exact source-pixel footprint and normalized positive weights;
- calibrated-estimate / reconstructed / censored / unknown support fractions per channel;
- conservative authority resolve;
- weighted p95 uncertainty when every contributor has known uncertainty;
- fixed single-frame/evidence counts;
- zero measured target claims.

No display transfer, tone map, gamut map, sharpening, semantic synthesis, neural prior or 3D inference occurs here.

## Continuous field convention

Source sample centres are at integer coordinates.

The finite source image domain extends half a sample outside the first and last centres. Inside the source-centre lattice the field is separable piecewise linear (bilinear in 2D). From the outer sample centre to the half-sample image boundary, the field is constant.

A target pixel footprint is mapped from its target raster cell into that source domain and integrated analytically with separable 1D basis integrals.

This yields normalized, non-negative weights whose 2D products sum to one.

## Why area integration matters

A target pixel represents a finite footprint.

For source field `F` and target footprint `A_j`:

```text
P_j = (1 / |A_j|) * integral_(A_j) F(x,y) dA
```

A 1 MP, 12 MP, 50 MP or 200 MP raster can therefore be resolved from the same field without redefining the scene state.

Raster density changes representation density, not evidence authority.

## Authority policy

The numerical target is always a derived projection. It is never labelled measured.

For each channel:

- any positive UNKNOWN support -> output authority UNKNOWN;
- otherwise any positive CENSORED support -> output authority CENSORED;
- otherwise -> output authority RECONSTRUCTED.

The source support fractions remain available separately, so a reconstructed target can still report how much calibrated direct support contributed.

## Uncertainty

When every contributing source channel has admitted p95 uncertainty, v0.2 propagates a positive-weight conservative linear mixture:

```text
u_out = sum_i w_i * u_i
```

This is not yet a covariance-aware uncertainty model. If any contributing uncertainty is unknown, target uncertainty is marked unknown rather than silently filled.

## Display separation

`bindOutputIntent()` records scene-state, appearance-model and display-target identifiers but explicitly sets:

`scientificSceneMutationAllowed=false`

The resolver itself receives no display transform and cannot alter scene values based on an SDR/HDR target.

## Tests

The reference test proves:

- axis weights are finite, positive, bounded and normalized;
- constant extended-range fields survive arbitrary output raster density;
- an affine interior field integrates to the expected centre value;
- UNKNOWN and CENSORED support fail closed;
- uncertainty is propagated without authority promotion;
- 0.5x / 1x / 4x raster density never upgrades target authority;
- changing display intent cannot change scene-linear resolve;
- negative and >1 scene-linear values remain legal.

## Deliberate limits

v0.2 does **not** yet model:

- measured PSF/MTF;
- CFA-native reconstruction directly from Direct CFA;
- spatial covariance;
- spectral reconstruction;
- depth/deep samples;
- geometry/material/illumination inference;
- physical relighting;
- CAM16/ACES-style appearance rendering.

Those belong to later gates.

The next research step is to bind this generic scene-plane interface to the existing Scientific Master + Open Scene Field without copying authority rules into a second inconsistent implementation.
