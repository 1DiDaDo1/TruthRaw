# TruthRaw v0.48 — targetSdk 37 RAW14 retarget control

Date: 2026-09-19

## Why this control exists

v0.47 ran successfully on Android 17 / SDK 37, but the TruthRaw package itself was still built with compileSdk 35 and targetSdk 35.

v0.47 established:

- the Android 17 runtime contains `ImageFormat.RAW14` with numeric value 44;
- no checked camera advertised RAW14 through the queried stream maps;
- physical Camera 5 still advertises:
  - RAW_SENSOR 4080x3072 default;
  - RAW10 4080x3072 default;
  - RAW_SENSOR / RAW10 8160x6144 maximum-resolution;
  - RAW_SENSOR / RAW10 16320x12288 high-resolution under the maximum-resolution map;
- public extension IDs 0..4 were unsupported for all checked cameras.

Before treating RAW14 non-exposure as final, v0.48 performs one clean compatibility control.

## Experimental change

The **measurement implementation remains the v0.47 `Api37Raw14ExtensionOracleActivity`**.

Only the app build/runtime contract is deliberately changed:

- compileSdk: **35 remains unchanged**
- targetSdk: 35 -> **37**
- versionCode: 17
- versionName: `0.17-v0.48-target37-raw14-retarget-control`

The launcher label is updated so the user can distinguish the control APK. The generated JSON intentionally retains the v0.47 oracle schema because the measurement logic is unchanged; the report itself exposes `truthRawCompileSdk` and `truthRawTargetSdk`, so the control is self-identifying by target SDK.

## Authority

`CAMERA2_AND_EXTENSION_CHARACTERISTICS_OBSERVATION_ONLY`

No camera open, CaptureRequest, ImageReader, image buffer access, Honor Binder call, vendor request write or CameraExtensionSession creation is permitted.

## Decision rule

Compare v0.48 against v0.47 on the same Android-17 build.

### If RAW14 remains absent

Then the RAW14 non-exposure result survives app retargeting to API 37. Do not make a blind RAW14 capture attempt through an unadvertised stream configuration.

### If RAW14 appears

Record exact camera ID, map class and output size first. Only then build a separate controlled RAW14 capture + payload audit.

### Extensions

If a public extension becomes visible after retargeting, record it but do not create an extension session in this control.

Unknown vendor extension IDs are still not brute-forced.