# TruthRaw — High-ISO item 153 — frozen v5.0g evaluation
**Date:** 2026-09-10  
**Project classification:** `POST_FREEZE_IMPLEMENTATION_EXPOSED_TARGET_UNSCORED_HIGH_ISO_GENERALIZATION_SET`  
**Result:** `SCORED_MIXED_FAIL_GENERALIZATION_REVIEW_OPEN_NO_RETUNE`

## Scientific boundary
The exact frozen v5.0g model, feature extractor, backend binding and v4.7i backend bytes were used without retuning. The four frames were captured after the v5.0g freeze and were absent from its historical development hash set, but they had already been exposed to implementation/parity/tile/memory work. Therefore the scorer's literal `PROSPECTIVE_PASS/FAIL` strings are retained as raw runner output, while the project-level evidence class remains **implementation-exposed high-ISO generalization evidence**, not pristine blind prospective certification.

## Frozen gates
- uncensored targets >= 80,000
- p50 coverage: 0.44–0.56
- p95 coverage: 0.90–0.99
- risk ordering required
- every Q1–Q5 predicted/observed p95 ratio: 0.75–1.35

## Results
| ISO | p50 cov | p95 cov | Q5 pred p95 | Q5 obs p95 | Q5 ratio | Literal scorer decision |
|---:|---:|---:|---:|---:|---:|---|
| 1600 | 0.4781 | 0.9221 | 0.019398 | 0.034306 | **0.565435** | `BACKEND_BOUND_UNCERTAINTY_PROSPECTIVE_FAIL` |
| 3200 | 0.4886 | 0.9333 | 0.025016 | 0.038642 | **0.647388** | `BACKEND_BOUND_UNCERTAINTY_PROSPECTIVE_FAIL` |
| 6400 | 0.4937 | 0.9407 | 0.033834 | 0.047307 | **0.715192** | `BACKEND_BOUND_UNCERTAINTY_PROSPECTIVE_FAIL` |
| 12800 | 0.5089 | 0.9517 | 0.047292 | 0.059496 | **0.794887** | `BACKEND_BOUND_UNCERTAINTY_PROSPECTIVE_PASS` |

All four pass the global p50/p95 coverage and risk-ordering gates. ISO1600/3200/6400 fail **only** because at least one risk-quintile p95 ratio falls below 0.75; in each case Q5 is the limiting tail. ISO12800 passes the literal frozen scorer gates.

## Q5 localization — frozen predictions unchanged
These are diagnostic slices only; they are not new gates or model parameters. Radial location uses explicit thirds of the already-frozen `radial_position_norm` feature.

| ISO | R | G1 | G2 | B | center | mid | outer |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 1600 | 0.752 | 0.471 | 0.506 | 0.836 | 0.385 | 0.649 | 0.559 |
| 3200 | 0.790 | 0.581 | 0.575 | 0.872 | 0.494 | 0.730 | 0.584 |
| 6400 | 0.813 | 0.664 | 0.648 | 0.846 | 0.574 | 0.789 | 0.650 |
| 12800 | 0.849 | 0.765 | 0.766 | 0.855 | 0.724 | 0.840 | 0.754 |

G1/G2 are again the weakest color roles at ISO1600–6400. In this specific sweep, the center radial slice is especially weak; this differs from the earlier 094414 diagnostic, where mid-field was weakest. That argues against promoting one fixed spatial explanation from a single scene.

## Q5 SNR diagnostic
The failure is not confined to low SNR. Q5 predicted/observed p95 ratios by increasing SNR quartile are:

- ISO1600: 0.731, 0.609, 0.631, 0.574
- ISO3200: 0.865, 0.763, 0.697, 0.637
- ISO6400: 0.929, 0.835, 0.834, 0.689
- ISO12800: 0.983, 0.921, 0.880, 0.801

At ISO1600–6400, higher-SNR Q5 samples remain undercovered and can be worse than the lowest-SNR quartile. This is evidence against a simple low-SNR-only diagnosis. A plausible **research hypothesis**, not a proven cause, is that the current error band mixes sensor-noise-linked uncertainty with a structure/topology interpolation error floor; when the NoiseProfile term is smaller, that structural tail can be relatively underrepresented.

## Determinism
All four evaluations were executed a second time. Each result JSON was byte-for-byte identical to its first run.

## Preservation / decision
- Do **not** retune frozen v5.0g on these four frames.
- Preserve all three FAILs and the ISO12800 PASS.
- Do **not** close the broader uncertainty generalization blocker.
- The historical 094414 Q5 failure remains binding.
- These four frames may now be used as **exposed diagnostic/development evidence** for a separate successor candidate, but then cannot be reused as independent promotion holdouts.
- A successor should explicitly test a structure/topology-risk floor and role/spatial interactions, and must be promoted only on genuinely new held-out captures.

## Exact result JSON SHA-256
- `IMG_20260908_194929_v5_0g_result.json`: `547cf3e43e41eb6c7832955b4d533f4588e41dc13721cb033ebf8928796437bd`
- `IMG_20260908_194939_v5_0g_result.json`: `e20a3804f7008f3dd8f7646cb43ed8816cd8418f81cdb1ad5157f82a49160b91`
- `IMG_20260908_194947_v5_0g_result.json`: `adc9a72b9648222834481c290af9140f7d337bf152e986c1671ac9f815f6eab8`
- `IMG_20260908_194951_v5_0g_result.json`: `f528caa447c68a46b0c39ee8adb0a06317ba869dd3c5f96830c1a3cb55d111ae`
