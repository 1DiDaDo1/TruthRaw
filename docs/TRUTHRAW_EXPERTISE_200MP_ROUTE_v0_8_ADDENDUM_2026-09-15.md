# TruthRaw Expertise Addendum — 200MP v0.8 logical-to-physical route correction

**Date:** 2026-09-15  
**Status:** MANDATORY ADDENDUM TO `TRUTHRAW_EXPERTISE_FOUNDATION_FREE_SPACE_200MP_2026-09-15.md`

## Why this addendum exists

The broad 200MP scientific method in the expertise foundation is correct, but the repository's later FotoGraaf v0.8 work contains one important implementation correction over the standalone v0.7 200MP probe.

A future chat must read this addendum before modifying or running Step 3B.

## v0.7 route limitation

The standalone v0.7 probe reconstructs records for physical Camera 5 while walking logical-camera characteristics, but its candidate construction can discard the `parent_logical_id`. That can make the capture path attempt `openCamera("5")` even when Android exposes Camera 5 only as a physical child of logical Camera 0.

The useful v0.7 work remains valid:

- exact 16320x12288 high-resolution RAW_SENSOR selection;
- bounded `ImageReader(maxImages=1)`;
- `OutputConfiguration.addSensorPixelModeUsed(MAXIMUM_RESOLUTION)`;
- `CaptureRequest.SENSOR_PIXEL_MODE = MAXIMUM_RESOLUTION`;
- direct ByteBuffer hashing/persistence;
- timestamp binding;
- applied-control/result logging;
- DNG as derivative container;
- fail-closed off-device runtime gate.

But **v0.7 direct-open routing is not the current route authority**.

## v0.8 route authority

`docs/full-sensor/FOTOGRAAF_ACQUISITION_DOMAIN_v0_8.md` is the current route interpretation.

When Camera 5 is exposed as physical child of logical Camera 0, the intended evidence route is:

`logical camera 0 -> OutputConfiguration bound to physical camera 5 -> RAW_SENSOR -> matching physical CaptureResult -> exact timestamp binding`

Therefore Step 3B should:

1. inspect `CameraManager.cameraIdList` and logical Camera 0 physical IDs;
2. preserve the parent-child relationship in the candidate object;
3. if Camera 5 is a physical child, open logical Camera 0;
4. call `OutputConfiguration.setPhysicalCameraId("5")` before session creation;
5. mark the 16320x12288 RAW output for `SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION`;
6. issue the still request with `SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION`;
7. use physical request overrides only when Android explicitly advertises support for the relevant physical override keys;
8. obtain the physical Camera-5 `TotalCaptureResult` from the logical capture result;
9. require the RAW image timestamp to match the physical Camera-5 `SENSOR_TIMESTAMP` used as evidence;
10. preserve exact payload bytes/hash/stride plus the logical and physical route identity.

Direct `openCamera("5")` is a separately named route and should only be used as proof when Android actually exposes Camera 5 as independently openable on the running device.

## 200MP classification boundary remains unchanged

A successful v0.8 16320x12288 capture proves one app-visible maximum-resolution Camera2 RAW sample lattice from the proven physical route.

It does not by itself prove:

- untouched photodiode/ADC codes;
- absence of sensor/HAL processing;
- physical Quad/Tetra topology;
- electron-domain calibration;
- FULL_PHYSICAL colour;
- FULL_PHYSICAL optics.

`SENSOR_INFO_LENS_SHADING_APPLIED=true` remains an upstream-processing boundary.

## Binning clarification

Do not turn the reported `SENSOR_INFO_BINNING_FACTOR=2x2` into a physical 2x2 operator without independent evidence.

Do not require a non-null `SENSOR_RAW_BINNING_FACTOR_USED` result on this device as a capture-pass condition. It is an optional Boolean result key and Android associates it specifically with the UHR + REMOSAIC_REPROCESSING case; this Honor does not advertise remosaic reprocessing.

## Current continuation point

The correct next experiment is therefore not "run the old v0.7 app unchanged".

It is:

> **Use the v0.8 FotoGraaf acquisition-domain routing model to execute the same strict 16320x12288 maximum-resolution evidence capture, preserving logical-0 -> physical-5 identity when that is the route Android exposes.**

After that payload exists, perform mode-specific 12.5MP / 50MP / 200MP noise, shading, colour and SFR/MTF calibration before promoting cross-mode physical models.
