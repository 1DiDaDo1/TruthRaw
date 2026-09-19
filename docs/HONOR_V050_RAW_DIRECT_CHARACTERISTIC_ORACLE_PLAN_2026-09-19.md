# TruthRaw v0.50 — Honor RAW direct CameraCharacteristics oracle

Date: 2026-09-19

## Trigger

The exact Android-16 Honor Camera .452 and Android-17 Honor Camera .706 APKs were statically diffed.

The central 200MP / UltraHighPixel route and RAW capability vocabulary are inherited rather than newly introduced in .706. No RAW14 literal was found in the app-layer DEX/native strings, while Android 17 itself exposes ImageFormat.RAW14 but TruthRaw's standard Camera2 maps do not advertise it.

The static APKs do, however, expose a stable set of Honor RAW vendor characteristic names and their expected value types.

v0.46 tested only whether selected route names were enumerable in `CameraCharacteristics.keys` and found them absent.

v0.50 tests a narrower hypothesis:

> an exact vendor characteristic name/type may still be directly readable through `CameraCharacteristics.get(Key)` even when omitted from the enumerable key list.

## Read-only targets

RAW family:

- `rawImgSupported` — Byte
- `hwCaptureRawStreamConfigurations` — IntArray
- `hwProfessionalRawCaptureMode` — Byte
- `professionalTeleRawLogicalCameraID` — Int
- `rawCaptureSize` — IntArray
- `rawForBokehSupported` — Byte
- `rawSensorResolution` — IntArray
- `rawZoomSupported` — Byte
- `supportOfflineRawSceneMode` — IntArray

Routing/control family, carried forward from v0.46 but now direct-get tested with inferred IntArray type:

- `physicalCameraScene`
- `sceneCameraIdCapability`
- `cameraIdCustomInfo`
- `needOpenPhysicalCamera`
- `ultraResolutionSwitchSupportedSize`

For every public/disclosed physical camera ID, v0.50 records separately:

- whether the exact name appears in `CameraCharacteristics.keys`;
- whether the Key object can be constructed;
- whether direct `get()` completes;
- null vs non-null;
- runtime type;
- raw value;
- exception class/message if access is rejected.

## RAW-save policy observation

The .452 → .706 diff found one meaningful policy change in `CustomConfigurationUtil.isSupportedRawSaved()`:

- same property: `msc.camera.rawphoto.save`;
- .452 default: true;
- .706 default: false;
- old MagicLite early-false branch removed.

v0.50 therefore makes one read-only reflection attempt to read that exact system property. It does not bypass hidden-API controls; failure is recorded as a valid result.

## Authority boundary

`CAMERA2_VENDOR_CHARACTERISTIC_OBSERVATION_ONLY`

v0.50 does not:

- open a camera;
- create or submit a CaptureRequest;
- instantiate ImageReader;
- read image pixels;
- invoke Honor Binder;
- write any vendor request;
- create an extension session;
- modify the Scientific Master;
- grant calibration authority.

A non-null vendor characteristic is capability metadata only. It cannot prove that an OEM shutter event used a route, that a RAW output succeeds, or that any payload is native Direct-CFA/ADC evidence.

## Decision rule

If direct reads are all null/rejected, this closes the exact-name/type third-party characteristics route.

If one or more RAW characteristics return values, freeze the exact per-camera values first. Use those values to design the next single-variable experiment; do not immediately capture or enumerate arbitrary vendor values.
