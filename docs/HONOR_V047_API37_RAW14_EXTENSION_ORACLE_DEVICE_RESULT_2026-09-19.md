# TruthRaw v0.47 — Android 17 RAW14 + Camera Extension device result

Date: 2026-09-19

## Source

- `TRUTHRAW_API37_RAW14_EXTENSION_ORACLE_v047.json`
- bytes: `19640`
- SHA-256: `d7667c125e425d4f60c2025e2e0580034296d5649cec7ffee898fa12907f8131`

Authority remains `CAMERA2_AND_EXTENSION_CHARACTERISTICS_OBSERVATION_ONLY`. No camera was opened, no capture was submitted, no image buffer was read, no Honor Binder method was called and no extension session was created.

## Android 17 is real and RAW14 exists at platform level

Runtime:

- Android 17
- SDK 37
- `ImageFormat.RAW14` resolves successfully
- RAW14 numeric format value: `44`

The current TruthRaw probe was intentionally still built with compileSdk 35 / targetSdk 35.

## Honor Camera package changed with the OS update

The installed package is now:

- `com.hihonor.camera`
- version `171.0.10.706`
- targetSdk 37
- minSdk 36
- compileSdk 36
- system app: true
- updated-system-app flag: false

The earlier Android-16 observation recorded Honor Camera `171.0.10.452`. Therefore the OS update also changed the installed OEM camera build.

## Camera 5 result

Physical Camera 5 remains 22.48 mm / f2.6 and keeps `android.sensor.pixelMode` as a request key.

Default stream map:

- RAW_SENSOR: 4080x3072
- RAW10: 4080x3072
- RAW12: none
- RAW14: **none**

Maximum-resolution stream map:

- RAW_SENSOR: 8160x6144
- RAW_SENSOR high-resolution: 16320x12288
- RAW10: 8160x6144
- RAW10 high-resolution: 16320x12288
- RAW12: none
- RAW14: **none**

The old Camera-5 RAW_SENSOR/RAW10 geometry therefore survives Android 17 unchanged at the characteristics layer.

## RAW14 conclusion

Android 17 exposes the RAW14 platform constant, but none of the checked camera IDs advertises a RAW14 output size to this TruthRaw build.

This does **not** yet prove that RAW14 is unavailable to all third-party apps. The probe is still targetSdk 35. Android documents RAW14 as a new API-37 capability for compatible sensors, while Android 17 also has target-SDK-specific behavior changes.

The clean next falsification is therefore not a capture attempt. It is to rebuild the same v0.47 measurement logic against compileSdk/targetSdk 37 and repeat the inventory.

## Camera Extensions

`CameraExtensionCharacteristics.isExtensionSupported(int)` is present at runtime.

For every checked camera:

- `getSupportedExtensions()` returned empty;
- extension IDs 0..4 returned false;
- no unknown vendor ID enumeration was attempted.

Device-specific extension IDs are not ruled out, because v0.47 deliberately did not brute-force unknown integer IDs.

## Next step

Build a retarget-control APK:

- same v0.47 measurement logic;
- Android 17 compileSdk 37;
- targetSdk 37;
- no capture.

Only if RAW14 or another new route appears after retargeting should TruthRaw proceed to a RAW14 capture/payload experiment.
