# TruthRaw TruthRange Latent Binding v0.2 — Validation Report

## Decision

**`LATENT_TRUTHRANGE_BINDING_RESEARCH_PASS`**

The v0.1 architecture is now connected to a real pre-render latent contract rather than directly to a display or DNG output.

## Hard passes

1. **Exact Stage-2 parity:** `max_abs = 0` against v4.7i `TruthRawProcessor.stage2Diagnostic` on the same decoded synthetic frame.
2. **Exact measured-CFA reinjection:** `max_abs = 0` in the v4.7i camera-RGB reconstruction.
3. **Signed master preservation:** negative numerical scene-linear estimates remain available.
4. **Over-1 preservation:** source-linear values above 1 remain available; 1 is not a TruthRange ceiling.
5. **High-side censor semantics:** clipped measured samples carry finite lower evidence bound and `+infinity` upper evidence bound.
6. **Dark-tail semantics:** an uncertainty band crossing physical zero yields a `-infinity` TruthRange lower tail.
7. **v5.0g unit bridge:** p50/p95 Stage-2 absolute-error bands can be transformed without changing units first.
8. **Self-gauge scale invariance:** whole-master ×37 scaling changes TruthRange by at most `1.09062e-07 EV`.
9. **Physical/common gauge fail-closed:** common absolute mode is rejected if common exposure/gain normalization is absent.

## New scientific result: calibration-free multiplicative gauge

For relative structure in one reconstructed scene, TruthRaw does not need the original ISO label or exposure scalar to define its zero-line coordinate.

A deterministic reference `L0` can be derived from positive uncensored scene evidence. Under any common positive multiplicative rescaling `L' = cL`, the derived reference becomes `L0' = cL0`, therefore:

`log2(L'/L0') = log2(L/L0)`.

This is not a trick that creates dynamic range. It removes an arbitrary multiplicative coordinate choice from the new house.

## Real-data evidence

On the eight-capture HONOR BKQ-N49 BnCam tele fixed-scene ISO sweep, a source-linear self-gauge used no ISO or exposure in the TruthRange formula. Relative upper-quantile structure remained within approximately 0.09–0.21 stop across the entire sweep.

The low quantiles vary more because underexposure/noise changes the finite evidence window. That is expected and is precisely why the new house separates unbounded address space from finite evidence quality.

## Boundaries

- SELF_GAUGE is scene-relative, not absolute radiometry.
- v5.0g remains a hidden-CFA/backend error proxy, not co-sited RGB ground truth.
- directly measured CFA channels still need their own sensor-noise uncertainty field.
- full-frame reconstructed-channel v5.0g feature extraction is not yet integrated into the native adapter.
- colorimetric covariance propagation remains open.
- the existing high-ISO black/white-dog Q5 failure remains visible and frozen.

## Next gate

**v0.3: dense uncertainty field.**

For every pixel/channel:

- measured CFA component -> source/noise/PTC uncertainty + censor bounds;
- reconstructed channels -> v5.0g p50/p95 backend-bound uncertainty;
- then TruthRange intervals;
- then covariance-aware camera-RGB -> XYZ/colorimetric master propagation.
