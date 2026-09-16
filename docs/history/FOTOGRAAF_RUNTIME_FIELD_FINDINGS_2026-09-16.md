# FotoGraaf runtime field findings — 2026-09-16

Status: historical/runtime evidence addendum. This does not promote a new scientific authority class.

## Device context

Field device: HONOR BKQ-N49, Android 16.

## Sequence

### v0.5 integrated Suite

- The combined Pro preview + RAW session did not provide a useful live preview during the field run.
- This was treated as an application/session-topology observation, not proof that physical camera 5 cannot preview.

### v0.6 preview-first physical-5 test

- The physical-camera-5-bound preview opened but did not provide a useful live image during the field run.
- The result did not reach a qualifying 200MP RAW capture and therefore did not change the 200MP capture gate.

### v0.7 staged logical-preview test

- The staged Activity crashed immediately, before the explicit capability step and before Camera2 access.
- v0.8 crash capture produced `TruthRaw_last_crash.txt`, 2,453 bytes, SHA-256 `33ee672716797df903917278ff11c3770c3867bfbba3e5d72796d83d7f615d45`.
- Exact root cause: `java.lang.UnsupportedOperationException: TextureView doesn't support displaying a background drawable`.
- Stack location: `FotoGraaf200MpStagedActivity.buildUi(FotoGraaf200MpStagedActivity.kt:145)` reached through `TextureView.setBackgroundColor -> View.setBackground -> TextureView.setBackgroundDrawable`.

This is an Android UI/runtime bug before Camera2. It is **not** negative evidence about:

- the advertised Camera-5 16320×12288 RAW_SENSOR capability;
- whether physical camera 5 works;
- whether a real maximum-resolution RAW frame can be delivered;
- untouched/native ADC semantics.

### v0.9 exact crash fix

- Removed `TextureView.setBackgroundColor(Color.BLACK)` from the recommended staged 200MP TextureView.
- Parent layout retains the black UI background; TextureView itself receives no background drawable.
- Added CI regression guard so the forbidden call cannot silently return in the staged route.
- Android build workflow run `35125224946`: SUCCESS.
- Built APK SHA-256: `7ed95543dddd4513ef7d5dab32eabc75ec863adb0a48883083e5b934e0da4229`.
- Build artifact ZIP SHA-256: `38d4070cbb14fe4903ad55e7886d8bab01b4ed321a20663b8dd9682622c604d5`.

## Remaining route

The next real device sequence remains strictly staged:

1. Activity/UI opens successfully.
2. Read Camera-5 capability only.
3. Start logical-camera-0 preview with 3.7× request and observe `LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID` rather than assuming tele selection.
4. Only after a live preview frame exists, create a separate physical-5 RAW_SENSOR `16320×12288` capture session and request `SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION`.
5. Admit a capture candidate only if the returned image dimensions/sample count, physical result, timestamp identity, pixel stride/row geometry and maximum-resolution pixel mode all pass.
6. Host Step-3B replay/promotion remains mandatory.

## Legacy UI audit

The same risky `TextureView.setBackgroundColor(Color.BLACK)` pattern was observed in older FotoGraaf preview activities (`FotoGraaf200MpTestActivity`, `FotoGraafProCameraActivity`, `FotoGraafSafePreviewActivity`, `FotoGraafLiveCameraActivity`). They are therefore not promoted as crash-fixed by the v0.9 result. The recommended 200MP device path is the v0.9 staged Activity only until those legacy routes are separately repaired and revalidated.

## Scientific boundary

This entire fix is UI/runtime engineering. It creates no new sensor evidence, does not alter Camera-5 static capability, does not change Scientific Master or Dynamic Authority, and does not close the physical 200MP capture gate.
