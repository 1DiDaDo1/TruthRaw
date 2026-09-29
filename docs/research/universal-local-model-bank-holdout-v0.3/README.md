# D.RAW Universal Local Model Bank Holdout v0.3

Date: 2026-09-29

Status: research-only selector successor above v0.2.

## Why v0.3 exists

The paired v0.1/v0.2 device result established two separate facts:

- v0.2 candidate geometry improved: DIRECTIONAL_STRIP_LINE no longer collapsed to AFFINE_PLANE, and the v0.2 post-reveal oracle was stronger than the v0.1 oracle;
- v0.2 target-blind selection regressed: selected MAE/RMSE and selector regret were worse than v0.1 on the paired source.

The most direct selector-level cause is that v0.2 evaluates candidates on held-out surrounding validation support and then applies an additional BIC-like complexity penalty. Cross-validation already measures out-of-fit predictive error, so that second penalty can suppress useful affine/quadratic candidates twice.

v0.3 therefore freezes every v0.2 candidate fit and changes only cross-family selection.

## Frozen candidate geometry

Exactly inherited from v0.2:

- ROBUST_MEDIAN_CONSTANT
- DIRECTIONAL_STRIP_LINE
- AFFINE_PLANE
- QUADRATIC_SURFACE
- NO_RECONSTRUCTION

The same support radius, support split, directional strip geometry, direction-internal selection and final pre-target refit remain unchanged.

## v0.3 selector

`TARGET_BLIND_COMMON_VALIDATION_RMS_V0_3`

All valid model families are compared on the same common target-blind validation anchors.

The candidate with the lowest common validation RMS is selected.

No second BIC/AIC/parameter-count penalty is applied across families.

Model complexity is used only as a deterministic tie-break when validation RMS values are numerically equal. Current model-id order is also the simplicity order: constant, directional, affine, quadratic.

The direction inside DIRECTIONAL_STRIP_LINE is still chosen internally by the frozen v0.2 strip-validation logic before cross-family comparison. v0.3 does not modify that candidate.

## Observation-regime diagnostics

Each holdout reports:

- best common validation RMS through the selected candidate;
- second-best common validation RMS;
- absolute selection margin in RMS;
- relative selection margin.

These values expose whether a local observation strongly supports one family or whether multiple models are nearly indistinguishable. They are diagnostics only and do not use the held-out target.

## Development evidence only

Because v0.3 reuses the exact v0.2 candidate predictions, its selector can be replayed retrospectively on the already-observed paired v0.2 sidecar without rerunning the models.

On that already-seen source, the retrospective v0.3 selector would produce:

- all 11,844 holdouts: MAE ≈ 0.00442126, RMSE ≈ 0.00932747;
- v0.1 on the same holdouts: MAE ≈ 0.00446583, RMSE ≈ 0.00936281;
- v0.2: MAE ≈ 0.00460044, RMSE ≈ 0.00975762;
- reference-comparable 11,031 holdouts: v0.3 MAE ≈ 0.00384285, RMSE ≈ 0.00679921;
- v0.3 model counts: constant 1,685; directional 2,006; affine 3,009; quadratic 5,144.

This is development evidence only because the source was already observed before v0.3 was designed. It must not be reported as independent validation.

## Permanent safety boundary

- source raster remains exact full-resolution MEASURED support, not world-resolution authority;
- 20-bit sparse raster-independent lattice remains the scientific coordinate domain;
- numeric target remains unread before selector/reference freeze;
- target, target error and post-reveal oracle remain forbidden selector inputs;
- no lens/camera/vendor identity or calibration in model selection;
- no AI/ML/neural/generative runtime;
- measured anchors remain immutable;
- unanchored coordinates are not promoted to MEASURED;
- candidate_applied=false;
- creates_new_evidence=false;
- Scientific Master/D.RAWnegative writeback=false.

## Validation boundary

A new, previously unscored sealed RAW/DNG is required.

On that same new source, export v0.1, v0.2 and v0.3. Their holdout coordinates should be identical. The primary question is whether changing only cross-family selection improves held-out prediction and selector regret while preserving the stronger v0.2 candidate bank.
