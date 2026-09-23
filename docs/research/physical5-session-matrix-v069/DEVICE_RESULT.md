# v0.69 device result — complete physical-5 session acceptance matrix

Date: 2026-09-23

Source:
`TRUTHRAW_PHYSICAL5_SESSION_ACCEPTANCE_MATRIX_v069.json`

Authority:
`CAMERA2_HAL_SESSION_ACCEPTANCE_MATRIX_NO_CAPTURE`

## Matrix summary

- candidates declared: 19
- candidates attempted: 18
- HAL/session configured: 18
- HAL/session rejected or failed: 0
- skipped because not a physical-5 session key: 1
- control-session failures: 0
- physical frame count: 0
- independent evidence count: 0

Final classification:

`PHYSICAL5_HAL_SESSION_ACCEPTANCE_MATRIX_CHECKPOINTED__NO_CAPTURE`

The v0.69 crash-safety changes also worked operationally: the full matrix completed and a final JSON report was preserved.

## Session-accepted candidates

Each of the following configured successfully in its own isolated matched-control/candidate cycle:

- `android.control.extendedSceneMode = 1`
- `com.hihonor.capture.metadata.MasterFilmSensorType = intArrayOf(1)`
- `com.hihonor.capture.metadata.aoRunningMode = intArrayOf(1)`
- `com.hihonor.capture.metadata.videoDynamicFrameRate = intArrayOf(1)`
- `org.codeaurora.qcamera3.sessionParameters.EnableVSR = intArrayOf(1)`
- `org.codeaurora.qcamera3.sessionParameters.ExtendedMaxZoom = intArrayOf(1)`
- `org.codeaurora.qcamera3.sessionParameters.enableQLL = intArrayOf(1)`
- `org.codeaurora.qcamera3.sessionParameters.inSensorZoomEnable = byteArrayOf(1)`
- `org.codeaurora.qcamera3.sessionParameters.EnableHDRDCGMode = intArrayOf(1)`
- `org.codeaurora.qcamera3.sessionParameters.EnableOfflineHALZSL = intArrayOf(1)`
- `org.codeaurora.qcamera3.sessionParameters.EnableAICameraHSR = intArrayOf(1)`
- `org.codeaurora.qcamera3.sessionParameters.AICameraMode = intArrayOf(1)`
- `org.codeaurora.qcamera3.sessionParameters.SnapshotHDRMode = intArrayOf(1)`
- `com.hihonor.capture.metadata.HDRVividEnable = byteArrayOf(1)`
- `com.hihonor.capture.metadata.cameraSceneMode = intArrayOf(1)`
- `com.hihonor.capture.metadata.extStreamSize = intArrayOf(1)`
- `com.hihonor.capture.metadata.teleconverterEnable = byteArrayOf(1)`
- `com.hihonor.capture.metadata.thirdPartyCamera = byteArrayOf(1)`

For every attempted candidate:
- its matched control configured;
- the candidate session configured;
- `onConfigureFailed=false`;
- no error was reported;
- no capture or repeating request was submitted;
- no image buffer was accessed.

## Deliberately skipped

`android.sensor.pixelMode = 1` was present in the physical request-key surface but not in physical camera 5's available session-key surface.

Therefore v0.69 correctly skipped it as a session parameter.

This does not invalidate the earlier capture-request experiments in which physical `SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION` was explicitly written/read back. It only establishes that it is not a physical-5 session key in this runtime surface.

## Interpretation

The evidence now establishes, for the exact tested context, that all 18 advertised physical-5 session candidates above are accepted by Camera2/HAL session configuration when supplied with the v0.65-resolved Java representation and value 1.

This is stronger than:
- key-name discovery;
- template-default reads;
- local Camera2 marshaling.

But session acceptance does not establish that a key is active, semantically decoded as assumed, changes sensor mode, changes producer topology, or affects captured RAW bytes.

A vendor key may be accepted yet ignored, routed to a no-op/default branch, or have semantics unrelated to the high-pixel question.

## Next experimental gate

The next useful step is capture-effect evidence.

The experiment should preserve isolation and matched controls:
1. create one physical-5 candidate session;
2. submit exactly one single-frame RAW request;
3. record result metadata plus exact payload topology/hash;
4. compare against a matched single-frame control captured in the same APK/run;
5. never combine multiple vendor candidates in one capture.

One APK may automate the sequence, but each candidate must remain a separate control/candidate single-frame pair.

Highest-value initial candidates:
- `MasterFilmSensorType`
- `aoRunningMode`
- `EnableVSR`
- `ExtendedMaxZoom`
- `inSensorZoomEnable`
- `cameraSceneMode`
- `extStreamSize`
- `teleconverterEnable`

The remaining accepted controls can follow if these show no differential.

## Boundary

Session acceptance alone does not prove:
- vendor semantic meaning;
- active processing effect;
- OEM high-pixel route identity;
- Direct-CFA 200MP;
- native sensor geometry;
- native ADC bit depth;
- calibration truth.

Permanent law:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
