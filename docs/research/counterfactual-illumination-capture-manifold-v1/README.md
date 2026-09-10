# TruthRaw Counterfactual Illumination & Capture Manifold (CICM) v1

CICM adds a strictly counterfactual world/capture layer without rewriting the sealed RAW, admitted Scene Master, TruthRange zero-line, uncertainty ledger, or evidence count.

## Three semantics

1. `RELATIVE_RADIANCE_SCALE_ONLY` — implemented. A neutral scalar world and relative shutter scale. Produces relative counterfactual radiance/exposure only; physical SNR is deliberately unavailable.
2. `CALIBRATED_NEUTRAL_ILLUMINATION_FORWARD` — implemented fail-closed. Computes expected electrons, temporal Poisson+dark+read variance, electron-domain SNR, pre-clip DN and censor/headroom only when a complete exact-bound sensor-mode calibration is supplied. `EXPLICIT_RESEARCH_FIXTURE` may exercise the math but cannot authorize a physical camera claim or physical Best Capture selection.
3. `GEOMETRY_BRDF_SPECTRAL_RELIGHT_RESERVED` — deliberately not implemented in v1. Real sun/night relighting requires geometry/normals/visibility, material BRDF, spatial illumination and spectrum. Missing state cannot be replaced by a brightness slider and called physical truth.

## Evidence invariant

Counterfactual observations are simulated hypotheses, never new evidence for the captured world. `physicalFrameCount=1`, `independentEvidenceCount=1`, `counterfactualObservationsAreEvidence=false`, `scientificMasterModified=false`.

## ISO/shutter semantics

Shutter in calibrated counterfactual capture changes hypothetical collected charge and therefore hypothetical SNR. Nominal ISO is bound to a separately calibrated sensor mode; arbitrary gain is not allowed to masquerade as a physical ISO model. With equal electron-domain noise parameters, downstream DN gain changes DN scale but not electron-domain SNR. Separate ISO modes may legitimately differ only when separately calibrated parameters say they do.

## Zero-line

The original TruthRange `L0` is never modified. CICM may report a counterfactual relative EV difference, but that is a world/capture comparison coordinate, not a redefinition of the scientific zero-line.
