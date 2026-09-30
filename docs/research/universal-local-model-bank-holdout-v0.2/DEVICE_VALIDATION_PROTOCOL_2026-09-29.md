# D.RAW Universal Local Model Bank Holdout v0.2
## Frozen paired device-validation protocol

Date frozen: 2026-09-29

Purpose: test whether the v0.2 support-crossfit selector and direction-conditioned support improve model selection over v0.1 on the same new sealed observation.

## Independence

The PR #92 tele capture DRAW_CAPTURE_1790703829897_tele_4080x3072 was already observed before v0.2 was designed. It is development evidence only.

The validation source must be a new, previously unscored sealed RAW/DNG.

Any lens/camera may be used. If tele, main or ultra-wide is used, that identity is test provenance only and must not enter the selector.

## Frozen constants

- holdout period: 64;
- support radius: 8 source pixels;
- lattice fraction bits: 20;
- v0.2 cross-fit validation partition: offset-lattice index sum modulo 3 equals zero;
- v0.2 directional strip half-width: 2 source pixels;
- v0.2 candidate bank: ROBUST_MEDIAN_CONSTANT, DIRECTIONAL_STRIP_LINE, AFFINE_PLANE, QUADRATIC_SURFACE, NO_RECONSTRUCTION;
- v0.2 selector score: TARGET_BLIND_SUPPORT_CROSSFIT_PREDICTIVE_SCORE_V0_2;
- reference: CENTER_EXCLUDED_MULTISCALE_V0_2 where valid.

These values must not be changed after inspecting the new result.

## Required paired export

On the same new source:

1. Run Universal Intake.
2. Export Universal Observation Model Selection v0.1 JSON when available.
3. Export Universal Local Model Bank Holdout v0.1 JSON.
4. Export Universal Local Model Bank Holdout v0.2 JSON.
5. Preserve the original sealed RAW/DNG and all JSON files together.

Because v0.1 and v0.2 use the same holdout period and full-resolution sampling rule, their holdout coordinates are expected to match. Any mismatch must be treated as a protocol failure until explained.

## Primary comparisons

Compare v0.2 against v0.1 and the historical reference on identical held-out anchors:

- MAE, RMSE and bias;
- selected-vs-reference win counts;
- v0.2-vs-v0.1 paired absolute-error delta;
- selector regret against each version's post-reveal oracle;
- selected-model distribution;
- oracle-model distribution;
- CFA-phase/channel balance;
- spatial block consistency;
- no-model-selected rate;
- target-leakage and writeback safety flags.

Specific v0.2 design questions:

- Does DIRECTIONAL_STRIP_LINE produce center estimates that are no longer identically equal to AFFINE_PLANE?
- Does strip-internal validation choose a useful direction?
- When all model families are then compared on the same common validation anchors, does v0.2 reduce selector regret and/or held-out CFA error versus v0.1?

## Target blindness

Target censor/saturation state may gate whether an anchor is a valid holdout.

The numeric held-out Stage-2 value must remain unread until:

- v0.2 model fitting is complete;
- support-crossfit selection is frozen;
- the historical reference prediction is frozen.

The target, target error and post-reveal oracle may not influence selection.

## Interpretation

A better held-out CFA prediction is predictive evidence only. It does not prove latent scene truth, denoising, added optical resolution or permission to modify measured evidence.

No v0.2 result automatically enables Scientific Master or D.RAWnegative writeback.

## Promotion boundary

This paired device result is still only one independent observation. Even if v0.2 improves on v0.1, promotion requires a separately frozen multi-observation validation criterion.
