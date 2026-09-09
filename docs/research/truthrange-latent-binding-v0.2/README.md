# TruthRaw TruthRange Latent Binding v0.2

Status: **RESEARCH PASS — NOT YET THE CANONICAL MASTER FORMAT**

v0.2 moves TruthRange off the v0.1 `ISO × exposure` proxy and onto an explicit Latent Camera Scene contract.

## What changed

The new native adapter creates, before appearance/rendering:

- signed Stage-2 CFA evidence;
- signed v4.7i reconstructed camera RGB;
- exact measured-channel identity per CFA site;
- source WhiteLevel censor mask;
- finite source high-side lower bounds;
- exact reconstruction/uncertainty provenance binding.

The adapter preserves negative numerical estimates and values above 1.0. Neither is silently clipped.

## Self-gauge zero line

For the user's zero-line idea, v0.2 introduces a deterministic **SELF_GAUGE**.

`L0` is derived from positive, uncensored Stage-2 evidence and:

`T = log2(L/L0)`

uses neither ISO nor exposure.

If the complete source-linear scene is multiplied by a positive constant `c`, both `L` and `L0` scale by `c`, so `T` remains unchanged. Native validation measured a maximum difference of only `1.09062e-07 EV` after multiplying a complete synthetic latent master by 37.

This is the strongest current implementation of the idea that the zero line can replace multiplicative capture-scale calibration for **relative structure within one scene**.

It does not make two unrelated scenes absolutely radiometrically comparable. A common/physical gauge remains an optional separate binding for that purpose.

## Exact v4.7i binding

The adapter uses the actual `ResearchEdgeAwareMeasuredPreservingReconstruction` backend. On the same synthetic decoded frame:

- Stage-2 diagnostic parity with `TruthRawProcessor`: `max_abs = 0`;
- measured CFA reinjection into camera RGB: `max_abs = 0`.

## v5.0g uncertainty

v5.0g outputs p50/p95 **absolute-error bands in Stage-2 normalized scene-linear units**. v0.2 transforms those bands into asymmetric TruthRange intervals.

If a linear p95 band crosses zero, the correct lower TruthRange tail becomes `-infinity`.

If a directly measured source channel is clipped, its evidence interval becomes a finite lower bound with `upper = +infinity`.

The black/white-dog generalization failure remains open and the model is not retuned here.

## Real BnCam ISO sweep without ISO/exposure scaling

The fixed-scene BnCam tele sweep ISO 100..12800 was re-evaluated with a self-gauge. ISO and exposure were read only as provenance and were not used in `L0` or `T`.

Relative spread across the eight captures:

- q75: `0.093109 EV`
- q90: `0.177174 EV`
- q95: `0.208256 EV`
- q99: `0.213213 EV`

This is evidence for a multiplicative-scale-free **relative** scene coordinate, not absolute radiometry.

## Next

The next v0.3 step is a dense per-channel uncertainty bridge: measured CFA noise + v5.0g reconstructed-channel bands over the full latent camera RGB master, followed by covariance-aware colorimetric propagation.
