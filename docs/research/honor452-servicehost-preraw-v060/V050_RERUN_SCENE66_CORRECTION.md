# TruthRaw v0.60 correction — v0.50 rerun + scene 66 meaning

Date: 2026-09-23

Authority:
- runtime values: `CAMERA2_CHARACTERISTICS_DIRECT_TYPED_VENDOR_KEY_READ_ONLY`
- scene meaning: `STATIC_APK_BYTECODE_ROUTE_ANALYSIS_ONLY`

Frozen production remains untouched.

## New v0.50 rerun artifact

File:
`TRUTHRAW_DIRECT_TYPED_VENDOR_CHARACTERISTICS_ORACLE_v050 (3).json`

Bytes:
`45,265`

SHA-256:
`49ab002751bea88e5ff1cccf77a783e4d1fd1e6c7099a55d2837198f68af96a8`

This rerun differs materially from the older v0.50 result stored in the repository.

For physical Camera 5, direct typed reads now return:
- `physicalCameraScene = [66]`
- `sceneCameraIdCapability = [1]`
- `teleSupport = 1`
- `rawZoomSupported = 1`

while:
- `rawSensorResolution = null`
- `needOpenPhysicalCamera = null`
- `ultraResolutionSwitchSupportedSize = null`

The same `physicalCameraScene=[66]` and `sceneCameraIdCapability=[1]` are also present for camera IDs 0, 2 and 4. Front camera 1 does not expose those two values in this rerun.

## Critical scene-66 correction

Direct bytecode analysis of the supplied HONOR Camera `171.0.10.452` APK resolves:

`com.hihonor.camera2.utils.CameraSceneModeUtil.getProPhotoSceneMode()`

The method selects among scene IDs 65, 66 and 2.

Observed control flow:
- scene 65 is selected on the JPEG-L ProPhoto branch when the scene is supported and `PreferencesUtil.isJpeglOpened()` is true;
- scene 66 is selected on the RAW ProPhoto branch when the scene is supported and `PreferencesUtil.isRawOpened("com.hihonor.camera2.mode.prophoto.ProPhotoMode")` is true;
- otherwise the method falls back to scene 2.

Therefore:

**scene 66 is a ProPhoto RAW scene identifier in this APK layer.**

It is not the UltraHighPixel/200M UI scene. The already-established high-pixel UI scene remains 53 (87 with Live Photo), while UltraResolution/50M remains 110.

Consequently, `physicalCameraScene=[66]` must not be promoted as evidence that the 200M UltraHighPixel scene uses physical-camera routing. It is instead direct runtime capability evidence consistent with physical-camera use in the ProPhoto RAW scene.

## cameraIdCustomInfo structure

The new runtime array has 310 integers.

For camera IDs 0, 2, 4 and 5:
- the arrays are byte/value-identical;
- the first 90 integers form nine non-zero/configured groups of 10;
- the remaining 220 integers are zero.

The nine groups are:

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

The existing v0.46 code grouped this characteristic in blocks of 10 based on prior static control-flow work. The exact semantic meaning of each field inside a group is still under trace; no field-label inference is promoted yet.

Notably, scene-like values `53` and `65` appear inside these groups, but this alone does not establish the meaning of their column.

## cameraIdCustomInfo consumer decoded

A full Dalvik instruction walk of:

`com.hihonor.camera2.camerafactory.CameraServiceFactory.createCameraAbility`

now resolves the consumer of `Ls8/a;->E5`, the field initialized from:

`com.hihonor.device.capabilities.cameraIdCustomInfo`.

The important correction is that this consumer does **not** parse the array in groups of 10.

Its exact algorithm is:

1. read the characteristic as `int[]`;
2. if it is null, replace it with an empty array;
3. if non-empty and not the special single-element `[1]` case, iterate from index 0 in steps of **2**;
4. treat each pair as `(array[i], array[i+1])`;
5. only retain pairs whose first element is one of:
   `59, 19, 23, 64, 67`;
6. store the retained mapping in `LI1/c;->h` as:
   `Integer(array[i]) -> String(array[i+1]) + ","`.

The static whitelist is initialized in `LI1/c.<clinit>()` as exactly:

`[59, 19, 23, 64, 67]`.

Known downstream uses establish that these first-element IDs select camera-ID mappings for specific modes:

- `59` -> MovieMode custom/persisted camera ID
- `19` -> AperturePhotoMode custom/persisted camera ID
- `23` -> BeautyMode custom/persisted camera ID
- `64` -> VideoMode camera-ID mapping
- `67` -> SuperNightVideoMode camera-ID mapping

Therefore the characteristic is, in this consumer, a **pair-oriented custom camera-route list**, not a ten-field record table.

### Consequence for the current Camera-5 runtime array

The earlier grouping into 10-integer rows was only a diagnostic display convention in the v0.46 TruthRaw code. It is not supported by this exact consumer.

In the first 90 configured integers of the current Camera-5 array, values such as `59`, `53`, `65` and `67` occur at odd positions in the displayed 10-wide view. In the decoded pair consumer, the whitelist is applied to the **even-index / first element of each pair**.

Therefore:

- the visible occurrence of `53` cannot be promoted as an UltraHighPixel route key from this consumer;
- the visible occurrences of `59` and `67` likewise do not become retained map keys merely because those numbers appear in the array;
- the previous idea that the group containing `53` might directly expose a 200MP route is not supported by `CameraServiceFactory.createCameraAbility`.

This closes one speculative path and replaces it with an exact pair-level interpretation for this consumer.

## Next exact target

Two higher-value targets remain:

1. trace other static consumers, if any, of the same `cameraIdCustomInfo` characteristic to determine whether a different parser gives meaning to the values `53`, `65` or `67` at their observed positions;
2. continue the Android-17/API-37 vendor-extension route, because it is now a separate official discovery surface that could expose HONOR-specific high-pixel processing without relying on this characteristic.


Trace the consumer of `cameraIdCustomInfo` in:

`com.hihonor.camera2.camerafactory.CameraServiceFactory.createCameraAbility`

The supplied APK bytecode shows this method reading the characteristic and logging it as a **scene logical cameraId list**. The next task is to recover the exact ten-field record semantics and determine whether the group containing 53 can be tied to UltraHighPixel camera routing without inference.

Permanent boundary:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
