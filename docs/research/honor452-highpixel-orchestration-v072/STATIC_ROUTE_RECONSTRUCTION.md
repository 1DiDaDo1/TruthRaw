# HONOR .452 high-pixel orchestration — static route reconstruction v0.72

Date: 2026-09-23

Static source:
`/mnt/data/Camera.apk`

Exact identity:
- package: `com.hihonor.camera`
- size: 84,167,938 bytes
- SHA-256: `3985d9ce23ca4e47cdd34231723b8006f033753c6a3f611685c5c8fffd457d52`

Authority:
`STATIC_APK_BYTECODE_ROUTE_ANALYSIS_ONLY`

This document records newly decoded orchestration facts. It does not promote static OEM logic to runtime capture truth.

## qcomRemosaicEnable key identity

In `classes2.dex`, the literal:

`com.hihonor.capture.metadata.qcomRemosaicEnable`

is created in `Ls8/c;.<clinit>` and assigned to:

`Ls8/c;->A2:Landroid/hardware/camera2/CaptureRequest$Key;`

The immediately adjacent MediaTek remosaic key is:

`com.mediatek.control.capture.remosaicenable`

assigned to `Ls8/c;->z2`.

## Qualcomm write path

Cross-DEX analysis finds the only runtime read of `Ls8/c;->A2` in:

`com.hihonor.camera2.function.resolution.uiservice.f.handle()`

That method:
1. checks `Util.isQualcommPlatform()`;
2. obtains the current mode CaptureFlow;
3. loads `Ls8/c;->A2` (`qcomRemosaicEnable`);
4. calls `PhotoResolutionFunction.j()`;
5. wraps the returned integer with `Integer.valueOf(...)`;
6. calls `Mode.CaptureFlow.setParameter(key, value)`.

On the non-Qualcomm branch it writes the MediaTek remosaic key instead.

Therefore this is a real OEM CaptureFlow write path for `qcomRemosaicEnable`, not merely a declared vendor key.

## qcomRemosaicEnable value source

`PhotoResolutionFunction.j()` is a direct getter for field:

`PhotoResolutionFunction.isRemosaicEnable:I`

The constructor initializes:

`isRemosaicEnable = 1`

The same field can later be updated by `PhotoResolutionFunction.q(...)`.

During `attach()`, HONOR also installs:
- a remosaic status capture callback;
- a pre-capture remosaic parameter handler.

The attach path contains explicit high-pixel/remosaic capability checks including:
- `isQcomSoftRemosaicSupport()`
- `isBackRemosaicSupported()`
- `CameraUtil.isHighPixelAlgoSupported()`

and retains `isRemosaicEnable=1` on the relevant path before registering the pre-capture handler.

Thus value `1` has direct OEM support as the enabled remosaic state for this Qualcomm CaptureFlow path.

This does not yet prove that directly writing the same key through experimental Camera2 reproduces all stock-app orchestration.

## UltraHighPixel processor pipeline mapping

In `classes.dex`:

`com.hihonor.camera2.impl.cameraservice.processor.UltraHighPixelModeProcessor.getJsonFileName()`

contains two packed switches.

Decoded cases:

- internal mode 22 -> `pipeline4arcmfnrmscap.json`
- internal mode 23 -> `pipeline4rawmfultrahighpixelcap.json`
- internal mode 24 -> `pipeline4rawmfultrahighpixelcap.json`

and:

- internal mode 31 -> `pipeline4hdrcap.json`
- internal mode 32 -> `pipeline4rawmfultrahighpixelcap.json`
- internal mode 33 -> `pipeline4rawmfultrahighpixelcap.json`

Therefore the already-known internal values 23/24/32/33 are now directly tied by decoded switch targets to the RAW MF UltraHighPixel pipeline filename.

This is stronger than string co-occurrence.

## Current orchestration vector status

Now statically tied together:

- UI/mode layer:
  - UltraHighPixel scene 53
  - UltraResolution scene 110
  - Pro Photo RAW scene 66
- resolution/remosaic function:
  - Qualcomm `qcomRemosaicEnable` CaptureFlow parameter
  - enabled state sourced from `isRemosaicEnable`, initialized to 1
- processor layer:
  - internal modes 23/24/32/33 -> `pipeline4rawmfultrahighpixelcap.json`
- ServiceHost layer (previously established):
  - distinct `SURFACE_FOR_CAPTURE_RAW` / `service_host_capture_raw`
  - `NormalProcessor.setRawFormat(...)` can select that raw capture surface based on capture metadata

## Still unresolved before a defensible combined runtime probe

Do not yet combine arbitrary values.

Still required:
1. recover the exact stock-app condition that selects internal processor values 23/24/32/33;
2. determine which of those values correspond to UltraHighPixel 200M vs adjacent high-pixel variants;
3. decode whether `qcomRemosaicEnable` is written only at attach/pre-capture time or is additionally altered by the remosaic status callback for the target mode;
4. recover the exact ServiceHost/raw-surface selection condition used by the UltraHighPixel route;
5. identify any logical-camera/session parameters that stock mode setup writes alongside scene 53 + remosaic.

Only then should TruthRaw test the smallest OEM-derived combination in one isolated single-frame capture.

Permanent boundary:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**
