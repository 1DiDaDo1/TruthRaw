# HONOR Camera-5 v0.31 INT32 value-domain sweep — device result — 2026-09-17

Status: **DEVICE SWEEP COMPLETE 6/6; ALL TESTED VALUES ATTACHED; NO MEASURED RAW-TOPOLOGY DIFFERENTIAL; v0.20 REMAINS SOURCE/PAYLOAD AUTHORITY**

Permanent boundary:

`APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF`

## Purpose

v0.31 followed the completed v0.30 16/16 binary interaction matrix. v0.30 had already screened UNSET versus numeric `1` across four type-resolved vendor controls without changing the measured Camera-5 RAW envelope/populated-prefix topology.

v0.31 therefore changed the independent variable rather than repeating the binary plane. It tested small alternate values on the two controls whose native metadata representation had been resolved as `INT32`:

- `RawCbSourceType` / tag `0x801F0009` / native `INT32`;
- `HALOutputBufferCombined` / tag `0x801F0034` / native `INT32`.

The BYTE controls `EnableIdealRAW` and `EnableXCFAOptimization` remained UNSET in every v0.31 run.

No vendor enum meaning was assumed.

## Tested profiles

Six individually readable evidence JSONs were captured:

- S01 `RawCbSourceType=0`;
- S02 `RawCbSourceType=2`;
- S03 `RawCbSourceType=3`;
- S04 `HALOutputBufferCombined=0`;
- S05 `HALOutputBufferCombined=2`;
- S06 `HALOutputBufferCombined=3`.

Each run wrote exactly one unknown vendor key. `builderReadbackBeforeSet` was null for the tested key, the requested value was accepted by the CaptureRequest builder, builder readback matched, built-request readback matched, and `SessionConfiguration.setSessionParameters(...)` succeeded.

Therefore, on this device and route, both INT32 controls are proven to accept the tested numeric values `0`, `2`, and `3` through the app/session-parameter path. Together with earlier v0.30/v0.26 evidence, numeric value `1` is also accepted. This is representation/value acceptance only; it does not identify enum semantics.

## Device evidence identities

| Run | Profile | Source RAW SHA-256 | Derived exact-prefix payload SHA-256 |
|---|---|---|---|
| S01 | `RAWCB=0` | `85b8a79b4c03271a6b959e9677ac4be1aefb9166aedba73e3b96ca08aa98d666` | `fd8623c7cf528265bca0b13af7db8cf58e989fd42e79c86b797fcc2a7f2227ab` |
| S02 | `RAWCB=2` | `6ad274a085b148492057abec3354e7a3f9b0e034d78ef9f2bf0e8ff598c62f20` | `b529433fecaa42fbfa8c46b28106daab9ceab088752866fc35a3cac0168aca5d` |
| S03 | `RAWCB=3` | `21d36605c7135f0936e804613682d314b2b4d37c5c9b53c72cbdddb0bdef377a` | `8a6e550cb65bc3bd1347f7eeba9481e1dec7d5bfe868bdac5ee7ceb5764d1a3d` |
| S04 | `HALCOMBINED=0` | `e9e998742d2a3ab29a006be14b5c42c891a6cfb8f8182c34b780c1678a8afc39` | `313714ca30fe290095d78d3305f0bb67a414c7ff95b333cb160632b8381932ba` |
| S05 | `HALCOMBINED=2` | `eda715615376bb980731b6948aada3f64526e33a95812752ad32f4768313580b` | `32261edfa33c46d7a5c902a98dd48d946d21e0c916e06781ccdc6ffe1264ee82` |
| S06 | `HALCOMBINED=3` | `1ad03baa250bd6eae4a5515eb314194137859e0683fd10c9a757771638fdd275` | `837388a066fcf689e82f9bbff63a10604f16758068dfffd93523c31dccb496fc` |

The differing source/payload hashes confirm distinct physical capture contents. Scene/exposure/focus differed between captures, so pixel-value differences are not treated as causal value-domain effects.

## Common app-visible envelope

All six runs delivered the same measured envelope class:

