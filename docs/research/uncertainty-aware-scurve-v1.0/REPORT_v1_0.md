# Uncertainty-Aware S-Curve / Confidence-Aware Color v1.0 — research report

## Decision
The early S-curve concept is retained as an appearance-layer mechanism, with one reversal: **uncertain chroma is restrained, not amplified**. Reliable chroma may be made fuller; unreliable chroma remains coupled toward luminance behavior.

## Real dog test
Source: `IMG_BNC_TRUTHRAW20260907_094423_122.dng` and its existing v0.7 Scene Master. The scientific master was not changed.

The test uses a conservative source-bound proxy for display luminance uncertainty: source DNG `NoiseProfile`, the exact Stage-2 GainMap dump, global maximum gain for reconstructed-channel safety, and a correlation-agnostic upper bound for linear combinations. This is **appearance validation evidence**, not a new calibrated RGB covariance model.

Measured result on the 1600px preview:
- low-confidence shadow median `sigma_out/sigma_in`: ~0.565;
- low-confidence shadow p95 `sigma_out/sigma_in`: ~0.581;
- reliable midtone median local tone slope: ~1.168;
- reliable midtone p95 local tone slope: ~1.377;
- safe color transform hue shift versus tone-only, p95: ~0.000068 degrees;
- deliberately unsafe independent per-channel curve hue shift versus tone-only, p95: ~2.29 degrees;
- unsafe low-chroma-confidence hue shift p95: ~3.11 degrees.

Interpretation: the tonal part can produce the desired stronger/cleaner appearance by reducing visible noise slope in weak shadows while increasing contrast slope in supported regions. Independent RGB curves are not safe enough as the default mechanism. Confidence-gated hue-preserving colorfulness is the accepted v1.0 direction.

## Non-claims
- no denoising/SNR gain is claimed from the curve;
- no scene truth changes;
- no recovered color is claimed in censored regions;
- the real-dog uncertainty proxy is not a replacement for full per-pixel calibrated covariance;
- no perceptual preference result is universal from one scene.
