# v0.61 device result — MotionCam-discovered session/request surface

Date: 2026-09-23

Source JSON:
`TRUTHRAW_MOTIONCAM_VENDOR_SESSION_ORACLE_v061.json`

Source SHA-256:
`ee8937d3f07791f1e4d7ed8b56156b81fee866f15c9cf253469ba657eafd51df`

Authority:
`CAMERA2_SESSION_REQUEST_SURFACE_READ_ONLY`

## Counts

Logical camera 0:
- session keys: 76
- capture request keys: 131

Physical camera 5 characteristics:
- session keys: 71
- capture request keys: 127

## Physical-5-only session keys

Compared with logical camera 0, physical camera 5 uniquely advertises:

- `android.control.extendedSceneMode`
- `com.hihonor.capture.metadata.MasterFilmSensorType`
- `com.hihonor.capture.metadata.aoRunningMode`
- `com.hihonor.capture.metadata.mmiLaserEyeSafeMode`
- `com.hihonor.capture.metadata.videoDynamicFrameRate`
- `org.codeaurora.qcamera3.sessionParameters.EnableVSR`
- `org.codeaurora.qcamera3.sessionParameters.ExtendedMaxZoom`
- `org.codeaurora.qcamera3.sessionParameters.enableQLL`
- `org.codeaurora.qcamera3.sessionParameters.inSensorZoomEnable`

Physical-5-only capture-request keys add:

- `android.sensor.pixelMode`

in addition to the same physical-only vendor/session group above.

This is concrete runtime evidence that camera 5 has a distinct request/session control surface.

## Logical-0-only keys

Logical camera 0 uniquely advertises several composition/logical-camera controls, including:

- `com.hihonor.capture.metadata.colorEffectMode`
- `dmWaterMarkMode`
- `hdr10ModeEnable`
- `logModeEnable`
- `movieEffectMode`
- `portraitMode`
- `streamMode`
- `subSceneMode`
- `com.qti.qualcomm.spatialVideo.SpatialVideoMode`
- `EnableMCXMasterCb`
- `EnableSnapshotOnlyInsensorZoom`
- `McxRawCallbackInfo`
- `McxYuvCallbackInfo`
- `enablePerReqSync`

This is consistent with logical camera 0 carrying orchestration / multi-camera / presentation controls that are not separately advertised by physical camera 5.

## Priority-key defaults

Two shared QTI keys expose non-null defaults on the logical still template:

- `EnableHDRDCGMode = [0]`
- `SnapshotHDRMode = [0]`

The observed runtime Java class for both is:

`[I` = `int[]`

This is important. Any future controlled write must preserve the observed array type rather than assuming a scalar integer.

The other selected priority keys returned null logical template defaults.

## Why all v0.61 physical default reads failed

Every attempted call to:

`builder.getPhysicalCameraKey(key, "5")`

returned:

`IllegalArgumentException: Physical camera id: 5 is not valid!`

v0.61 constructed the builder using:

`createCaptureRequest(TEMPLATE_STILL_CAPTURE)`

without supplying a physical-camera-ID set.

Therefore these failures are not interpreted as proof that physical camera 5 lacks those keys. Camera-5 characteristics independently advertise them.

v0.63 corrects this experiment by constructing the request builder with the API-28 overload:

`createCaptureRequest(TEMPLATE_STILL_CAPTURE, setOf("5"))`

and then performs the same read-only physical default queries.

No session or capture is required for that correction.

## Scientific boundary

This result establishes runtime key-surface differences and template values only.

It does not establish vendor-key semantics, sensor geometry, native ADC bit depth, Direct-CFA 200MP, remosaic identity, or calibration truth.