- physical result Camera `5`;
- app-visible RAW_SENSOR geometry `16320x12288`;
- source bytes `401,080,320`;
- row stride `32,640`;
- pixel stride `2`;
- exact Image/CaptureResult timestamp identity;
- returned `SENSOR_PIXEL_MODE=0`;
- `rawBinningFactorUsed=true`.

The original Image.Plane source remained sealed before returned-pixel-mode interpretation and before Stage 3.6/3.7 analysis.

## Stage 3.6 result

All six runs produced exactly:

`ONLY_FIRST_768_ROWS_NONZERO__EXACT_12P5MP_BYTE_PAYLOAD_SIGNATURE`

Measured common structure:

- first band bytes `25,067,520`;
- exactly the first `768` declared rows contain the payload domain;
- remaining `11,520` declared rows are zero;
- `firstBandBytes == 4080*3072*2`;
- Stage 3.6 reads the sealed file only and does not modify source evidence.

No tested value changed the raster-population topology.

## Stage 3.7 result

All six runs produced:

`UNIQUE_ADVERTISED_STANDARD_RAW_BYTE_MATCH_DECODED`

For every run:

- payload bytes `25,067,520`;
- payload samples U16 `12,533,760`;
- source byte interval `[0, 25,067,520)`;
- exactly one advertised standard RAW geometry has the same U16 byte count;
- selected candidate `4080x3072`;
- derived `.rawpayload` is an exact prefix copy with `copyTransform=NONE_EXACT_BYTE_PREFIX_COPY`;
- payload SHA-256 matches the Stage-3.6 first-band SHA-256;
- source authority remains unchanged.

## Returned route-side metadata

The coarse route-side metadata inspected in every v0.31 capture also remained in the same class:

- HONOR `binningFactor=4`;
- HONOR `AECRealCropWindow` starts `[11,8,4058,3055,...]`;
- HONOR `allISPCropWindow=[0,0,16320,12288,0,0,16320,12288]`;
- HONOR `isInSensorZoom=0`;
- Android `rawBinningFactorUsed=true`.

No systematic route-state change was observed in these fields across values `0`, `2`, or `3` for either tested INT32 key.

## Bounded conclusion

The v0.31 result is:

`DEVICE_SWEEP_COMPLETE_6_OF_6__RAWCB_AND_HALOUTPUTBUFFERCOMBINED_INT32_VALUES_0_2_3_ACCEPTED_AND_ATTACHED__NO_MEASURABLE_CAMERA5_RAW_ENVELOPE_OR_POPULATED_PREFIX_TOPOLOGY_DIFFERENTIAL`

Combining v0.31 with the earlier accepted numeric `1` and UNSET references gives the following bounded tested value domain for each INT32 control:

`{UNSET, 0, 1, 2, 3}`

Within that tested domain, neither `RawCbSourceType` nor `HALOutputBufferCombined`, when exercised on the tested Camera-5 route, produced a measurable change in the app-visible RAW envelope or populated-prefix topology.

This is stronger than the completed v0.30 binary result for these two controls because explicit zero is now distinguished from UNSET, and values `2` and `3` are also proven accepted/attached.

## What is not proven

The result does **not** prove:

- that numeric values `0`, `1`, `2`, or `3` have any particular vendor enum meaning;
- that explicit zero is semantically equivalent to UNSET;
- that values outside `0..3` cannot matter;
- that the two controls are globally ineffective in other route contexts;
- that another session/request prerequisite is not required for their semantic effect;
- that other upstream vendor controls cannot change the route;
- untouched/native photodiode ADC output;
- native physical sensor geometry;
- exact electrical binning/remosaic mechanism;
- 200 MP optical resolution.

## Research consequence

Do not continue blindly enumerating arbitrary larger INT32 values without new evidence about the value domain. The current tested small-value domain is already negative for the topology response.

The next useful route investigation should move to either:

1. a new upstream vendor-route family, with native type resolved before intervention; or
2. a context/prerequisite experiment that tests whether these accepted INT32 controls only become effective when paired with a separately justified route-enabling condition.

Candidate names remain observations only. Any future candidate must pass the same representation-oracle and provenance discipline before intervention.

## Authority

TruthRaw v0.20 remains source/payload authority:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

v0.31 extends route-control evidence only. It does not modify the Scientific Master or increase source authority.
