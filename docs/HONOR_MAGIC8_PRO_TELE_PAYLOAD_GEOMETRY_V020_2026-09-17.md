# HONOR Magic8 Pro Camera-5 payload geometry — v0.20 observed result

Date: 2026-09-17

Status: **CURRENT DOWNSTREAM PAYLOAD-GEOMETRY RESULT / SOURCE AUTHORITY UNCHANGED**

This document records both the v0.19 source-population finding and the completed v0.20 Stage-3.7 geometry result. It does not supersede the v0.14 acquisition authority and does not promote app-visible RAW_SENSOR to untouched photodiode/ADC truth.

Permanent boundary:

`APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF`

## 1. Proven v0.19 observation

For capture `TRUTHRAW_1789652916539_CAM5_200MP_16320x12288_v019.rawsensor`:

- physical result Camera ID: `5`
- delivered Image geometry: `16320 x 12288`
- Image/SENSOR_TIMESTAMP identity: pass
- source bytes: `401080320`
- pixel stride: `2`
- row stride: `32640`
- sealed source SHA-256: `e93c735b51c77e5c968f92a51bd826321d40dd39a263630d4ed85dc4469fc87d`
- Stage 3.6 re-read SHA-256: identical
- total declared U16 positions: `200540160`
- non-zero positions: `12533760`
- all-zero positions: `188006400`
- non-zero rows: exactly `0..767` (`768` rows)
- rows `768..12287`: all zero
- first non-zero byte offset: `0`
- last non-zero byte offset: `25067518`
- populated prefix byte count: `25067520`
- populated prefix values in that capture: min `60`, max `691`
- bands `1..15` share the all-zero 25,067,520-byte SHA-256 `c2c6b5c221fefd4bad354ce0941fe48cf5e1e1734b2c3c16020c566569850b96`

Exact geometry identity:

`16320 * 768 * 2 = 25067520 bytes`

and

`4080 * 3072 * 2 = 25067520 bytes`.

Therefore the v0.19 source proves a full-size Android/HAL buffer envelope whose populated byte prefix is exactly the byte count of a 4080x3072 U16 RAW raster. At v0.19 that was still only a geometry candidate.

Relevant same-capture vendor observations remained sidecar evidence only:

- `com.hihonor.capture.metadata.binningFactor = 4`
- `AECRealCropWindow` close to the `4080x3072` domain
- `allISPCropWindow` in the `16320x12288` domain
- returned `android.sensor.pixelMode = 0`
- returned `android.sensor.rawBinningFactorUsed = true`.

## 2. v0.20 Stage 3.7 — Payload Geometry Decoder contract

v0.20 preserves the v0.19/v0.17/v0.14 acquisition chain unchanged. Stage 3.7 runs only after:

1. original Plane[0] source bytes are persisted and SHA-256 sealed;
2. the live HAL/HardwareBuffer envelope is observed read-only;
3. auxiliary DNG creation has completed;
4. Image, CameraCaptureSession, ImageReader and CameraDevice are closed;
5. Stage 3.6 has re-read the sealed file and verified source SHA identity.

Stage 3.7 then:

- reads the sealed file only;
- derives the populated-prefix byte count from Stage 3.6;
- compares that byte count against the physical Camera-5 advertised standard RAW_SENSOR output sizes at runtime;
- selects a geometry only when exactly one advertised standard RAW size has the same U16 byte count;
- writes an auxiliary `.rawpayload` that is an exact byte-for-byte copy of source range `[0, payloadBytes)`;
- SHA-256 hashes the copied prefix and compares it with Stage 3.6 band evidence;
- computes exact U16 statistics and 2x2 phase sums for the selected interpretation;
- computes derived F64 neighbor/correlation diagnostics without changing source codes;
- creates a small grayscale diagnostic PNG from 4x4 block means for human geometry inspection.

## 3. Completed v0.20 physical result

Evidence file:

`TRUTHRAW_1789654132526_CAM5_200MP_EVIDENCE_v020.json`

Primary source named by that evidence:

`TRUTHRAW_1789654132526_CAM5_200MP_16320x12288_v020.rawsensor`

Source facts:

- physical result Camera ID: `5`
- Image geometry: `16320 x 12288`
- source bytes: `401080320`
- source SHA-256: `39da26192d1278af797b0c304d6ad29862735b886bdcf768b6239bc74f6a3636`
- row stride: `32640`
- pixel stride: `2`
- timestamp identity: pass
- ISO: `125`
- exposure: `9999993 ns`
- focal length: `22.48 mm`
- focus distance: `0.07848677 D`
- returned `SENSOR_PIXEL_MODE=0`
- requested/output MAX path: true
- returned `rawBinningFactorUsed=true`.

Stage 3.6 again established a contiguous populated prefix of exactly `25,067,520` bytes.

## 4. Unique runtime-advertised geometry match

Camera 5 advertised exactly one standard RAW_SENSOR output whose U16 byte count matched the populated prefix:

`4080 x 3072 x 2 = 25,067,520 bytes`.

Stage-3.7 status:

`UNIQUE_ADVERTISED_STANDARD_RAW_BYTE_MATCH_DECODED`

Selected candidate:

- width `4080`
- height `3072`
- bytes `25067520`
- interpretation only: true.

