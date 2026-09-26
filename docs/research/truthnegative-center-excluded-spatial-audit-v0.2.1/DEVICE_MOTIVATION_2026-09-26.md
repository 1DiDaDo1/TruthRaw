# N2 v0.2 Center-Excluded — Real-Device Result 2026-09-26

Status: **USER-DEVICE OBSERVATION — MOTIVATION FOR v0.2.1 SPATIAL AUDIT**

This note records the first real-device run of the center-excluded v0.2 crop
audit that motivated the v0.2.1 whole-frame spatial sidecar.

It does not create camera evidence and does not alter Direct CFA, Scientific
Master, TruthNegative, authority or the current N2 v0.1 appearance candidate.

## Source-level anchors

The companion N2 Spatial Audit v0.1 for this admitted Camera-5 4080x3072 DNG
reported:

- sampled: 783360
- v0.1 candidate corrected: 516599
- preserved: 266761
- structure protected: 245088
- CENSORED protected: 15329
- censor-boundary protected: 2213
- removed residual energy fraction: 0.055320921947
- max absolute proposed Stage-2 correction: 0.005368825726
- CFA phase samples: 195840 / 195840 / 195840 / 195840
- creates_new_evidence=false
- scientific_writeback_allowed=false
- candidate_applied=false

## 1:1 quiet/noise candidate crop

Source crop x=2048, y=640, 192x192:

v0.1 / support-closure:

- sampled: 36864
- v0.1 candidate: 34704
- preserved / structure: 2160 / 2160
- candidate Stage-2 after support closure: 6362
- support-guard suppressed: 30202
- protected core: 2160
- protected changed: 0
- reconstruction influence radius: 3 px
- baseline mismatch: 0
- changed display pixels: 8014 / 36864
- display gradient-energy B/A: 0.975999

center-excluded v0.2 audit:

- v0.1 candidate centers: 34704
- predictor valid / invalid: 34500 / 204
- predictor-valid fraction: 99.4122%
- pairs considered / accepted / rejected:
  416448 / 395437 / 21011
- pair acceptance: 94.9547%
- scales considered / accepted / rejected:
  104064 / 89580 / 14484
- scale acceptance: 86.0816%
- center-only residual <=1 sigma / 1-2 sigma / >2 sigma:
  26442 / 7795 / 263
- >2 sigma among valid predictors: 0.7623%
- mean / max absolute center-excluded residual:
  0.001186041 / 0.00964911
- max directional / cross-scale disagreement:
  15.941 sigma / 5.690 sigma
- audit-only=true
- candidate-applied=false

## 1:1 structure crop

Source crop x=3328, y=1856, 192x192:

v0.1 / support-closure:

- sampled: 36864
- v0.1 candidate: 13457
- preserved: 23407
- structure protected: 23141
- CENSORED / boundary: 133 / 127
- candidate Stage-2 after support closure: 869
- support-guard suppressed: 14128
- protected core: 23407
- protected changed: 0
- reconstruction influence radius: 3 px
- baseline mismatch: 0
- changed display pixels: 599 / 36864
- display gradient-energy B/A: 0.999302

center-excluded v0.2 audit:

- predictor valid / invalid: 12126 / 1331
- predictor-valid fraction: 90.1092%
- pairs considered / accepted / rejected:
  161440 / 95902 / 65538
- pair acceptance: 59.4041%
- scales considered / accepted / rejected:
  36949 / 19335 / 17614
- scale acceptance: 52.3289%
- center-only residual <=1 sigma / 1-2 sigma / >2 sigma:
  8352 / 3334 / 440
- >2 sigma among valid predictors: 3.6286%
- mean / max absolute center-excluded residual:
  0.00465363 / 0.18854341
- max directional / cross-scale disagreement:
  49.849 sigma / 34.867 sigma

## 1:1 censor/highlight crop

Source crop x=2752, y=512, 192x192:

v0.1 / support-closure:

- sampled: 36864
- v0.1 candidate: 3216
- preserved: 33648
- structure protected: 5943
- CENSORED / boundary: 26908 / 795
- candidate Stage-2 after support closure: 183
- support-guard suppressed: 3547
- protected core: 33648
- protected changed: 0
- reconstruction influence radius: 3 px
- baseline mismatch: 0
- changed display pixels: 48 / 36864
- display gradient-energy B/A: 0.999926

center-excluded v0.2 audit:

- predictor valid / invalid: 2773 / 443
- predictor-valid fraction: 86.2251%
- pairs considered / accepted / rejected:
  38565 / 21240 / 17325
- pair acceptance: 55.0758%
- scales considered / accepted / rejected:
  9156 / 3842 / 5314
- scale acceptance: 41.9616%
- center-only residual <=1 sigma / 1-2 sigma / >2 sigma:
  1985 / 734 / 54
- >2 sigma among valid predictors: 1.9473%
- mean / max absolute center-excluded residual:
  0.00340663 / 0.05529141
- max directional / cross-scale disagreement:
  48.289 sigma / 29.747 sigma

## Interpretation bounded to this device run

The center-excluded predictor was most coherent in the quiet/noise crop and
substantially less coherent in structure and censor/highlight crops.

This is not yet evidence that every valid quiet predictor identifies removable
noise. It is evidence that center exclusion and directional/multiscale
agreement carry discriminative information that was not available from the
v0.1 center-conditioned neighborhood alone.

The support-closure remained intact in all three crops:

- baseline-mismatch=0
- protected-changed=0
- reconstruction influence radius=3 px

## v0.2.1 consequence

The next test is spatial coverage across the whole admitted source.

v0.2.1 therefore remains audit-only and records per 64x64 tile:

- predictor validity;
- pair and scale rejection;
- center-only residual z;
- combined residual z as a secondary diagnostic only;
- predictor and center variance;
- variance ratio;
- CFA phase balance.

No appearance candidate is changed by v0.2.1.
