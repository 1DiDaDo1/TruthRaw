# TruthNegative Optics Support v0.7

Status: **EXECUTABLE CALIBRATION GATE — NO DECONVOLUTION YET**

This is the deliberately conservative optics step after the v0.6 round-trip oracle.

## Purpose

A target pixel is already explained by a continuous source footprint. Real optics broaden the acquisition support through the lens/sensor PSF.

v0.7 binds a declared PSF/MTF calibration to that footprint so D.RAW can say not only which source samples contributed numerically, but also which neighbouring source region the admitted optical calibration says belongs to the effective support.

## Scientific gate

A calibration may be:

- MEASURED;
- CALIBRATED_ESTIMATE;
- INFERRED;
- UNKNOWN.

Only MEASURED and CALIBRATED_ESTIMATE calibrations may be used for **scientific support propagation**.

INFERRED optics can exist as research hypotheses but fail closed when requested for scientific support.

## Important limitation

v0.7 does **not** sharpen, deconvolve or invent detail.

It reports:

- `numericSceneValueChanged = false`;
- `authorityUpgraded = false`;
- `deconvolutionApplied = false`;
- `createsNewEvidence = false`;
- `scientificWritebackAllowed = false`.

This is intentional. Before inverse optics are allowed, D.RAW first needs real device/lens calibration evidence.

## Calibration identity

The calibration SHA-256 binds:

- sealed source identity;
- lens identity;
- sensor identity;
- calibration-evidence identity;
- authority class;
- normalized PSF kernel;
- MTF50 X/Y.

Changing PSF or MTF changes the calibration identity.

## Border handling

PSF support that falls outside the finite sensor raster is clamped to the nearest valid source coordinate and duplicate contributions are merged. The final effective support remains positive and normalized.

## Next gate

Only after measured/calibrated PSF/MTF is available should an inverse-optics reconstruction reference be built. That later stage must compare reconstructed frequency support against calibration limits and may never relabel prior-driven high frequencies as measured detail.
