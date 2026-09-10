# TruthRaw Core Vision Extension — Uncertainty-Aware Appearance

Status: **RESEARCH ARCHITECTURE EXTENSION**

TruthRaw keeps scientific scene inference and photographic appearance separate.

Current ordering:

`Immutable Evidence -> Measurement Domain -> Latent Camera Scene -> Colorimetric Scene Master -> Virtual Observation/conditioning -> display preparation -> Uncertainty-Aware Appearance -> SDR/HDR encoding`

The Uncertainty-Aware Appearance layer may use upstream uncertainty, censoring and topology confidence to decide how strongly an output curve may reveal contrast or chroma. It may not write those appearance choices back into CFA evidence, latent scene values, uncertainty, covariance, topology certification or physical-color claims.

The first module is `Uncertainty-Aware S-Curve / Confidence-Aware Color v1`.

Binding principles:
- low-confidence tonal regions may be visually compressed rather than falsely sharpened;
- low-confidence chroma is coupled/restrained, not made more certain by saturation;
- high-confidence chroma may receive modest appearance-only colorfulness enhancement;
- source-censored chroma cannot be promoted as recovered color truth;
- visible noise compression is not uncertainty reduction;
- all transforms remain downstream of the authoritative scene master.
