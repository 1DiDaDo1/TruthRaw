# v0.66 device result — MasterFilmSensorType HAL/session acceptance

Date: 2026-09-23

Source:
`TRUTHRAW_PHYSICAL5_MASTERFILM_SESSION_ACCEPTANCE_v066.json`

Authority:
`CAMERA2_HAL_SESSION_ACCEPTANCE_NO_CAPTURE`

## Preconditions confirmed

- logical camera 0
- physical camera 5
- physical ID 5 disclosed by logical characteristics
- target key `com.hihonor.capture.metadata.MasterFilmSensorType`
- target present in physical request surface
- target present in physical session surface
- Java representation resolved by v0.65: `int[]`
- candidate value: `[1]`
- physical output surface: SurfaceTexture 1920x1080 bound to physical camera 5

## Control session

The control used the same physical-5 SurfaceTexture output with no vendor session parameter.

Result:
- createCaptureSession call completed
- `onConfigured=true`
- `onConfigureFailed=false`
- close callback observed
- no error
- no capture
- no repeating request
- no image-buffer access

## Candidate session

Candidate:
`MasterFilmSensorType = intArrayOf(1)`

Result:
- session parameter attached
- local set completed
- local readback = `[1]`
- createCaptureSession call completed
- `onConfigured=true`
- `onConfigureFailed=false`
- close callback observed
- no error
- no capture
- no repeating request
- no image-buffer access

Classification:

`MASTERFILMSENSORTYPE_INT_ARRAY_1__HAL_SESSION_CONFIGURED__NO_CAPTURE`

## Interpretation

This is the first controlled result in this sequence that moves beyond local Camera2 marshaling and establishes HAL/session acceptance of a physical-camera-5 vendor session parameter in a real session configuration.

Because the matched control also configured successfully, the candidate result is not confounded by the physical SurfaceTexture session itself being invalid.

This does **not** prove:
- what `MasterFilmSensorType` semantically means;
- whether value 1 activates processing;
- whether it changes producer topology;
- whether it selects an HONOR high-pixel route;
- RAW/200MP/Direct-CFA evidence;
- sensor geometry or ADC bit depth;
- calibration truth.

No frame was captured, therefore `physicalFrameCount=0` and `independentEvidenceCount=0`.

## Next gate

Repeat the exact same control/candidate structure for one additional physical-5-only key while preserving its v0.65 type.

Next candidate:
`com.hihonor.capture.metadata.aoRunningMode = intArrayOf(1)`

No capture should be added yet.
