# TruthRaw v0.51 — CameraDeviceSetup RAW14 session-query device result

Date: 2026-09-19

## Source

- `TRUTHRAW_CAMERA_DEVICE_SETUP_RAW14_SESSION_ORACLE_v051.json`
- bytes: `14,530`
- SHA-256: `e1d1378adce5e8e98e41eba01b0f8c39e6f907c2616c4d7e8a03fff2291276ef`

Authority remained `CAMERA2_CAMERA_DEVICE_SETUP_SESSION_QUERY_ONLY`. No camera, real capture session, ImageReader or capture was created.

## CameraDeviceSetup availability

Android 17 reports CameraDeviceSetup support for both public camera IDs 0 and 1.

## Camera 5 controls

When the deferred output is explicitly bound to physical camera 5, the public support query returns **true** for both RAW_SENSOR and RAW10 at all three already-observed geometries:

- 4080x3072
- 8160x6144
- 16320x12288

This is configuration-support evidence only.

It is especially important not to confuse the 16320x12288 `true` result with meaningful 200MP payload population. The earlier Android-16 capture route already demonstrated that a 16320x12288 RAW_SENSOR envelope can contain only a 4080x3072-sized populated prefix.

## RAW14 result

RAW14 format value 44 is rejected at every predeclared candidate:

Physical 5:

- 4080x3072: false
- 8160x6144: false
- 16320x12288: false

Cross-camera controls:

- physical 2, 4096x3072: false
- physical 2, 8192x6144: false
- physical 4, 4032x3024: false

Together with v0.47/v0.48, the public third-party Camera2 RAW14 route is now strongly closed for this firmware.

## Important correction: maximum-pixel-mode variants

The intended physical `SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION` session-parameter variants did **not** execute as parameterized queries.

For physical IDs 5 and 2, the CameraDeviceSetup request builder rejected:

`setPhysicalCameraKey(... SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION ...)`

with:

`IllegalArgumentException: Physical camera id: X is not valid!`

The code then still queried the SessionConfiguration, but without the failed session parameter. Those entries therefore duplicate the corresponding unparameterized configuration-support query.

They must not be cited as evidence that CameraDeviceSetup accepted a physical maximum-resolution pixel-mode parameter.

## Next experiment

Do not spend another iteration trying to force RAW14 through the public session API.

The highest-value next measurement is a controlled Android-17 rerun of the already-proven Camera-5 16320x12288 RAW_SENSOR acquisition route, keeping the established logical-0 + physical-5 output binding and capture-result identity gate.

After sealing the returned app-visible RAW buffer, run the existing read-only raster audit and payload geometry decoder.

The comparison question is simple:

> Did the Android-17 HAL preserve the Android-16 16320x12288 envelope / 25,067,520-byte populated-prefix behavior, or did payload topology change?

This is a pre/post OS/HAL measurement. It does not depend on RAW14 being exposed.
