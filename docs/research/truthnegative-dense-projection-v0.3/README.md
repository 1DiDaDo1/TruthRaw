# TruthNegative dense projection v0.3

First executable pixel implementation of the historical TruthNegative v0.2 dense-projection contract.

## Scope

For the Camera-5 evidence case:

- admitted source / Scientific Master domain: `4080x3072`
- dense target: `16320x12288`
- scale: exactly 4x per axis / 16x sample count
- target authority: `RECONSTRUCTED_DENSE_SUPPORT`
- measured target claims: **zero**
- physical frames: **one**
- independent evidence: **one**

The historical `16320x12288 / 401,080,320-byte` RAW_SENSOR envelope is not reclassified as 200 MP measured CFA. v0.19/v0.20 showed that only a `4080x3072 U16` prefix was populated. The 200 MP geometry is used only as a reconstructed target lattice.

## v0.3 interpolation

The initial implementation deliberately uses pixel-centre bilinear interpolation in Float64 and stores Float32 output.

Properties:

- local convex interpolation; no overshoot;
- negative and >1 Scientific-Master values remain legal;
- no sharpening, texture synthesis, AI hallucination or appearance transform;
- no new extrema are introduced by the interpolation;
- bounded tile-local source reads;
- deterministic canonical projected-raster digest.

This is intentionally conservative. A later optics-aware dense reconstruction may replace the interpolation only after independent PSF/MTF/calibration evidence exists and only under a new method/authority identifier.

## Relationship to the sealed house

`sealed CFA -> v4.7i camera-native Scientific Master -> TruthNegative dense projection`

The dense projection is a derivative representation. It does not replace or mutate the sealed source or Scientific Master.
