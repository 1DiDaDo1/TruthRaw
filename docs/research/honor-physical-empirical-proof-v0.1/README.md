# Honor Physical Empirical Proof v0.1

Status: **PHYSICAL DEVICE PASS — SCOPED TO ONE EXACT SOURCE / BUILD / DEVICE LINEAGE**

This record captures the first physical-device execution in which the validated TruthRaw finalized source-bound Scientific Preview route completed on the target Honor device and produced a visible preview.

It is deliberately narrower than a blanket device/lens/camera certification.

## Exact tested lineage

- Device manufacturer: `HONOR`
- Device model: `BKQ-N49`
- Device codename: `HNBKQ`
- Android release: `16`
- Android SDK: `36`
- App package: `com.truthraw.adaptiveui`
- App version: `0.3` (`versionCode=3`)
- Installed APK SHA-256: `fb89764b235ae3895e558f8e023ff8d3e8a3d4b4eb47040827f66d32113f9aa9`
- Validated scientific route SHA: `42b49ba16a6c5a0d2d6dbc407330acde3e161a46`
- UI/layout head used by the tested APK: `257aff1470619c2ef40460ad2a2205412a273fbf`

Source:
- filename: `IMG_260830_143012_297_014.dng`
- declared bytes: `25,369,034`
- exact source SHA-256: `fac842110cfd5b527aae2fa46237824d444aa5701017c2b68021962ebce47da3`
- source stable across pre/post empirical wrapper: `true`
- color probe stable across pre/post empirical wrapper: `true`

User-exported empirical JSON:
- bytes: `4,846`
- SHA-256: `319d8b17f8d7c5b51ec0db5b6b06d327157228e7a43e8ea6728518068ea8364d`

User-exported visible JPEG preview:
- dimensions: `1536 x 1156`
- color mode: `RGB`
- bytes: `125,852`
- SHA-256: `da225f57726051d123a91ed3a07fd2a9cf7f8fcb605d15dd3db672bcdc65f43d`
- JPEG bytes are **not** stored in this repository.

## Physical pass result

Preview outcome:
- `READY`
- authority: `FINALIZED_SOURCE_BOUND_SCIENTIFIC_PREVIEW`
- source-bound appearance release: `true`
- Scientific Preview release: `true`
- stronger physical-color scientific claim: `false`

Evidence contract:
- `physicalFrameCount = 1`
- `independentEvidenceCount = 1`
- full RAW materialized: `false`
- source dimensions: `4080 x 3072`
- GainMap/gain field present: `true`
- orientation: `1`
- pass-1 tiles: `768`
- pass-2 tiles: `768`

This confirms physical execution of the direct-native finalized route for this exact lineage. It does **not** promote source metadata color to independent physical calibration.

## DNG color result

The source was processed through `DUAL_ILLUMINANT_V0_2` rather than the historical single-illuminant delegate.

Observed color-binding state:
- status code: `0`
- dual illuminant used: `true`
- third calibration seen: `false`
- ForwardMatrix used: `true`
- same ForwardMatrix across temperatures: `false`
- CameraCalibration present: `true`
- CameraCalibration signature matched: `true`
- CameraCalibration applied: `true`
- CalibrationIlluminant1: `21`
- CalibrationIlluminant2: `17`
- solved white temperature: `4841.036 K`
- inverse-CCT interpolation low weight: `0.267578212`
- neutral solve iterations: `8`
- color authority code: `2` (`SOURCE_METADATA_BOUND` lineage)

Pre- and post-probe values matched, including exact source SHA-256 and the full color-binding result above.

## Runtime baseline

Scope: finalized tile preview loader only; empirical pre/post probes excluded.

- pipeline wall time: `110860.745531 ms`
- worker thread CPU time: `108010.258178 ms`
- PSS before: `87,215 KiB`
- PSS peak: `90,586 KiB`
- PSS after: `87,102 KiB`
- thermal start: `NONE`
- thermal peak: `NONE`
- thermal end: `NONE`

UI frame pacing over the full visible empirical run:
- intervals: `7657`
- p50: `16.59243 ms`
- p95: `16.59735 ms`
- max: `17.5 ms`

I/O counters from the finalized preview route:
- RAW payload bytes read: `196,873,760`
- metadata bytes read: `8,824,391`
- tile read calls: `13,824`
- source-resident upper bound: `5,254 bytes`
- logical-resident upper bound: `1,679,366 bytes`

The roughly 7.76x RAW-payload reread relative to the ~25.37 MB source is now the primary performance-optimization target. Any optimization must preserve Scientific Master identity, zero-line/scene-scale bindings, Backplane lineage, final preview pixels and claim authority.

## Claim boundary

This physical run proves only:

`HONOR BKQ-N49 + Android 16 + APK fb89764b…f9aa9 + source fac84211…7da3 -> finalized source-bound Scientific Preview PASS`

It does **not** prove:
- all Honor RAWs;
- all MotionCam RAWs;
- all lenses or sensor modes;
- vendor-RAW Gatehouse decoding;
- independent camera/lens physical color calibration;
- `FULL_PHYSICAL` color;
- multi-frame evidence;
- universal runtime performance.

The source metadata color route remains `SOURCE_METADATA_BOUND`. `FULL_PHYSICAL` remains blocked until independent camera/lens calibration exists.

## Performance optimization baseline law

The baseline for the next phase is immutable:

- source SHA-256 must remain `fac842110cfd5b527aae2fa46237824d444aa5701017c2b68021962ebce47da3`;
- physical/evidence counts remain `1/1`;
- Scientific Master digest must remain unchanged for the same input and scientific implementation;
- zero-line and scene-scale bindings must remain unchanged;
- Technical Backplane phase-2 bytes must remain unchanged;
- finalized preview pixels must remain bit-identical for the same renderer configuration;
- color authority remains `SOURCE_METADATA_BOUND`;
- no cache/scheduling optimization may change truth authority.

Primary engineering target: reduce redundant source/tile reads and wall time while preserving all invariants above.

**Measured where measured. Reconstructed where necessary. Never invented.**
