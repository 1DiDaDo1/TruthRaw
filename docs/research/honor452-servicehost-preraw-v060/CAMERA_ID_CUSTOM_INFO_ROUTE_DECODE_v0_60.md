# TruthRaw v0.60 — HONOR cameraIdCustomInfo route decode

Date: 2026-09-23

Status: **STATIC BYTECODE + READ-ONLY RUNTIME CHARACTERISTIC CORRELATION**

Frozen production v0.84.3 remains untouched.

## Runtime source

The new Android-17 v0.50 rerun returns a 310-int `cameraIdCustomInfo` array for camera IDs 0, 2, 4 and 5. These four arrays are identical.

The first nine 10-int records are non-zero/configured:

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

The remainder is zero-filled.

## Record width is proven

HONOR method `r7/a.a(ArrayList, SilentCameraCharacteristics)` reads characteristic
`com.hihonor.device.capabilities.cameraIdCustomInfo`, requires the array length to be divisible by 10, and stores every consecutive 10 integers as one camera-id-info record.

Therefore the 10-int grouping is software-proven, not a heuristic.

## Column 0 = record enable/valid marker

HONOR route-selection code ignores a record unless index 0 equals 1.

For all nine configured rear records index 0 is 1.

## Column 1 = sceneMode

HONOR method `r7/a.b(ArrayList, n7/a, int)` compares record index 1 directly with
`n7/a.a`.

`r7/a.h(...)` logs `n7/a.a` explicitly as `sceneMode` before calling that selector.

Therefore record index 1 is proven to be the scene-mode field.

This resolves the configured rear records as:

- -1 = wildcard/default record
- 0 = normal Photo scene
- 47 = SuperMacro scene
- 33 = MovieSlowMotion scene
- 59 = Movie/ProVideo-LUT/log-dependent scene namespace
- 71 = MovieBokehBeauty scene
- 53 = UltraHighPixel / 200M scene
- 65 = ProPhoto JPEG-L scene
- 67 = scene still under exact symbolic trace

## Column 2 = videoFps selector

The same selector compares record index 2 to `n7/a.b`.

`r7/a.h(...)` logs `n7/a.b` as `videoFps`.

Thus index 2 is the video-FPS discriminator. Value -1 is a wildcard in the observed records.

## Camera-ID output columns

HONOR method `r7/a.c(type, record)` returns a camera ID from the record.

### Index 9 = normal/default back target for type=1

When `type == 1`, the function returns record index 9.

In `CameraUtil.getDeviceMode(...)`, the value passed as `type` is derived from whether the persisted camera is front/back; the back path resolves to type 1.

Therefore for the normal rear path, index 9 is the selected camera ID.

For scene 53:

```
[1, 53, -1, -1, -1, -1, 10, -1, -1, 0]
                                             ^
                                             index 9
```

So the configured normal rear target for UltraHighPixel scene 53 is:

`camera ID 0`.

### Index 6 = teleconverter-state target

The selector checks HONOR's `TeleconverterManager` state. When it is active and record index 6 is not -1, index 6 is returned.

For scene 53, index 6 is 10.

This is a camera-routing alternative for HONOR's teleconverter state; it must not be confused with physical tele camera ID 5.

### Index 8 = special gimbal/UI-style target

The selector can return index 8 under a gimbal/UI-style condition. In the current rear records index 8 is -1, so it does not alter the scene-53 result.

## physicalCameraScene relationship

Runtime direct typed reads return:

`physicalCameraScene = [66]`

for the rear camera set.

Static bytecode resolves scene 66 as the ProPhoto RAW scene.

`CameraUtil.isUsePhysicalCamera(modeName)` performs:

```
modeName
 -> CameraSceneModeUtil.getSceneModeEnum(modeName)
 -> CameraDeviceUtil.w(scene)
 -> physicalCameraScene membership check
```

UltraHighPixel mode resolves to scene 53 (or 87 for Live Photo; UltraResolution/50M resolves to 110).

Because the runtime list is only `[66]`, scene 53 is not admitted by this direct-physical-camera scene gate.

A second OEM code path makes the consequence explicit: when entering UltraHighPixelMode, HONOR only resets the persisted camera ID to `CAMERA_BACK_PHYSICAL_ID` if `CameraUtil.isUsePhysicalCamera(UltraHighPixelMode)` is true. With `physicalCameraScene=[66]`, the 53 scene does not satisfy that gate.

## Current route model

For the normal rear 200M/high-pixel UI state on this runtime, the strongest software/runtime model is now:

```
UI 200M
 -> UltraHighPixelMode
 -> UI scene 53
 -> cameraIdCustomInfo record(scene=53)
 -> normal rear target camera ID 0
 -> logical multi-camera / zoom routing
 -> physical tele 5 can become the active/bound physical sensor
 -> HONOR ServiceHost UltraHighPixel processing
 -> internal processor scene/pipeline selection
 -> remosaic/high-pixel output path
```

This is consistent with TruthRaw's independently observed successful logical-0 -> physical-5 Camera2 binding.

It does NOT prove that the OEM UltraHighPixel shutter exposes the same bytes as public Camera2, nor does it prove native 200MP ADC sampling.

## Screenshot binding

The user-supplied screenshots are retained conceptually as UI-state evidence for the visible Hi-Res/200MP tele workflow. UI labels and lens-selection controls help bind the manual test procedure, but do not establish internal camera IDs or RAW topology by themselves.

## Next exact task

The remaining high-value gap is no longer “which logical camera does scene 53 choose?” — that is now resolved to camera 0 for the normal rear route.

The next task is to trace **how logical camera 0 is driven into physical tele 5 inside UltraHighPixelMode**, specifically:

1. zoom/lens-selection state written by UltraHighPixelMode;
2. request/result vendor keys around active physical sensor selection;
3. the handoff into `UltraHighPixelModeProcessor`;
4. whether scene 53 is translated into ServiceHost processor scene 5/23/24/32/33 and under what conditions;
5. buffer/surface topology before and after remosaic.

Permanent boundary:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
