# TruthRaw Frozen Tele Regression Suite — Final 2026-09-07

**Fixed appearance regression:** PASS

| Source | ISO | nσ @2% | v4.7j ΔY p99 | v5.0e ΔY p99 | v5.0e overshoot p99 | v4.7k max ΔY / cap | uncertainty Q5 |
|---|---:|---:|---:|---:|---:|---:|---:|
| `IMG_BNC_TRUTHRAW20260906_151110_490.dng` | 100 | 0.001453 | 0.015676 | 0.012019 | 0.0300 | 0.006228 / 0.006228 | — |
| `IMG_BNC_TRUTHRAW20260906_151129_256.dng` | 100 | 0.001473 | 0.001903 | 0.001395 | 0.0300 | 0.005253 / 0.006103 | — |
| `IMG_BNC_TRUTHRAW20260907_094414_423.dng` | 1619 | 0.002783 | 0.002647 | 0.002037 | 0.0295 | 0.001520 / 0.004500 | 0.705 |
| `IMG_BNC_TRUTHRAW20260907_094449_565.dng` | 638 | 0.002020 | 0.002140 | 0.001700 | 0.0300 | 0.000487 / 0.004500 | 0.917 |

## Fixed checks
- **IMG_BNC_TRUTHRAW20260906_151110_490.dng** — v47j_rgb_direction_p99_lt_2e_5: PASS, v50e_rgb_direction_p99_lt_2e_5: PASS, v47k_rgb_direction_p99_lt_2e_5: PASS, v50e_lf_drift_p99_lt_0_0025: PASS, v47k_delta_cap_respected: PASS, v50e_overshoot_p99_le_0_031: PASS
- **IMG_BNC_TRUTHRAW20260906_151129_256.dng** — v47j_rgb_direction_p99_lt_2e_5: PASS, v50e_rgb_direction_p99_lt_2e_5: PASS, v47k_rgb_direction_p99_lt_2e_5: PASS, v50e_lf_drift_p99_lt_0_0025: PASS, v47k_delta_cap_respected: PASS, v50e_overshoot_p99_le_0_031: PASS
- **IMG_BNC_TRUTHRAW20260907_094414_423.dng** — v47j_rgb_direction_p99_lt_2e_5: PASS, v50e_rgb_direction_p99_lt_2e_5: PASS, v47k_rgb_direction_p99_lt_2e_5: PASS, v50e_lf_drift_p99_lt_0_0025: PASS, v47k_delta_cap_respected: PASS, v50e_overshoot_p99_le_0_031: PASS
- **IMG_BNC_TRUTHRAW20260907_094449_565.dng** — v47j_rgb_direction_p99_lt_2e_5: PASS, v50e_rgb_direction_p99_lt_2e_5: PASS, v47k_rgb_direction_p99_lt_2e_5: PASS, v50e_lf_drift_p99_lt_0_0025: PASS, v47k_delta_cap_respected: PASS, v50e_overshoot_p99_le_0_031: PASS

## ISO 1619 uncertainty localization
- Frozen Q5 ratio: **0.705044** (FAIL; gate lower bound 0.75).
- By CFA role: R=0.839, G1=0.629, G2=0.652, B=0.910.
- By field position: center=0.838, mid=0.606, outer=0.799.
- By Q5 SNR quartile: Q1=0.882, Q2=0.884, Q3=0.889, Q4=0.884.

Interpretation: the failure concentrates most strongly in green and the mid-field region, while all four Q5 SNR quartiles underpredict by a similar amount. Therefore the evidence does **not** support explaining the failure as merely a low-SNR tail.

## GainMap repeatability finding

The two 2026-09-07 frames have green GainMaps that are almost invariant (mean absolute relative difference ~0.04–0.05%), while R/B differ by ~1.2% on average and >4% at the largest grid differences. Treat this as DNG metadata-path evidence only, not as independent lens calibration.

## Scientific boundary

The regression uses the frozen v5.0e DNG ForwardMatrix/AsShotNeutral color path. Independent per-lens color/illuminant calibration remains open and is not closed by these tests.

The pre-freeze 2026-09-06 files remain regression/stress material only. The two 2026-09-07 files remain the valid prospective uncertainty tests. No model coefficients, thresholds, or holdout gates were changed after seeing the results.