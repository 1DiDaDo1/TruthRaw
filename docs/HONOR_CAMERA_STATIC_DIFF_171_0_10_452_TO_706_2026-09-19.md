# Honor Camera 171.0.10.452 → 171.0.10.706 static route diff

Date: 2026-09-19

Authority: `STATIC_APK_SOFTWARE_ROUTE_ANALYSIS_ONLY`

## Artifact identity

Android-16 Honor Camera:

- version `171.0.10.452`
- bytes `84,167,938`
- SHA-256 `3985d9ce23ca4e47cdd34231723b8006f033753c6a3f611685c5c8fffd457d52`

Android-17 Honor Camera:

- version `171.0.10.706`
- bytes `86,018,325`
- SHA-256 `bbc6312e0289d01a51dbe45efc519e56715e6250521227df81cf64a633b8f027`

Both artifacts carry the same Honor camera signing certificate:

- subject/issuer `C=CN, O=Honor, OU=Hihonor, CN=apkkey_camera_v2`
- serial `018809BF7E40`
- certificate SHA-256 `02:33:10:E4:00:6E:C1:DE:44:BA:F2:6E:9A:5C:EB:21:EB:48:97:F2:6A:19:C5:49:0B:7F:22:C4:31:C2:FA:A0`

This proves signer continuity only.

## Package growth

- ZIP entries: `10,425 → 10,754`
- net: `+329`
- concrete add/remove count: `+364 / -35`
- DEX files: `5 → 6`
- total uncompressed DEX: `25,626,236 → 27,229,152 bytes`
- new native library: `libods.so` (189,104 bytes)

Static strings identify `libods.so` as an Honor secure/encryption ODS helper; no RAW14 marker was found there.

The added Java/Kotlin surface is dominated by intelligent/gimbal/audio/analytics/camera2 growth. The newly named `com.hihonor.cameraextension` package is an **OpenGimbal** API, not Android Camera Extension imaging support.

## RAW14 search

No literal `RAW14`, `raw14`, `14-bit` or `14bit` imaging marker was found in either APK's DEX strings.

No such marker was found in the Android-17 APK's arm64 native libraries either.

This is negative static evidence only. It does not rule out:

- numeric format IDs;
- reflection without a RAW14 literal;
- framework/HAL implementation;
- privileged system service implementation;
- vendor-native implementation outside this APK.

A particularly relevant fact is that .706 still reports compileSdk 36 while Android 17 RAW14 is API 37. The APK therefore cannot have been compiled against the public API-37 `ImageFormat.RAW14` symbol in the normal source-level way. It could still use a numeric/vendor/private route.

## 200MP / UltraHighPixel route

The central high-pixel mapping is unchanged:

- `50M → UltraResolutionMode`
- `200M → UltraHighPixelMode`

The `UltraHighPixelModeProcessor.getJsonFileName` pipeline set is also unchanged:

- `pipeline4capdavinci.json`
- `pipeline4rawmfultrahighpixelcap.json`
- `pipeline4arcmfnrmscap.json`
- `pipeline4hdrcap.json`
- `pipeline4capbackremosaic.json`
- `pipeline4mfdncap.json`

Therefore the OEM app-level high-pixel ServiceHost architecture was already present in .452. Android 17 did not introduce that route as a new .706 RAW14 mechanism.

## RAW assets

These resources are byte-identical:

- `assets/raw_in.json` — `a422db58fd08768e3480b7bf95b609a6411dd1e1cec193138540008948d74beb`
- `assets/raw_out.json` — `17e1caf3dc9badf68b255ec05a4620dee68d6f4f2de475b43f90b61fef9ffeb4`
- `assets/pipelineFor3D.json` — `0a401ae9ce187812796d5ad53c6e264b05e88814bb5519a2430456e30290a622`
- `assets/rules_camera_v0.1.json` — `507e49d1e66cd8b1a88d3ee62a36ce4845cb8059ed054f21bc984457d79163ad`

## Vendor capability delta

The Android-17 APK adds eleven exact `com.hihonor.device.capabilities.*` name tokens and removes none:

- `sensorDuvRange`
- `WBCorrectionSupported`
- `timelapseStaticVersion`
- `timelapseProMoveVersion`
- `bitrateSupported`
- `encoderSupported`
- `colorModeSupported`
- `longBatterySupported`
- `NormalVideoSupportArriLut`
- `hw-sensor-exposure-range-custom`
- `videoResolution21to9Support`

None is RAW14-specific.

The RAW-named capability set itself is unchanged. Relevant unchanged targets include:

- `rawImgSupported` — Byte
- `hwCaptureRawStreamConfigurations` — IntArray
- `hwProfessionalRawCaptureMode` — Byte
- `professionalTeleRawLogicalCameraID` — Integer
- `rawCaptureSize` — IntArray
- `rawForBokehSupported` — Byte
- `rawSensorResolution` — IntArray
- `rawZoomSupported` — Byte
- `supportOfflineRawSceneMode` — IntArray

Core consumers such as `CameraUtil.isRawSupport`, `getRawSize`, `getTeleRawCameraId`, `getRawSensorResolution` and `isRawSupportZoom` retain the same route logic apart from obfuscation/static-field index movement.

## One real RAW-policy change

`CustomConfigurationUtil.isSupportedRawSaved()` changes materially.

### .452

1. if `isMagicLite()` → false;
2. otherwise read system property `msc.camera.rawphoto.save`;
3. property default = `Boolean.TRUE`.

### .706

1. MagicLite early-false gate removed;
2. same system property read;
3. property default = **`Boolean.FALSE`**.

The rest of the fallback is retained.

This means the new OEM build makes RAW-saving enablement stricter and more dependent on product/system configuration. It is a real behavioral change but is **not** evidence of RAW14.

## Pro mode changes

The Android-17 build adds ProPhoto callback plumbing including:

- a pre-capture handler;
- `HnPostCamData` callback;
- capture-process callback.

The post-camera-data callback recognizes `DATA_TYPE_CAPTURE_FRAME_DONE = 96`.

The surrounding control flow is consistent with capture/gimbal synchronization. No RAW14-specific marker was found in these additions.

## Scientific interpretation

The strongest static conclusion is not "Honor added RAW14 to Camera.apk."

It is almost the opposite:

**the .706 APK overwhelmingly inherits the .452 high-pixel and RAW route, while the public Android-17 RAW14 format exists at framework level but remains unadvertised through TruthRaw Camera2.**

Therefore, if RAW14 functionality is genuinely active on this firmware, the current evidence shifts probability toward a lower or privileged layer—framework, camera provider/HAL, ServiceHost/system service, or another vendor-native component—rather than a newly visible Java/Kotlin RAW14 branch in the Honor camera app.

This is an inference, not a sensor fact.

## Next experiment

Do not brute-force capture.

The next read-only test should construct the unchanged Honor RAW vendor characteristic keys directly with the exact statically inferred types and call `CameraCharacteristics.get(key)`, while separately recording whether each name is present in `CameraCharacteristics.keys`.

That tests a narrower hypothesis than v0.46:

> an Honor vendor key may be gettable by exact name/type even when omitted from the enumerable key list visible to a third-party package.

No camera-open or capture is required.
