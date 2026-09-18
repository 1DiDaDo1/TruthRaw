# HONOR v0.42 passive MediaStore + CameraManager timeline — plan and provenance

Date: 2026-09-18

## Why v0.42 exists

v0.40 proved that the exported Honor CameraAccessoriseService can be bound by an ordinary TruthRaw process.

v0.41 then proved on-device that callback registration is protected by Honor's package/signature authorization boundary:

`registerOutputConfigCallback(IOutputConfigCallback)`

returned:

`java.lang.SecurityException: Package not allowed: 10573`

TruthRaw stopped at that first method boundary exactly as designed.

The direct Honor callback route is therefore closed for an ordinary TruthRaw process. v0.42 deliberately
moves to an independent, system-visible observation layer instead of probing sibling methods behind the
same security gate.

## v0.42 hypothesis

If the user operates Honor Camera normally and changes from the default main-preview to 200 MP, then
even if Camera2 physical-availability callbacks do not expose that internal switch, the final capture may
still become externally observable through Android MediaStore.

v0.42 tests whether a manual Honor capture creates a distinguishable system-visible output event and
output geometry while Honor remains the only camera owner.

## Observed layers

v0.42 records two independent system interfaces:

### 1. CameraManager availability timeline

Recorded callbacks:

- camera available
- camera unavailable
- physical camera available
- physical camera unavailable
- camera access priorities changed

These are availability/ownership observations only. They do not prove the active physical lens.

### 2. MediaStore Images metadata timeline

v0.42 registers a ContentObserver on the external Images collection and records:

- raw MediaStore change notification
- changed URI when provided
- metadata-row snapshots only

Projection:

- row ID
- display name
- MIME type
- width
- height
- byte size
- date taken
- date added
- date modified
- relative path
- pending state
- owner package when exposed
- volume name

No media file is opened.

## Strict negative capabilities

v0.42 must not:

- call CameraManager.openCamera
- create CaptureRequest
- submit a capture
- instantiate ImageReader
- access JPEG/HEIF/RAW pixel bytes
- call ContentResolver.openInputStream on a media URI
- decode Bitmap/ImageDecoder
- parse EXIF
- call an Honor Binder method
- register an Honor callback
- write a vendor request/session key
- mutate a media item

The only output stream used by the Activity is the user-selected JSON export destination.

## Media permission boundary

On Android 13+ Android requires `READ_MEDIA_IMAGES` for broad access to image rows belonging to other
apps. Android 14+ may additionally expose selected-photo access via
`READ_MEDIA_VISUAL_USER_SELECTED`.

v0.42 therefore works fail-soft:

- ContentObserver registration and CameraManager observation are attempted regardless;
- MediaStore metadata queries are attempted;
- permission state is recorded in the report;
- if Android rejects metadata access, the exact exception is recorded;
- no attempt is made to bypass scoped-storage or permission controls.

The runtime permission technically grants more capability than v0.42 uses. The source/build invariants
explicitly prohibit media input streams and image decoding.

## Recommended first run

1. Grant image/media permission if Android offers it.
2. Start v0.42 observer.
3. Mark and move TruthRaw to background.
4. Open Honor Camera manually.
5. Leave the default main preview stable for a few seconds.
6. Switch manually to 200 MP.
7. Wait 2–3 seconds.
8. Make exactly one photo.
9. Wait 3–5 seconds.
10. Leave Honor Camera and return to TruthRaw.
11. Mark return.
12. Trigger one explicit MediaStore snapshot.
13. Stop observer.
14. Save JSON.

The first run should contain only one new Honor photo so row correlation remains unambiguous.

## Interpretation rules

A new MediaStore row with width/height, MIME and byte size is a **system-visible output observation**.

It does not prove:

- RAW sensor geometry
- native ADC geometry
- active physical lens
- remosaic semantics
- optical resolution
- internal Honor pipeline identity
- TruthRaw capture evidence

If a 200 MP user action produces a row whose width/height differs materially from an ordinary-photo row,
that becomes a candidate output-route differential to test separately.

## Authority

All v0.42 output remains:

`PASSIVE_SYSTEM_VISIBLE_OUTPUT_OBSERVATION_ONLY`

Representation can exceed the source. Knowledge claims cannot exceed the evidence.
