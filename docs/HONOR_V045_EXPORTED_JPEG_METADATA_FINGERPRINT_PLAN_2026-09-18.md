# HONOR v0.45 exported JPEG metadata fingerprint plan

Date: 2026-09-18

## Purpose

v0.44 established a reproducible output distinction inside Honor HI-RES:

- HI-RES main → 8192 × 6144 system-visible JPEG;
- user-selected tele / UI-described 200MP state → same MediaStore row transitions from 6144 × 8192 to final 16320 × 12288.

Camera 0 availability spans both UI states and is therefore not a reliable main→tele switch detector.

v0.45 adds a new, explicitly lower-authority observation layer: selected metadata exported inside the final Honor JPEG.

## New authority layer

Selected JPEG EXIF fields are read only after:

- the MediaStore row belongs to `com.hihonor.camera`;
- MIME type is `image/jpeg`;
- MediaStore reports a non-null, positive file size.

The authority of this metadata is:

`EXPORTED_OUTPUT_METADATA_ONLY`

It is never:

- RAW/CFA capture evidence;
- proof of active physical camera ID;
- proof of sensor-native resolution;
- calibration truth;
- Scientific Master input.

## What v0.45 reads

Using a read-only file descriptor and platform EXIF parser, v0.45 requests only a selected non-GPS tag set when present:

- Make
- Model
- Software
- DateTimeOriginal
- OffsetTimeOriginal
- FNumber
- ExposureTime
- ISOSpeedRatings
- PhotographicSensitivity
- FocalLength
- FocalLengthIn35mmFilm
- DigitalZoomRatio
- LensMake
- LensModel
- LensSpecification
- WhiteBalance
- ExposureProgram
- MeteringMode
- Flash
- Orientation
- ImageWidth
- ImageLength
- PixelXDimension
- PixelYDimension

GPS tags are deliberately not requested.

No bitmap or pixel array is decoded.

## Why this is useful

If Honor exports different focal-length or lens metadata for:

- HI-RES main 50MP output;
- tele / UI-described 200MP final output;

then that becomes an additional independent **export-layer association** between user-visible state and final file metadata.

Even if a tag appears to identify a lens, it remains a claim written into the exported JPEG by Honor software. It does not independently prove Camera2 physical ID 5 or sensor-native sampling.

## Retained v0.44 behavior

v0.45 keeps:

- `HIRES_MAIN_CONFIRMED`
- `TELE_200MP_UI_SELECTED`
- `SHUTTER_PRESSED`
- CameraManager availability callbacks
- MediaStore delayed snapshots at 0/25/100/250/1000 ms
- screenshot-vs-Honor-output metadata classification
- same-row metadata-state transition tracking
- explicit human-marker correlation

## EXIF fingerprint event

For each unique stable Honor JPEG state, v0.45 emits:

`EXPORTED_JPEG_METADATA_FINGERPRINT`

The event records:

- source URI;
- display name;
- MediaStore width/height/size;
- MediaStore geometry class;
- associated user state;
- selected EXIF tags actually present;
- whether read-only file descriptor opening succeeded;
- parse timing;
- `gpsTagsRequested=false`;
- `imagePixelArrayDecoded=false`;
- `captureEvidenceGranted=false`;
- authority `EXPORTED_OUTPUT_METADATA_ONLY`.

## Research target

The first device run should contain, in one fresh HI-RES session:

1. `HIRES_MAIN_CONFIRMED`
2. one HI-RES-main capture
3. `SHUTTER_PRESSED`
4. user switches to tele / visible 200MP UI state
5. `TELE_200MP_UI_SELECTED`
6. one tele/200MP capture
7. `SHUTTER_PRESSED`
8. wait several seconds for finalization and metadata fingerprint
9. return, snapshot, stop, export JSON

The key comparison is the pair of exported metadata fingerprints associated with the 8192×6144 and final 16320×12288 outputs.
