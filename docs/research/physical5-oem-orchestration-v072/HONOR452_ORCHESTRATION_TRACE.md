# HONOR .452 high-pixel orchestration trace for v0.72

Date: 2026-09-23

Authority:
`STATIC_APK_BYTECODE_ROUTE_ANALYSIS_ONLY`

Exact source:
`/mnt/data/Camera.apk`

Identity:
- package `com.hihonor.camera`
- bytes `84,167,938`
- SHA-256 `3985d9ce23ca4e47cdd34231723b8006f033753c6a3f611685c5c8fffd457d52`

This document narrows the next runtime experiment. It does not promote static OEM code to runtime proof.

## 1. qcomRemosaicEnable exact key/type path

Exact request-key string:

`com.hihonor.capture.metadata.qcomRemosaicEnable`

Static field:

`Ls8/c;->A2:Landroid/hardware/camera2/CaptureRequest$Key;`

The key is constructed through `Ls8/e.a(Class,String)`.

In the exact `Ls8/c.<clinit>` bytecode, the qcom key is created using the register holding `java.lang.Integer.TYPE`.

Therefore the OEM Java-side key value type is scalar integer, not `int[]`:

`qcomRemosaicEnable -> Integer/int`

This is a static type fact. v0.72 still verifies runtime request/session surfaces before writing it.

## 2. Qualcomm pre-capture consumer

Consumer:

`com.hihonor.camera2.function.resolution.uiservice.f.handle(...)`

On Qualcomm and main-entry context, the handler obtains the current `PhotoResolutionFunction.isRemosaicEnable` value and performs:

`captureFlow.setParameter(qcomRemosaicEnable, Integer.valueOf(isRemosaicEnable))`

On MediaTek the same handler uses the separate key:

`com.mediatek.control.capture.remosaicenable`

The HONOR PhotoResolutionFunction constructor initializes:

`isRemosaicEnable = 1`

The attach path registers remosaic callbacks/pre-capture handling depending on device capabilities, so the value can be runtime-managed. Static code therefore supports `1` as a defensible OEM remosaic-enable test value, but does not prove that every route consumes it identically.

## 3. UltraHighPixel processor binding

`ProcessorFactory` maps:

`com.hihonor.camera2.mode.ultrahighpixel.UltraHighPixelMode`

to:

`com.hihonor.camera2.impl.cameraservice.processor.UltraHighPixelModeProcessor`

This ties the UI/mode layer to the ServiceHost processor layer.

## 4. Processor internal scene-code -> pipeline mapping

`UltraHighPixelModeProcessor.getJsonFileName(int sceneMode)` resolves:

- scene 2 -> `pipeline4mfdncap.json`
- scene 5 -> `pipeline4capbackremosaic.json`
- scene 11 -> `pipeline4hdrcap.json`
- scene 22 -> `pipeline4arcmfnrmscap.json`
- scene 23 -> `pipeline4rawmfultrahighpixelcap.json`
- scene 24 -> `pipeline4rawmfultrahighpixelcap.json`
- scene 31 -> `pipeline4hdrcap.json`
- scene 32 -> `pipeline4rawmfultrahighpixelcap.json`
- scene 33 -> `pipeline4rawmfultrahighpixelcap.json`
- default -> `pipeline4capdavinci.json`

Thus 23/24/32/33 are confirmed ServiceHost processor scene codes selecting the RAW MF ultra-high-pixel capture pipeline.

They are **not** shown by this code to be direct Camera2 request values.

## 5. Where 23/24/32/33 come from

`UltraHighPixelMode$2.onCaptureCompleted(...)` reads:

`com.hihonor.capture.metadata.hintUserValue`

from the capture result.

The exact result-key field is:

`Ls8/d;->k:Landroid/hardware/camera2/CaptureResult$Key;`

When the returned hint value changes, UltraHighPixelMode writes that integer into both capture and preview flows as:

`Key.SMART_SCENE_MODE`

`ServiceHostCaptureFlowImpl.setParameterInternal(...)` intercepts `SMART_SCENE_MODE` and calls:

`CameraService.setSceneMode(int)`

The processor then receives that scene mode and `UltraHighPixelModeProcessor.getJsonFileName(...)` selects the corresponding pipeline.

Therefore the observed architecture is:

`HAL/result hintUserValue -> UltraHighPixelMode SMART_SCENE_MODE -> CameraService.setSceneMode -> UltraHighPixelModeProcessor sceneMode -> pipeline JSON selection`

This is the critical namespace correction:

- UI/vendor request `cameraSceneMode=53` is the UltraHighPixel/200M scene identifier.
- ServiceHost processor codes `23/24/32/33` are a separate downstream internal scene namespace.
- They must not be written as though they were `cameraSceneMode` values.

## 6. Minimal OEM-derived runtime vector for v0.72

The smallest defensible direct-Camera2 vector to test now is:

1. physical Camera 5 RAW10 MAX envelope;
2. `cameraSceneMode = intArrayOf(53)`;
3. `qcomRemosaicEnable = 1` as scalar Integer;
4. read physical and logical capture-result `hintUserValue`;
5. preserve the exact same manual acquisition lock as v0.71;
6. compare against matched controls.

v0.72 uses four isolated vectors:

- CONTROL
- `qcomRemosaicEnable=1`
- `cameraSceneMode=53`
- `cameraSceneMode=53 + qcomRemosaicEnable=1`

The combination is the first test that joins two independently justified OEM layers instead of brute-forcing unrelated values.

## 7. Success criteria

A meaningful positive result would be one or more of:

- `hintUserValue` moves into 23/24/32/33 only under the OEM-derived vector;
- app-visible RAW10 topology changes;
- selected result metadata changes reproducibly;
- a new populated region appears beyond the established 15,728,640-byte prefix.

A mere change in per-frame SHA-256 is not enough.

## Boundary

Even a positive result would establish only an app-visible effect in this exact route. It would not by itself prove untouched native sensor geometry, Direct-CFA 200MP, ADC bit depth or calibration truth.

Permanent law:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
