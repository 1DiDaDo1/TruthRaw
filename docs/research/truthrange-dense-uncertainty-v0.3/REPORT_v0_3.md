# TruthRaw TruthRange v0.3 — Dense Uncertainty Validation Report

## Decision

**`DENSE_UNCERTAINTY_CONTRACT_RESEARCH_PASS_TOPOLOGY_PROXY_OPEN`**

## Closed in v0.3

1. A dense `3 × width × height` uncertainty container exists in the exact v0.2 Latent Camera Scene scale.
2. Direct measured CFA components receive source-bound DNG NoiseProfile Gaussian-equivalent marginal bands.
3. GainMap/noise transformation is explicitly consistent with Stage-2: `Var(stage2)=g*S*max(stage2,0)+g^2*O`.
4. Source-clipped measured components fail closed to censor bounds instead of receiving an exact latent-value uncertainty band.
5. Backend anchor fields are accepted only on physically measured same-channel CFA sites.
6. Supplying a v5.0g anchor on an unmeasured channel topology is rejected.
7. Same-color anchor transport fills all synthetic missing RGB entries while keeping every transported entry `topologyCertified=false`.
8. Dense marginal bands transform to asymmetric TruthRange p50/p95 intervals with correct `-infinity` dark tails and `+infinity` clipped evidence tails.
9. Whole-coordinate multiplicative rescaling of `mu`, uncertainty widths, and `L0` leaves TruthRange intervals invariant to `2.17723e-08 EV` in the native test.

## Native test results

- measured valid: `1918`
- measured clipped: `2`
- measured formula max abs error: `4.63367e-09`
- reconstructed proxy entries: `3840`
- unresolved reconstructed after transport: `0`
- topology-certified reconstructed: `0`
- finite reconstructed TruthRange intervals: `3791`
- dark lower-infinity p95 tails: `87`
- bright evidence upper-infinity: `2`

## Real source evidence

The BnCam tele fixed-scene ISO sweep was evaluated in pre-GainMap normalized sensor units using each source DNG NoiseProfile.

Median green sigma:

- ISO100: `0.0009928875252372472`
- ISO200: `0.0014889103841191637`
- ISO400: `0.0024348609817445408`
- ISO800: `0.0029206895739442833`
- ISO1600: `0.0038413122565194805`
- ISO3200: `0.005118023806927728`
- ISO6400: `0.006928242621665336`
- ISO12800: `0.009839206510685997`

This proves neither physical read noise nor electrons. It demonstrates the source-bound finite-evidence behavior that the zero-line architecture is designed to keep separate from the unbounded TruthRange address space.

## Still open

### Missing-channel topology certification

v5.0g's calibrated hidden-CFA role domain is not the same as every missing-channel demosaic topology. Transported anchors therefore remain backend proxies.

### Exact real-DNG GainMap bridge

The native formula is tested with decoded GainMap fields, but the repo path still needs an exact OpcodeList2 decoder/source-admission bridge for real DNGs.

### Covariance

Off-diagonal camera-RGB error covariance is unresolved. v0.3 does not assume independence.

### Physical uncertainty

DNG NoiseProfile is not a substitute for the open PTC/electron calibration campaign.

## Next gate

**v0.4 / v0.3b: real-anchor and topology study**

- make v5.0g feature extraction portable and source-bound;
- decode/apply OpcodeList2 exactly once in the real-DNG path;
- produce real measured-role anchor fields;
- evaluate whether missing-channel transported proxies can be independently bounded;
- only then attempt covariance-aware camera-RGB to XYZ uncertainty propagation.
