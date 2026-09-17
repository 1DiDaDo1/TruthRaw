# HONOR Camera-5 IdealRAW v0.22 type dry-run — device result

Date: 2026-09-17

Status: **device result recorded; app-side Java marshalling remains ambiguous; no HAL/session intervention occurred**.

## Device observation

The v0.22 diagnostic ran on the trusted logical-0 / tele-5 preview route and stopped before any vendor-modified session/capture submission.

Observed UI result:

- logical camera: `0`
- zoom: `3.7x`
- active physical camera: `5`
- `TELE 5 CONFIRMED`
- diagnostic classification: `MULTIPLE_APP_SIDE_MARSHALLING_CANDIDATES__AMBIGUOUS_NO_HAL_SUBMISSION`
- passing Java kinds: `Byte`, `byte[]`, `Int`, `int[]`
- HAL/session submission: `false`
- capture: `false`
- v0.20 control: untouched.

## Meaning

The v0.22 public-Java `CaptureRequest.Key(String, Class<T>)` dry-run successfully narrowed the candidate family but did **not** uniquely establish the underlying native `camera_metadata` type.

The important negative result is that the Long/Float/Double families did not survive the same local round-trip, while BYTE-family and INT32-family representations did.

Scalar versus one-element array is not a native metadata-type distinction by itself. The native camera metadata contract distinguishes element type (`BYTE`, `INT32`, etc.) plus count.

Therefore it is still unsafe to submit `EnableIdealRAW=1` to a real session based only on v0.22.

## Next safe discriminator

v0.23 must use the Android NDK request metadata validator on disposable requests:

1. resolve the actual vendor tag ID for `org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW`;
2. create a native still-capture request without creating a session;
3. test `ACaptureRequest_setEntry_u8(..., count=1, value=1)`;
4. independently test `ACaptureRequest_setEntry_i32(..., count=1, value=1)`;
5. inspect `ACaptureRequest_getConstEntry` only on the accepted type;
6. free the request/device/manager;
7. do not create a capture session and do not submit any vendor-modified request to HAL.

Android's NDK contract states that the `setEntry_*` call must return `ACAMERA_ERROR_INVALID_PARAMETER` when the requested setter type does not match the metadata tag type. This makes the NDK request metadata layer a stronger type oracle than the Java custom-key marshalling dry-run while still avoiding a capture/session intervention.

## Authority boundary

This experiment can establish the native metadata **value type** of the vendor key if exactly one setter family is accepted. It cannot establish the semantic meaning of `EnableIdealRAW`, cannot establish that value `1` changes RAW routing, and cannot promote any source to untouched/native ADC evidence.

The current source authority remains the v0.20 Camera-5 result until a later single-variable intervention produces device evidence.
