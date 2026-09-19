# HONOR Camera 171.0.10.452 -> 171.0.10.706 static route diff

Date: 2026-09-19

## Authority

`STATIC_APK_SOFTWARE_ROUTE_EVIDENCE_ONLY`

This analysis compares two exact Honor Camera APK artifacts. It can establish software structure, strings, typed CameraCharacteristics keys and control-flow relationships. It cannot establish sensor sampling, a successful capture, native ADC topology, Direct-CFA authority or calibration truth.

## Exact artifacts

Android 16 reference:

- version: `171.0.10.452`
- bytes: `84,167,938`
- SHA-256: `3985d9ce23ca4e47cdd34231723b8006f033753c6a3f611685c5c8fffd457d52`

Android 17 artifact acquired by v0.49:

- version: `171.0.10.706`
- bytes: `86,018,325`
- SHA-256: `bbc6312e0289d01a51dbe45efc519e56715e6250521227df81cf64a633b8f027`

The Android-17 APK was re-hashed after extraction from the v0.49 bundle and matched the device-side v0.49 fingerprint exactly.

## Structural changes

- .452 contains 5 DEX files.
- .706 contains 6 DEX files; `classes6.dex` is new.
- APK entry count: 10,425 -> 10,754.
- 364 entries were added, 35 removed, and 10,390 paths are common.
- .706 adds `libods.so` (189,104 bytes).
- `libods.so` exposes white-box/AES/decryption-oriented symbols; no RAW14/camera string evidence was found in that library. It is not promoted as a RAW14 component.
- The APKs retain the same Honor camera signing certificate identity.

## RAW14 search result

A recursive string search across the fully extracted APKs and DEX string pools found no literal:

- `RAW14`
- `raw14`
- `RAW_14`
- `14-bit`
- `14bit`
- `bit14`

in either .452 or .706.

The Android-17 APK does contain generic `bitDepth/getBitDepth/setBitDepth` strings, but the observed uses are in intelligent-voice/TTS code, not camera RAW handling.

No app-level `ImageReader.newInstance` path was found that couples the Android RAW14 numeric format value 44 to an ImageReader construction in .706.

Therefore the static APK diff does not support the claim that .706 added a straightforward Java/Dex `ImageFormat.RAW14` capture path.

## High-pixel route remains semantically stable

The earlier preliminary statement that `UltraHighPixelModeProcessor.capture()` had materially changed is corrected here.

After direct method-level disassembly and reference resolution:

- `UltraHighPixelModeProcessor.capture()` retains the same ServiceHost capture structure and control flow. Differences observed at raw DEX level are attributable to changed/obfuscated referenced symbols, not a demonstrated new RAW14 route.
- `UltraHighPixelModeProcessor.getJsonFileName()` retains the same scene-to-pipeline semantics.
- processor scene values 23, 24, 32 and 33 still select:
  `pipeline4rawmfultrahighpixelcap.json`
- `UltraResolutionSwitchFunction` still maps:
  - `50M` -> `UltraResolutionMode`
  - `200M` -> `UltraHighPixelMode`

The UI/mode scene namespace must still be kept separate from the processor-internal scene namespace.

## ProPhoto / DNG route

No app-level RAW14 rewrite was demonstrated in the ProPhoto/DNG path.

- `ProPhotoMode.capture()` is semantically unchanged in the compared capture route.
- Android-17 adds gimbal/lifecycle handling around ProPhoto.
- The DNG saver still constructs `android.hardware.camera2.DngCreator` from CameraCharacteristics + TotalCaptureResult and saves through the same RAW/DNG storage logic.
- `PreferencesUtil.readRawStatus`, `writeRawStatus`, and composition RAW-resolution handling remain semantically stable for the relevant route.

This does not prove the underlying HAL data format used by Honor internally.

## Physical-camera routing

The app-level high-pixel physical-camera routing model remains semantically stable:

- `CameraUtil.isUsePhysicalCamera(mode)` resolves a scene and checks the physical-camera scene capability.
- the physical back camera ID remains dynamically obtained through Honor's CameraAbility layer rather than being hard-coded as camera 5 in the observed method.

## Newly recovered exact hidden-key types

The most actionable result is the exact Java type with which Honor constructs several vendor `CameraCharacteristics.Key` objects.

In .706, the static initializer constructs keys through Honor helper `LW8/e.c(Class, String)`, which creates `CameraCharacteristics.Key` objects. The same relevant typing pattern is also present in .452.

Exact observed types:

### int[] / `IntArray`

- `com.hihonor.device.capabilities.physicalCameraScene`
- `com.hihonor.device.capabilities.rawSensorResolution`
- `com.hihonor.device.capabilities.sceneCameraIdCapability`
- `com.hihonor.device.capabilities.cameraIdCustomInfo`
- `com.hihonor.device.capabilities.needOpenPhysicalCamera`
- `com.hihonor.device.capabilities.ultraResolutionSwitchSupportedSize`

### scalar byte / `Byte.TYPE`

- `com.hihonor.device.capabilities.teleSupport`
- `com.hihonor.device.capabilities.ultraHighPixelMonoSupported`
- `com.hihonor.device.capabilities.rawZoomSupported`

This is static software evidence of the key names and types. It is not runtime evidence that a third-party app is permitted to read them.

## Consequence for the next experiment

v0.46 tested whether five names appeared in `CameraCharacteristics.keys`; they did not on Android 16 or Android 17.

The exact key types now justify one new read-only falsification:

1. construct the exact typed `CameraCharacteristics.Key` locally;
2. call `CameraCharacteristics.get(key)` without opening a camera;
3. record value, null, `IllegalArgumentException`, `SecurityException`, or other failure;
4. compare that direct lookup with normal key enumeration.

This is not a security bypass. It is a standard read-only CameraCharacteristics lookup using an exact name/type pair recovered from the installed OEM software. If Android rejects the lookup, that rejection is the result.

No capture should be attempted in this experiment.
