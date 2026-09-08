# TruthRaw Zero-Line Dynamic Range v0.1

**Status: RESEARCH_ARCHITECTURE_CANDIDATE — NOT YET CANONICAL CORE**

This research candidate studies the user's zero-line extension of the sealed-house vision.

## Proposed coordinate

For positive latent scene light `L` and an arbitrary positive project reference `L0`:

`T = log2(L / L0)`

- `T = 0` at `L0`
- `T -> +infinity` as `L -> +infinity`
- `T -> -infinity` as `L -> 0+`

If both `L` and `L0` are multiplied by the same positive scale factor, `T` is unchanged. The zero line therefore fixes a **gauge/reference**, not a ceiling or floor.

## Main result of the study

TruthRaw should distinguish four meanings of dynamic range:

1. **Address-space DR** — the TruthRange coordinate is unbounded.
2. **Evidence-supported DR** — finite, set by the sealed capture's noise/detection floor and saturation.
3. **Reconstructed-support DR** — may extend beyond direct evidence, but must carry bounds/support/uncertainty.
4. **Presentation DR** — finite SDR/HDR/DNG/display projection only.

This makes the user's vision scientifically workable:

**calibration no longer determines how high or low the new house is allowed to extend; it determines where sensor evidence lies on an already unbounded axis and how uncertain that placement is.**

## What the zero line can remove

- a bounded RAW10/WhiteLevel master range;
- ISO as the identity/scale of the reconstructed scene;
- the need for an absolute brightness unit when only relative stop differences matter;
- display/DNG range as a master constraint.

## What it cannot remove

The source forward model is still required for black/offset, exposure, gain/readout state, linearity, noise/quantization, clipping/censoring, color response and optics.

A zero line cannot make a clipped highlight exact or a noise-dominated shadow known.

## Censor semantics

- saturated source: `T >= finite_lower_bound`, with `upper = +infinity` until other evidence narrows it;
- below-detection source: `T <= finite_upper_bound`, with a possible tail toward `-infinity`;
- uncensored supported source: finite estimate/bounds.

## Dual-master rule

Do not replace the existing signed scene-linear estimator. Keep:

- signed linear reconstruction for unbiased numerical work;
- TruthRange as a companion positive-light ratio coordinate.

Negative numerical residuals are not negative physical light.

## BnCam 094414 demonstration

Using the exact DNG NoiseProfile only as metadata-bound illustration and choosing 18% as a temporary visualization reference:

- saturation: `+2.474 EV`;
- illustrative 3-sigma dark threshold: R `-6.244 EV`, G `-7.257 EV`, B `-6.320 EV`;
- corresponding finite direct windows: about `8.72`, `9.73`, `8.79` stops.

The coordinate itself remains unbounded. This example is **not physical PTC calibration**.

## Promotion test

If later testing supports this architecture, TruthRaw should canonize:

**UNBOUNDED_REPRESENTATIONAL_DR** as a property of the Latent Scene Master.

**EVIDENCE_SUPPORTED_DR** remains a finite measured property of each source capture/domain.

Changing coordinates can enlarge the first. It can never enlarge the second.

Permanent boundary:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
