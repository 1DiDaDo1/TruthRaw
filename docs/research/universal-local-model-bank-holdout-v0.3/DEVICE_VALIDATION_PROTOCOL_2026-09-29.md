# D.RAW Universal Local Model Bank Holdout v0.3
## Frozen three-way device-validation protocol

Date frozen: 2026-09-29

Purpose: test whether changing only cross-family selection from v0.2's penalized score to common validation RMS improves predictive selection while preserving identical v0.2 candidate fits.

## Independence

The previously observed PR #92 tele source and the paired v0.1/v0.2 source IMG_BNC_TRUTHRAW20260907_094414_423 are development evidence only for v0.3.

The validation source must be a new, previously unscored sealed RAW/DNG.

Lens identity may be recorded as provenance only. It must not enter selection.

## Frozen v0.3 selector

- candidate fits: exactly v0.2;
- directional strip geometry and direction-internal choice: exactly v0.2;
- common validation anchors: exactly v0.2;
- cross-family score: common validation RMS;
- cross-family BIC/AIC/parameter-count penalty: none;
- complexity: deterministic tie-break only;
- holdout period: 64;
- support radius: 8 source pixels;
- lattice fraction bits: 20.

These rules must not change after observing the new result.

## Required exports on the same new source

1. Run Universal Intake.
2. Export Universal Observation Model Selection v0.1 JSON when available.
3. Export Universal Local Model Bank Holdout v0.1 JSON.
4. Export Universal Local Model Bank Holdout v0.2 JSON.
5. Export Universal Local Model Bank Holdout v0.3 JSON.
6. Preserve the original sealed RAW/DNG and all JSON sidecars together.

v0.1, v0.2 and v0.3 must use the same holdout period and coordinate rule. Holdout-coordinate mismatch is a protocol failure until explained.

## Primary comparisons

- v0.3 vs v0.2 on identical held-out anchors;
- v0.3 vs v0.1 on identical held-out anchors;
- each version vs CENTER_EXCLUDED_MULTISCALE_V0_2 where valid;
- MAE, RMSE, bias and pairwise win/tie counts;
- selector regret against each version's post-reveal oracle;
- selected-model distribution;
- CFA phase/channel balance;
- spatial block consistency;
- second-best validation RMS and selection-margin diagnostics;
- no-model-selected rate;
- all target-leakage/lens/device/writeback safety flags.

Because v0.3 reuses v0.2 candidate fits, v0.2 and v0.3 per-model estimates should be bit-identical for every valid candidate. Any candidate-estimate mismatch is a protocol failure.

## Interpretation boundary

A better held-out CFA prediction is predictive evidence only. It does not prove latent scene truth, denoising, added optical resolution or permission to modify measured evidence.

No result automatically enables Scientific Master or D.RAWnegative writeback.
