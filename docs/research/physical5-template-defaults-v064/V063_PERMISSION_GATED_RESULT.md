# v0.63 device result — permission-gated partial result

Date: 2026-09-23

Source:
`TRUTHRAW_PHYSICAL5_TEMPLATE_DEFAULTS_ORACLE_v063.json`

Authority:
`CAMERA2_REQUEST_TEMPLATE_READ_ONLY`

## What succeeded

The device report confirms that logical camera 0 discloses physical IDs:
`2, 4, 5`

and explicitly confirms:
`physicalId5ListedByLogicalCharacteristics=true`.

The physical-camera characteristics surface again contains physical-5-only request keys including:
- `android.control.extendedSceneMode`
- `android.sensor.pixelMode`
- `com.hihonor.capture.metadata.MasterFilmSensorType`
- `com.hihonor.capture.metadata.aoRunningMode`
- `org.codeaurora.qcamera3.sessionParameters.EnableVSR`
- `org.codeaurora.qcamera3.sessionParameters.ExtendedMaxZoom`
- `org.codeaurora.qcamera3.sessionParameters.enableQLL`
- `org.codeaurora.qcamera3.sessionParameters.inSensorZoomEnable`

## What did not run

The report records:
- `cameraPermissionGranted=false`
- `logicalBuilderCreated=false`
- `physicalTargetedBuilderCreated=false`
- classification `KEY_SURFACE_COMPLETE__DEFAULT_READ_SKIPPED_NO_CAMERA_PERMISSION`

Therefore v0.63 did **not** test whether the corrected physical-targeted builder can read physical-camera defaults.

No conclusion may be drawn from the null default fields in this file.

## v0.64 correction

v0.64 keeps the same read-only request-builder experiment but explicitly requests Android CAMERA permission from the user when needed. If permission is granted, it automatically runs the oracle.

It still:
- writes no vendor key;
- creates no capture session;
- submits no capture;
- accesses no image buffer.

Permanent boundary:

**Request-template defaults are runtime configuration evidence only.**
