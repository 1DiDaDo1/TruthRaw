# HONOR 171.0.10.452 — exact high-pixel physical-camera gate trace

Date: 2026-09-23

Authority:
- route meaning: `STATIC_APK_BYTECODE_ROUTE_ANALYSIS_ONLY`
- runtime capability value: current v0.50 direct typed characteristics rerun

Frozen production remains untouched.

## Exact static chain

The supplied HONOR Camera APK resolves:

`CameraUtil.isUsePhysicalCamera(String modeName)`

as:

1. `CameraSceneModeUtil.getSceneModeEnum(modeName)`
2. pass the resulting integer scene ID to `r7/a.w(sceneId)`
3. `r7/a.w` first checks its general physical-camera prerequisite;
4. if that prerequisite is satisfied, it calls `r7/a.s(sceneId)`;
5. `r7/a.s` reads `Ls8/a;->m5`;
6. `Ls8/a;->m5` is initialized from the exact vendor characteristic name:
   `com.hihonor.device.capabilities.physicalCameraScene`;
7. `r7/a.s` linearly scans that int array and returns true only if the requested scene ID is present.

Therefore the OEM physical-camera decision is directly gated by membership in `physicalCameraScene`.

## High-pixel scene resolution

`CameraSceneModeUtil.getSceneModeEnum(modeName)` special-cases both:

- `com.hihonor.camera2.mode.ultrahighpixel.UltraHighPixelMode`
- `com.hihonor.camera2.mode.ultrahighpixel.UltraResolutionMode`

and forwards them to:

`CameraSceneModeUtil.getHighPixelSceneMode(modeName)`.

That method returns exactly:

- `87` if Live Photo is open;
- `110` for `UltraResolutionMode`;
- `53` otherwise, i.e. ordinary `UltraHighPixelMode`.

## Runtime Camera-5 capability

The current direct-typed v0.50 rerun reports:

`physicalCameraScene = [66]`

for Camera 5.

Scene 66 has separately been proven in the same APK to be the ProPhoto RAW scene.

Consequently, for the currently observed characteristics:

- ordinary UltraHighPixel scene `53` is not in `[66]`;
- Live Photo high-pixel scene `87` is not in `[66]`;
- UltraResolution scene `110` is not in `[66]`.

Thus the static `CameraUtil.isUsePhysicalCamera(...)` gate cannot return true for those high-pixel scene IDs merely from the observed Camera-5 `physicalCameraScene` capability.

## What this does and does not mean

Supported conclusion:

`HONOR_HIGH_PIXEL_SCENES_53_87_110_NOT_ADMITTED_BY_OBSERVED_PHYSICAL_CAMERA_SCENE_66_GATE`

This is useful because it separates HONOR's high-pixel mode from the ProPhoto RAW physical-camera route.

It does **not** prove that UltraHighPixel cannot produce 200MP imagery. It may operate through:
- a logical-camera OEM pipeline;
- remosaic inside the OEM pipeline;
- ServiceHost/offline processing;
- another private producer path;
- processed output rather than a physical-camera RAW surface.

It also does not prove anything about native photodiode/ADC geometry.

## cameraIdCustomInfo correction

The independently decoded `cameraIdCustomInfo` consumer in `CameraServiceFactory.createCameraAbility` is pair-oriented and filtered to route IDs `59,19,23,64,67`.

Therefore the occurrence of value `53` inside the current 310-int characteristic is not part of the physical-camera gate described here and is not a demonstrated UltraHighPixel routing record.

## Research consequence

Further attempts to force physical Camera ID 5 merely by replaying the public high-pixel scene ID are not justified by this OEM control flow.

Higher-value next routes are:
1. Android-17 vendor-defined Camera Extensions;
2. logical-camera/OEM high-pixel producer topology;
3. the existing ServiceHost/offline processor path;
4. direct comparison of processed 200MP output provenance versus public Camera2 RAW surfaces.

Permanent law:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