This is materially stronger than the earlier arithmetic coincidence because the geometry is also a runtime-advertised Camera-5 standard RAW_SENSOR size.

## 5. Exact-prefix identity

Derived file:

`TRUTHRAW_1789654132526_CAM5_PAYLOAD_STANDARD_RAW_BYTE_MATCH_v020.rawpayload`

Facts:

- bytes: `25067520`
- SHA-256: `d41d48d644c4aa8804825a529a0ea651352979a7023e4948e39913b71ce16e3e`
- source byte start: `0`
- source byte end exclusive: `25067520`
- copy transform: `NONE_EXACT_BYTE_PREFIX_COPY`
- SHA-256 equals Stage-3.6 first-band SHA: true
- source replacement: false.

The `.rawpayload` is a derived exact-prefix view. The sealed 401,080,320-byte `.rawsensor` remains Source Evidence.

## 6. U16 code-domain result

For the 4080x3072 candidate in the v0.20 capture:

- sample count: `12,533,760`
- min code: `64`
- max code: `1023`
- exact sum: `1,907,825,214`
- exact sum of squares: `517,410,700,256`.

The `64..1023` range is compatible with a 10-bit-like code domain stored in U16 words and a black offset near 64, but this is not yet calibration or ADC-bit-depth proof.

## 7. 2x2 phase evidence

Phase means under the 4080x3072 interpretation:

- `(y0,x0)`: `131.97894103604983`
- `(y0,x1)`: `174.72990642871733`
- `(y1,x0)`: `174.94584673713234`
- `(y1,x1)`: `127.20496610753676`.

The two cross phases are nearly equal while the diagonal phases differ. This is strongly consistent with a normal Bayer-like 2x2 CFA organization. It does not alone identify which diagonal is R versus B.

## 8. Spatial-correlation evidence

Sampled pair statistics:

- horizontal dx=1: mean abs diff `45.7344369`, correlation `0.8457711`
- horizontal dx=2: mean abs diff `7.6460561`, correlation `0.9846274`
- vertical dy=1: mean abs diff `43.6312263`, correlation `0.8791863`
- vertical dy=2: mean abs diff `7.3667139`, correlation `0.9818455`.

Distance 2 preserves CFA phase and is dramatically more similar than immediate cross-colour neighbors. This is strong spatial support for indexing the prefix as a normal Bayer-like 4080x3072 raster.

## 9. Diagnostic image result

Diagnostic file:

`TRUTHRAW_1789654132526_CAM5_PAYLOAD_GEOMETRY_DIAGNOSTIC_v020.png`

Properties:

- source geometry: `4080x3072`
- diagnostic geometry: `1020x768`
- transform: 4x4 U16 mean -> min/max normalization -> square-root display gamma
- appearance-only: true
- scientific evidence: false.

The uploaded PNG shows a coherent full-frame street scene when the exact prefix is indexed as 4080x3072. This visual coherence is diagnostic corroboration of the geometry interpretation; the PNG itself is not source evidence.

## 10. Current bounded interpretation

Recommended current classification:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

Useful descriptive shorthand:

`APP_VISIBLE_4080x3072_BAYER_LIKE_RAW_PAYLOAD_EMBEDDED_IN_16320x12288_HAL_ENVELOPE`

This does **not** prove that the physical sensor is natively 4080x3072. It describes the app-visible payload organization observed on this route.

## 11. Authority rules

The sealed 401,080,320-byte `.rawsensor` remains Source Evidence.

The Stage-3.7 `.rawpayload` is an exact-prefix derived view and does not replace the source.

The Stage-3.7 PNG is appearance-only.

No current result proves:

- native ADC raster geometry;
- one independent photodiode/ADC measurement per payload sample;
- absence or exact mechanism of sensor binning/remosaic/resampling;
- 200 MP optical resolving power;
- calibrated black level 64;
- 10-bit ADC solely from maximum code 1023;
- physical meaning of an undocumented vendor-key name.

## 12. Request-side implication

Because the missing 15/16 already exists in the original sealed source, downstream DNG repair is not the primary solution. The next controlled experiment belongs upstream at the request/session boundary.

The untouched v0.20 route exposes candidate session/request names including:

- `EnableIdealRAW`
- `RawCbSourceType`
- `EnableXCFAOptimization`
- `HALOutputBufferCombined`
- `EnableInsensorZoom`
- `EnableSnapshotOnlyInsensorZoom`
- `EnableMCXMasterCb`.

The v0.20 control changed none of them. `EnableXCFAOptimization` was visible as zero; `EnableIdealRAW` and `RawCbSourceType` were available but had null builder current/default values in the captured fingerprint.

The first future route-changing experiment should use v0.20 as control and change exactly one candidate variable, beginning with runtime-type-verified `EnableIdealRAW` if safely representable.

## 13. Related current documents

Read together:

- `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md`
- `docs/HONOR_MAGIC8_PRO_TELE_200MP_FULL_RAW_V014_2026-09-17.md`
- `docs/HONOR_CAMERA5_TWO_DOOR_AIRLOCK_V017_2026-09-17.md`
- `docs/handoff/TRUTHRAW_CAMERA5_RAW_ROUTE_V014_TO_V020_2026-09-17.md`
- `state/CURRENT_CAMERA5_RAW_ROUTE_STATE_2026-09-17.json`.
