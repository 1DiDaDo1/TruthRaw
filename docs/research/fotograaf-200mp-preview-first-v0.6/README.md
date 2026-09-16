# FotoGraaf 200MP preview-first acquisition v0.6

Status: research/integration candidate, not canonical promotion.

## Why this exists

Field testing of the v0.5 integrated APK exposed two usability/runtime problems:

1. the intended Camera-5 200MP route was hidden among general Pro-camera routes;
2. FotoGraaf Pro created a Camera2 session that combined a live preview output with the selected RAW_SENSOR ImageReader. On HONOR this can make preview depend on support for a large preview+RAW stream combination, including maximum-resolution candidates.

The resulting black/no-preview state is not evidence that physical Camera 5 or the static 200MP route is absent. It is an application/session-topology problem until device evidence says otherwise.

## v0.6 acquisition split

The 200MP test is now one explicit path:

`logical camera 0 -> physical camera 5 -> RAW_SENSOR 16320x12288 -> SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION`

Preview and capture are intentionally separate:

`ordinary physical-5 preview-only session`

followed, only after explicit shutter action, by:

`RAW-only maximum-resolution session -> one RAW_SENSOR Image -> one TotalCaptureResult`

The preview request deliberately does **not** set `SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION`. It is framing/3A observation only and creates no additional sensor evidence.

## 200MP admission requirements

A device capture is not admitted as a 200MP capture candidate unless all of the following are simultaneously observed:

- requested logical camera is 0;
- requested physical output is camera 5;
- RAW Image dimensions are exactly 16320x12288;
- sample count is exactly 200,540,160;
- the physical Camera-5 result is present;
- Image timestamp binds to the capture result;
- physical result reports `SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION`;
- RAW_SENSOR has one plane with 2-byte pixel stride and valid row-stride geometry;
- valid sample bytes are SHA-256 sealed before DNG presentation;
- DNG orientation is explicitly requested as TIFF Orientation=1.

The maximum permitted interpretation remains:

`APP_VISIBLE_MAXIMUM_RESOLUTION_RAW_SENSOR_CFA_CANDIDATE`

until host Step-3B replay/promotion validates the returned capture artifact. This does not prove untouched native ADC/photodiode output.

## Color metadata field evidence

The v0.6 evidence JSON also retains capture-time:

- `CONTROL_AWB_STATE`;
- `COLOR_CORRECTION_GAINS`;
- `COLOR_CORRECTION_TRANSFORM`.

This was added after a real Camera-5 4080x3072 field capture showed that the DNG's source-bound color/white-balance metadata can produce visibly incorrect color. These capture-time values are diagnostic/provenance data, not independent color calibration and do not upgrade calibration authority.

## Resource rule

The 16320x12288 RAW ImageReader uses `maxImages=1`. Raw sample hashing is streamed row-by-row from a duplicate ByteBuffer; no second full-frame byte copy is intentionally created. Resource constraints may change implementation strategy, but never weaken the evidence gate.

## User-facing route

The suite launcher exposes a dedicated first-class button:

`200MP TELE TEST · physical 5 · 16320x12288`

The user no longer needs to infer the correct route from a general spinner.
