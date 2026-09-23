# TruthRaw v0.61 — HONOR RAW/remosaic vendor-characteristics follow-up

Date: 2026-09-23

Status: **READ-ONLY RESEARCH / FROZEN v0.84.3 UNCHANGED**

Branch:

`research/honor-raw-remosaic-v061`

## Trigger

A fresh user-supplied v0.50 report from the Android-17 BKQ-N49 produced non-null direct typed vendor-characteristic values that were not present in the earlier recorded v0.50 run.

Fresh report identity:

- file: `TRUTHRAW_DIRECT_TYPED_VENDOR_CHARACTERISTICS_ORACLE_v050 (3).json`
- bytes: `45,265`
- SHA-256: `49ab002751bea88e5ff1cccf77a783e4d1fd1e6c7099a55d2837198f68af96a8`
- createdAtUtc: `2026-09-23T15:17:42.679500Z`
- device: HONOR BKQ-N49 / Android 17 / SDK 37
- firmware fingerprint: `HONOR/BKQ-N49/HNBKQ:17/HONORBKQ-NXX/11.0.0.120C901E8:user/release-keys`

The report still states read-only authority, no camera open, no capture, no image-buffer access, no vendor request write, no HONOR Binder call and no package spoofing.

## Fresh direct-read observations

Physical camera 5 returns:

- `physicalCameraScene = [66]`
- `sceneCameraIdCapability = [1]`
- `teleSupport = 1`
- `rawZoomSupported = 1`

while at least these targeted values remain null:

- `rawSensorResolution`
- `needOpenPhysicalCamera`
- `ultraResolutionSwitchSupportedSize`

The fresh report also returns a populated `cameraIdCustomInfo` array.

This does not erase the older null-only v0.50 result. It establishes a later read-only observation with different runtime-returned values. The current evidence does not prove why the behavior changed because the report does not contain an installed TruthRaw APK self-hash or camera-service state identity.

## Exact .452 bytecode meaning of physicalCameraScene scene 66

Source APK:

- HONOR Camera `171.0.10.452`
- SHA-256 `3985d9ce23ca4e47cdd34231723b8006f033753c6a3f611685c5c8fffd457d52`

Static DEX trace:

1. `com.hihonor.device.capabilities.physicalCameraScene` is constructed as `CameraCharacteristics.Key<int[]>`.
2. Obfuscated method `r7/a.s(int)` reads that exact key and returns true when the requested scene integer exists in the returned array. Its log string is `getPhysicalCameraScene: characteristics is null`.
3. `CameraSceneModeUtil.getProPhotoSceneMode()` tests:
   - scene `65`; when supported and JPEG-L is open, returns `65`;
   - scene `66`; when supported and ProPhoto RAW is open, returns `66`;
   - otherwise returns ordinary scene `2`.
4. `zoom/controller/d.isCurrentRawOpen()` independently tests `physicalCameraScene(66)` together with `PreferencesUtil.isRawOpened(ProPhotoMode)`.

Therefore, within this exact OEM software version:

`physicalCameraScene contains 66`

has a concrete software-route meaning:

`PROPHOTO_RAW_PHYSICAL_CAMERA_SCENE_CAPABILITY`

It is not sensor-geometry or ADC evidence.

## cameraIdCustomInfo structure

The exact .452 parser treats `cameraIdCustomInfo` as `int[]` and splits it into records of exactly **10 integers**.

The fresh rear/physical-camera array contains 310 integers, i.e. 31 record slots. Its first nine non-zero records are:

```
[1, -1, -1, -1, -1, -1, 10, -1, -1, 8]
[1,  0, -1, -1, -1, -1, 10, -1, -1, 8]
[1, 47, -1, -1, -1, -1, 10, -1, -1, 4]
[1, 33, -1, -1, -1, -1,  9, -1, -1, 2]
[1, 59, -1, -1, -1, -1, 10, -1, -1, 8]
[1, 71, -1, -1, -1, -1, 10, -1, -1, 2]
[1, 53, -1, -1, -1, -1, 10, -1, -1, 0]
[1, 65, -1, -1, -1, -1, 10, -1, -1, 6]
[1, 67, -1, -1, -1, -1, 10, -1, -1, 2]
```

The parser and downstream selection code prove that these records are camera-ID routing information and are selected in relation to scene/UI state. Not every column has yet been assigned a semantic name. Do not invent labels for the unresolved positions.

A notable fact is that scene `65` appears in this custom-ID table while scene `66` does not. Scene 66 instead has the separate `physicalCameraScene` route above. This is consistent with ProPhoto RAW using the OEM physical-camera path, but that is still software-route evidence only.

## v0.61 exact typed-key expansion

v0.61 adds read-only direct lookups for exact .452-derived types:

- `physicalCameraScene : int[]`
- `sceneCameraIdCapability : int[]`
- `cameraIdCustomInfo : int[]`
- `rawSensorResolution : int[]`
- `rawCaptureSize : int[]`
- `hwCaptureRawStreamConfigurations : int[]`
- `supportOfflineRawSceneMode : int[]`
- `ultraResolutionSwitchSupportedSize : int[]`
- `rawImgSupported : byte`
- `hwProfessionalRawCaptureMode : byte`
- `rawForBokehSupported : byte`
- `rawZoomSupported : byte`
- `jpeglZoomSupported : byte`
- `teleSupport : byte`
- `remosaicSupported : byte`
- `sensorRemosaicSupported : byte`
- `frontSensorRemosaicSupported : byte`
- `ultraHighPixelMonoSupported : byte`
- `professionalTeleRawLogicalCameraID : int`
- `rearSensorZoomRemosaicSupported : int`

The experiment remains characteristics-only.

## Why these keys now matter

The public Camera2 high-resolution RAW payload experiments v0.56-v0.59 repeatedly expose a populated sample domain structurally equivalent to 4080x3072 despite larger app-visible envelopes.

The v0.61 question is therefore different:

> Does the OEM capability surface expose a distinct RAW/remosaic/tele-RAW route definition that can explain or identify the OEM route without opening a camera or invoking privileged ServiceHost interfaces?

A non-null answer may select the next lawful experiment. It does not by itself admit any Scientific Master.

## Hard boundary

v0.61 must not:

- open a CameraDevice;
- create CaptureRequest or ImageReader;
- write vendor request keys;
- invoke HONOR Binder/ServiceHost;
- spoof package identity;
- alter the stock camera app;
- convert capability metadata into Direct-CFA authority.

Permanent rule:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
