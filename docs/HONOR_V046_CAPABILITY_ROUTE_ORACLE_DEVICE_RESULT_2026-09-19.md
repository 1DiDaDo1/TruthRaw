# Honor v0.46 capability-route oracle — Android 16 device result

Date: 2026-09-19

## Result

The v0.46 read-only oracle was executed on the HONOR BKQ-N49 **before the Android 17 update**.

Source file:

- `TRUTHRAW_HONOR_CAPABILITY_ROUTE_ORACLE_v046.json`
- bytes: `7790`
- SHA-256: `6c4bf18ca80477ae95e17d4d14f959f21f8cc9aca77f51df44d7a5d178775430`

Runtime identity is unambiguous:

- Android release: **16**
- SDK: **36**
- build fingerprint: `HONOR/BKQ-N49/HNBKQ:16/HONORBKQ-N49/10.0.0.199C636E4R106P1:user/release-keys`

The observer did not open a camera, submit a request, read an image buffer, write a vendor request key or call the Honor Binder service.

## Standard Camera2 identity remains stable

Public IDs:

- logical rear: `0`
- front: `1`

Logical rear `0` discloses physical IDs:

- `2` — 6.55 mm / f1.6 main
- `4` — 1.82 mm / f2.0 ultra-wide
- `5` — **22.48 mm / f2.6 tele**

Camera 5 is therefore still independently identifiable through standard Camera2 characteristics, without using APK semantics.

## Targeted Honor capability result

v0.46 looked only for these five names selected by static analysis of the exact Honor Camera beta APK:

1. `com.hihonor.device.capabilities.physicalCameraScene`
2. `com.hihonor.device.capabilities.sceneCameraIdCapability`
3. `com.hihonor.device.capabilities.cameraIdCustomInfo`
4. `com.hihonor.device.capabilities.needOpenPhysicalCamera`
5. `com.hihonor.device.capabilities.ultraResolutionSwitchSupportedSize`

Result on **every checked ID 0, 1, 2, 4 and 5**:

**present = false**

No runtime value was returned because none of the five names appeared in the CameraCharacteristics key set visible to the TruthRaw package.

## What this closes

v0.46 falsifies the specific hypothesis:

> the five static-APK capability names can be read directly as ordinary third-party Camera2 CameraCharacteristics on the current Android-16 runtime.

That route is closed for this runtime/package context.

This is useful negative evidence. It means the next step must not brute-force these names as if they were ordinary exposed CameraCharacteristics keys.

## What this does NOT close

The result does **not** establish that the capabilities are absent inside Honor's OEM camera stack.

Static analysis still shows the exact Honor Camera APK referencing these names and using them in high-pixel routing logic. The runtime result instead shows that TruthRaw cannot see them through its normal Camera2 characteristics surface on Android 16.

A plausible explanation is that Honor resolves them through an internal/privileged CameraAbility layer or another capability provider, but v0.46 does not prove which mechanism.

No claim is promoted to:

- OEM active physical camera ID;
- OEM capture/session evidence;
- Direct-CFA 200MP;
- calibration authority.

## Clean Android 16 → Android 17 experiment

The user reported that an Android 17 update is now available for the same phone.

The first post-update experiment must therefore be **the exact same already-built v0.46 APK, unchanged**.

That creates a controlled OS/HAL delta:

`same hardware + same TruthRaw APK + same five queried names + new OS/HAL`

If any target key becomes visible after the update, that is a direct runtime-exposure change. If they remain absent, the negative result survives the OS transition.

Only after that unchanged rerun should a new API-37-specific probe inventory Android 17 `RAW14` and the new extension-query surface.
