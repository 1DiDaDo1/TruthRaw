# TruthRaw v0.48 — targetSdk 37 RAW14 retarget control device result

Date: 2026-09-19

## Source

- `TRUTHRAW_API37_RAW14_EXTENSION_ORACLE_v047 (1).json`
- bytes: `19640`
- SHA-256: `7143c11c370adde2afa6484acfd0da931a0ede9406de2344675336c9309821e3`

Authority remains `CAMERA2_AND_EXTENSION_CHARACTERISTICS_OBSERVATION_ONLY`.

## Controlled change succeeded

v0.47:

- compileSdk 35
- targetSdk 35

v0.48:

- compileSdk 35
- targetSdk **37**

The Android 17 runtime, device fingerprint, Honor Camera package and measurement implementation otherwise remained the same.

A structural JSON comparison against the v0.47 source found only two value differences:

1. `createdAtUtc`, as expected for a new run;
2. `device.truthRawTargetSdk`: **35 -> 37**.

All camera capability values, RAW stream maps, extension answers and package metadata were otherwise identical.

## Result

`ImageFormat.RAW14` still exists on the runtime with value 44, but no checked camera advertises RAW14 output sizes.

Camera 5 remains:

- 22.48 mm / f2.6
- default RAW_SENSOR: 4080x3072
- default RAW10: 4080x3072
- maximum-resolution RAW_SENSOR: 8160x6144
- maximum-resolution RAW10: 8160x6144
- high-resolution RAW_SENSOR under maximum map: 16320x12288
- high-resolution RAW10 under maximum map: 16320x12288
- RAW12: none
- RAW14: **none**

Public Camera Extension IDs 0..4 remain unsupported and `getSupportedExtensions()` remains empty.

## Closed hypothesis

The experiment falsifies the specific hypothesis that changing only TruthRaw's target SDK from 35 to 37 unlocks an advertised RAW14 or public Camera Extension route on this Android-17 firmware.

A blind RAW14 capture attempt is therefore not justified.

## Important remaining clue

The Android-17 update also replaced the installed Honor Camera package:

- previous Android-16 package: `171.0.10.452`
- current Android-17 system package: `171.0.10.706`

The next useful experiment should move away from repeatedly querying the same standard Camera2 surface.

The updated OEM camera package is now the best next artifact to acquire and compare against the exact Android-16 Honor Camera APK already preserved. That comparison can answer whether the advertised Android-17 "RAW14 support" is implemented through:

- a new OEM-internal image format;
- a privileged Camera2 capability;
- a ServiceHost pipeline change;
- a vendor extension;
- or another system-only route.

APK code remains software-route evidence only and must not be promoted to Direct-CFA, capture or calibration authority.
