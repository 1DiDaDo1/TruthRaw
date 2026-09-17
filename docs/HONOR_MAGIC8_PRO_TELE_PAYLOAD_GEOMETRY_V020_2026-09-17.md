# HONOR Magic8 Pro Camera-5 payload geometry — v0.20 contract

Date: 2026-09-17

Status: experimental downstream interpretation of already sealed Camera2 evidence. This document does not supersede the v0.14 acquisition authority or promote app-visible RAW_SENSOR to untouched photodiode/ADC truth.

## Proven v0.19 observation

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

Therefore the v0.19 source proves a full-size Android/HAL buffer envelope whose populated byte prefix is exactly the byte count of a 4080x3072 U16 RAW raster. It does **not** by itself prove that 4080x3072 is the true sensor geometry; byte-count equality is a geometry candidate, not semantic proof.

Relevant same-capture vendor observations remain sidecar evidence only:

- `com.hihonor.capture.metadata.binningFactor = 4`
- `AECRealCropWindow` begins approximately `11,8,4058,3055`
- `allISPCropWindow` reports `16320x12288`
- returned `android.sensor.pixelMode = 0`
- returned `android.sensor.rawBinningFactorUsed = true`

## v0.20 Stage 3.7 — Payload Geometry Decoder

v0.20 preserves the v0.19/v0.17/v0.14 acquisition chain unchanged. Stage 3.7 runs only after:

1. original Plane[0] source bytes are persisted and SHA-256 sealed;
2. the live HAL/HardwareBuffer envelope is observed read-only;
3. auxiliary DNG creation has completed;
4. Image, CameraCaptureSession, ImageReader and CameraDevice are closed;
5. Stage 3.6 has re-read the sealed file and verified source SHA identity.

Stage 3.7 then:

- reads the sealed file only;
- derives the populated-prefix byte count from Stage 3.6;
- compares that byte count against the physical Camera-5 **advertised standard RAW_SENSOR output sizes at runtime**;
- selects a geometry only when exactly one advertised standard RAW size has the same U16 byte count;
- writes an auxiliary `.rawpayload` that is an exact byte-for-byte copy of source range `[0, payloadBytes)`;
- SHA-256 hashes the copied prefix and compares it with Stage 3.6 band evidence when applicable;
- computes exact U16 statistics and 2x2 phase sums for the selected interpretation;
- computes derived F64 neighbor/correlation diagnostics without changing source codes;
- creates a small grayscale diagnostic PNG from 4x4 block means for human geometry inspection.

## Authority rules

The sealed 401,080,320-byte `.rawsensor` remains Source Evidence.

The Stage 3.7 `.rawpayload` is a **derived exact-prefix view**, not a replacement source. Byte identity is preserved, but geometry interpretation remains provisional until independently validated.

The Stage 3.7 PNG is appearance-only diagnostic output and is never scientific evidence.

A unique advertised-size byte match may support the statement:

`EXACT_PREFIX_BYTES_MATCH_ONE_ADVERTISED_STANDARD_RAW_GEOMETRY`

It must not be promoted to any of the following without further evidence:

- native ADC raster proof;
- one independent photodiode per interpreted sample;
- absence of sensor binning/remosaic/resampling;
- optical-resolution proof;
- 200MP scientific authority.

## Next evidence step

If v0.20 selects `4080x3072`, save and inspect both auxiliary outputs:

- the exact-prefix `.rawpayload` (~25.1 MB), which is small enough for direct host analysis;
- the diagnostic PNG, which tests whether the same bytes become a geometrically coherent scene when indexed as the matched standard RAW size.

The `.rawpayload` should then undergo direct CFA/parity, row/column correlation, bit occupancy, black/white-code, noise/topology and spatial-structure analysis before any reconstruction or Scientific Master use.
