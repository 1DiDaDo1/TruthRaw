# v0.75 device result and v0.76 matched scene-state method

Date: 2026-09-24

## v0.75 runtime result

Source report:

`TRUTHRAW_PHYSICAL5_VISIBLE_OEM_STATE_ORACLE_v075.json`

Authority:

`CAMERA2_OEM_SCENE53_VISIBLE_STATE_SINGLE_FRAME_ORACLE`

Classification:

`V075_VISIBLE_OEM_STATE_TRACE_COMPLETED_HINT_NOT_OBSERVED`

The v0.75 device run completed both the scene-53 diagnostic frame and the physical Camera-5 RAW10/MAX capture.

Observed on the scene-53 frame, on both logical and physical visible-result state where available:

- `android.logicalMultiCamera.activePhysicalId = "5"`;
- returned `android.sensor.pixelMode = 0`;
- `android.sensor.rawBinningFactorUsed = true`;
- HONOR `cameraSceneMode = [53]`;
- HONOR `aiCaptureHint = [0]`;
- HONOR `smartSuggestHint = [0]`;
- HONOR `FrameType = [0]`;
- HONOR `masterSensorSlotId = [4]`;
- HONOR `previewCameraPhysicalId = [5]`;
- HONOR `previewPhysicalCam = [2]`;
- HONOR `binningFactor = [4]`;
- HONOR `EnvBrightnessForA200Decision = [-1]`;
- QTI `Feature1Mode = [2]`;
- QTI `Feature2Mode = [0]`.

During the physical RAW capture the selected state remained the same for the above fields except that `FrameType` changed from `[0]` to `[1]`. This transition is runtime evidence only. v0.75 did not establish the semantic meaning of `FrameType` 0 or 1.

`hintUserValue` remained unavailable / null on logical and physical results. `qcomRemosaicEnable` remained unavailable on the tested normal Camera2 request surfaces.

## v0.75 RAW10 topology

The physical Camera-5 RAW10/MAX frame remained the known v0.59 topology:

- accessible bytes: 250,675,200;
- rowStride: 20,400;
- pixelStride: 0;
- first non-zero byte: 0;
- last non-zero byte: 15,728,619;
- tail non-zero byte count: 0;
- effective rows with data: 3,072;
- effective padding non-zero bytes: 0;
- full-plane SHA-256: `220ad128cc533e7a30582c99fd90f2f66ac412e5b22b441ba5d856aa404f3726`;
- `knownV059TopologyMatch = true`.

Therefore direct Camera2 scene 53 is accepted and returned as scene 53, but the tested route still does not reproduce a different app-visible RAW10 population topology.

## Exact static-analysis basis

HONOR Camera APK:

- version family: Camera 171.0.10.452;
- bytes: 84,167,938;
- SHA-256: `3985d9ce23ca4e47cdd34231723b8006f033753c6a3f611685c5c8fffd457d52`.

The uploaded Android-17 beta camera APK was byte-identical to the APK used for the existing static route reconstruction.

Static APK findings remain separate from runtime Camera2 evidence.

## Why v0.76 uses matched pairs

A single scene-53 frame cannot establish which visible-result differences are scene-specific versus ordinary still-capture or temporal variation.

v0.76 therefore reuses the proven v0.71 locked-acquisition design and adds the v0.75 visible-state observations.

Three matched pairs are measured:

1. DEFAULT control versus `cameraSceneMode=53` (UltraHighPixel / 200M);
2. DEFAULT control versus `cameraSceneMode=110` (UltraResolution / 50M);
3. DEFAULT control versus `cameraSceneMode=66` (Pro Photo RAW).

For each control/candidate RAW frame:

- camera is freshly opened;
- physical Camera 5 is used;
- output is physical-bound RAW10 16320 x 12288 MAXIMUM_RESOLUTION envelope;
- manual ISO is requested identically;
- manual exposure time is requested identically;
- manual frame duration is requested identically;
- manual focus distance is requested identically;
- AE and AF are disabled when supported;
- noise reduction and edge processing are disabled when supported;
- result acquisition state is compared for exact equality;
- exact Image/result timestamp identity is required;
- RAW10 byte topology is sealed and analyzed;
- selected logical and physical HONOR/QTI runtime-result state is recorded;
- `hintUserValue` is probed without making it a fatal prerequisite.

Control/candidate order is mirrored across the three scene pairs to reduce simple ordering bias.

## Evidence accounting

Every delivered RAW frame is an independent single-frame observation.

The matrix may contain up to six physical RAW frames in total, but:

- they are not fused;
- they do not become one Scientific Master;
- comparisons do not create new source evidence;
- no DNG is created by the oracle;
- no ServiceHost binder path is invoked;
- no calibration authority is granted;
- no native sensor geometry or Direct-CFA 200MP claim follows from an envelope or scene-state difference.

Permanent law:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
