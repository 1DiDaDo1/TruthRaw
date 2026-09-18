# TruthRaw v0.46 — Honor Capability Route Oracle

Date: 2026-09-19

## Goal

Read the exact runtime CameraCharacteristics values behind five Honor vendor-characteristic names found by static analysis of the exact user-supplied Honor Camera APK (SHA-256 `3985d9ce23ca4e47cdd34231723b8006f033753c6a3f611685c5c8fffd457d52`).

This experiment runs on the user's actual **Android 16 / SDK 36** device. The Honor camera APK may target newer APIs; APK target SDK is not treated as the runtime Android version.

## Target characteristic names

- `com.hihonor.device.capabilities.physicalCameraScene`
- `com.hihonor.device.capabilities.sceneCameraIdCapability`
- `com.hihonor.device.capabilities.cameraIdCustomInfo`
- `com.hihonor.device.capabilities.needOpenPhysicalCamera`
- `com.hihonor.device.capabilities.ultraResolutionSwitchSupportedSize`

The app enumerates public camera IDs and physical IDs disclosed through logical-camera characteristics and reads the matching keys only when they are present in `CameraCharacteristics.keys`.

## Static APK findings used only to choose what to observe

Static analysis of the exact Honor APK shows:

- feature value `200M` targets `UltraHighPixelMode`;
- feature value `50M` targets `UltraResolutionMode`;
- `CameraSceneModeUtil.getHighPixelSceneMode()` returns:
  - 53 for ordinary `UltraHighPixelMode`;
  - 87 when Live Photo is open;
  - 110 for `UltraResolutionMode`;
- `CameraUtil.isUsePhysicalCamera(modeName)` resolves a scene enum and checks it against `physicalCameraScene`, subject to an additional device-state guard;
- `ConstantValue.CAMERA_BACK_PHYSICAL_ID` is resolved dynamically through Honor's `CameraAbilityInterface.getPhysicalBackCameraId()`;
- the high-pixel controller has a branch that can reset the persisted camera ID to that dynamic physical-back ID;
- `UltraHighPixelModeProcessor.getJsonFileName(sceneMode)` selects `pipeline4rawmfultrahighpixelcap.json` for **processor-internal sceneMode values 23, 24, 32 and 33**.

### Critical namespace correction

The UI/mode scene **53** must not be equated with the processor-internal scene values **23/24/32/33**. They are observed in different code layers and v0.46 keeps them separate.

## Authority / safety boundary

v0.46 is:

`CAMERA2_CHARACTERISTICS_OBSERVATION_ONLY`

It does **not**:

- open a camera;
- create or submit a CaptureRequest;
- create an ImageReader;
- read image pixels;
- call Honor Binder services;
- spoof package/signature identity;
- write vendor request keys;
- change the Scientific Master;
- create calibration authority.

Static APK findings are `STATIC_APK_TARGET_SELECTION_ONLY`; runtime key values are the only v0.46 device observations.

## Expected useful outcomes

A useful device result can establish whether the exposed runtime characteristics contain, for camera 0 and/or physical IDs 2/4/5:

- scene 53 / 87 / 110 in `physicalCameraScene`;
- exact scene-to-camera-ID pairs in `sceneCameraIdCapability`;
- exact raw `cameraIdCustomInfo` groups;
- exact `needOpenPhysicalCamera` groups;
- exact switchable high-resolution size pairs, including whether 8192×6144 and/or 16320×12288 are present.

This can tighten the OEM route model, but it still cannot prove that a particular OEM shutter event used physical Camera 5 or delivered 200MP Direct-CFA samples.
