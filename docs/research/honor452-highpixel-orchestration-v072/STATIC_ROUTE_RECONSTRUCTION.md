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


## New exact bridge: hintUserValue -> SMART_SCENE_MODE -> ServiceHost processor

Further cross-DEX decoding closes an important missing link.

The CaptureResult key field:

`Ls8/d;->k`

is constructed from the literal:

`com.hihonor.capture.metadata.hintUserValue`

using the Integer-class key factory. Therefore the OEM Java value type is scalar `Integer`.

In:

`UltraHighPixelMode$2.onCaptureCompleted(...)`

the stock app performs this sequence:

1. read `hintUserValue` from the CaptureResult;
2. convert it to an integer;
3. compare it against `UltraHighPixelMode.sceneMode`;
4. when changed, store the new integer as `sceneMode`;
5. when `lastSceneMode != sceneMode`, write `Key.SMART_SCENE_MODE = Integer.valueOf(sceneMode)` to both:
   - the mode CaptureFlow;
   - the mode PreviewFlow;
6. then update `lastSceneMode = sceneMode`.

In:

`ServiceHostCaptureFlowImpl.setParameterInternal(...)`

`Key.SMART_SCENE_MODE` is recognized as an internal app key and forwarded to:

`CameraService.setSceneMode(int)`.

The ServiceHost processor chain eventually reaches:

`UltraHighPixelModeProcessor.setSceneMode(int)`

and `getJsonFileName()` maps:

- 23 -> `pipeline4rawmfultrahighpixelcap.json`
- 24 -> `pipeline4rawmfultrahighpixelcap.json`
- 32 -> `pipeline4rawmfultrahighpixelcap.json`
- 33 -> `pipeline4rawmfultrahighpixelcap.json`

This establishes an exact orchestration bridge:

`CaptureResult hintUserValue -> UltraHighPixelMode.sceneMode -> SMART_SCENE_MODE -> CameraService.setSceneMode -> UltraHighPixelModeProcessor -> ServiceHost JSON pipeline`

The still-unresolved step is what exact request/state combination causes the HAL/result side to emit hint values 23/24/32/33.

## qcomRemosaicEnable Java type and state semantics

The static initializer for:

`com.hihonor.capture.metadata.qcomRemosaicEnable`

uses the `Integer.TYPE` key factory argument.

Thus the stock OEM Java representation is scalar `Integer`, not `int[]`.

The Qualcomm pre-capture handler writes:

`qcomRemosaicEnable = Integer.valueOf(PhotoResolutionFunction.isRemosaicEnable)`.

`PhotoResolutionFunction` initializes `isRemosaicEnable = 1`.

Its remosaic-status callback reads the scalar Integer result `hintUserValue` and updates the remosaic state as:

- `hintUserValue == 5` -> `isRemosaicEnable = 1`
- otherwise -> `isRemosaicEnable = 0`

Therefore blindly combining `qcomRemosaicEnable=1` with the RAW-MF UltraHighPixel internal modes 23/24/32/33 would not be a faithful reconstruction of the observed state machine. The remosaic state is downstream of the previous result hint in this path.

## Revised next runtime gate

The next experiment should not brute-force more output keys.

It should directly measure the missing bridge:

- matched locked control;
- `cameraSceneMode=53` (UltraHighPixel/200M);
- `cameraSceneMode=110` (UltraResolution/50M);
- `cameraSceneMode=66` (Pro Photo RAW reference);
- `qcomRemosaicEnable=1` as an isolated stock-backed remosaic control.

For every frame, read the physical Camera-5 scalar result:

`com.hihonor.capture.metadata.hintUserValue`.

The decisive question is whether any defensible request candidate causes `hintUserValue` to become 23, 24, 32 or 33.

If so, direct Camera2 has reached the OEM processor-scene trigger while still bypassing the ServiceHost processing leg. If not, additional stock-mode orchestration is required upstream of the hint generation.


## Newly resolved: 23/24/32/33 are HAL result-driven, not app-selected constants

The exact key behind the UltraHighPixel preview callback is:

`com.hihonor.capture.metadata.hintUserValue`

Static field:

`Ls8/d;->k:Landroid/hardware/camera2/CaptureResult$Key;`

`UltraHighPixelMode$2.onCaptureCompleted(...)` reads this CaptureResult value. When it changes, the app:
1. stores the returned integer as the mode's current sceneMode;
2. writes that exact integer to the internal `Key.SMART_SCENE_MODE` on the capture flow;
3. writes the same value to the preview flow.

