# HONOR v0.43 passive mode/shutter timeline — PHOTO / PRO / HI-RES replication plan

Date: 2026-09-18

## Why v0.43 changed from the first draft

The current Honor Camera UI does not expose a mode literally labeled "200 MP" after a fresh app reset.
The visible mode grid includes, among others:

- PHOTO
- PRO
- HI-RES

Therefore v0.43 does **not** assume that HI-RES is semantically identical to the previously user-described
200 MP mode.

The experiment is renamed around observed UI labels only.

## Profiles

Each fresh Honor app-state run uses exactly one user-declared profile:

- `FRESH_PHOTO_CONTROL_REPLICATE`
- `FRESH_PRO_CONTROL_REPLICATE`
- `FRESH_HIRES_CANDIDATE_REPLICATE`

The profile is a test label only. It does not grant camera-pipeline semantics.

## Human markers

v0.43 records:

- `MAIN_STABLE`
- mode-selected marker derived from profile:
  - `MODE_PHOTO_SELECTED`
  - `MODE_PRO_SELECTED`
  - `MODE_HIRES_SELECTED`
- `SHUTTER_PRESSED`
- return marker

Markers can be issued from notification actions while Honor Camera remains foreground.

Markers use the same `elapsedRealtimeNs` clock as CameraManager and MediaStore events, but they are
human timing markers only; they are not hardware shutter timestamps.

## Delayed MediaStore metadata schedule

After each MediaStore Images change notification, the same URI is queried at scheduled delays:

- 0 ms
- 25 ms
- 100 ms
- 250 ms
- 1000 ms

Each change gets a unique `changeGroupId`.

Each delayed query stores:

- requested URI
- query start/end monotonic timestamps
- actual delay from triggering change
- row metadata
- width/height
- byte size
- MIME type
- is_pending
- owner package
- date fields
- path/volume

No media file is opened.

## Strict invariants

TruthRaw v0.43 does not:

- open any camera;
- create or submit a CaptureRequest;
- create ImageReader;
- read JPEG/RAW pixels;
- call openInputStream on a media URI;
- decode Bitmap/ImageDecoder;
- parse EXIF;
- invoke Honor Binder methods;
- register Honor callbacks;
- write vendor request/session keys.

Authority:

`PASSIVE_SYSTEM_VISIBLE_OUTPUT_OBSERVATION_ONLY`

## Reproducibility objective

v0.42 established one PHOTO-class output and one high-resolution output:

- ordinary JPEG: 3072x4096
- prior high-resolution JPEG: 16320x12288

v0.43 asks:

1. Does PHOTO reproducibly publish the lower-resolution class?
2. What does PRO publish after a fresh reset?
3. Does the currently visible HI-RES mode reproducibly publish 16320x12288?
4. Does the short Camera-0 availability pulse reproduce specifically in HI-RES runs?
5. Are there transient MediaStore row states that were missed by v0.42?

Only if HI-RES repeatedly produces the prior 16320x12288 output class may the project describe it as
empirically associated with that output class on this device/app build. The UI label alone is not enough.

## Recommended run order

For each run:

1. clear Honor Camera app data/cache;
2. start the selected v0.43 profile;
3. mark launch and open Honor Camera manually;
4. mark MAIN_STABLE from the v0.43 notification;
5. select the profile mode in Honor Camera;
6. mark MODE SELECTED from the notification;
7. take one photo;
8. mark SHUTTER_PRESSED immediately after pressing the shutter;
9. wait 3-5 seconds;
10. leave Honor Camera;
11. return to TruthRaw;
12. mark return;
13. request one MediaStore snapshot;
14. stop v0.43;
15. save JSON.

Run PHOTO, PRO and HI-RES separately. Do not combine modes in one trace.
