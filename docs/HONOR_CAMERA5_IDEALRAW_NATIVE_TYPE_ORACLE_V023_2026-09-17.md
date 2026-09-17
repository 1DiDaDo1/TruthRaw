# HONOR Camera-5 IdealRAW v0.23 native metadata-type oracle

Date: 2026-09-17

Status: **BUILD SUCCESS — DEVICE RESULT PENDING**.

## Why v0.23 exists

v0.21 correctly failed closed because Java reflection could not expose the runtime value type of `org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW`.

v0.22 then performed a no-HAL Java Camera2 marshalling dry-run. On the device, four Java representations survived local builder/request round trips:

- `Byte`
- `byte[]`
- `Int`
- `int[]`

This narrowed the problem to BYTE-family versus INT32-family but did not identify the native `camera_metadata` element type. No vendor-modified session/request reached HAL in v0.21 or v0.22.

## v0.23 method

v0.23 moves only the type-discrimination step to Android's NDK metadata layer.

The native probe:

1. obtains logical Camera 0 characteristics;
2. resolves the device-specific tag ID for `org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW`;
3. opens logical Camera 0 only to create disposable still-capture request metadata objects;
4. tests the same tag separately with `ACaptureRequest_setEntry_u8` and `ACaptureRequest_setEntry_i32`, each with count 1 and value 1;
5. reads back an accepted entry with `ACaptureRequest_getConstEntry` and checks its native type/count;
6. frees requests, closes the native CameraDevice and deletes the manager.

It deliberately creates **no capture session**, attaches **no session parameters**, submits **no capture**, accesses **no RAW pixels**, and mutates **no source evidence**.

## Expected classifications

A unique result is either:

`NATIVE_METADATA_TYPE_BYTE__NO_SESSION_OR_CAPTURE_SUBMISSION`

or:

`NATIVE_METADATA_TYPE_INT32__NO_SESSION_OR_CAPTURE_SUBMISSION`.

Ambiguous and unresolved outcomes remain fail-closed.

A unique native type authorizes only a later, separate experiment to represent the vendor value correctly. It does not prove what `EnableIdealRAW` means and does not prove that value `1` changes the sensor/RAW route.

## Build

GitHub Actions run: `35241955761`

Workflow head: `e5293ce48d3eeb77664edfd5b8ffac996208d9e2`

APK:
- bytes: `4,874,225`
- SHA-256: `1315d4882fa8c304edfb5d04d7e988eaabc0f7af02e7bd47e9aa853e0755ecc0`

Artifact ZIP:
- artifact ID: `10505942094`
- bytes: `1,589,787`
- SHA-256: `25cf953842d3f74e53103b934c00536a94c967bf20f8d657bfa57c0e385b9452`

## Device procedure

Only **Step 1** is required. v0.23 intentionally stops at `STAGE 1.5 DIAGNOSTIC STOP` before preview/session/capture work.

Record or export:
- `classification`
- `resolvedNativeType`
- `tag`
- `u8Accepted`
- `i32Accepted`
- the small v0.23 type-oracle JSON if available.

## Authority

v0.20 remains the current Camera-5 source/payload authority. v0.23 is a request-metadata type diagnostic only.

Permanent boundary:

`APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF`
