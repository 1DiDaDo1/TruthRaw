# TruthRaw v0.81 — Canonical Output Acutance v4.7k — 2026-09-20

## Status

Integration candidate on:

`integration/truthraw-suite-v0-81-output-acutance-v47k`

Android version:

`0.47-v0.81-output-acutance-v47k`

v0.81 integrates canonical v4.7k strictly at the finite output boundary.

## Exact order

`final resized linear SDR base -> canonical v4.7k Output Acutance -> HDR rebase against that adjusted base -> highlight shoulder -> sRGB OETF / ARGB presentation`

The stage does not run on:
- source RAW/CFA;
- Scientific Master;
- pre-resize reconstruction;
- Open Scene authority state;
- Restoration scientific state.

## Canonical implementation

Android compiles the exact source:

`canonical/output-acutance/v4.7k/native/output_acutance.cpp`

The TruthRaw v0.81 bridge calls:
- `choose_output_acutance_plan()`
- `apply_output_acutance()`

The filter math is not reimplemented in the Android bridge.

Output profile:
- Detail off -> canonical `Neutral`
- Detail on -> canonical `AdaptiveDetail`

Noise input is the same source-metadata NoiseProfile sigma-at-2%-signal quantity used by canonical v4.7j/v4.7k semantics.

Resize ratio is measured from the actual oriented source/display dimensions to the actual final preview dimensions, not guessed from the requested max edge.

## Final SDR base

The existing downstream bounded Light adjustment remains explicitly `APPEARANCE_ONLY`.

For v0.81 the final resized base is constructed as:
1. streamed SDR is sampled into the final preview raster;
2. optional presentation-only restoration changes only eligible preview presentation pixels;
3. bounded Light appearance is applied only to non-censored support;
4. this resulting linear preview raster is the input to canonical v4.7k.

v4.7k therefore runs after the final resize and after other base-defining presentation changes, but before HDR gain and OETF.

## HDR rebase

The pre-v0.81 pipeline stored bounded HDR transport derived before output acutance.

v0.81 does not reuse that gain blindly.

For every non-censored pixel that already had positive upstream HDR gain:
1. compute the previous effective HDR display target from the pre-acutance final SDR base and the existing bounded display gain;
2. run canonical v4.7k;
3. derive a new direct display gain relative to the acutance-adjusted base;
4. cap it at the existing maximum display gain;
5. apply that rebased gain after v4.7k.

### Stronger invariant found by falsification

The first v0.81 test iteration showed that exact luminance preservation could create gain >1 at a pixel whose upstream HDR gain was exactly unity when v4.7k locally darkened the SDR base.

That is forbidden.

Current rule:

> Output acutance may re-express existing positive HDR transport, but may not create a new positive HDR gain where upstream HDR gain was unity.

Therefore:
- upstream zero/no-HDR gain -> output gain remains exactly 1;
- censored support -> output gain remains exactly 1;
- only pre-existing positive HDR transport is rebased.

This preserves authority semantics even when exact pre-acutance luminance cannot be retained at a zero-HDR pixel.

## Output child binding

Schema/domain:

`TruthRawOutputAcutanceBinding/0.81`

The binding includes:
- canonical Open Scene v0.70 SHA;
- v0.78 channel-authority SHA;
- v0.79 uncertainty-admission decision SHA;
- optional v0.80 Adaptive Detail binding SHA;
- output profile;
- actual final raster width/height;
- canonical plan noise sigma;
- actual resize ratio;
- resize need;
- strength;
- delta cap;
- changed-pixel count;
- HDR-rebased-pixel count;
- maximum effective HDR-target absolute error.

It also binds the fixed semantic statements:
- post-final-resize only;
- acutance before HDR rebase;
- HDR rebase before OETF;
- Scientific Master unchanged;
- authority unchanged;
- no optical/sensor evidence created;
- zero upstream HDR stays unity;
- censored HDR stays unity.

## Memory accounting

v0.81 changes the Advanced preview sink's resident accounting from current vector capacities to a deterministic upper bound known before `beginFrame()`.

The bound includes:
- final resized linear SDR raster;
- sampled upstream half-log gain;
- pre-acutance base;
- acutance-adjusted base;
- rebased direct display gain;
- ownership maps;
- censor mask;
- ARGB presentation raster.

Compute/memory strategy may change execution cost, never scientific authority.

## Standalone validation

The v0.81 host gate is green on:
- GCC Release;
- Clang Release;
- Clang ASan/UBSan.

It verifies:
- canonical plan float-bit parity;
- canonical v4.7k output float-bit parity;
- positive-HDR target preservation in the unclamped domain;
- no new gain at upstream zero-HDR pixels;
- censored gain exactly 1;
- display gain never below 1 or above the existing maximum;
- invalid geometry/noise/resize contracts fail closed.

## Frozen science

Unchanged:
- immutable source evidence;
- physicalFrameCount=1;
- independentEvidenceCount=1;
- Scientific Master reconstruction v4.7i;
- PURE v0.63;
- Zero-Line/L0;
- scene-scale;
- Technical Backplane;
- canonical Open Scene v0.70;
- v0.78 authority;
- v0.79 uncertainty admission;
- v0.80 Adaptive Detail authority boundary;
- Restoration v0.67 scientific algorithm.

## Remaining closure gate

Android integration CI and APK verification must pass before v0.81 becomes the current closed integration baseline.
