# TruthRaw v0.61 — MotionCam-discovered session vendor-key oracle

Date: 2026-09-23

Status: EXPERIMENT PLAN / READ-ONLY DISCOVERY FIRST

## Correction of provenance

The vendor/session-key names shown in the user's screenshots were discovered in MotionCam's camera profile/vendor-key editor, not extracted from the HONOR Camera APK.

Therefore their provenance is recorded as:

`RUNTIME_CAMERA2_SESSION_KEY_DISCOVERY_VIA_MOTIONCAM_UI`

and not as:

`HONOR_CAMERA_APK_STATIC_KEY_DISCOVERY`.

This distinction matters. A key displayed by MotionCam is evidence that the device/framework exposes that vendor/session-key name to that application/profile context. The key name alone does not establish its semantics, active default value, native metadata type, or that HONOR Camera uses it in a given shutter route.

## Online corroboration

Public MotionCam source repositories confirm MotionCam is a RAW Camera2 application, while public Qualcomm/CamX logs and camera-community discussions independently show several of the same QTI session keys in real vendor-camera stacks.

Community evidence is target-selection evidence only, not semantic authority.

## Keys visible in the user's MotionCam screenshots

QTI / CodeAurora examples include:

- `AICameraMode`
- `BlurMode`
- `DepthMode`
- `EISMode`
- `EnableAFBracketing`
- `EnableAFFocusMap`
- `EnableAICameraHSR`
- `EnableAutoHDR`
- `EnableCinematicMode`
- `EnableHDRDCGMode`
- `EnableIdealRAW`
- `EnableInsensorZoom`
- `EnableOfflineHALZSL`
- `EnableSportHDRMode`
- `EnableVAI`
- `EnableVIULL`
- `EnableVSR`
- `EnableXCFAOptimization`
- `ExtendedMaxZoom`
- `ExtraPreviewMaxBuffers`
- `HALOutputBufferCombined`
- `HDRMode`
- `HDRModePreference`
- `HorizonLevelControl`
- `InduceDelayTime`
- `InduceErrorCode`
- `InduceEventType`
- `InduceKMDNotifyType`
- `InduceRequestId`
- `InduceSessionName`
- `RawCbSourceType`
- `SnapshotHDRMode`
- `dynamicFPSConfig`
- `enableQLL`
- `enableSecureMode`
- `enableStatsVisualizer`
- `inSensorZoomEnable`
- `numHDRexposure`
- `numPCRsBeforeStreamOn`
- `overrideResourceCostValidation`

HONOR vendor metadata visible in the same MotionCam UI includes:

- `HDRVividEnable`
- `MasterFilmSensorType`
- `aoRunningMode`
- `bioFaceRunningMode`
- `blurZoomRatio`
- `cameraExtension`
- `cameraFoldState`
- `cameraSceneMode`
- `currentUserId`
- `eagleEyeRunningMode`
- `enableMagicMoment`
- `enableMagicMomentCapture`
- `extStreamSize`
- `faceBeautyMode`
- `foldState`
- `gimbalFoldState`
- `hwCamera2Flag`
- `logEnable`
- `mmiLaserEyeSafeMode`
- `openGimbalState`
- `packageName`
- `portraitBokehStatus`
- `teleconverterEnable`
- `thirdPartyCamera`
- `timeLapseMode`
- `videoDynamicFrameRate`
- `videoStabilizationMode`

## Already screened keys

Do not repeat blind value-1 topology tests for keys already covered by prior TruthRaw experiments, including:

- `EnableIdealRAW`
- `RawCbSourceType`
- `EnableXCFAOptimization`
- `HALOutputBufferCombined`
- `EnableInsensorZoom`
- `EnableSnapshotOnlyInsensorZoom`
- `EnableMCXMasterCb`
- `inSensorZoomEnable`
- `EnableVSR`
- `ExtendedMaxZoom`
- `enableQLL`

Those tests did not change the measured app-visible Camera-5 RAW payload topology in the tested contexts.

## v0.61 first-stage target

The next experiment should not write values.

For logical camera 0 and physical camera 5:

1. enumerate all `availableSessionKeys`;
2. enumerate all `availableCaptureRequestKeys`;
3. preserve each full key name exactly;
4. record whether each key exists at logical and/or physical scope;
5. resolve the native vendor tag ID and metadata type where possible without creating a capture session;
6. record existing request-template/default values read-only where the framework permits;
7. classify MotionCam-discovered keys separately from APK-derived keys;
8. perform no capture and no Scientific Master writeback.

Priority candidates for native type/default-state resolution:

- `org.codeaurora.qcamera3.sessionParameters.EnableHDRDCGMode`
- `org.codeaurora.qcamera3.sessionParameters.EnableOfflineHALZSL`
- `org.codeaurora.qcamera3.sessionParameters.EnableAICameraHSR`
- `org.codeaurora.qcamera3.sessionParameters.AICameraMode`
- `org.codeaurora.qcamera3.sessionParameters.SnapshotHDRMode`
- `com.hihonor.capture.metadata.MasterFilmSensorType`
- `com.hihonor.capture.metadata.HDRVividEnable`
- `com.hihonor.capture.metadata.aoRunningMode`
- `com.hihonor.capture.metadata.cameraSceneMode`
- `com.hihonor.capture.metadata.extStreamSize`
- `com.hihonor.capture.metadata.teleconverterEnable`
- `com.hihonor.capture.metadata.thirdPartyCamera`

## Authority boundary

A discovered or type-resolved vendor key remains routing/configuration evidence only.

No key name or numeric value proves:
- physical sensor topology;
- native ADC bit depth;
- 200MP Direct-CFA;
- remosaic identity;
- DCG behavior;
- calibration authority;
- a specific OEM shutter path.

Permanent law:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
