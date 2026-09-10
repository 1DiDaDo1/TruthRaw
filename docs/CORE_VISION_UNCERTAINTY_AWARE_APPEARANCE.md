# TruthRaw Core Vision Extension — Uncertainty-Aware Appearance

The scientific master remains scene-linear and immutable to appearance operations.

Output ordering:

`Latent / Colorimetric Scene Master`
`-> uncertainty + support read-only inputs`
`-> Uncertainty-Aware S-Curve`
`-> Confidence-Aware Colorfulness`
`-> gamut management`
`-> SDR / HDR encoding`

The tone curve may reduce **visible** uncertainty where its local slope is below one and increase supported contrast where evidence is strong. This never changes capture SNR or measurement confidence.

Color rule:
- high chroma confidence: bounded colorfulness increase is permitted;
- low/unknown chroma confidence: no positive colorfulness boost;
- censored color support: no positive colorfulness boost;
- v1.0 does not allow arbitrary independent RGB curves.

This is an appearance policy, not a reconstruction prior.
