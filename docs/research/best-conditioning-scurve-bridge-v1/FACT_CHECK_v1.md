# Fact check — Best Conditioning → S-Curve Bridge v1

## CLOSED at the representation/math level

- Multiplying both signal and standard deviation by the same positive power-of-two gain leaves SNR invariant.
- Covariance/variance must scale by gain squared; a gauge change cannot erase uncertainty.
- Best Conditioning EV therefore means **numerically favorable coordinates**, not more captured information.
- TruthRange conditioning shifts are gauge changes only; the stored/global zero-line reference is not redefined.
- Virtual EV/ISO views remain one evidence root and are not independent measurements.
- Appearance must operate after deconditioning and cannot rewrite sealed RAW or the scientific Scene Master.

## Corrected optimization

The earlier per-pixel luma-confidence S-curve can make equal scene luminance map to different output luminance when confidence changes abruptly. That is an appearance artifact risk, not a sensor-truth problem.

Bridge v1 moves uncertainty-adaptive luma suppression to an edge-aware local residual around a common global tone curve. A constant field has zero residual, so a confidence discontinuity alone produces zero new luminance edge in the synthetic falsification fixture.

## External technical consistency

- Mixed signal-dependent image noise is commonly modeled with Poisson-Gaussian statistics; a variance-stabilizing transform can be useful, but unbiased inversion matters, particularly at low counts. See Mäkitalo & Foi, IEEE TIP 2013, DOI 10.1109/TIP.2012.2202675.
- Edge-preserving filtering is a principled way to prevent a local base layer from leaking across strong image edges. Guided filtering (He, Sun & Tang, ECCV 2010 / TPAMI 2013) and bilateral-filter tone-mapping literature support this design class.
- These references support the mathematics/design class; they do not validate TruthRaw's particular parameter values or prove physical sensor-noise removal.

## Still OPEN

- The confidence fields used by the final product must come from qualified upstream uncertainty/topology evidence, not visual appearance heuristics.
- The real 094423 bridge probe uses a diagnostic confidence proxy only; it is not canonical uncertainty calibration.
- The O(radius^2) reference filter is intentionally simple and not yet a mobile-performance design. A guided/constant-time edge-aware implementation can be studied later under exact/parity gates.
- Broader real scenes, low-light color, clipping/censor boundaries, hue edges, HDR target behavior and mobile performance remain open.
