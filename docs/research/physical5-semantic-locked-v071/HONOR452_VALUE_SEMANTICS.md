# HONOR .452 vendor-value semantics used for v0.71

Date: 2026-09-23

Static source:
`/mnt/data/Camera.apk`

Exact identity:
- package: `com.hihonor.camera`
- size: 84,167,938 bytes
- SHA-256: `3985d9ce23ca4e47cdd34231723b8006f033753c6a3f611685c5c8fffd457d52`

This note records only value semantics that are directly supported by the exact HONOR Camera APK. It does not promote static OEM behavior to runtime evidence.

## MasterFilmSensorType

Vendor request key:
`com.hihonor.capture.metadata.MasterFilmSensorType`

Static field:
`Ls8/c;->J4:Landroid/hardware/camera2/CaptureRequest$Key;`

Consumers:
- `MasterVideoMode.active()`
- `MasterBokehVideoMode.active()`
- `MasterHighSpeedVideoMode.active()`

All call:
`CameraUtil.getMasterVideoSensorType()`

and pass that integer into the OEM CaptureFlow request wrapper.

The OEM value selector distinguishes camera roles:
- wide-angle camera -> 1
- OEM tele camera -> 3
- remaining/main cases -> 2

Static call-chain evidence:
- `I1/b.getWideAngleId()` delegates to `I1/c.D()`
- `Ln0/e.getTeleCameraId()` delegates to `I1/c.A()`
- `CameraUtil.getMasterVideoSensorType()` returns 1 for the wide ID and 3 for the OEM tele ID.

Therefore v0.70's direct-Camera2 `MasterFilmSensorType=intArrayOf(1)` was a valid native type but was not the OEM tele sensor-type value.

v0.65 established that direct Camera2 marshaling for this device uses `int[]`, so v0.71 encodes OEM semantic value 3 as:

`intArrayOf(3)`

This still does not prove that Camera-5 will consume that value with the same meaning in the experimental still-RAW route.

## cameraSceneMode

Vendor request key:
`com.hihonor.capture.metadata.cameraSceneMode`

Static field:
`Ls8/c;->j1:Landroid/hardware/camera2/CaptureRequest$Key;`

`CameraSceneModeUtil.writeModeName()` writes the integer returned by `getSceneModeEnum(...)` into capture and preview flows.

For high-pixel modes, `getHighPixelSceneMode(...)` returns:
- 87 when Live Photo is active
- 110 for `UltraResolutionMode`
- 53 otherwise for `UltraHighPixelMode`

Known mode mapping from the same exact APK:
- `UltraResolutionMode` = 50M route
- `UltraHighPixelMode` = 200M route

For Pro Photo:
- 65 = JPEG-L branch
- 66 = RAW branch
- fallback = 2

Therefore v0.71 uses these defensible scene values as isolated candidates:
- 53 = OEM UltraHighPixel/200M scene value
- 110 = OEM UltraResolution/50M scene value
- 66 = OEM Pro Photo RAW scene reference

Direct Camera2 representation remains `int[]` per v0.65, so values are encoded as single-element arrays.

## teleconverterEnable

Vendor request key:
`com.hihonor.capture.metadata.teleconverterEnable`

Static field:
`Ls8/c;->r1:Landroid/hardware/camera2/CaptureRequest$Key;`

The OEM TeleConverterFunction sets this parameter from:

`"on".equals(value)`

and passes a Boolean through the HONOR CaptureFlow wrapper.

Direct Camera2 marshaling on this device resolves the underlying representation as `byte[]` in v0.65.

Therefore v0.71 retains:

`byteArrayOf(1)`

as the direct-Camera2 representation of the OEM semantic TRUE/on state.

## extStreamSize

Vendor request key:
`com.hihonor.capture.metadata.extStreamSize`

Static field:
`Ls8/c;->F0:Landroid/hardware/camera2/CaptureRequest$Key;`

This is not a scalar enable/disable control.

HONOR code obtains it from `CameraUtil.filterVideoResolutionForLivePhoto(...)`, which returns an `int[]`. The static implementation builds a four-element integer tuple selected from characteristic-derived Live Photo resolution data.

Therefore v0.70's `intArrayOf(1)` was type-correct but not a semantically valid ext-stream-size tuple.

The exact four-field tuple meaning/order is not yet sufficiently decoded to justify a v0.71 capture candidate, so `extStreamSize` is deliberately excluded from the semantic-value test.

## QTI / unresolved keys

The exact HONOR APK does not expose a sufficiently direct consumer/value domain for:
- `EnableVSR`
- `ExtendedMaxZoom`
- `inSensorZoomEnable`
- `aoRunningMode`

Their v0.65 types and v0.69 session acceptance remain valid evidence, but value 1 is not promoted to OEM semantic truth merely from the key name.

They are not repeated in v0.71 until a defensible value domain is available.

## v0.71 experimental rule

v0.71 is restricted to values justified above and removes the strongest v0.70 confound by forcing the same manual acquisition state on both sides of every control/candidate comparison:
- AE off
- same requested ISO
- same exposure time
- same frame duration
- AF off
- same focus distance
- same physical Camera-5 RAW10 MAX envelope
- same physical SENSOR_PIXEL_MODE request
- one frame per side
- fresh camera open per frame
- mirrored pair order across candidates

Permanent boundary:

**Static OEM consumer semantics justify what value to test; only runtime capture evidence can establish an app-visible effect.**
