# v0.74 device result and v0.75 visible OEM-state plan

Date: 2026-09-24

## Source

Device result:

`TRUTHRAW_PHYSICAL5_HINT_REMOSAIC_ORACLE_v074.json`

Authority:

`CAMERA2_OEM_DERIVED_SCENE53_HINT_REMOSAIC_SINGLE_FRAME_ORACLE`

Classification:

`V074_HINT_NOT_OBSERVED_BUT_RAW_CAPTURE_COMPLETED`

## v0.74 runtime result

The v0.74 oracle completed both stages:

- scene-53 priming frame completed;
- physical Camera-5 RAW10/MAX capture completed;
- logical Camera 0 listed physical Camera 5;
- `cameraSceneMode` was present on logical and physical request surfaces;
- scene 53 was attached as a logical session parameter and written on the logical request;
- `hintUserValue` was not advertised on logical or physical CaptureResult characteristics;
- `hintUserValue` was not present in the runtime logical or physical result-key sets;
- constructed scalar-Integer `hintUserValue` reads returned null;
- `qcomRemosaicEnable` was not exposed on either normal Camera2 request surface;
- therefore no remosaic write occurred;
- physical `SENSOR_PIXEL_MODE` local request readback was 1 (MAXIMUM_RESOLUTION).

The correct decoded HONOR state rule remains:

`hintUserValue == 5 -> qcomRemosaicEnable = 1; otherwise -> 0`

Because no hint was observed, the derived remosaic state remained 0.

## RAW10 topology

The delivered 16320 x 12288 RAW10 envelope repeated the known v0.59 topology:

- accessible bytes: 250,675,200;
- rowStride: 20,400;
- pixelStride: 0;
- first non-zero byte: 0;
- last non-zero byte: 15,728,619;
- non-zero bytes after effective prefix: 0;
- effective rows with data: 3,072;
- effective padding non-zero bytes: 0;
- full-plane SHA-256: `ad8e75f4d2303bfe3790241ffbaab9b554c55ca5956213384d2e0d966f65e843`;
- `knownV059TopologyMatch=true`.

Therefore scene 53 by itself did not alter the app-visible RAW10 population geometry away from the repeated 4080 x 3072-equivalent packed prefix inside the larger Camera2 envelope.

This is a runtime statement about the tested direct Camera2 route only. It does not prove that the stock HONOR ServiceHost/CameraService high-pixel path is absent.

## Exact HONOR Camera static-analysis basis

The uploaded Android-17 beta HONOR Camera APK was rechecked and is byte-identical to the previously analyzed build:

- bytes: 84,167,938;
- SHA-256: `3985d9ce23ca4e47cdd34231723b8006f033753c6a3f611685c5c8fffd457d52`;
- version family: Camera 171.0.10.452.

Therefore the existing static route reconstruction remains directly applicable as the static-analysis basis for this runtime line. Static APK facts remain separate from runtime Camera2 facts.

## Why v0.75 exists

v0.74 narrowed the missing bridge:

- scene 53 is writable and capture succeeds;
- `hintUserValue` is hidden/not emitted to the tested third-party result surface;
- `qcomRemosaicEnable` is hidden from the tested third-party request surface;
- the RAW10 topology remains unchanged.

The next useful probe is therefore not another blind scene-53 repetition and not an invented hidden-key write.

v0.75 records the values of app-visible HONOR/QTI routing/state results during:

1. the scene-53 priming frame;
2. the physical-5 RAW10/MAX frame.

Selected state includes:

- `android.logicalMultiCamera.activePhysicalId`;
- `android.sensor.pixelMode`;
- `android.sensor.rawBinningFactorUsed`;
- HONOR `cameraSceneMode`;
- HONOR `aiCaptureHint`;
- HONOR `smartSuggestHint`;
- HONOR `FrameType`;
- HONOR `masterSensorSlotId`;
- HONOR `previewCameraPhysicalId`;
- HONOR `previewPhysicalCam`;
- HONOR `activeSensors`;
- HONOR `binningFactor`;
- HONOR `EnvBrightnessForA200Decision`;
- HONOR `hwFirstValidFrame`;
- QTI CHI metadata owner and multi-camera state;
- QTI tuning Feature1Mode / Feature2Mode.

For every selected key, v0.75 records runtime-key presence, Java value class, value, and read error.

## Boundary

v0.75 remains:

- single RAW physical frame;
- independentEvidenceCount = 1;
- no burst;
- no repeating request;
- no multi-frame fusion;
- no DNG detour;
- no ServiceHost Binder invocation;
- no Scientific-Master writeback;
- no calibration authority.

Visible Camera2 state is not automatically equivalent to stock ServiceHost internal state.

Permanent law:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
