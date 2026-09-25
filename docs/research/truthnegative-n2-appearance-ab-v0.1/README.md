# TruthNegative N2 Appearance A/B v0.1

Status: **DISPLAY-ONLY VALIDATION — NO SCIENTIFIC WRITEBACK**

This experiment places an isolated N2 candidate beside the unchanged
TruthNegative Continuous / Free-World Appearance preview.

## A / B definition

- **A** is the existing TruthNegative Continuous v0.5 Appearance result.
- **B** is an appearance-only hypothesis derived from the N2 measured-CFA
  candidate audit.
- A is never modified by the B path.
- B is never written into Direct CFA, Scientific Master, TruthNegative,
  Deep Scene, restoration state or an export primary.

The native packet therefore keeps:

- primary candidate applied = false;
- creates new evidence = false;
- scientific writeback allowed = false;
- source scene mutated = false.

## Candidate projection

The N2 CFA audit remains parity-balanced at the configured sampling period.
For the Android A/B preview the audit additionally accumulates a target-sized
correction grid.

For each target pixel and camera-native channel:

```
appearanceCorrection =
    sum(N2 correction for sampled CFA sites in footprint/channel)
    / count(all sampled CFA sites in footprint/channel)
```

Protected samples therefore contribute an exact zero through the denominator.
CENSORED, censor-boundary, structure-protected and other fail-conservative
samples are not silently replaced by neighboring candidate values.

The correction is added only to a copy of the already resolved low-resolution
scene used for the B display hypothesis. The copied scene has its scientific
authority and uncertainty flags cleared before Appearance/Display resolve and
receives a new candidate-scene identity bound to the N2 grid digest.

This is intentionally **not** a full candidate RAW reconstruction. It is a
visual validation surface for deciding whether the currently measured N2
corrections appear structure-preserving before any production appearance
denoise is considered.

## Identity

The correction grid SHA-256 is bound to:

- sealed admitted source SHA-256;
- TruthNegative state SHA-256;
- N2 candidate SHA-256;
- target grid geometry;
- per-bin correction sums and sampled/corrected/protected counts.

The Android UI reports changed preview pixels, adjusted channels, candidate
display-clamp count and the correction-grid digest.

## Permanent boundary

No result from this experiment may be promoted to measured evidence. A future
appearance denoise may consume a validated candidate policy, but Scientific
Master and TruthNegative remain immutable.
