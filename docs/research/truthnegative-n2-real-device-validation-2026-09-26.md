# TruthNegative N2 Real-Device Full-Colour Validation — 2026-09-26

Status: **USER-DEVICE OBSERVATION — SCREENSHOT-BOUND, NOT A NEW SENSOR EVIDENCE SOURCE**

The user ran the exact-Scientific-Master-gated N2 1:1 Full-colour A/B/Delta
diagnostic on a Camera-5 admitted 4080x3072 DNG. The Android route completed
successfully and all three automatically selected 192x192 crops reported
`baseline-mismatch=0`.

This is important because the baseline reconstruction is checked against the
exact canonical Scientific Master source pixel Float32 bit pattern before any
B candidate is admitted. The screenshot therefore supports that the tested
device route reached the intended full-colour candidate comparison path.

## Quiet / noise-candidate crop

Source crop: x=1728, y=512, 192x192.

Observed UI metrics:

- sampled: 36864
- candidate: 32635
- preserved: 4229
- structure-protected: 4229
- censored / boundary: 0 / 0
- changed display pixels: 29235 / 36864
- mean encoded display delta: 0.0021003
- maximum encoded display delta: 0.0571661
- removed residual energy: 6.395%
- max absolute Stage-2 correction: 0.00077635
- candidate Stage-2 sites including reconstruction halo: 34533
- reconstructed RGB channels changed: 104526
- baseline mismatch: 0

The displayed Delta crop is spatially noise-like rather than a uniform tone
shift, but this observation alone is not sufficient to authorize production
denoise.

## Structure crop

Source crop: x=2688, y=1856, 192x192.

Observed UI metrics:

- sampled: 36864
- candidate: 21835
- preserved: 15029
- structure-protected: 15028
- censored / boundary: 0 / 0
- changed display pixels: 19653 / 36864
- mean encoded display delta: 0.0013471
- maximum encoded display delta: 0.0588659
- removed residual energy: 6.436%
- max absolute Stage-2 correction: 0.00190135
- candidate Stage-2 sites including reconstruction halo: 23506
- reconstructed RGB channels changed: 74267
- baseline mismatch: 0

This crop still contains substantial candidate activity despite explicit
structure protection. The relatively large local maximum display delta is the
main reason to add percentile, location, chroma/luma and edge-energy risk
diagnostics before enabling any production appearance denoise.

## Censor / highlight crop

Source crop: x=2304, y=896, 192x192.

Observed UI metrics:

- sampled: 36864
- candidate: 784
- preserved: 36080
- structure-protected: 1918
- censored / boundary: 33612 / 550
- changed display pixels: 649 / 36864
- mean encoded display delta: 0.0000393
- maximum encoded display delta: 0.0387189
- removed residual energy: 5.719%
- max absolute Stage-2 correction: 0.00074587
- candidate Stage-2 sites including reconstruction halo: 1137
- reconstructed RGB channels changed: 3125
- baseline mismatch: 0

The highlight crop remains strongly fail-conservative: most of the crop is
explicitly protected by CENSORED or censor-boundary authority and only a small
fraction of output pixels change.

## Next gate

Do not enable production N2 appearance denoise yet.

The next validation layer is the N2 Risk / Quality Audit v0.1:

- p50 / p95 / p99 / max encoded display delta;
- independent R/G/B delta distributions;
- display-luma versus chroma delta;
- exact source coordinate of the maximum delta;
- distance from that maximum to Structure and CENSORED/boundary protection;
- A/B display-luma gradient-energy comparison.

The edge metric is diagnostic only and must not be described as optical MTF.

Permanent invariants remain unchanged:

- Direct CFA immutable;
- Scientific Master immutable;
- TruthNegative immutable;
- candidate display only;
- createsNewEvidence=false;
- scientificWritebackAllowed=false.
