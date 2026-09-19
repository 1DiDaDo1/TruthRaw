# TruthRaw v0.50 — direct typed Honor vendor-characteristics oracle

Date: 2026-09-19

## Trigger

v0.46 established that five Honor capability names selected from the OEM camera APK were **not listed** in the ordinary `CameraCharacteristics.keys` enumeration on logical IDs 0/1 and disclosed physical IDs 2/4/5.

The exact .452 -> .706 static APK diff then recovered the Java types with which Honor itself constructs the relevant `CameraCharacteristics.Key` objects.

Observed in both compared Honor Camera builds:

### int[]

- `physicalCameraScene`
- `rawSensorResolution`
- `sceneCameraIdCapability`
- `cameraIdCustomInfo`
- `needOpenPhysicalCamera`
- `ultraResolutionSwitchSupportedSize`

### scalar byte

- `teleSupport`
- `ultraHighPixelMonoSupported`
- `rawZoomSupported`

This creates one new evidence-based, read-only falsification that v0.46 could not perform.

## Experiment

For each public/disclosed Camera2 ID:

1. obtain `CameraCharacteristics`;
2. record whether each exact key name is listed in `characteristics.keys`;
3. construct a `CameraCharacteristics.Key` using the exact APK-observed name and Java type;
4. call `CameraCharacteristics.get(key)`;
5. record separately:
   - non-null value;
   - null;
   - key-construction error;
   - `IllegalArgumentException`;
   - `SecurityException`;
   - any other framework error.

No retries with guessed names or guessed types are permitted.

## Authority

`CAMERA2_CHARACTERISTICS_DIRECT_TYPED_VENDOR_KEY_READ_ONLY`

The APK analysis used to select names/types remains:

`STATIC_APK_SOFTWARE_ROUTE_EVIDENCE_ONLY`

v0.50 does **not**:

- open a CameraDevice;
- create/submit a CaptureRequest;
- instantiate ImageReader;
- access image pixels;
- invoke Honor Binder;
- write a vendor key;
- spoof package/signature identity;
- create an extension session;
- grant capture or calibration authority.

This is not a security bypass. It is a normal read-only `CameraCharacteristics.get(Key)` call with an evidence-derived exact name/type pair. A framework rejection is a valid negative result.

## Decision gates

### If direct values are returned

Interpret them only as runtime characteristics metadata.

Especially inspect:

- whether `physicalCameraScene` contains UI high-pixel scene 53;
- whether `sceneCameraIdCapability` can be parsed as a scene/camera mapping;
- whether `ultraResolutionSwitchSupportedSize` contains 16320x12288 or 8192x6144;
- whether `rawSensorResolution` contains a camera-specific RAW geometry;
- whether `teleSupport`, `rawZoomSupported`, or `ultraHighPixelMonoSupported` differ across camera IDs.

Any structural parsing beyond the raw arrays must remain a separate interpretation step unless the APK code establishes the layout.

### If all direct reads fail or return null

Close this third-party direct-key route. Do not mutate, spoof, or bypass framework/package security.

## Scientific boundary

Even a successful hidden-key read cannot prove that a specific Honor shutter event used physical camera 5, that 16320x12288 is native Direct-CFA, or that a sensor exposes 200MP photodiode samples.
