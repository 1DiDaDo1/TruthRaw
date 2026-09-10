# TruthRaw Uncertainty-Aware S-Curve / Confidence-Aware Color v1.0

Status: `RESEARCH CANDIDATE` until repository CI and promotion gates pass.

## Purpose
This is an **appearance-only** output transform. It may change display contrast and colorfulness, but it must never modify the Latent Scene Master, measurement evidence, uncertainty calibration, topology status, or physical-frame count.

The early TruthRaw S-curve idea is retained with stricter uncertainty semantics:
- dark + low luminance confidence -> flatter local tone slope, compressing visible uncertainty;
- reliable midtones -> steeper S-curve is allowed, increasing supported contrast;
- high chroma confidence -> bounded colorfulness gain is allowed;
- low/unknown chroma confidence -> positive chroma boost is prohibited;
- source-censored color support -> positive chroma boost is prohibited;
- arbitrary independent R/G/B tone curves are not admitted in v1.0.

## Mathematical boundary
For `y=f(x)`, first-order output uncertainty obeys `sigma_out ~= |f'(x)| sigma_in`. A smaller visible sigma does not mean the sensor measurement became physically more certain.

The chroma path uses a hue-preserving appearance transform plus gamut limiting. Caller-supplied chroma confidence controls colorfulness. This module does not manufacture that confidence.

## v0.7 integration
`xyz_uncertainty_adapter_v1` reads the closed v0.7 XYZ(D50) uncertainty contract: exact Y variance when known, otherwise the conservative Y variance upper bound, otherwise unknown. Unknown covariance is never replaced by independence.

## Claim boundary
`Representation can exceed the source. Knowledge claims cannot exceed the evidence.`

This module makes the photograph look stronger where evidence supports it and quieter where evidence is weak. It does not make weak evidence physically stronger.
