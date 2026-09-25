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


## Risk / Quality device result

A subsequent Camera-5 admitted DNG completed the Risk / Quality v0.1
diagnostic with `baseline-mismatch=0` in all three automatically selected
crops.

### Quiet / noise-candidate crop

Source x=3328, y=2112, 192x192.

- candidate: 35103 / 36864
- preserved / structure-protected: 1761 / 1761
- changed display pixels: 32291 / 36864
- display delta p50 / p95 / p99 / max:
  0.0037831 / 0.0118110 / 0.0203161 / 0.0660779
- RGB p99: 0.0086956 / 0.0056419 / 0.0201859
- display-luma p99: 0.0047001
- chroma p99: 0.0112065
- max delta source coordinate: x=3415, y=2203
- distance to Structure protection: 1.41 px
- edge-energy B/A: 0.842389
- mean absolute gradient-magnitude delta: 0.0015954

The edge-energy drop is large, but this crop was selected as a quiet/noise
candidate region, so the metric cannot by itself distinguish useful
high-frequency noise removal from detail loss.

### Structure crop

Source x=2688, y=1728, 192x192.

- candidate: 15471 / 36864
- preserved: 21393
- structure-protected: 21028
- censored / boundary: 145 / 205
- changed display pixels: 15373 / 36864
- display delta p50 / p95 / p99 / max:
  0.0008280 / 0.0073301 / 0.0155366 / 0.0666121
- RGB p99: 0.0057230 / 0.0034107 / 0.0154263
- display-luma p99: 0.0029554
- chroma p99: 0.0086190
- max delta source coordinate: x=2835, y=1763
- distance to Structure protection: **0.00 px**
- distance to censor/boundary: 67.23 px
- edge-energy B/A: 0.983860
- mean absolute gradient-magnitude delta: 0.0005713

The 0.00 px structure distance is the decisive finding. It does not imply that
the protected CFA centre itself was corrected. It shows that admitted
neighboring CFA corrections can propagate through full-colour reconstruction
and change reconstructed channels at a Structure-protected output pixel.
Source-site gating alone is therefore insufficient for output-space structure
protection.

### Censor / highlight crop

Source x=2048, y=768, 192x192.

- candidate: 5094 / 36864
- preserved: 31770
- structure-protected: 4211
- censored / boundary: 26901 / 656
- changed display pixels: 4643 / 36864
- display delta p50 / p95 / p99 / max:
  0.0000000 / 0.0040492 / 0.0080879 / 0.0378958
- RGB p99: 0.0034679 / 0.0021755 / 0.0077469
- display-luma p99: 0.0019433
- chroma p99: 0.0046949
- max delta source coordinate: x=2231, y=775
- distance to Structure protection: 1.41 px
- distance to censor/boundary: 13.00 px
- edge-energy B/A: 0.985534
- mean absolute gradient-magnitude delta: 0.0001785

The highlight protection remains strongly conservative.

## Consequence

Do not enable production N2 appearance denoise.

The next diagnostic candidate closes N2 protection over the reconstruction
support. An otherwise-admitted CFA correction is suppressed when it falls
within `requiredHalo()` of any protected core output pixel. After
reconstruction every protected core RGB output must be Float32 bit-identical
to baseline; otherwise the route fails closed.
