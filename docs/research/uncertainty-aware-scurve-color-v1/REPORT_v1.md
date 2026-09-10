# Research report — Uncertainty-Aware S-Curve / Confidence-Aware Color v1

## Decision
The early Lightroom-inspired S-curve concept is retained as an appearance-only transform with a reversed uncertainty rule for color: **uncertain chroma is restrained, not amplified**. Reliable chroma may be made modestly fuller.

The luminance S-curve is monotonic. Uncertainty-driven extra compression is restricted to shadows, preventing low-confidence midtones from receiving additional local contrast merely because they are uncertain.

## Real-dog prototype
The 094423 real-life TruthRaw dog prototype showed:
- low chroma-confidence fraction: 14.4455%;
- high chroma-confidence fraction: 76.1608%;
- low-confidence hue shift p95: 0.02798 degrees for the confidence-aware approach versus 0.34662 degrees for the deliberately unsafe control;
- dark/low-confidence high-frequency luminance proxy: 0.00072691 baseline -> 0.00044752 confidence-aware.

These metrics validate the **appearance concept**, not physical color accuracy. The prototype confidence map used a conservative source-NoiseProfile bound because complete frame-specific reconstructed-RGB covariance remains unresolved in those pixels. The production module therefore accepts qualified confidence from upstream instead of embedding that proxy.

## Claim boundary
The scientific Scene Master remains unchanged. A lower output noise proxy means visible variation was compressed by the rendering transfer function; it does not mean sensor uncertainty or photon noise was reduced.
