# TruthRaw v0.53 — Android 17 replay of proven v0.14 route: device result

Date: 2026-09-19

## Result

The Android-17 BKQ-N49 successfully replayed the proven Android-16 v0.14 acquisition route.

Request topology:

- opened logical camera 0;
- requested physical camera 5;
- physical-scoped request used;
- global SENSOR_PIXEL_MODE write intentionally suppressed;
- physical Camera-5 SENSOR_PIXEL_MODE MAXIMUM_RESOLUTION write succeeded;
- physical readback = 1 / MAXIMUM_RESOLUTION;
- physical override was not advertised;
- output physically bound to camera 5;
- output declared for maximum-resolution sensor pixel mode.

The returned physical capture result again reported SENSOR_PIXEL_MODE = 0. As established by v0.14, this metadata was treated as advisory only after source-first sealing.

## Capture identity

- physical result camera ID: 5
- dimensions: 16320x12288
- declared samples: 200,540,160
- focal length: 22.48 mm
- ISO: 406
- exposure: 9,999,993 ns
- Image timestamp == physical Camera-5 SENSOR_TIMESTAMP exactly

Primary app-visible RAW buffer:

- 401,080,320 bytes
- rowStride 32,640
- pixelStride 2
- SHA-256: `cb6828b65e9b9acafb54455b6e92b671439ae2c57548ede9e7aa8dd401db3cca`

## Whole-raster audit

The Android-17 buffer has the same topology class as the Android-16 v0.19/v0.20 result:

- total rows: 12,288
- non-zero rows: **768**
- all-zero rows: **11,520**
- first non-zero row: 0
- last non-zero row: 767
- total samples: 200,540,160
- non-zero samples: **12,533,760**
- last non-zero byte offset: 25,067,518
- contiguous populated prefix: **25,067,520 bytes**

The first 25,067,520 bytes exactly equal the byte count of physical Camera 5's advertised standard RAW_SENSOR size:

`4080 × 3072 × 2 = 25,067,520`

The geometry decoder therefore again selects one unique advertised standard-RAW candidate:

**4080 × 3072**

Candidate SHA-256:

`f70f353dcd2d59aa79edf159669bffb1573704d68233e337439d8c42458789da`

## Android 16 -> Android 17 delta

All three structural comparison gates match:

- 401,080,320-byte declared envelope: unchanged;
- 25,067,520-byte populated prefix: unchanged;
- unique 4080x3072 standard RAW candidate: unchanged.

Classification:

`ANDROID17_REPLICATES_ANDROID16_CAM5_ENVELOPE_AND_4080x3072_POPULATED_PREFIX_TOPOLOGY`

This is stronger than merely observing the same stream configuration. The same physically bound acquisition route was executed and the returned app-visible RAW payload was sealed and audited.

## Scientific consequence

The public physical-Camera-5 16320x12288 RAW_SENSOR route must not be treated as 200MP sample evidence.

On both Android 16 and Android 17, the app-visible 16320x12288 envelope contains a contiguous populated payload whose exact byte count matches one 4080x3072 U16 RAW raster, followed by zero-filled remainder.

This does not establish how Honor constructs or processes the OEM 200MP JPEG path upstream. It establishes only the third-party Camera2 payload topology.

The Android-17 update, targetSdk 37, Honor Camera 171.0.10.706, and the restored v0.14 request semantics did not expose a new public 200MP RAW payload through this route.

## Next direction

Do not keep iterating the same public 16320x12288 RAW_SENSOR route.

The next useful Android-17 experiment should observe OEM-produced Pro/RAW output and fingerprint its container metadata read-only, especially bit depth, CFA/storage dimensions, compression, WhiteLevel and capture/lens metadata. This is the most direct empirical way to test the advertised Android-17 "RAW14" capability without pretending that the ordinary third-party Camera2 route exposes RAW14.
