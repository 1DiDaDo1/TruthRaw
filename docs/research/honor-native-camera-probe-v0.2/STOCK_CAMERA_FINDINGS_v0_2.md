# HONOR stock Camera.apk findings — v0.2

Status: **STATIC IMPLEMENTATION CLUES ONLY — runtime device proof required**

Source APK SHA-256: `3985d9ce23ca4e47cdd34231723b8006f033753c6a3f611685c5c8fffd457d52`

These findings are used only to improve the FotoGraaf acquisition implementation. They do not determine TruthRaw evidence, calibration, topology, color, noise or Scientific Master values.

## 1. Professional tele RAW route

The stock app defines `CameraUtil.getTeleRawCameraId()` and reads:

`com.hihonor.device.capabilities.professionalTeleRawLogicalCameraID`

Static key construction indicates an integer value. The stock physical-camera zoom controller calls this helper in `ProPhotoMode` while RAW is open. Therefore the next FotoGraaf build must query this value from the actual device instead of hard-coding a guessed stock tele-RAW logical ID.

The existing physical-ID-5 Direct-CFA route remains a separate independently proven candidate and must not be overwritten by the stock hint.

## 2. Runtime key types recovered from stock code

Important static expectations include:

- `professionalTeleRawLogicalCameraID` -> `int`
- `rawSensorResolution` -> `int[]`
- `rawCaptureSize` -> `int[]`
- `hwCaptureRawStreamConfigurations` -> `int[]`
- `hwProfessionalRawCaptureMode` -> `byte`
- `rawZoomSupported` -> `byte`
- `physicalCameraScene` -> `int[]`
- `needOpenPhysicalCamera` -> `int[]`
- `opticalZoomThreshold` -> `byte[]`
- `opticalZoomSupported` -> `byte`
- `sensorRemosaicSupported` -> `byte`
- `rearSensorZoomRemosaicSupported` -> `int`

Request-side static expectations include:

- `cameraSaveRawMode` -> `int`
- `teleconverterEnable` -> `boolean`
- `superTelephotoEnable` -> `java.lang.Boolean`
- `qcomRemosaicEnable` -> `int`
- `quadraRemosaicHdMode` -> `byte`
- `zoomRatio` -> `float`
- `macroEnable` -> `int[]` in the inspected stock-key construction, so it must **not** be treated as a simple Boolean without runtime confirmation.

Result-side expectations include:

- `previewCameraPhysicalId` -> `java.lang.Byte`
- `opticalSwitchStatus` -> `byte`
- `switchMacroType` -> `int`

All of these remain `STATIC_APK_INFERRED` until runtime enumeration/value-class observation confirms them on the actual device build.

## 3. OIS behavior

The stock camera contains an OIS setting path that writes the standard Camera2 key:

`CaptureRequest.LENS_OPTICAL_STABILIZATION_MODE`

When its persisted setting equals `"on"`, the path writes integer `1`; the off path writes integer `0`. The setting is applied to both preview and capture flows.

FotoGraaf implication:

- expose OIS as an explicit standard Camera2 control when the selected camera advertises it;
- apply the same requested state to preview and capture;
- record the requested state and the actual `CaptureResult.LENS_OPTICAL_STABILIZATION_MODE` separately;
- never infer stabilization state merely from the UI toggle.

## 4. Manual focus behavior

The stock manual-focus function (`MfFunction`) takes a float focus distance. Its camera-request path:

1. writes `CaptureRequest.CONTROL_AF_MODE = 0` to preview and capture;
2. writes `CaptureRequest.LENS_FOCUS_DISTANCE = <selected float>` to preview and capture.

FotoGraaf implication:

- add an explicit `MF` mode only when the camera reports a positive minimum focus distance/manual-focus support;
- use standard `CONTROL_AF_MODE_OFF` plus `LENS_FOCUS_DISTANCE` rather than inventing a vendor focus tag;
- persist/request focus distance in diopters and record actual `CaptureResult.LENS_FOCUS_DISTANCE`;
- do not translate a requested focus distance into a stronger focus-state claim without result confirmation.

## 5. Teleconverter and super-tele controls

The stock app writes `teleconverterEnable` as a Boolean to both preview and capture flows. It also writes `superTelephotoEnable` as a Boolean-style value to both flows.

These are **not** suitable for the Direct-CFA evidence baseline. They should first appear only on the `NON_CALIBRATION_EXPERIMENT` page because they may enable computational/ISP behavior or a different camera-routing policy.

## 6. High-resolution/remosaic path

The stock app has separate remosaic controls, including `quadraRemosaicHdMode` and `qcomRemosaicEnable`. At least one stock high-quality path sets the remosaic-HD request on preview/capture.

This supports keeping maximum-resolution/remosaic RAW as its own candidate route. A larger output must not be assumed to be the same sample topology as the already proven 4080x3072 Direct-CFA tele route.

## 7. Immediate FotoGraaf control surface

The next build should therefore have a conservative standard-control layer before vendor A/B testing:

- AF: `AF_CONTINUOUS`, tap/single AF, or explicit `MF`;
- MF: focus-distance slider in diopters with actual-result readback;
- OIS: Off/On when advertised, with actual-result readback;
- exposure: standard Camera2 auto/manual controls with requested versus actual result recorded;
- route: ordinary logical, forced physical, HAL-advertised pro-tele RAW candidate;
- RAW size: standard RAW versus separately proven max-resolution/remosaic candidate;
- vendor controls: hidden from evidence baseline and available only in explicit experimental mode.

This should make the camera app more usable without weakening the acquisition proof model.
