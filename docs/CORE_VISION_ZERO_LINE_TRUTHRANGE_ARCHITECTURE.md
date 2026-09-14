# TruthRaw Core Vision — Zero-Line / TruthRange Architecture

**Status: CANONICAL PROJECT ARCHITECTURE**

This document extends the canonical sealed-house/new-house vision. Future TruthRaw work and future chats must preserve this distinction unless the user explicitly changes it.

## 1. The zero line / nul-lijn

TruthRaw may describe positive latent scene light on a logarithmic relative-light coordinate:

`T = log2(L / L0)`

where:

- `L` is a positive latent scene-light quantity in a documented relative or physical gauge;
- `L0 > 0` is the chosen project/reference level;
- `T = 0` is the **zero line / nul-lijn**.

The zero line is a reference/gauge. It is **not sensor black, display black, absolute darkness, middle gray by definition, or a clipping point**.

If `L -> +infinity`, then `T -> +infinity`.

If `L -> 0+`, then `T -> -infinity`.

Therefore the TruthRange address space is not required to have a finite bright ceiling or dark floor.

### Historical design intent: both directions

The recovered house metaphor was deliberately two-sided around the nul-lijn. The new reconstructed house was intended to be extensible without a finite representational ceiling **above** the line and without a finite representational floor **below** the line.

Conceptually:

`darkness <- ... <- -EV <- nul-lijn -> +EV -> ... -> brighter light`

This is the project meaning of saying the new house can keep being built upward and downward. It is an address-space property, not a physical-sensor claim.

**The house can be unbounded in both directions. The evidence inside it is finite.**

## 2. Dynamic range has four different meanings

TruthRaw permanently distinguishes four ranges.

### A. TruthRange address space

The mathematical coordinate can extend from `-infinity` to `+infinity`.

This is a property of the **new house**.

RAW10, WhiteLevel, source ISO, DNG, SDR/HDR displays and integer storage do not define its ceiling or floor.

### B. Evidence-supported dynamic range

The sealed source RAW provides useful direct evidence only over a finite capture-dependent interval.

The lower side is limited by black/read/shot/quantization uncertainty and the upper side by saturation/censoring.

This remains real and must never be hidden.

### C. Reconstructed-support dynamic range

TruthRaw may estimate scene values beyond the directly measured interval where the forward model and neighboring/channel/optical/statistical evidence support reconstruction.

This range may exceed the evidence range, but reconstructed estimates retain support class, bounds and uncertainty.

### D. Presentation dynamic range

SDR, HDR, DNG, JPEG, display or virtual-camera output use a finite projection of the master.

Presentation range never redefines the scientific master.

## 3. Canonical consequence

**Unbounded representational dynamic range is an architectural property.  
Evidence-supported captured dynamic range is a measurement property.**

They are not the same claim.

The first may be unbounded by construction.

The second cannot be enlarged merely by changing coordinates.

## 4. Scale/gauge invariance

If scene light and its reference are multiplied by the same positive constant `c`:

`L' = cL`

`L0' = cL0`

then:

`log2(L'/L0') = log2(L/L0)`.

Therefore relative stop distances do not require an absolute radiometric unit.

Absolute radiometric calibration may later bind the gauge to physical units. If it changes only the common multiplicative scale, the TruthRange coordinate changes by at most the corresponding common reference choice/offset; scene-relative stop relationships remain intact.

This is the strongest valid interpretation of the user's statement that the zero line can replace calibration:

**the zero line can replace the need for a bounded or intrinsically absolute master brightness scale. It does not replace the forward model required to place source evidence correctly on that scale.**

## 5. Calibration changes role

Calibration is not what grants TruthRaw permission to have more dynamic range.

The TruthRange address space already has no finite source-container ceiling/floor.

Calibration instead answers:

- where does this source measurement lie on the common scene coordinate?
- how uncertain is that placement?
- what source state produced it?
- how much of the value is measured, bounded, reconstructed or unknown?

Black/offset, exposure, gain/readout, linearity, clipping, noise/quantization, color/channel response and optical response therefore remain scientifically relevant.

**Calibration locates evidence in the house; it does not determine how high or deep the house is allowed to exist.**

### Capture/sample-domain identity

The 2026-09-14 Honor dark-frame experiments add a specific measurement-layer rule:

**noise calibration must be bound to the exact capture/sample domain, not to ISO magnitude alone.**

Two independent exact-ISO8192 dark captures reproduced an unusual high-scale/censored domain, while nearby ISO8184 and higher ISO10244 at the same exposure remained in the ordinary domain. A simple monotonic `ISO >= 8192` rule is therefore rejected.

Until the causal control variable is independently identified, this is named the **reproducible discrete ISO8192-associated capture/sample domain**.

A calibrated noise model must fail closed rather than interpolate across such an observed domain discontinuity. This affects evidence placement/uncertainty; it does not redefine TruthRange or the Scientific Master.

## 6. ISO rule

ISO/gain remains immutable capture provenance.

It can alter:

- the finite evidence window;
- clipping behavior;
- read/noise behavior;
- uncertainty;
- source-response scaling.

It does **not** have to remain the numerical identity of the reconstructed scene.

