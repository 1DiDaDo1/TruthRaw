# HONOR v0.44 device result — HI-RES main → tele/200MP UI

Date: 2026-09-18

## Source

User-uploaded report:

- `main 50mp tele 200mp.json`
- bytes: 112594
- SHA-256: `5f067d339d9a781bde875c86542ab6e87e1d1ac694b81ecbf940ccff9b4e8097`
- schema: `truthraw.passive-hires-tele-state-timeline.v0.44`
- run profile: `HIRES_MAIN_TO_TELE_200MP_UI_SESSION`

## Authority

`PASSIVE_SYSTEM_VISIBLE_OUTPUT_OBSERVATION_ONLY`

The report explicitly preserves:

- `cameraOpenedByTruthRaw=false`
- `captureSubmittedByTruthRaw=false`
- `imagePixelBytesRead=false`
- `mediaInputStreamOpened=false`
- `exifDecoded=false`
- `vendorRequestWrittenByTruthRaw=false`
- `honorBinderMethodInvoked=false`
- `honorCallbackRegistered=false`

Human markers remain user-declared UI/timing anchors, not hardware timestamps.

## HI-RES main state

The user explicitly marked:

`HIRES_MAIN_CONFIRMED`

The subsequent Honor-camera MediaStore output was:

- ID 114362
- `IMG_20260918_232933.jpg`
- JPEG
- 8192 × 6144
- 50,331,648 system-visible output pixels
- 8,001,762 bytes
- `DCIM/Camera/`
- owner `com.hihonor.camera`

v0.44 classified it as:

- `HONOR_CAMERA_SYSTEM_VISIBLE_OUTPUT_CANDIDATE`
- `SYSTEM_VISIBLE_50_331_648PX_CLASS`

## User-selected tele / 200MP UI state

The user explicitly marked:

`TELE_200MP_UI_SELECTED`

The first subsequent MediaStore change for the new Honor output carried that exact marker as
`lastHumanMarkerLabel`.

The new row was:

- ID 114363
- `IMG_20260918_232949.jpg`
- JPEG
- owner `com.hihonor.camera`

### Early state

- 6144 × 8192
- 50,331,648 system-visible output pixels
- `_size=null`
- `is_pending=0`
- geometry class `SYSTEM_VISIBLE_50_331_648PX_CLASS`

### Final state

The same MediaStore URI later changed to:

- 16320 × 12288
- 200,540,160 system-visible output pixels
- 27,191,131 bytes
- `is_pending=0`
- geometry class `SYSTEM_VISIBLE_200_540_160PX_CLASS`

v0.44 automatically recorded:

- `metadataStateChangedSincePreviousObservation=true`
- previous geometry 6144 × 8192
- current geometry 16320 × 12288
- `sameMediaStoreUri=true`
- `pixelBytesRead=false`

This independently replicates the same-row 50MP-class → 16320×12288 finalization pattern first
observed in v0.43, but now with an explicit tele/200MP UI-state marker attached to the event chain.

## CameraManager result

Camera 0 became unavailable before the HI-RES-main marker and remained unavailable across:

- the HI-RES-main capture;
- the explicit tele/200MP UI-state marker;
- the tele/200MP output capture and its MediaStore finalization.

Camera 0 later became available again after the capture sequence.

Therefore:

`CAMERA0_UNAVAILABLE_IS_NOT_A_RELIABLE_MAIN_TO_TELE_MODE_SWITCH_DETECTOR`

The CameraManager availability boundary is useful route/ownership observation but must not be interpreted
as active-lens telemetry for this run.

## Supported conclusion

The current evidence supports:

`HIRES_MAIN_50MP_REPLICATED__EXPLICIT_TELE_200MP_UI_STATE_ASSOCIATED_WITH_SAME_ROW_50MP_TO_200MP_OUTPUT_FINALIZATION__CAMERA0_UNAVAILABLE_NOT_MODE_SWITCH_SPECIFIC`

The tele/200MP UI state is now directly associated, by the user-state marker plus subsequent system-visible
MediaStore output, with a row that finalizes as 16320 × 12288.

## Boundaries that remain

This does NOT prove:

- physical camera ID 5 was active;
- 200,540,160 independently measured RAW/CFA samples;
- native 200MP ADC sampling;
- remosaic implementation;
- optical resolving power;
- that the early 6144 × 8192 state is sensor-native;
- that Camera 0 availability semantics identify the selected lens.

The prior RAW evidence remains separate and unchanged.

## Next research consequence

The next useful passive discriminator is exported JPEG metadata, especially output-level lens/focal-length
metadata, read only after the Honor file has reached a stable non-null size.

That metadata must remain:

`EXPORTED_OUTPUT_METADATA_ONLY`

It may strengthen main-versus-tele association, but can never be promoted to RAW/CFA capture evidence or
used to alter Scientific Master authority.
