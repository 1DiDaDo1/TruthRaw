# v0.65 device result — Physical-5 local Camera2 value types

Date: 2026-09-23

Source:
`TRUTHRAW_PHYSICAL5_LOCAL_TYPE_ORACLE_v065.json`

Authority:
`CAMERA2_LOCAL_REQUEST_MARSHALING_ONLY`

## Result

All 19 tested Camera-5 request keys accepted exactly one tested Java representation on a fresh physical-targeted `TEMPLATE_STILL_CAPTURE` builder.

No session was created and no request was submitted to HAL.

### Scalar Int

- `android.control.extendedSceneMode` -> `java.lang.Integer`
- `android.sensor.pixelMode` -> `java.lang.Integer`

### int[] / native INT32 array

- `com.hihonor.capture.metadata.MasterFilmSensorType`
- `com.hihonor.capture.metadata.aoRunningMode`
- `com.hihonor.capture.metadata.videoDynamicFrameRate`
- `org.codeaurora.qcamera3.sessionParameters.EnableVSR`
- `org.codeaurora.qcamera3.sessionParameters.ExtendedMaxZoom`
- `org.codeaurora.qcamera3.sessionParameters.enableQLL`
- `org.codeaurora.qcamera3.sessionParameters.EnableHDRDCGMode`
- `org.codeaurora.qcamera3.sessionParameters.EnableOfflineHALZSL`
- `org.codeaurora.qcamera3.sessionParameters.EnableAICameraHSR`
- `org.codeaurora.qcamera3.sessionParameters.AICameraMode`
- `org.codeaurora.qcamera3.sessionParameters.SnapshotHDRMode`
- `com.hihonor.capture.metadata.cameraSceneMode`
- `com.hihonor.capture.metadata.extStreamSize`

### byte[] / native BYTE array

- `org.codeaurora.qcamera3.sessionParameters.inSensorZoomEnable`
- `com.hihonor.capture.metadata.HDRVividEnable`
- `com.hihonor.capture.metadata.teleconverterEnable`
- `com.hihonor.capture.metadata.thirdPartyCamera`

## Native-type evidence from rejection messages

For array-valued vendor keys, Camera2's marshaling layer reported explicit native type mismatches:

- expected native type 0 when BYTE-array keys were tested with INT32/FLOAT/INT64/DOUBLE arrays;
- expected native type 1 when INT32-array keys were tested with BYTE/FLOAT/INT64/DOUBLE arrays.

Combined with successful set/readback, this is stronger than name-based type guessing.

## Important correction to earlier blind tests

A scalar value such as `1` is not equivalent to `intArrayOf(1)` or `byteArrayOf(1)`.

Therefore any prior experiment that wrote one of these vendor keys using the wrong Java representation cannot be treated as a valid negative semantic/topology test for that key.

This particularly matters for keys such as:

- `MasterFilmSensorType`
- `aoRunningMode`
- `EnableVSR`
- `ExtendedMaxZoom`
- `inSensorZoomEnable`
- `HDRVividEnable`
- `cameraSceneMode`
- `extStreamSize`
- `teleconverterEnable`
- `thirdPartyCamera`

Existing tests that already used the correct representation remain valid within their tested context.

## Next experimental gate

The next useful step is not another local builder test.

It should be a tightly controlled HAL/session-acceptance probe, one candidate at a time, preserving the resolved representation exactly and initially stopping before capture.

Recommended first candidates are physical-5-only controls with plausible routing relevance:

1. `MasterFilmSensorType` as `int[]`
2. `aoRunningMode` as `int[]`
3. `EnableVSR` as `int[]`
4. `ExtendedMaxZoom` as `int[]`
5. `inSensorZoomEnable` as `byte[]`

A later stage may test HONOR logical/shared routing controls such as `cameraSceneMode`, `extStreamSize`, `teleconverterEnable`, and `thirdPartyCamera`, but only in isolated runs.

## Boundary

Classification:

`PHYSICAL5_LOCAL_REQUEST_TYPE_ACCEPTANCE__NO_SESSION_NO_HAL_SUBMISSION`

This proves Camera2 marshaling compatibility only.

It does not prove:
- HAL acceptance;
- vendor semantic meaning;
- OEM high-pixel route identity;
- Direct-CFA 200MP;
- native sensor geometry;
- native ADC bit depth;
- calibration truth.

Permanent law:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