`ServiceHostCaptureFlowImpl.setParameterInternal(...)` intercepts `Key.SMART_SCENE_MODE` and calls:

`CameraService.setSceneMode(int)`

`ServiceHostProcessor.setSceneMode(int)` delegates to the active Processor, and `UltraHighPixelModeProcessor.setSceneMode(int)` stores the integer consumed by `getJsonFileName(sceneMode)`.

Therefore the values 23/24/32/33 are not selected in the Java app by a direct constant assignment in UltraHighPixelMode. They arrive from the camera/HAL as `hintUserValue` and are then mirrored into the ServiceHost processor.

This resolves the earlier question about the exact app-side selector:

**the app-side selector is the returned `hintUserValue`; the upstream condition that causes the HAL to emit 23/24/32/33 is not encoded as a direct Java constant branch.**

## Exact UltraHighPixel mode-name request path

`AbstractPhotoMode.active()` calls:

`CaptureMode.setModeNameFlag(mode, configurationName)`

which calls:

`CameraSceneModeUtil.writeModeName(...)`

For `UltraHighPixelMode`, `getHighPixelSceneMode(...)` returns:
- 87 when Live Photo is open;
- 110 for `UltraResolutionMode`;
- 53 for ordinary `UltraHighPixelMode`.

`writeModeName(...)` writes `com.hihonor.capture.metadata.cameraSceneMode` to both capture and preview flows, then triggers the preview flow once.

Thus scene 53 is the directly app-authored UltraHighPixel mode request, while 23/24/32/33 are later HAL-returned `hintUserValue` states used to select the ServiceHost capture pipeline.

## Exact remosaic feedback loop for UltraHighPixel

`PhotoResolutionFunction.isBackRemosaicSupported()` is true only when:
- mode name is `UltraHighPixelMode`; and
- `CameraUtil.isSensorRemosaicSupported(characteristics)` is true.

On that path, PhotoResolutionFunction registers:
- a preview capture callback that updates remosaic state;
- a pre-capture handler that writes the remosaic parameter.

The callback reads the same `hintUserValue` result:
- if hintUserValue is present and not 5 -> `isRemosaicEnable = 1`;
- if the result is null or value 5 -> `isRemosaicEnable = 0`.

Immediately before capture, on Qualcomm, the handler writes:

`com.hihonor.capture.metadata.qcomRemosaicEnable = isRemosaicEnable`

through the mode CaptureFlow.

Therefore, for the raw-MF UltraHighPixel processor values 23/24/32/33, the stock Java orchestration leads to remosaic enable 1.

This shows that scene 53 and qcomRemosaicEnable are not independent blind switches: scene 53 establishes the mode; HAL `hintUserValue` feeds the processor scene; the same hint feeds the remosaic state; the pre-capture handler writes that derived state.

## Processor factory binding

`ProcessorFactory.<clinit>()` maps the exact mode string:

`com.hihonor.camera2.mode.ultrahighpixel.UltraHighPixelMode`

to:

`UltraHighPixelModeProcessor.class`

So the UltraHighPixel UI/mode path and the previously decoded processor pipeline are statically joined by the processor factory, not merely by similar naming.

## Raw-surface boundary refinement

A separate static ServiceHost path in `NormalProcessor.setRawFormat(...)` reads:

`com.hihonor.capture.metadata.captureFormat`

and, when the metadata byte is present and is not 32, removes the normal capture target and adds `rawCaptureHolder`.

This proves a metadata-controlled raw-surface swap for NormalProcessor.

It must **not** yet be generalized to UltraHighPixelModeProcessor, which directly extends AbstractProcessor and has its own ServiceHost capture path. The exact UltraHighPixel raw-surface binding remains a separate unresolved gate.

## Next runtime oracle

The next runtime probe should not inject 23/24/32/33 as if they were request controls.

Instead it should:
1. submit the defensible OEM mode request `cameraSceneMode=53`;
2. observe logical and physical-5 `hintUserValue` on a single preview/priming request;
3. derive the stock remosaic state from that returned hint (hint != 5 -> 1);
4. apply qcomRemosaicEnable only with a locally proven Camera2 representation;
5. perform one isolated physical-5 RAW10/MAX capture;
6. record the returned hintUserValue, result metadata and exact payload topology.

If direct Camera2 never produces a raw-MF hint (23/24/32/33), that is evidence that additional OEM mode/session/ServiceHost orchestration is still missing; it is not evidence that the stock route does not exist.
