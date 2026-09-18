# HONOR v0.43 device result — PHOTO / PRO / HI-RES main / user-selected tele-200MP

Date: 2026-09-18

## Authority

This result remains:

`PASSIVE_SYSTEM_VISIBLE_OUTPUT_OBSERVATION_ONLY`

TruthRaw did not open a camera, submit a capture, read image pixels, open media bytes, decode EXIF,
invoke Honor Binder methods, register Honor callbacks, or write vendor request/session keys.

The v0.43 markers are user timing/state markers, not hardware timestamps.

## Source files

- `pro mode.json`
  - bytes: 94480
  - SHA-256: `bd6f01e4da87f78f133c55faf627af1c7a84d1cabe43fd5de8e650a57d14`
- `hires.json`
  - bytes: 97139
  - SHA-256: `797158683400007d729beebccacc115631e0a5c76d7c83fb27c01f09a7cab1be`

## Human clarification that changes interpretation

The first PHOTO runs did not contain a real mode change. Honor Camera was opened in its normal main/PHOTO
state and the photo was taken there.

In the HI-RES run, the user reports this sequence:

1. enter HI-RES;
2. HI-RES initially lands on the main camera;
3. first HI-RES photo is taken in that main-camera state;
4. user manually switches inside HI-RES to the tele / UI-described 200MP state;
5. second photo is taken there.

This is user-reported operational context. It is not promoted to a CameraManager-proven physical camera ID.
In particular, this file does not prove active physical ID 5 for the second state.

## PRO result

A real transition from main/PHOTO to PRO was user-marked.

Two separate Honor JPEGs were observed because the first photo was taken before the user could reach the
notification marker and a second photo was then taken for the marked attempt.

Both PRO outputs were:

- 3072 × 4096;
- JPEG;
- `DCIM/Camera/`;
- `owner_package_name=com.hihonor.camera`.

No `CAMERA_UNAVAILABLE` event was observed in the PRO run.

## HI-RES main result

After the first user marker for HI-RES, Camera 0 became unavailable.

The first HI-RES output was:

- row ID 114351;
- `IMG_20260918_225648.jpg`;
- 8192 × 6144;
- 50,331,648 system-visible output pixels;
- 7,553,801 bytes;
- JPEG;
- `DCIM/Camera/`;
- owner `com.hihonor.camera`.

Per the user's operational account, this corresponds to HI-RES while still on the main-camera state.

This is output-geometry evidence only. It does not establish native ADC sample count, optical resolution,
or sensor-native geometry.

## HI-RES user-selected tele / 200MP state

After Camera 0 became available again, the user marked HI-RES again while manually switching to the
tele / UI-described 200MP state. Camera 0 then became unavailable again.

The second row was ID 114352, `IMG_20260918_225721.jpg`.

v0.43 captured a same-row metadata transition:

### Early state

- 6144 × 8192;
- 50,331,648 system-visible pixels;
- `_size=null`;
- `is_pending=0`;
- owner `com.hihonor.camera`.

The early 50MP-like geometry was still present through the delayed snapshots before later MediaStore
change notifications.

### Final state

The same MediaStore row ID 114352 later became:

- 16320 × 12288;
- 200,540,160 system-visible output pixels;
- 27,132,295 bytes;
- JPEG;
- owner `com.hihonor.camera`.

This final geometry matches the earlier v0.42 16320 × 12288 Honor output class.

## What is now supported

The current Honor Camera build exposes a UI path called HI-RES whose output behavior differs from
PHOTO and PRO.

Within the user-described HI-RES sequence:

- HI-RES on main produced an 8192 × 6144 JPEG;
- after the user's manual switch to the tele / UI-described 200MP state, a subsequent output row
  transitioned from 6144 × 8192 metadata to a final 16320 × 12288 JPEG.

The Camera-0 availability boundary also differed from PHOTO/PRO and recurred around the HI-RES state
changes in this run.

## What is NOT supported

This result does not prove:

- native 200MP ADC sampling;
- 200,540,160 independently measured RAW/CFA samples;
- active physical camera ID 5 from CameraManager telemetry;
- remosaic implementation details;
- optical resolving power;
- that 6144 × 8192 is a sensor-native intermediate;
- that Camera-0 availability semantics identify a specific physical sensor.

The 16320 × 12288 JPEG result does not alter the separate RAW-topology result: the prior 16320 × 12288
RAW_SENSOR envelope contained an exact 4080 × 3072 × 2 populated prefix candidate rather than a proven
fully populated 200MP RAW raster.

## v0.44 design consequence

v0.44 must stop overloading `MODE_HIRES_SELECTED` for two different user states.

For the HI-RES run profile, notification actions become explicit:

- `HIRES_MAIN_CONFIRMED`
- `TELE_200MP_UI_SELECTED`
- `SHUTTER_PRESSED`

The word `UI` is deliberate: the marker records the user's visible/manual state only.

v0.44 also classifies system-visible MediaStore rows as screenshot UI-anchor candidates,
Honor-camera output candidates, or other images without opening their bytes, and records same-row
metadata state transitions directly.
