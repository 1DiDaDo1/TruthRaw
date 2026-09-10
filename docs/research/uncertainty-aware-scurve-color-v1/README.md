# TruthRaw Uncertainty-Aware S-Curve / Confidence-Aware Color v1

Status: **RESEARCH IMPLEMENTATION — APPEARANCE ONLY**

This module restores the early TruthRaw S-curve/RGB-channel idea inside the current architecture without contaminating the scientific master.

## Position in the pipeline

`Latent/Colorimetric Scene Master -> display-space exposure/shoulder/gamut preparation -> this module -> SDR/HDR transfer/encoding`

The input to this module is non-negative normalized **linear display RGB**. It is not raw CFA and not the authoritative scene master.

## Rules

1. A global monotonic luminance S-curve controls photographic contrast.
2. Low luma-confidence may add extra compression **only in the shadow zone**; it is not allowed to create extra midtone microcontrast.
3. Low chroma-confidence couples RGB more strongly and restrains colorfulness.
4. High chroma-confidence may receive modest appearance-only colorfulness enhancement.
5. Source-censored support cannot receive positive chroma enhancement.
6. Confidence is supplied by upstream TruthRaw uncertainty/topology logic. This module does not infer confidence from RGB appearance.
7. Invalid inputs/configuration fail closed to identity.
8. `scientificMasterModified` is always false.

No physical-color, spectral-recovery, denoising, additional-photon, or additional-evidence claim is made.
