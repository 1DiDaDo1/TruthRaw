# Manifold Conditioning v1 — fact-check and design corrections

## Retained from TruthRaw

1. Virtual EV/gain is a deterministic view of one evidence root, not an independent capture.
2. Exposure/gain scaling must transform uncertainty consistently; scalar covariance scales by the square of the scalar.
3. The TruthRange zero-line is a scene gauge, not sensor BlackLevel or display black.
4. The legacy Multi-EV reconstruction-modification candidate remains rejected.

## Mathematical correction

A positive invertible scalar reparameterization cannot by itself create information or improve physical SNR. If both mean and standard deviation are multiplied by the same positive scale, their ratio is unchanged. If likelihood coordinates are transformed correctly, normalized residuals are likewise unchanged. Therefore “Best Observation” is renamed operationally to **Best Conditioning Gauge**.

For binary floating-point, integer power-of-two scaling is preferred in this exact path. `std::scalbn` changes the exponent without introducing the general rounding associated with arbitrary multiplication, provided the operation stays in a representable non-destructive range. The implementation rejects destructive underflow/overflow and tests bit-exact round trips over a broad normal-range fixture set. A later broad IEEE-754 fuzz test showed that extreme/subnormal combinations still require an explicit round-trip-safety check; v1 therefore retreats the requested gauge toward EV=0 until exact round-trip safety is proven.

## RAW-noise fact-check

Foi et al. (IEEE TIP 2008, DOI 10.1109/TIP.2008.2001399) model RAW sensor noise with a signal-dependent Poissonian component plus a stationary Gaussian component and explicitly account for clipping. This supports keeping noise likelihood tied to the source/noise model rather than pretending virtual EV creates lower sensor noise.

Mäkitalo & Foi (IEEE TIP 2013, DOI 10.1109/TIP.2012.2202675) show that a generalized Anscombe variance-stabilizing transform can make Poisson–Gaussian noise approximately homoscedastic for a denoising stage, with careful inverse transformation. That is a legitimate separate conditioning/estimation research direction, but it is nonlinear and must not be conflated with exact Virtual-EV reparameterization.

## Appearance boundary

Spatially varying tone/exposure operations can create boundary artifacts if naively blended. Edge-aware multiscale tone mapping such as Local Laplacian Filters (Paris, Hasinoff, Kautz, SIGGRAPH 2011 / CACM 2015) demonstrates a principled family of halo-resistant appearance operations. TruthRaw should keep any Best-Observation-driven tone adaptation in the appearance renderer, spatially regularized and outside the scientific master.

## Open claims intentionally not solved here

- physical electron-domain PTC / conversion gain / full-well truth;
- physical virtual-ISO forward behavior;
- co-sited missing-channel topology certification;
- frame-specific full RGB covariance where off-diagonals are unresolved;
- L2/L3 color and spectral/illuminant truth;
- optical PSF/MTF/SFR/flare calibration;
- any claim that an EV manifold itself removes physical sensor noise.
