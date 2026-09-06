# TruthRaw v4.7k — Output Acutance

Purpose: recover a small, validated amount of perceived fine detail lost by the **final output resize**, without changing the scientific master or pretending to recover missing optical information.

## Rules
- Runs after final SDR resize, before final OETF/encoding.
- Luminance-only: RGB direction/chromaticity is preserved.
- Strength is driven by resize ratio + NoiseProfile-derived confidence.
- Strong edges, deep shadows and near-highlights are protected.
- Noise-aware soft threshold suppresses flat-field noise amplification.
- No skin/semantic segmentation.
- If HDR/Ultra HDR is exported, the gain map must be derived/recomputed against the **final acutance-adjusted SDR base**.

## Validation headline
- Synthetic fine-detail gain: ~4.5%.
- Hard-edge halo: <1% overshoot of the test contrast step.
- Flat-noise amplification: ~1.031x low-noise, ~1.022x mid-noise, ~1.003x high-noise.
- Python/C++ max error: 1.19e-7.
- Real scenes: 14/14 gates PASS.
- People skin-ROI median output-acutance gradient change: ~+0.31%; p95 ~+0.43%.

## Current limitation
The GLSL compute shader is written as a semantic reference but has **not** been compiled to SPIR-V or tested on Adreno in this environment.
