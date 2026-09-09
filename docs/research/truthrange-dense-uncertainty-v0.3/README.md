# TruthRaw TruthRange Dense Uncertainty v0.3

Status: **RESEARCH CONTRACT PASS — RECONSTRUCTED-CHANNEL TOPOLOGY NOT YET CERTIFIED**

v0.3 adds the first complete `pixel × RGB-channel` uncertainty container above the exact v0.2 Latent Camera Scene.

## What is now dense

For every camera-RGB sample, v0.3 can carry a marginal p50/p95 absolute-error band in the same signed Stage-2 scale as the Latent Camera Scene and can transform that band into TruthRange intervals.

### Directly measured CFA component

The actually measured channel at each Bayer site uses source-bound DNG `NoiseProfile` with the exactly-once GainMap scale:

`Var(stage2) = g * S * max(stage2, 0) + g^2 * O`

where `g` is the Stage-2 GainMap factor and `(S,O)` are the source DNG NoiseProfile coefficients for that color channel.

For the current Gaussian-equivalent marginal bridge:

- `p50_abs = 0.6744897502 * sigma`
- `p95_abs = 1.9599639845 * sigma`

This is a **NoiseProfile Gaussian-equivalent uncertainty coordinate**, not physical electron/PTC calibration.

A WhiteLevel-censored measured sample does not receive a false exact-value Gaussian band. It remains a finite evidence lower bound with an unbounded bright tail in TruthRange.

### Reconstructed RGB components

v5.0g was calibrated on hidden-CFA recovery at real measured CFA-role locations. It was **not** calibrated directly for the two missing color channels at every arbitrary Bayer topology.

v0.3 therefore refuses to run v5.0g as if those missing-channel topologies were already certified.

Instead, the contract accepts v5.0g anchors only where the target color was physically measured. A missing channel can receive the local maximum p50/p95 of nearby same-color measured-role anchors. This produces a dense numeric field, but every transported value is marked:

`V5G_LOCAL_MAX_TRANSPORT_PROXY`

and:

`topologyCertified = false`.

The local-maximum rule is deliberately non-optimistic relative to the supplied anchors. It is still only a proxy for missing-channel topology, not co-sited RGB ground truth.

## Covariance

v0.3 does **not** invent zero correlation between RGB errors.

Status remains:

`MARGINAL_ONLY_OFF_DIAGONAL_UNRESOLVED`

Therefore p50/p95 marginal bands must not be promoted to a full RGB covariance matrix or a certified XYZ p95 ellipsoid yet.

## Native validation

Synthetic exact-contract test:

- measured uncensored entries: `1918`
- measured clipped entries: `2`
- measured NoiseProfile formula max error: `4.63367e-09`
- reconstructed transported proxy entries: `3840`
- unresolved reconstructed entries after anchor transport: `0`
- reconstructed entries incorrectly topology-certified: `0`
- finite reconstructed TruthRange p95 upper intervals: `3791`
- dark-side p95 lower tails reaching `-infinity`: `87`
- clipped measured evidence upper bounds at `+infinity`: `2`
- common coordinate ×37 rescale interval error: `2.17723e-08 EV`

The test also verifies fail-closed rejection when a backend anchor is placed on a channel that was not directly measured at that CFA site.

## Real BnCam source diagnostic

Eight HONOR BKQ-N49 BnCam tele DNGs spanning ISO 100..12800 were analyzed in **pre-GainMap normalized source units**.

The real source diagnostic deliberately does not pretend that OpcodeList2/GainMap has already been decoded by the v0.3 real-DNG path.

Median green source-domain sigma rises from approximately:

- ISO 100: `0.000992888`
- ISO 12800: `0.009839207`

This is source-bound NoiseProfile behavior, not a PTC/electron measurement and not yet the full real-DNG Stage-2 uncertainty field.

## Scientific boundary

v0.3 has achieved a dense **data contract and mechanically complete proxy field**.

It has **not** achieved topology-certified uncertainty for missing RGB channels, independent RGB covariance, physical electron uncertainty, or full real-DNG OpcodeList2-to-Stage2 integration.

The next scientifically justified step is to integrate the exact v5.0g measured-role anchor feature extractor and exact GainMap decoder, then test whether transported missing-channel uncertainty can be independently bounded or must remain a proxy class.
