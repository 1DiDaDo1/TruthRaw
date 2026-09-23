# TruthRaw v0.62 — Android 17 vendor-defined Camera Extension oracle

Date: 2026-09-23

Status: ISOLATED READ-ONLY RESEARCH BUILD

## Motivation

Android 17 / API 37 adds `CameraExtensionCharacteristics.isExtensionSupported(int)`.

Unlike `getSupportedExtensions()`, this API can query device-specific extension modes that are not required to appear in the legacy supported-extension list.

This creates a new official third-party discovery path that is distinct from TruthRaw's ordinary Camera2 RAW_SENSOR / RAW10 acquisition route.

## Experiment

For every public and disclosed physical camera ID:

1. obtain `CameraExtensionCharacteristics`;
2. preserve the legacy `getSupportedExtensions()` result;
3. reflectively resolve API-37 `isExtensionSupported(int)` because TruthRaw intentionally remains compileSdk 35;
4. scan extension IDs 0 through 255 read-only;
5. for every supported ID, record:
   - whether it is one of the public IDs 0..4;
   - extension-specific CameraCharacteristics keys and values;
   - available CaptureRequest keys;
   - available CaptureResult keys;
   - supported JPEG, YUV_420_888, JPEG_R, YCBCR_P010 and DEPTH_JPEG sizes;
   - SurfaceTexture and SurfaceView sizes;
   - postview and capture-progress availability;
   - explicit flags for 8160x6144 and 16320x12288 capture-size appearances.

## Non-actions

The oracle does **not**:
- open a CameraDevice;
- create a CameraExtensionSession;
- create or submit a CaptureRequest;
- allocate an ImageReader;
- read pixels;
- write vendor keys;
- call HONOR Binder services;
- change the Scientific Master or production route.

## Scientific boundary

A positive vendor-defined extension ID proves only that the Android-17 extension framework advertises that device-specific extension for that camera ID.

Even if an extension exposes 8160x6144 or 16320x12288 JPEG/YUV output, that does not prove:
- RAW access at that size;
- a Direct-CFA 200MP source;
- a remosaic source identity;
- native sensor/ADC geometry;
- OEM UltraHighPixel route identity;
- calibration truth.

A discovered mode may become a target for a later isolated extension-session probe, but only after its read-only characteristics have been reviewed.

Permanent law:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
