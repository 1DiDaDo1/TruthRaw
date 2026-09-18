# HONOR v0.42 passive MediaStore + CameraManager — three-run fresh-start device result

Date: 2026-09-18

## Scope

Three user-controlled runs were performed after clearing Honor Camera app data/cache between runs:

A. fresh launch, no capture;
B. fresh launch, one ordinary photo;
C. fresh launch, manual switch to 200 MP, one photo.

TruthRaw v0.42 remained passive in all three runs:
- no camera open;
- no capture submission;
- no image-byte read;
- no EXIF decode;
- no media input stream;
- no vendor request;
- no Honor Binder method;
- no Honor callback registration.

Authority remains:

`PASSIVE_SYSTEM_VISIBLE_OUTPUT_OBSERVATION_ONLY`

## Source JSON identities

A — fresh no capture
- file: `v042_A_fresh_no_capture (1).json`
- bytes: 19,275
- SHA-256: `a6190137311a4e89c4b195c581d1f7eaaf236e73845f08a406d62621369fc0df`

B — fresh normal photo
- file: `v042_B_fresh_normal_photo.json`
- bytes: 25,938
- SHA-256: `7ab39d8a0dfaf2cde71e463f24325dca08393e19ae48bd5b0de696ebdc16c180`

C — fresh 200 MP photo
- file: `v042_C_fresh_200mp_photo.json`
- bytes: 26,939
- SHA-256: `2d97afdbc645707179df95354a54652897cfb293d2d85e0557025ad02b9f2be0`

## A — fresh launch, no capture

Between the user marker before opening Honor Camera and the return marker, the observer recorded camera-access-priority changes only.

No MediaStore Images change event was present in the run.

Bounded result:

`FRESH_HONOR_LAUNCH_WITHOUT_CAPTURE__NO_MEDIASTORE_IMAGE_INSERT_OBSERVED`

This is the fresh-start control for B/C.

## B — fresh ordinary photo

One new MediaStore Images item was observed:

- row ID: 114331
- file: `IMG_20260918_215448.jpg`
- MIME: `image/jpeg`
- width: 3072
- height: 4096
- pixel count: 12,582,912
- byte size: 3,578,531
- relative path: `DCIM/Camera/`
- owner package: `com.hihonor.camera`
- is_pending: 0 at every observed metadata snapshot
- dateAddedAfterObserverStart: true

Three MediaStore change notifications were emitted for the same row. The row was already final
(`is_pending=0`) at the first metadata query v0.42 was able to perform.

No Camera-0 unavailable/available pulse was present in this run.

Bounded result:

`FRESH_NORMAL_CAPTURE__HONOR_JPEG_3072x4096__MEDIASTORE_OUTPUT_DIFFERENTIAL_FROM_NO_CAPTURE_CONTROL`

## C — fresh 200 MP photo

One new MediaStore Images item was observed:

- row ID: 114333
- file: `IMG_20260918_215836.jpg`
- MIME: `image/jpeg`
- width: 16320
- height: 12288
- pixel count: 200,540,160
- byte size: 26,450,019
- relative path: `DCIM/Camera/`
- owner package: `com.hihonor.camera`
- is_pending: 0 at every observed metadata snapshot
- dateAddedAfterObserverStart: true

Three MediaStore change notifications were emitted for the same row.

A short Camera-0 availability pulse occurred in the same event cluster:

- `CAMERA_UNAVAILABLE cameraId=0`
  at 2026-09-18T19:58:51.643529Z
- `CAMERA_AVAILABLE cameraId=0`
  at 2026-09-18T19:58:51.651138Z
- observed interval: approximately 7.609 ms

This pulse was not present in the fresh no-capture control or the single fresh ordinary-photo run.

Bounded result:

`FRESH_USER_SELECTED_200MP_CAPTURE__HONOR_JPEG_16320x12288__SHORT_CAMERA0_AVAILABILITY_PULSE_OBSERVED`

The availability pulse is a candidate 200-MP route differential, not yet a causal or semantic claim.
One run is insufficient to say that 200 MP always causes it or to identify the internal mechanism.

## B vs C output differential

Ordinary JPEG:
`3072 x 4096 = 12,582,912 pixels`

200 MP JPEG:
`16320 x 12288 = 200,540,160 pixels`

Pixel-count ratio C/B:
`15.9375x`

File-size ratio C/B:
approximately `7.3913x`

The 200 MP system-visible JPEG geometry is therefore a real, measured output differential.

The 16320x12288 JPEG dimensions also equal the app-visible high-resolution RAW envelope used in the
separate Camera-5 acquisition experiments. This is a cross-layer dimensional equality only; it does
not prove that the JPEG and RAW envelope share the same populated sensor sample domain or internal
pipeline.

## Timing observation

Using Honor's MediaStore `DATE_TAKEN` as an application-provided timestamp field:

- B first MediaStore change occurred approximately 13.54 s after the row's DATE_TAKEN value;
- C first MediaStore change occurred approximately 14.65 s after the row's DATE_TAKEN value.

These are publication-latency observations conditioned on the meaning/accuracy of Honor's DATE_TAKEN
field. They are not independent shutter-timestamp proof.

## Scientific boundary

The result proves only what Android exposed outside the Honor camera pipeline:

- a normal fresh capture published a 3072x4096 JPEG;
- a user-selected 200 MP fresh capture published a 16320x12288 JPEG;
- both were owned by `com.hihonor.camera`;
- C additionally showed a short Camera-0 availability pulse.

It does NOT prove:

- native ADC geometry;
- RAW populated sample geometry;
- remosaic behavior;
- optical resolving power;
- active physical camera ID during the 200 MP exposure;
- internal Honor pipeline identity;
- that Camera 5 generated every 16320x12288 JPEG sample independently;
- that the Camera-0 pulse is caused by 200 MP mode.

## Next decision

The 16320x12288 output differential is already established.

The next experiment should focus narrowly on reproducibility of the short Camera-0 pulse and tighter
user-action timing:

- add explicit manual markers for MAIN_STABLE, MODE_200MP_SELECTED and SHUTTER_PRESSED;
- repeat fresh ordinary-photo and fresh 200-MP runs at least three times each;
- preserve MediaStore metadata-only operation;
- schedule metadata snapshots at short delays after each changed URI to detect transient `IS_PENDING`
  or row-state transitions that v0.42 may have missed;
- no image-byte read, no EXIF, no Honor Binder callback, no camera ownership by TruthRaw.

Do not return to Honor's package/signature-gated callback interface.
