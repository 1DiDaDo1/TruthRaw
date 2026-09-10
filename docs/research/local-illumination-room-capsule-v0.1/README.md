# TruthRaw Local Illumination Room / Room Capsule v0.1

Purpose: simulate light/dark only inside the photographer-selected local domain instead of constructing a relightable model of the entire scene.

## Core architecture

`sealed evidence -> scientific Scene Master -> room selection -> compact Room Capsule -> tile-local counterfactual lighting -> appearance output`

The scientific master remains global and immutable. The Room Capsule is disposable/reproducible derived state.

## Mobile-first contract

- CPU/C++ is the mandatory baseline. GPU acceleration is optional, never required for correctness.
- Full-resolution output is processed in bounded tiles; image megapixels do not determine tile workspace size.
- Geometry is stored only for the selected room and is adaptively downsampled (1/4, 1/8, 1/16, 1/32, 1/64) to fit a small working-set budget.
- Default persistent sample is exactly 8 bytes: quantized depth, octahedral normal, visibility, confidence, roughness, flags.
- The room boundary is vector/polygon-first rather than a full-resolution bitmap mask.
- Arbitrarily many lighting variants store descriptors only; rendered copies are not part of the capsule.
- The rest of the scene is represented only by a compact Boundary Illumination Envelope where possible.

## Claim boundary

v0.1 computes `RelativeAppearanceOnly` local illumination. It does **not** infer intrinsic reflectance from one photograph and therefore does not claim physically exact relighting. A future calibrated intrinsic/BRDF/spectral route is reserved and fails closed today.

Counterfactual light states are not independent evidence and never modify the sealed source, the scientific master or TruthRange zero line.

## Bound dependency

Room Capsule v0.1 is built against CICM v1 as promoted on `main` at commit `71dcd031b309658ef39b99b6c2b46031f09165d9` (tree `5c645005b82195659360b2ade6cebb5e16c1143a`). The Room Capsule CI reruns CICM evidence-integrity before any Room Capsule compiler or integrity gate is accepted.
