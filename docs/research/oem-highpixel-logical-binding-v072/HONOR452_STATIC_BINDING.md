# HONOR .452 high-pixel logical-binding static findings for v0.72

Date: 2026-09-23

Exact static source:
`/mnt/data/Camera.apk`

Identity:
- package `com.hihonor.camera`
- 84,167,938 bytes
- SHA-256 `3985d9ce23ca4e47cdd34231723b8006f033753c6a3f611685c5c8fffd457d52`

## qcomRemosaicEnable exact key/type

Exact vendor key string:

`com.hihonor.capture.metadata.qcomRemosaicEnable`

In `Ls8/c;<clinit>` it is constructed through the HONOR CaptureRequest-key factory using `java.lang.Integer.TYPE`, then stored in:

`Ls8/c;->A2:Landroid/hardware/camera2/CaptureRequest$Key;`

Therefore the OEM Java value representation is scalar `Integer`, not `int[]`.

## OEM consumer

The only runtime read of `Ls8/c;->A2` found across the exact APK DEX set is in:

`com.hihonor.camera2.function.resolution.uiservice.f.handle(...)`

That pre-capture handler obtains the mode's `CaptureFlow` and performs:

`CaptureFlow.setParameter(qcomRemosaicEnable, Integer.valueOf(isRemosaicEnable))`

On Qualcomm the qcom key is used; the sibling MTK path writes the MTK remosaic-enable key.

This is important for v0.72: HONOR writes this parameter through the mode CaptureFlow without an explicit physical-camera-ID binding at the callsite. The closest direct-Camera2 analogue is therefore a **logical request value**, not only the physical-key write used by the earlier physical-5 experiments.

## isRemosaicEnable state

`PhotoResolutionFunction.isRemosaicEnable` is initialized to 1.

For `UltraHighPixelMode`, `isBackRemosaicSupported()` requires:
- mode name = `com.hihonor.camera2.mode.ultrahighpixel.UltraHighPixelMode`
- `CameraUtil.isSensorRemosaicSupported(characteristics)` = true

When back remosaic is supported and `CameraUtil.isHighPixelAlgoSupported()` is true, HONOR explicitly assigns:

`isRemosaicEnable = 1`

and attaches the pre-capture remosaic handler to the CaptureFlow.

Thus `qcomRemosaicEnable = 1` is a statically defensible OEM value for the UltraHighPixel path.

## Runtime feedback used by HONOR

The remosaic-status capture callback reads result key:

`com.hihonor.capture.metadata.hintUserValue`

If the returned integer equals 5, it stores:

`isRemosaicEnable = 1`

otherwise it stores 0.

v0.72 therefore records `hintUserValue` from both logical and physical capture results when advertised.

## ServiceHost processor scene mapping

`UltraHighPixelModeProcessor.getJsonFileName(int sceneMode)` maps processor-internal scene values:

- 23 -> `pipeline4rawmfultrahighpixelcap.json`
- 24 -> `pipeline4rawmfultrahighpixelcap.json`
- 32 -> `pipeline4rawmfultrahighpixelcap.json`
- 33 -> `pipeline4rawmfultrahighpixelcap.json`

Other observed mappings include:
- 5 -> `pipeline4capbackremosaic.json`
- 11 -> `pipeline4hdrcap.json`
- 22 -> `pipeline4arcmfnrmscap.json`
- 31 -> `pipeline4hdrcap.json`

The processor's `capture(...)` passes that JSON into HONOR `ServiceHostSession.capture(...)`.

This proves that the stock high-pixel processor has an OEM ServiceHost route distinct from simply creating a public Camera2 RAW10 envelope.

It does **not** prove that third-party Camera2 can select those processor-internal scene values.

## Why v0.72 changes the binding experiment

v0.71 correctly tested OEM-backed values on the **physical Camera-5 key namespace**, with matched acquisition state.

But the exact HONOR consumer for `qcomRemosaicEnable` writes through the mode CaptureFlow, and the stock `cameraSceneMode` path likewise uses the mode CaptureFlow abstraction.

Therefore v0.72 tests the next missing binding dimension:
- logical-only request binding;
- logical + physical mirrored binding;
- the minimal OEM-derived combination `cameraSceneMode=53` + `qcomRemosaicEnable=1`.

Each still uses a matched single-frame physical-5 RAW10/MAX control.

## Boundary

These static findings justify which values/binding patterns deserve runtime testing.

They do not establish:
- HAL effect;
- access to HONOR ServiceHost from TruthRaw;
- native sensor geometry;
- Direct-CFA 200MP;
- ADC bit depth;
- calibration truth.
