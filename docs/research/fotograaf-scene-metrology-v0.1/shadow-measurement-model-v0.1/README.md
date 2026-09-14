# FotoGraaf CalibrationModelBinding / MeasurementLab shadow adapter v0.1

**Status: RESEARCH SHADOW EXECUTION ONLY / NO SCIENTIFIC-MASTER ROUTE CHANGE**

This module is the first executable step after `FotoGraaf Calibration -> Scene Admission v0.1`.

It consumes three things:

1. the current sealed source measurement model;
2. an already-admitted immutable `CalibrationBindingPacket`;
3. calibration-model parameters bound to that exact binding.

It then computes the current source-bound measurement interpretation and a calibration-assisted interpretation **side by side**. The calibration-assisted result is diagnostic only. It is not written into canonical v4.7i, the Scientific Master, the Technical Backplane, Certificate v0.1 or Android production output.

## Why shadow mode first

The project now knows how to decide whether a calibration pack is eligible for a particular sealed scene. Eligibility is not enough to modify scientific pixels.

Before route promotion TruthRaw must prove that the numerical model itself behaves correctly, propagates uncertainty, preserves source identity, remains worker-count invariant and never double-applies source corrections.

Shadow mode makes those differences observable without letting them affect current truth authority.

## Source reference branch

The source branch intentionally mirrors the current canonical v4.7i Stage-2 scalar arithmetic:

`((rawCode - sourceBlackPhase) / max(sourceWhiteLevel - sourceBlackPhase, 1)) * sourceGainField`

The canonical file remains byte-frozen and is not edited by this research module.

## Calibration-assisted shadow branch

v0.1 can independently enable only:

- phase black offset;
- R/G/B affine noise profile;
- one scalar response scale;
- calibrated saturation code;
- dark-side SNR threshold.

A parameter that has not been calibrated remains source-bound. This is deliberate: a read-noise calibration must not silently become color calibration or exposure-response calibration.

The v0.1 response scale is scalar. Per-channel response scaling is intentionally excluded because it could become a color transform and therefore needs its own C4/color authority.

## Signed dark side

Both branches retain signed values below the respective black reference. Shadow execution never clamps negative post-black estimates to zero.

Noise-limited darkness is a support/uncertainty state, not proof of zero physical light.

## Censoring

Source clipping remains authoritative evidence history. Even if a future calibrated saturation threshold is numerically higher than the DNG WhiteLevel, a source sample already classified at/above source WhiteLevel remains high-side censored.

A calibration may tighten a censor boundary; it may not retroactively uncensor source evidence.

## Noise semantics

The source and calibrated NoiseProfile arrays use the declared R/G/B `(S,O)` affine variance shape.

For each sample, v0.1 evaluates noise in the pre-GainMap normalized coordinate and multiplies sigma by the same existing source GainMap value used by the signal branch. A second GainMap/lens-shading correction is explicitly rejected.

This does not settle the still-open MotionCam/device NoiseProfile semantic question. Real Honor promotion requires independent semantics validation and a matching CalibrationPack.

## Worker invariance

The test runs the same diagnostic sample set with one and four workers writing disjoint result slots. Every authoritative diagnostic float and state must be bit-identical.

Worker count is execution state only and is not present in the admitted binding or measurement model.

## What PASS means

A host PASS means:

- the source reference formula remains intact;
- calibration binding/source identity checks fail closed;
- signed below-black values survive;
- source censoring cannot be removed by calibration;
- partial calibration changes only explicitly enabled quantities;
- second GainMap correction is rejected;
- 1-worker and 4-worker diagnostics are bit-identical under the fixture.

It does **not** mean the Honor tele is physically calibrated. It does **not** authorize the calibration-assisted branch to replace the current Scientific Master route.

## Promotion gates still open

Before any route promotion, TruthRaw still needs:

- real `CalibrationPack` model parameters from controlled Honor dark/flat/linearity data;
- held-out physical validation;
- calibrated uncertainty residuals;
- exact source/master identity regression;
- 1/2/4-worker invariance on real DNGs;
- capture/sample-domain regression, including the discrete exact-ISO8192-associated domain;
- proof that upstream GainMap/lens shading is not applied twice;
- authority review and an explicit versioned integration decision.

Permanent rule:

**Calibration may improve the measurement model only after admission and validation; shadow differences are diagnostics, not new truth.**
