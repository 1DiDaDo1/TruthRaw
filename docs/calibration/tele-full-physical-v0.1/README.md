# TruthRaw Tele FULL_PHYSICAL Campaign v0.1

This directory is the compact canonical index for the physical-calibration campaign designed for the **HONOR BKQ-N49 tele / System ID 5 / 22.48 mm f/2.6 / 4080x3072 BGGR** domain.

Current status remains **`PURE_TRUTH_DERIVED`**. The campaign does not itself create a `FULL_PHYSICAL` result.

## What the campaign closes

The remaining physical tracks are:

1. independent per-unit/lens L2 color;
2. physical illuminant Level-N / Level-C evidence;
3. physical electron/PTC calibration by actual gain/readout state;
4. PSF, MTF/SFR, CA, flare and residual shading;
5. new independent review of the frozen high-ISO green/mid-field uncertainty undercoverage before any broad high-ISO scope.

## Execution order

`S0_PREFLIGHT -> S1_GAIN_STATE_RECON -> S2_PTC_FULL -> S3_COLOR_ILLUMINANT_FIT -> S4_COLOR_ILLUMINANT_VALIDATION -> S5_OPTICS_PSF_SFR_CA_FIT -> S6_OPTICS_FLARE_SHADING_FIT -> S7_OPTICS_INDEPENDENT_VALIDATION -> S8_HIGH_ISO_UNCERTAINTY_INDEPENDENT_REVIEW -> S9_PROMOTION_REVIEW`

The first expensive optimization is deliberate: **do gain-state reconnaissance before full PTC**. ISO is retained as provenance; full PTC is expanded only for genuinely distinct measured readout/noise states.

## Shared capture without shared truth

The same original uniform-flat RAW may be analyzed:
- before GainMap for PTC temporal statistics;
- after exactly one GainMap for residual shading.

The fit models, units, provenance and held-out validation remain separate.

Likewise, slanted edges can support SFR+CA, point sources PSF+chromatic centroid, and neutral-target fields Level-N+neutral-axis validation.

## Hard boundaries

- original admitted camera RAWs only; no TruthRaw-derived DNG as calibration input;
- no ordinary scene object as physical color or optical ground truth;
- no WhiteLevel-clipped code treated as exact latent radiance;
- no GainMap double application;
- no PTC fit in post-GainMap units;
- no AsShotNeutral treated as measured SPD;
- never retune v5.0g on the frozen black/white-dog holdout;
- training and final physical color validation use disjoint physical target IDs.

## Package integrity

The complete external campaign bundle is intentionally not committed as a binary archive.

`TRUTHRAW_TELE_FULL_PHYSICAL_CAMPAIGN_v0_1.zip`

SHA-256:

`66523fe4459fd9ce75b667e37973fbcb49d26ad8f6f0761a6cb83f0f4143c0f9`

The JSON index records the exact child-kit hashes and planning counts.

## Promotion boundary

`FULL_PHYSICAL` can only apply to an explicitly enumerated camera/lens/source/gain/illuminant/focus domain where every required physical measurement **and independent validation gate** passes. Unmeasured domains remain outside scope.
