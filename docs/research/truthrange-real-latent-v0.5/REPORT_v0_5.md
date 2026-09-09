# TruthRange v0.5 validation report

## Decision

**REAL_DNG_LATENT_DENSE_TRUTHRANGE_RESEARCH_PASS**

## What v0.5 closes

1. A real sealed BnCam DNG can enter v0.4 and arrive in the v0.2 Latent Camera Scene with Stage-2 exactly preserved.
2. The measured CFA value is reinjected exactly into the corresponding Latent Camera Scene RGB channel.
3. The exact frozen v5.0g 18-feature extractor is reproduced, including its historical one-hot label/order quirk.
4. Target anti-leakage is enforced by construction and the synthetic test changes the central measured target without changing any feature or prediction.
5. Dense uncertainty/TruthRange can be evaluated tile-wise using one global self-gauge without changing the result.

## Real-data gates

- 8/8 Stage-2 parity: `0` max absolute error.
- 8/8 measured-CFA reinjection: `0` max absolute error.
- 64/64 independent probes span all four roles.
- max independent feature error: `4.76837158e-07`.
- max independent scalar absolute error: `1.90734863e-06`.
- max independent scalar relative error: `1.50580961e-06`.
- tile128 versus tile256: exact for all eight captures.
- ISO12800 one full-frame tile versus tile256: exact.

## Censor behavior

Source-white samples are not assigned exact scene radiance. In the real series, measured high-censor entries occur at ISO3200/6400/12800 and the same counts receive `evidenceUpperEv=+infinity`.

## Memory architecture

For ISO12800, tile256 peaked at about 480,316 KiB; one full-frame tile peaked at about 686,964 KiB. Tiling therefore preserves the scientific result while reducing dense working memory. The full Latent Camera Scene remains first-class in v0.5.

## Not closed

- missing-channel topology certification;
- RGB covariance;
- absolute radiometry;
- physical electron/read-noise/full-well calibration;
- FULL_PHYSICAL color/illuminant/optics.

The next scientifically justified step is covariance-aware propagation toward the colorimetric Scene Master, without converting these open items into false certainty.
