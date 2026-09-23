# TruthRaw v0.61 — expanded exact Honor RAW/remosaic characteristics oracle

Date: 2026-09-23

Status: **ISOLATED READ-ONLY EXPERIMENT / FROZEN v0.84.3 UNCHANGED**

## Trigger

A fresh Android-17 rerun of the unchanged v0.50 oracle produced a materially different runtime result from the 2026-09-19 v0.50 device result.

Historical v0.50 report:

- SHA-256: `735e4280909867b3485f72db11cb58ee8256f2c1b38e689d07e2fdae2a248fea`
- 45 exact typed lookups;
- non-null values: 0;
- completed null: 40;
- tag-not-found: 5.

Fresh rerun:

- file: `TRUTHRAW_DIRECT_TYPED_VENDOR_CHARACTERISTICS_ORACLE_v050 (3).json`
- SHA-256: `49ab002751bea88e5ff1cccf77a783e4d1fd1e6c7099a55d2837198f68af96a8`
- same device fingerprint:
  `HONOR/BKQ-N49/HNBKQ:17/HONORBKQ-NXX/11.0.0.120C901E8:user/release-keys`
- non-null values: 19;
- completed null: 21;
- tag-not-found: 5.

The v0.50 Activity source is byte-identical in the historical v0.50 branch and the current research lineage:

`DirectTypedVendorCharacteristicsOracleActivity.kt`
blob SHA:
`25df33a8379ecf6ae5707e7479111136909c1c1b`

Therefore the changed result must not be explained by silently changing the v0.50 lookup code.

The runtime-state reason remains unknown.

## Fresh Camera-5 values

On physical Camera 5 the fresh v0.50 rerun returned:

- `physicalCameraScene : int[] = [66]`
- `sceneCameraIdCapability : int[] = [1]`
- `cameraIdCustomInfo : int[] = non-null structured array`
- `teleSupport : byte = 1`
- `rawZoomSupported : byte = 1`

Still null:

- `rawSensorResolution`
- `needOpenPhysicalCamera`
- `ultraResolutionSwitchSupportedSize`

Still tag-not-found:

- `ultraHighPixelMonoSupported`

No interpretation is assigned to `66` without an APK-established field/layout mapping.

## Exact new static type recovery

Direct DEX inspection of the supplied HONOR Camera .452 and the current .706 package confirms the same constructor type pattern in both APKs.

The relevant `CameraCharacteristics.Key` names are constructed with:

### Byte.TYPE

- `rawImgSupported`
- `rawForBokehSupported`
- `hwProfessionalRawCaptureMode`
- `remosaicSupported`
- `softRemosaicSupported`
- `frontSensorRemosaicSupported`
- `sensorRemosaicSupported`
- `subSensorRemosaic`
- `remosaicFlashSupported`
- `highPixelAlgoSupported`

### Integer.TYPE

- `professionalTeleRawLogicalCameraID`
- `rearSensorZoomRemosaicSupported`
- `isUltraHighPixelSupportBeauty`
- `ultraHighPixelWatermarkSupported`

### int[]

- `rawCaptureSize`
- `supportOfflineRawSceneMode`
- `hwCaptureRawStreamConfigurations`
- `customIdWithA200`
- `brightnessThresholdWithA200`
- `ultraHighPixel`
- `highPixelLivePhotoSupported`
- `highPixelLivePhotoResolution`

These are not guessed types. They are selected from the OEM bytecode constructor path in both compared APKs.

## v0.61 experiment

v0.61 keeps the original nine v0.50 keys as controls and adds the exact RAW/high-pixel/remosaic keys above.

For each public/disclosed Camera2 ID:

1. obtain `CameraCharacteristics`;
2. record ordinary key enumeration;
3. construct each exact OEM-observed key with its exact Java type;
4. perform the first read;
5. if the first read completes, repeat it twice;
6. record SHA-256 of each encoded value and whether the repeated reads equal the first result.

No CameraDevice is opened.

## Authority

`CAMERA2_CHARACTERISTICS_DIRECT_TYPED_VENDOR_KEY_READ_ONLY`

The experiment does not:

- open a camera;
- submit a CaptureRequest;
- create an ImageReader;
- write a vendor request;
- invoke Honor Binder/ServiceHost;
- spoof package/signature identity;
- create an extension session;
- change Scientific Master or calibration authority.

## Decision gates

The highest-value Camera-5 outputs are:

- `rawCaptureSize`
- `hwCaptureRawStreamConfigurations`
- `professionalTeleRawLogicalCameraID`
- `remosaicSupported`
- `sensorRemosaicSupported`
- `rearSensorZoomRemosaicSupported`
- `ultraHighPixel`
- `customIdWithA200`

A non-null result is runtime characteristics evidence only.

It does not prove:
- an OEM shutter route;
- a particular active physical sensor;
- 200MP native ADC/CFA topology;
- a pre-remosaic RAW buffer;
- physical colour/noise calibration.

## Relation to v0.59

Four independent v0.59 16320x12288 RAW10 captures now share the same population topology:

- full allocation: 250,675,200 bytes;
- meaningful front domain: 15,728,640 bytes;
- 3072 effective rows;
- 5100 packed bytes + 20 zero padding per effective row;
- every byte after 15,728,640 is zero;
- each capture has a different source and meaningful-prefix SHA-256.

This strengthens the stop rule against further public Camera2 envelope scaling.

v0.61 therefore investigates metadata/capability surfaces, not another larger public RAW envelope.

Permanent law:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
