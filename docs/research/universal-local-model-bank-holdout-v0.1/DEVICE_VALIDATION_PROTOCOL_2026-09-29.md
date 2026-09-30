# D.RAW Universal Local Model Bank Holdout v0.1
## Independent real-device validation protocol

Date frozen: 2026-09-29

Purpose: pre-register the first independent device test of PR #92 before
observing its result.

## Independence

The previously observed PR #90 tele capture
`DRAW_CAPTURE_1790686814757_tele_4080x3072` is **development evidence only**.
It must not be used to claim independent validation of PR #92.

The validation input must be a new, previously unscored sealed RAW observation.

No particular lens, camera model or vendor is required. That is deliberate:
the experiment tests a universal source-agnostic route.

## Required source properties

- one physical/source observation only;
- admitted native DNG route for this implementation version;
- source remains byte-sealed and SHA-bound;
- original CFA raster remains the MEASURED anchor geometry;
- no burst, temporal fusion or other-lens fusion;
- no lens profile, camera-model profile or vendor lookup is used by the
  selector;
- no AI/ML/neural/generative runtime.

The scene does not need to reproduce the PR #90 Dark-Chroma scene. PR #92 is a
full-resolution hold-out audit and is intentionally not gated by Dark Chroma.

## Frozen audit constants

These must not be changed after looking at the new result:

- lattice fraction bits: 20;
- hold-out period: 64;
- support radius: 8 source pixels;
- core tile extent: 256;
- candidate bank:
  - `ROBUST_MEDIAN_CONSTANT`
  - `DIRECTIONAL_LINE`
  - `AFFINE_PLANE`
  - `QUADRATIC_SURFACE`
  - `NO_RECONSTRUCTION`
- selector:
  `TARGET_BLIND_RESIDUAL_BIC_LIKE_COMPLEXITY_SCORE`;
- reference:
  `CENTER_EXCLUDED_MULTISCALE_V0_2` when its own contract is valid.

The held-out Stage-2 target value is not read until the research selector and
the independent reference predictor have both been frozen.

## Device procedure

1. Install the PR #92 APK built from the frozen code checkpoint.
2. Choose one new RAW/DNG observation not previously used to tune PR #90–#92.
3. Run **Lees bron universeel**.
4. Export **Universal Observation Model Selection v0.1 · JSON** when available.
   This records the prospective observation-derived policy and must be retained
   unchanged.
5. Export **Universal Local Model Bank Holdout v0.1 · JSON**.
6. Do not change the model bank, hold-out period, support radius, selector or
   thresholds after inspecting the result.
7. Preserve the original RAW/DNG and both JSON files together.

For review, provide:
- the original sealed RAW/DNG;
- Universal Observation Model Selection JSON when available;
- Universal Local Model Bank Holdout JSON;
- optional screenshots only as UI evidence, never as replacement for the JSON
  or source.

## Primary audit questions

The first independent result must answer these separately:

1. **Coverage** — how many real CFA hold-outs have a valid selected model?
2. **Reference comparison** — on anchors where both are valid, how often and by
   how much does the frozen selected model differ from the center-excluded
   reference?
3. **Model-bank capacity** — which candidate is the post-reveal oracle per
   hold-out?
4. **Selector quality** — what selector regret remains between the frozen
   pre-target selection and the diagnostic post-reveal oracle?
5. **CFA balance** — are all CFA phases represented and do failures cluster by
   phase/channel?
6. **Safety** — do all target-leakage, lens/device dependency, authority and
   writeback flags remain false?

No single aggregate score may hide a phase-specific or regime-specific
regression.

## Interpretation boundary

A lower held-out CFA prediction error means only that the predictor forecasts
the noisy measured target better under this experiment.

It does **not** by itself prove:
- latent scene truth;
- denoising improvement;
- added optical resolution;
- permission to modify a MEASURED anchor;
- permission to write the candidate into Scientific Master.

The source raster remains full-resolution measurement support. The free lattice
remains the scientific solution domain.

## Promotion boundary

This first independent device test is a scientific gate, not automatic
promotion.

Regardless of outcome:

- measured anchors remain unchanged;
- candidate_applied remains false;
- Scientific Master writeback remains false;
- D.RAWnegative writeback remains false.

If the selector fails, the negative result is preserved and used to design a
versioned successor. If it succeeds, a separate promotion criterion and
additional independent validation must be frozen before any writeback research.
