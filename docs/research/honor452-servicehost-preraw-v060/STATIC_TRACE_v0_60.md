# TruthRaw HONOR .452 ServiceHost high-pixel / pre-RAW static trace v0.60

Date: 2026-09-23

Status: **STATIC APK RESEARCH / NO PRODUCTION CODE CHANGE**

Source APK supplied by the user:

- file: `Camera.apk`
- bytes: 84,167,938
- SHA-256: `3985d9ce23ca4e47cdd34231723b8006f033753c6a3f611685c5c8fffd457d52`
- version: `171.0.10.452`

This source is treated as static software-route evidence only.

## Why this trace exists

v0.59 closed the tested public Camera2 high-resolution RAW10 route as a route to a populated 50MP/200MP sample-domain. The next useful question is therefore not another public Camera2 envelope test, but whether HONOR's own ServiceHost/high-pixel route exposes a distinct pre-render/pre-JPEG surface or buffer.

## Exact high-pixel processor

Class:

`com.hihonor.camera2.impl.cameraservice.processor.UltraHighPixelModeProcessor`

Important methods:

- `capture(...)`
- `getJsonFileName(int sceneMode)`
- `setSceneMode(int)`
- `setYuvCaptureCount(int)`
- `isSingleFrameCapture()`

The capture path initializes/uses ServiceHost sessions and calls:

`ServiceHostSession.capture(...)`

rather than being only a normal direct Camera2 request path.

## Exact scene -> pipeline mapping

`UltraHighPixelModeProcessor.getJsonFileName(sceneMode)` statically maps:

- scene 2 -> `pipeline4mfdncap.json`
- scene 5 -> `pipeline4capbackremosaic.json`
- scene 11 -> `pipeline4hdrcap.json`
- scene 22 -> `pipeline4arcmfnrmscap.json`
- scenes 23, 24 -> `pipeline4rawmfultrahighpixelcap.json`
- scene 31 -> `pipeline4hdrcap.json`
- scenes 32, 33 -> `pipeline4rawmfultrahighpixelcap.json`
- other/default -> `pipeline4capdavinci.json`

This confirms the previously inferred scene-5 remosaic route directly from method bytecode.

## Vendor metadata bridge

The APK defines:

- CaptureResult key `com.hihonor.capture.metadata.hintUserValue`;
- CaptureRequest key `com.hihonor.capture.metadata.hintUserValue`;
- CaptureRequest key `com.hihonor.capture.metadata.smartscene_mode`.

`NormalServiceHostSceneHelper.setHintValue(CaptureResult)` reads the result-side hint-user value and forwards scene/hint state into ServiceHost capture/preview parameter flows.

This is a concrete bridge between HAL/result-side scene hints and request-side ServiceHost scene selection.

## QCOM remosaic request

The APK defines the CaptureRequest key:

`com.hihonor.capture.metadata.qcomRemosaicEnable`

as field `s8/c.A2`.

`PhotoResolutionFunction` owns an integer `isRemosaicEnable`.

During attach, the back-remosaic/high-pixel support path can set:

`isRemosaicEnable = 1`.

Its pre-capture handler writes that value to the capture flow through both the Qualcomm remosaic key and the corresponding MediaTek remosaic key.

This is stronger than a string-only observation: the key is actually used in a capture-flow `setParameter` path.

## ServiceHost capture surfaces

The APK contains explicit ServiceHost surface types and names:

- `SURFACE_FOR_PREVIEW` -> `service_host_preview`
- `SURFACE_FOR_CAPTURE` -> `service_host_capture`
- `SURFACE_FOR_CAPTURE_RAW` -> `service_host_capture_raw`
- `SURFACE_FOR_CAPTURE_SLAVE` -> `service_host_capture_slave`
- `SURFACE_FOR_METADATA` -> `service_host_metadata`
- `SURFACE_FOR_VIDEO` -> `service_host_video`

`AbstractProcessor.convertShSurfaceToSurfaceWrap(...)` receives `SHSurface` objects from ServiceHost, classifies them by SurfaceType and stores dedicated holders including:

`rawCaptureHolder`

for `service_host_capture_raw`.

This is the most important new static finding in v0.60 so far.

## Raw-surface selection path

The APK defines CaptureRequest metadata:

- `captureWithPreviewBuffer`
- `captureWithSlaveBuffer`
- `captureFormat`

`NormalProcessor.setRawFormat(builder, ServiceHostMetadata)`:

1. reads `captureFormat`;
2. checks for the raw-format condition;
3. removes the normal `captureHolder` surface from the request;
4. adds `rawCaptureHolder.surface` instead.

The method therefore proves that HONOR's ServiceHost architecture contains a distinct RAW capture target surface, not merely a JPEG filename/presentation concept.

Static evidence alone does not yet prove:
- its pixel format;
- its dimensions in the 200M path;
- whether it is pre-remosaic or post-remosaic;
- whether third-party code can access/own it;
- whether it is the same content as public Camera2 RAW;
- whether it can be sealed without modifying privileged/package boundaries.

## Immediate next static trace

The next exact task is:

1. trace creation and configuration of `service_host_capture_raw` / `rawCaptureHolder`;
2. recover its width, height, format and producer/consumer ownership where visible;
3. determine which ServiceHost metadata value selects `captureFormat`;
4. determine whether `UltraHighPixelModeProcessor` scene 5 / 23 / 24 / 32 / 33 can ever route into that raw surface;
5. identify callbacks or storage paths that consume the raw surface before JPEG/post-processing.

Do not attempt package spoofing, signature bypass or privileged Binder access.

## Authority boundary

Permanent law remains:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

Any ServiceHost RAW/high-pixel surface must first be independently identified and sealed before it can be considered for TruthRaw evidence admission.
