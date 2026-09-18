# HONOR v0.44 passive HI-RES main → tele/200MP UI state-anchor plan

Date: 2026-09-18

## Motivation

v0.43 showed that one generic `MODE_HIRES_SELECTED` marker was too coarse.

The device run established two user-visible HI-RES states:

- HI-RES while still on the main-camera UI state;
- HI-RES after the user manually selected the tele / UI-described 200MP state.

Those two states produced different system-visible output geometries in the observed run:

- HI-RES main: 8192 × 6144;
- tele / UI-described 200MP state: same-row MediaStore transition from 6144 × 8192 to 16320 × 12288.

v0.44 records those states explicitly.

## Run profiles

- `PHOTO_MAIN_BASELINE_SESSION`
- `PRO_SWITCH_SESSION`
- `HIRES_MAIN_TO_TELE_200MP_UI_SESSION`

PHOTO is explicitly treated as a no-mode-switch baseline.

## Notification actions

PHOTO:
- `MAIN/PHOTO`
- `SHUTTER`

PRO:
- `MAIN STABLE`
- `PRO SELECTED`
- `SHUTTER`

HI-RES:
- `HIRES MAIN`
- `TELE / 200MP UI`
- `SHUTTER`

The tele/200MP marker is deliberately named as a UI/user-state marker. It does not prove:

- active physical camera ID 5;
- native 200MP ADC sampling;
- 200MP populated RAW/CFA;
- remosaic internals.

## Screenshot support

The user may take screenshots before captures.

v0.44 does not open screenshot bytes. It classifies MediaStore rows from metadata only:

- `SCREENSHOT_UI_ANCHOR_CANDIDATE`
- `HONOR_CAMERA_SYSTEM_VISIBLE_OUTPUT_CANDIDATE`
- `OTHER_MEDIASTORE_IMAGE`

Classification uses visible metadata such as display name, relative path, and owner package only.

A screenshot candidate is appearance/UI evidence, not sensor or capture evidence.

## Same-row state transitions

For each MediaStore URI, v0.44 stores the last observed metadata state:

- width;
- height;
- size;
- date_modified;
- is_pending.

If the same URI changes state, the later observation records:

- `metadataStateChangedSincePreviousObservation=true`;
- previous state;
- current state;
- `sameMediaStoreUri=true`;
- `pixelBytesRead=false`.

This is specifically intended to expose transitions like the v0.43 row 114352 path:

`6144×8192, size=null → 16320×12288, size=27132295`

without manually reconstructing the transition afterward.

## Marker context attached to MediaStore changes

Each MediaStore change records the latest human marker label and monotonic marker time, when available.

This does not turn a human marker into a shutter timestamp. It only makes event correlation explicit.

## Geometry classes

System-visible output geometry is tagged by exact pixel count when recognized:

- 12,582,912 pixels;
- 50,331,648 pixels;
- 200,540,160 pixels.

These classes are output geometry only. They are not sensor-native sample-count claims.

## Invariants

TruthRaw v0.44 does not:

- open a camera;
- create or submit a CaptureRequest;
- create ImageReader;
- read image or screenshot pixel bytes;
- open a media input stream;
- decode bitmap/image;
- parse EXIF;
- invoke Honor Binder methods;
- register Honor Binder callbacks;
- write vendor request/session keys.

Authority remains:

`PASSIVE_SYSTEM_VISIBLE_OUTPUT_OBSERVATION_ONLY`
