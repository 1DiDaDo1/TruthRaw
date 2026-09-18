# HONOR Camera-5 v0.33 / v0.34 isolated in-sensor route results — 2026-09-18

Status: **DEVICE RESULTS COMPLETE** for the first two v0.32 upstream candidates.

## Scope

These experiments preserve v0.20 as the source/payload authority. They do not reinterpret the vendor key names and do not promote numeric value `1` to a proven semantic “enabled” state.

Both experiments reuse the same bounded chain:

`Gate A untouched fingerprint -> exactly one vendor session intervention -> session attach -> logical 0 / physical 5 MAX capture -> original Plane[0] source seal -> HAL envelope observation -> Stage 3.6 full-raster audit -> Stage 3.7 payload geometry decoder`.

## v0.33 — EnableInsensorZoom

v0.32 had already resolved:

- key: `org.codeaurora.qcamera3.sessionParameters.EnableInsensorZoom`
- tag: `0x801F0013`
- native metadata type: `INT32`

v0.33 then used exactly one stimulus: `INT32(1)`.

Device evidence confirmed builder readback = 1, built-request readback = 1, session parameters attached, and exactly one vendor key written. No other v0.32 candidate was written.

Result:

- source envelope: 16320x12288 / 401,080,320 bytes
- source SHA-256: `a519d4eaa4ff30136bbeb26afeb0621006d4e5bb3bb053d32879e6a76b2f9815`
- Stage 3.6: rows 0..767 non-zero; rows 768..12287 zero
- populated prefix: 25,067,520 bytes
- Stage 3.7: unique advertised standard RAW exact byte match = 4080x3072
- exact-prefix payload SHA-256: `0134d55d0a8f1e376fcac333b9de7f8196b9c1328094bd0a778a4fec2f8cc49a`
- measured RAW envelope/populated-prefix topology differential: **none**

## v0.34 — EnableSnapshotOnlyInsensorZoom

v0.32 had already resolved:

- key: `org.codeaurora.qcamera3.sessionParameters.EnableSnapshotOnlyInsensorZoom`
- tag: `0x801F0014`
- native metadata type: `INT32`
- logical session/request advertised: true
- physical session/request advertised: false

v0.34 again used exactly one stimulus: `INT32(1)`.

Device evidence confirmed:

- builder set succeeded
- builder readback = 1
- built-request readback = 1
- session parameters attached
- vendorKeysWritten = 1
- `EnableInsensorZoom` not written
- `EnableMCXMasterCb` not written
- no semantic promotion allowed

Result:

- source envelope: 16320x12288 / 401,080,320 bytes
- source SHA-256: `b8edc8e37998167489dac087f433ac0885c8640724fc004fd7c387e6134f99c5`
- Stage 3.6 sealed-source rehash identity: PASS
- non-zero rows: exactly 0..767
- zero rows: exactly 768..12287
- populated prefix: exactly 25,067,520 bytes
- Stage 3.6 classification unchanged: `ONLY_FIRST_768_ROWS_NONZERO__EXACT_12P5MP_BYTE_PAYLOAD_SIGNATURE`
- Stage 3.7 unique advertised standard RAW exact match: 4080x3072
- exact-prefix payload SHA-256: `32cb6970d603e1ee383731cc946376864e0ca4cab2a29821b93002b7a45cfd43`
- Stage 3.7 classification unchanged: `EXACT_PREFIX_BYTES_MATCH_ONE_ADVERTISED_STANDARD_RAW_GEOMETRY__GEOMETRY_INTERPRETATION_CANDIDATE_NOT_SENSOR_PROOF`
- measured RAW envelope/populated-prefix topology differential: **none**

The separately uploaded v0.34 `.rawpayload` was independently hashed after capture and matched `32cb6970...` exactly. The separately uploaded diagnostic PNG likewise matched its JSON SHA-256 `3a7198bb3d7d92049c1d3781bb5f086d31fc5ffeea30608eda8863e31d29e78d`.

Observed coarse result-side route indicators also remained in the previously seen class:

- HONOR `binningFactor = 4`
- AEC crop prefix `[11,8,4058,3055]`
- ISP crop `[0,0,16320,12288,0,0,16320,12288]`
- `isInSensorZoom = 0`
- `rawBinningFactorUsed = true`

These names remain observations, not semantics.

## Combined bounded conclusion

Within the tested logical-0 -> physical-5 MAX capture context, neither isolated native-type-correct `INT32(1)` intervention changed the measured app-visible RAW envelope or populated-prefix topology:

1. `EnableInsensorZoom`
2. `EnableSnapshotOnlyInsensorZoom`

This is stronger than an unverified “toggle did nothing” statement because both interventions were representation-validated, builder/request read back successfully, were attached as session parameters, and were measured through the unchanged source-first v0.20 audit chain.

It still does **not** prove:

- numeric 1 means “enabled”;
- the vendor names describe their actual semantics;
- either key is globally ineffective;
- other numeric values are ineffective;
- a prerequisite/context state cannot make either key causal;
- the 4080x3072 candidate is native physical sensor/ADC geometry;
- 200 MP optical resolution is absent or present.

## Next experiment

The remaining v0.32 candidate is:

`org.codeaurora.qcamera3.sessionParameters.EnableMCXMasterCb`

v0.32 resolved it as native `INT32`, tag `0x801F0005`, logical-session/request advertised and not physical-session/request advertised.

The next clean experiment is therefore v0.35 with **exactly one** unknown vendor intervention:

`EnableMCXMasterCb = INT32(1)`

All other v0.32 candidates and the earlier IdealRAW / RawCbSourceType / XCFA / HALOutputBufferCombined interventions remain UNSET. The v0.20 source-first chain must remain unchanged.
