# TruthRaw v0.51 — CameraDeviceSetup RAW14 session-support oracle

Date: 2026-09-19

## Trigger

v0.50 closed the fixed-characteristics direct vendor-key route:

- 45 exact typed lookups;
- 0 non-null values;
- 40 clean null returns;
- 5 explicit tag-not-found errors for `ultraHighPixelMonoSupported`.

Separately, v0.47/v0.48 established that Android 17 exposes the RAW14 platform constant (44) but no RAW14 stream size is advertised.

A blind capture is not justified.

## Public Android query selected for v0.51

Android's public `CameraDevice.CameraDeviceSetup` API can query a `SessionConfiguration` **without opening a CameraDevice**. Android also provides deferred ImageReader-style `OutputConfiguration(format, Size)` objects specifically for this kind of support query.

v0.51 therefore tests only a small evidence-based list of configurations.

## Evidence-based Camera 5 candidates

From v0.47/v0.48 characteristics:

- 4080x3072 default RAW_SENSOR / RAW10;
- 8160x6144 maximum-resolution RAW_SENSOR / RAW10;
- 16320x12288 high-resolution RAW_SENSOR / RAW10.

The same three geometries are tested with RAW14 format value 44.

For the two maximum/high-resolution geometries, v0.51 queries both:

- no session parameter;
- physical Camera 5 `SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION`.

That physical pixel-mode variant is not guessed: TruthRaw's earlier Camera-5 route already established that the physical request could carry the maximum-resolution sensor-pixel-mode control while the logical global key was not advertised.

Narrow RAW14 cross-camera controls are included only at already observed geometries for physical Camera 2 and 4.

## Physical binding

The query is performed through logical Camera 0. Each deferred `OutputConfiguration` is explicitly bound with `setPhysicalCameraId` to the already disclosed physical ID.

No camera is opened.

## Authority

`CAMERA2_CAMERA_DEVICE_SETUP_SESSION_QUERY_ONLY`

The experiment must not:

- open a CameraDevice;
- create a real CameraCaptureSession;
- create ImageReader;
- access pixels;
- submit a CaptureRequest;
- invoke Honor Binder;
- write vendor keys;
- enumerate arbitrary sizes or formats.

A CaptureRequest.Builder may be created through CameraDeviceSetup only to attach the already-established physical `SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION` value as a **session query parameter**. It is never submitted.

## Interpretation

A `true` result means Android reports that the hypothetical SessionConfiguration is supported.

It does not prove:

- an actual capture succeeds;
- RAW14 contains 14 effective sensor bits;
- payload topology or populated bytes;
- native ADC/CFA geometry;
- OEM high-pixel route identity.

A `false` or `IllegalArgumentException` is equally useful negative evidence.

If RAW14 is rejected at all evidence-based candidates while RAW_SENSOR/RAW10 controls succeed, the public CameraDeviceSetup route corroborates the stream-map non-exposure and RAW14 should remain closed for normal third-party Camera2 on this firmware.
