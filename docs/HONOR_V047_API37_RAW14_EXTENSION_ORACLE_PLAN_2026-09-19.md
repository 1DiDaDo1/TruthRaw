# TruthRaw v0.47 — Android 17 RAW14 + Camera Extension capability oracle

Date: 2026-09-19

## Trigger

The same v0.46 characteristic oracle was run before and after the BKQ-N49 OS update:

- Android 16 / SDK 36 baseline: all five targeted Honor APK capability names absent from ordinary third-party CameraCharacteristics.
- Android 17 / SDK 37 rerun: the same five names remain absent.
- Standard physical-camera identity remained stable; camera 5 is still 22.48 mm / f2.6.

The OS update therefore did not expose that particular Honor-internal capability surface through normal CameraCharacteristics.

## v0.47 goal

Measure what **new official Android 17 / API 37 surfaces** are actually exposed to this unchanged-target TruthRaw package before attempting any capture.

v0.47 is capability-only. It inventories:

- RAW_SENSOR
- RAW10
- RAW12
- RAW14, resolved from the actual Android 17 runtime

for each public and disclosed physical camera ID, separately across:

- default StreamConfigurationMap output sizes;
- default-map high-resolution output sizes;
- maximum-resolution StreamConfigurationMap output sizes;
- maximum-resolution-map high-resolution output sizes.

The report also inspects CameraExtensionCharacteristics:

- getSupportedExtensions();
- API-37 isExtensionSupported(int), invoked reflectively for the known public extension IDs 0..4;
- no extension session is created.

## Why reflection

The existing TruthRaw suite remains compileSdk/targetSdk 35.

RAW14 and isExtensionSupported(int) are therefore resolved from the **actual API-37 runtime** by reflection. This avoids conflating "we rebuilt against API 37" with "the Android 17 device exposes this capability."

## Authority

`CAMERA2_AND_EXTENSION_CHARACTERISTICS_OBSERVATION_ONLY`

v0.47 does not:

- open a CameraDevice;
- create or submit a CaptureRequest;
- instantiate ImageReader;
- access image pixels;
- create CameraExtensionSession;
- call Honor Binder;
- write vendor request keys;
- alter the Scientific Master;
- grant calibration authority.

## Interpretation gates

An advertised RAW14 size proves only that Android 17 exposes that output format/geometry to the queried camera characteristics.

It does not yet prove:

- capture success;
- populated payload topology;
- 14 effective sensor bits;
- untouched ADC data;
- native physical photodiode layout;
- OEM 200MP route identity;
- Direct-CFA 200MP.

If camera 5 exposes RAW14, the next step will be a separate controlled capture probe bound to TotalCaptureResult and exact payload audit.

## Camera Extensions limitation

Android 17 adds `CameraExtensionCharacteristics.isExtensionSupported(int)`, which can query device-specific extension modes when a candidate extension ID is known. v0.47 deliberately does not brute-force unknown vendor extension IDs. It records getSupportedExtensions() and the standard IDs 0..4 only.

Static Honor APK research can later supply an evidence-based vendor extension candidate if one is found.
