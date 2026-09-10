# TruthRaw Best Conditioning → S-Curve Bridge v1 — report
**Date:** 2026-09-10  
**Status:** `RESEARCH_PASS_NOT_YET_CANONICAL_APPEARANCE_REPLACEMENT`

## 1. Problem being solved

TruthRaw's Virtual Observation Manifold and Best Observation work showed that a single physical RAW can be represented at many EV/gain gauges without creating new evidence. Manifold Conditioning v1 then made the safe form explicit: choose a numerically useful power-of-two gauge only when the complete state can round-trip exactly, and decondition before returning to scientific coordinates.

The remaining appearance problem was spatial: the existing S-curve v1 increases shadow curve strength when local luma confidence is low. On a perfectly flat RGB field, an abrupt confidence discontinuity therefore changes absolute output luminance and can create a false seam.

## 2. Bridge rule

Best Conditioning EV is upstream-only. The appearance API deliberately has **no EV parameter**. It accepts only deconditioned/gauge-invariant evidence after a successful conditioning audit. Missing audit, evidence-root mismatch, invalid input or invalid config fails closed to the original RGB.

This prevents virtual EV becoming a hidden local exposure map, virtual views being counted as extra photons, a numerical gauge changing the scientific zero-line, or appearance feeding back into Scene Master truth.

## 3. New appearance decomposition

`display-linear RGB → global monotonic S-curve → edge-aware local base → uncertainty-aware local residual compression → confidence-aware chroma → analytic gamut-headroom cap → output`

For luma after the global curve, write `Y = B + r`, where `B` is an edge-aware local base and `r` is the residual. Bridge v1 uses `Y_out = B + g*r` with `g` in `[1-maxResidualCompression, 1]`, driven by shadow location and regularized upstream luma confidence. Thus a constant field (`r=0`) stays constant regardless of confidence variation.

For chroma, the desired confidence-aware gain is additionally capped by the largest scalar that keeps all three RGB components inside `[0,1]` around the neutral axis. This prevents individual post-gain channel clipping from rotating the chroma direction at saturated pixels.

## 4. Native falsification results

GCC Release, Clang Release and ASan/UBSan all PASS.

Synthetic native fixtures:
- flat RGB + abrupt luma-confidence step: max output luminance gradient = `0`;
- monotonic grayscale ramp + confidence transition: minimum forward difference = `0.00178124` (PASS);
- deterministic adversarial monotonic-ramp fuzz: `200/200` random confidence/configuration trials without a negative forward luma step;
- strong step edge retention = `0.973036` (>0.95 gate);
- low-confidence/high-confidence shadow HF energy ratio = `0.629412`;
- saturated red high-confidence fixture activates the analytic gamut-headroom cap before per-channel clamp;
- censored support chroma gain never exceeds 1;
- missing conditioning audit fails closed to identity;
- integrated Manifold Conditioning power-of-two round-trip retains float32 bits and SNR.

For comparison, the previous per-pixel luma-confidence curve rule produced a synthetic flat-field seam of approximately `0.00346012` luminance for the same confidence step. Bridge v1 removes that specific artifact mechanism in the constant-field fixture.

The fuzz result is evidence for the tested parameter/domain distribution, not a mathematical proof that every possible 2-D confidence/image field preserves ordering. Broader no-reversal/no-halo testing remains open.

## 5. Real 094423 appearance stress probe

Source SHA-256: `ade9d84542916678d1a198c6baff219806d44fa8f050ef8d7545dfae90644f65`.

Diagnostic representation only: source black subtraction and WhiteLevel normalization; source OpcodeList2 GainMap applied once; measured 2x2 Bayer-cell RGB; AsShotNeutral and a display-fit scalar only for appearance inspection. No L2 color-calibration claim.

The confidence in this real probe is explicitly **diagnostic, not canonical**. It uses the source NoiseProfile/GainMap noise form and a monotone SNR mapping solely to stress the bridge.

Compared with the same global S-curve baseline:
- dark/low-confidence/flat-ish HF RMS: `0.00188985 → 0.00173829`;
- ratio: `0.919800` (~8.0% reduction in this appearance proxy);
- strong-edge median gradient retention: `0.999860`;
- bridge-vs-baseline low-frequency difference RMS: `4.99817e-05`;
- bridge-vs-baseline absolute luma difference p95: `0.000450023`.

These numbers are appearance diagnostics only. They are **not** a claim that physical sensor noise or scientific uncertainty decreased.

## 6. Scientific decision

Bridge v1 is a better research architecture than directly using Best Observation EV or raw local confidence as a spatial exposure/tone-strength control.

Promotable research claim after clean repo/CI gate: **Best Observation / Virtual EV may select numerically favorable conditioning coordinates, but only deconditioned gauge-invariant evidence may influence appearance. Visible low-confidence noise can then be compressed around an edge-aware local base without treating the virtual EV as extra evidence or a local exposure measurement.**

## 7. Open gates before replacing current S-curve v1

All three dog scenes with proper upstream confidence fields; dark high-ISO scenes; colored low-light and same-luma hue-edge fixtures; heavy source-censor/highlight transitions; SDR versus HDR behavior; broader 2-D no-halo/local-gradient tests; mobile performance; and genuinely new held-out promotion evidence where applicable.