The TruthRange master may be ISO-neutral after the documented source forward model is accounted for.

**ISO belongs to the measurement history, not to the scene coordinate.**

The capture/sample-domain finding above also means ISO alone may be insufficient to identify the relevant sensor/noise coordinate regime.

## 7. Highlight censoring

Source WhiteLevel is not the top of TruthRange.

When a source sample clips, the correct high-side statement is a lower bound such as:

`T >= T_clip_lower`

with:

`upper_bound = +infinity`

until additional valid evidence/model support narrows the posterior.

TruthRaw may reconstruct a finite highlight estimate above the clipped bound, but that estimate remains reconstructed/bounded rather than newly measured.

## 8. Darkness and the lower tail

Sensor black is not `T = -infinity`.

A noisy low signal generally cannot prove exact zero physical light.

When evidence becomes noise-limited, TruthRaw may represent a statement such as:

`T <= T_dark_upper`

with a lower tail toward `-infinity`.

Exact physical darkness is not asserted merely because the coordinate permits `-infinity`.

### Empirical dark-side reinforcement — 2026-09-14

Honor/MotionCam source measurements now provide direct physical test evidence that post-black numerical values below zero occur in real dark/shadow data.

In one real-scene tele ISO series, below-black samples increased to 59,573 sensels at ISO12800, about `0.4753%` of all sensels, with a minimum observed black-corrected value of `-22 DN`.

Covered/dark-frame repeats then showed approximately `0.79 DN` temporal sigma at ISO100 and `0.87 DN` at ISO400 across CFA channels.

These observations reinforce the existing rule:

**do not hard-clip the signed post-black/scientific estimator to zero.**

They do not mean physical light is negative. They mean the estimator must remain signed around an uncertain measurement zero while the positive-light TruthRange coordinate carries the physical-light interpretation and its dark-side bounds.

Detailed evidence is recorded in:

`docs/research/zero-line-empirical-validation-v0.1/TRUTHRAW_ZERO_LINE_HISTORY_AND_EMPIRICAL_STATE_2026-09-14.md`

and:

`docs/research/zero-line-empirical-validation-v0.1/TRUTHRAW_ZERO_LINE_EMPIRICAL_STATE_2026-09-14.json`

## 9. Dual-coordinate master

TruthRange does not replace TruthRaw's signed scene-linear reconstruction domain.

TruthRaw should retain both:

1. **signed scene-linear estimator** — used for reconstruction, residuals, unbiased numerical estimation and color/forward-model operations;
2. **TruthRange companion coordinate** — used to express positive latent light relative to the zero-line gauge, including bounds and unbounded tails.

Small negative numerical scene-linear estimates are not negative physical light and must not be naively transformed through `log2`.

The new dark-frame evidence makes this separation empirically important rather than merely theoretical.

## 10. Required TruthRange semantics

Where practical, each sample/tile/master representation should be able to carry:

- `mu_linear_signed`
- `truth_range_estimate_ev`
- `truth_range_lower_ev`
- `truth_range_upper_ev`
- `truth_range_uncertainty`
- `truth_range_support_class`
- `censor_state`
- `scale_gauge_id`
- optional absolute physical-scale binding

Support classes remain distinguishable, including:

- measured uncensored;
- measured high-side censored lower bound;
- measured dark/noise-side upper bound;
- reconstructed strong;
- reconstructed weak;
- unknown;
- appearance only.

Future empirical calibration identity may additionally need a versioned `capture_sample_domain_id` or equivalent. It must not be inserted silently into a frozen Backplane/certificate serialization; any such addition requires an explicit versioned schema.

## 11. Current practical validation

TruthRaw zero-line implementation v0.1 tested an eight-capture HONOR BKQ-N49 BnCam tele sweep from ISO 100 through ISO 12800.

Using the provisional research coordinate:

`L_proxy = black-subtracted source-white-normalized signal / (ISO * exposure_seconds)`

the fixed central-scene TruthRange spread was approximately:

- p50: `0.093109 EV`
- p90: `0.132132 EV`
- p99: `0.133236 EV`

This is practical evidence that an ISO-neutral relative scene coordinate is viable for this sweep.

However, the specific `ISO * exposure` mapping is **research/provisional**, not a canonical physical gain calibration. The next implementation must bind TruthRange to the exact Stage-2 / Latent Scene Master and backend uncertainty, using stronger measured gain mapping where available.

The 2026-09-14 MotionCam campaigns add source-bound evidence for gain response, repeatability, signed below-black behavior and dark-frame temporal noise. They do not turn the provisional TruthRange mapping into absolute radiometric calibration.

## 12. Permanent claim boundary

The canonical TruthRaw statement is:

> **The new house has an unbounded TruthRange address space around a zero-line gauge. The sealed RAW exposes only a finite evidence window inside that space. TruthRaw may reconstruct outside that window with explicit bounds and uncertainty, but it never converts unmeasured range into measured photons.**

In shorter form:

**The axis can be infinite in both directions. The evidence is finite. The reconstruction may go beyond the evidence only as reconstruction.**

This architecture is part of the sealed-house vision and must be read before interpreting dynamic-range, ISO, clipping, calibration or HDR work.
