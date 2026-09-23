# v0.64 device result — physical-5 targeted template defaults

Date: 2026-09-23

Authority:
`CAMERA2_REQUEST_TEMPLATE_READ_ONLY`

## Builder correction succeeded

The device report confirms:

- logical camera 0 discloses physical IDs 2, 4 and 5;
- physical ID 5 is listed by logical characteristics;
- camera permission granted;
- logical request builder created;
- physical-targeted request builder created with physical camera 5;
- no session created;
- no capture submitted;
- no image buffer accessed;
- no vendor request written.

Classification:
`READ_ONLY_PHYSICAL5_TARGETED_TEMPLATE_DEFAULTS`

This resolves the v0.61 ambiguity: the earlier `Physical camera id: 5 is not valid!` errors were caused by using a builder not created with a physical-camera ID set.

## Physical-5-only request defaults

### android.control.extendedSceneMode
- physical-only request key
- read completed successfully
- runtime class: `java.lang.Integer`
- physical template default: `0`

### android.sensor.pixelMode
- physical-only request key
- read completed successfully
- template default: `null`

A null template value does not mean the key is unsupported. Earlier capture probes explicitly wrote physical SensorPixelMode and read it back successfully.

### com.hihonor.capture.metadata.MasterFilmSensorType
- physical-only request key
- read completed successfully
- template default: `null`

### com.hihonor.capture.metadata.aoRunningMode
- physical-only request key
- read completed successfully
- template default: `null`

### com.hihonor.capture.metadata.mmiLaserEyeSafeMode
- physical-only request key
- read completed successfully
- template default: `null`

### com.hihonor.capture.metadata.videoDynamicFrameRate
- physical-only request key
- read completed successfully
- template default: `null`

### org.codeaurora.qcamera3.sessionParameters.EnableVSR
- physical-only request key
- read completed successfully
- template default: `null`

### org.codeaurora.qcamera3.sessionParameters.ExtendedMaxZoom
- physical-only request key
- read completed successfully
- template default: `null`

### org.codeaurora.qcamera3.sessionParameters.enableQLL
- physical-only request key
- read completed successfully
- runtime class: `[I` = `int[]`
- physical template default: `[0]`

### org.codeaurora.qcamera3.sessionParameters.inSensorZoomEnable
- physical-only request key
- read completed successfully
- template default: `null`

## Shared keys with concrete physical defaults

### EnableHDRDCGMode
- logical present: true
- physical present: true
- logical default: `[0]`
- physical default: `[0]`
- runtime class on both: `int[]`

### SnapshotHDRMode
- logical present: true
- physical present: true
- logical default: `[0]`
- physical default: `[0]`
- runtime class on both: `int[]`

These are exact runtime type observations. Future writes must preserve the observed `int[]` representation if those keys are tested.

## Shared keys whose template read succeeds but returns null

The following keys are present on both logical and physical request surfaces and the read completes successfully, but the still template carries no value:

- `EnableOfflineHALZSL`
- `EnableAICameraHSR`
- `AICameraMode`
- `HDRVividEnable`
- `cameraSceneMode`
- `extStreamSize`
- `teleconverterEnable`
- `thirdPartyCamera`

This means only that TEMPLATE_STILL_CAPTURE does not provide a value. It does not establish type or inactive semantics.

## Consequence for next experiment

The highest-value next probe should be a controlled, no-capture type/acceptance experiment.

Priority order:

1. preserve known types exactly:
   - `EnableHDRDCGMode` as `int[]`
   - `SnapshotHDRMode` as `int[]`
   - `enableQLL` as `int[]`
   - `extendedSceneMode` as scalar `Int`;
2. recover native types for null-default physical-only keys before writing them;
3. avoid blind scalar-int assumptions for keys whose type is still unknown;
4. keep each candidate isolated so acceptance cannot be confused with another key.

## Boundary

Template defaults and successful key reads are routing/configuration evidence only.

They do not prove:
- vendor semantic meaning;
- 200MP Direct-CFA;
- native sensor geometry;
- native ADC bit depth;
- remosaic identity;
- calibration truth.

Permanent law:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
