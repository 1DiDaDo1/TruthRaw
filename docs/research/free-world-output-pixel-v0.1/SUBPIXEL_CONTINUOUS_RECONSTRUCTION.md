# Continuous / Subpixel Reconstruction

Status: **RESEARCH MODEL**

## 1. The pixel grid is not the scene

A sensor value is not an infinitesimal point sample of the world.

A simplified measurement model is:

```text
y_i =
  integral_space
  integral_time
  integral_wavelength
    L_scene(x, y, lambda, t)
    * H_optics_sensor_i(x, y, lambda, t)
    * S_cfa_i(lambda)
  d lambda d t dA
  + noise_i
```

where:

- `L_scene` is scene radiance reaching the camera;
- `H_optics_sensor_i` includes optical PSF, pixel aperture, microlens, motion and readout footprint;
- `S_cfa_i` is channel spectral sensitivity;
- `y_i` is the recorded sensor sample.

This is already an integration over a finite footprint.

Therefore a correct continuous reconstruction model should not start by pretending that each RAW code is a point RGB value at the centre of a square.

## 2. Continuous reconstruction is an estimate, not extra measurement

Sampling theory allows a discrete set of samples to define or estimate a continuous function under assumptions about the function space and acquisition operator.

For ideal band-limited sampling, sinc reconstruction is the classical result. Practical image reconstruction usually works in broader shift-invariant function spaces such as splines or other reconstruction bases.

D.RAW should use the term **continuous reconstruction** rather than “new subpixels” when no independent samples exist.

A continuous query at `(x,y)` can have:

- a numeric value;
- source footprint weights;
- support bandwidth;
- uncertainty;
- authority.

It does **not** become measured simply because it exists between sensor centres.

## 3. Required separation: addressability, optical support, reconstruction support

For every spatial frequency or local detail claim, keep separate:

1. **sample-grid addressability**;
2. **optical transfer support** (PSF/MTF/SFR);
3. **sensor/CFA support**;
4. **reconstruction prior support**;
5. **appearance/acutance**.

An output raster can be arbitrarily dense while optical/evidence bandwidth remains fixed.

This is the correct interpretation of a future very-high-resolution D.RAW raster.

## 4. Proposed Continuous Evidence-Constrained Field

Define a conceptual field:

```text
F(x, y, lambda_or_channel)
```

or, for a tristimulus working form:

```text
F(x, y) -> {X, Y, Z, covariance, provenance}
```

Every query returns:

```text
value
authority
creation_role
uncertainty
censor_state
source_footprint
effective_bandwidth
calibration_domain
```

The source footprint is essential. It tells us which measured samples actually constrain that query.

## 5. Reconstruction operator

The first reference implementation should be linear and inspectable before any learned model:

```text
F_hat(x) = sum_i w_i(x) * z_i
```

with:

- weights derived from a declared reconstruction kernel/operator;
- explicit handling of CFA geometry;
- optional PSF/MTF-aware regularisation;
- no hidden semantic prior;
- a deterministic footprint/provenance map.

Later operators may be nonlinear, edge-aware, Bayesian or learned, but must still return provenance and uncertainty.

## 6. Why this differs from ordinary resizing

A resize consumes an already reconstructed raster.

A D.RAW continuous field should instead bind directly to:

- source CFA geometry;
- reconstruction state;
- optics/calibration state where admitted;
- censoring state;
- uncertainty.

The target raster is then sampled/integrated from the field.

This removes the false idea that a 4x target lattice is itself the reconstruction.

## 7. Output pixel as an area integral

A target output pixel should generally be treated as an integral over its footprint:

```text
P_j =
  (1 / A_j) *
  integral_(target pixel footprint)
    F_hat(x, y) * K_j(x, y)
  dA
```

where `K_j` is a declared reconstruction/resampling filter.

This is more physically and mathematically coherent than evaluating only at the pixel centre.

For very small target pixels, the target sample may approach a point evaluation, but the scientific support does not increase merely because the footprint is smaller.

## 8. Anti-aliasing and scale

The same continuous field should be able to produce:

- a 1 MP preview;
- a 12 MP image;
- a 50 MP image;
- a 200 MP image;

without changing the underlying scientific scene state.

Only the output integration footprint changes.

This becomes a major D.RAW invariant:

> **Changing output raster density must not silently change evidence authority.**

## 9. Single-frame subpixel limit

A single frame can support subpixel localisation of some structures under a known forward model because a finite optical footprint influences neighbouring samples.

It cannot generally create unlimited independent high-frequency scene information.

Any super-resolution claim beyond admitted optical/sensor support must therefore be typed as reconstructed or inferred, with uncertainty.

## 10. Research candidates

Evaluate in this order:

1. area-aware separable spline reconstruction;
2. windowed-sinc / Lanczos reference;
3. PSF/MTF-aware regularised inversion;
4. edge-aware continuous field;
5. Bayesian continuous field with covariance;
6. optional learned implicit field, only with explicit authority constraints.

Learned implicit fields must never be allowed to silently turn semantic plausibility into measured structure.

## References

- Unser, *Sampling—50 Years After Shannon*: https://bigwww.epfl.ch/publications/unser0001/
- PBRT 4e Sampling and Reconstruction: https://www.pbr-book.org/4ed/Sampling_and_Reconstruction
- PBRT Image Reconstruction: https://www.pbr-book.org/4ed/Sampling_and_Reconstruction/Image_Reconstruction
