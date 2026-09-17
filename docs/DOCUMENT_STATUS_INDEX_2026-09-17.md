# TruthRaw document status index — 2026-09-17

Status: **CURRENT DOCUMENT GOVERNANCE INDEX** for the 2026-09-17 Camera-5 line through the completed v0.30 full-factorial matrix and completed v0.31 INT32 value-domain device sweep.

This index supersedes `docs/DOCUMENT_STATUS_INDEX_2026-09-16.md` as the current navigation/governance index. Historical indexes remain preserved as historical state.

## A. Global current reading order

1. `README.md`
2. `START_HERE_NEW_CHAT.md`
3. `docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md`
4. `docs/CURRENT_SCENE_PHYSICS_RESTORATION_200MP_2026-09-16.md`
5. `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md`
6. `state/CURRENT_CAMERA5_RAW_ROUTE_STATE_2026-09-17.json`
7. `docs/CURRENT_CAMERA5_ROUTE_EXPERIMENT_2026-09-17.md`
8. `docs/HONOR_CAMERA5_V031_INT32_VALUE_DOMAIN_SWEEP_DEVICE_RESULT_2026-09-17.md`
9. `state/CAMERA5_V031_INT32_VALUE_DOMAIN_SWEEP_STATE_2026-09-17.json`
10. `docs/HONOR_CAMERA5_V031_INT32_VALUE_DOMAIN_SWEEP_BUILD_2026-09-17.md`
11. `docs/HONOR_CAMERA5_V030_VENDOR_ROUTE_MATRIX_COMPLETE_DEVICE_RESULT_2026-09-17.md`
12. `state/CAMERA5_V030_VENDOR_ROUTE_FULL_FACTORIAL_MATRIX_COMPLETE_STATE_2026-09-17.json`
13. `docs/DOCUMENT_STATUS_INDEX_2026-09-17.md`
14. `docs/handoff/TRUTHRAW_CAMERA5_RAW_ROUTE_V014_TO_V020_2026-09-17.md`
15. exact canonical/research/module documents relevant to the task.

Historical partial v0.30 matrix documents and failed builds remain provenance, not current completion authority.

## B. Current Camera-5 authority chain

Read Camera-5 work in this order:

1. `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md` — current bounded source/payload interpretation through completed v0.31 route-control evidence;
2. `state/CURRENT_CAMERA5_RAW_ROUTE_STATE_2026-09-17.json` — machine-readable current route state;
3. `docs/CURRENT_CAMERA5_ROUTE_EXPERIMENT_2026-09-17.md` — current route-control conclusion and next direction;
4. `docs/HONOR_CAMERA5_V031_INT32_VALUE_DOMAIN_SWEEP_DEVICE_RESULT_2026-09-17.md` — completed six-profile v0.31 device result;
5. `state/CAMERA5_V031_INT32_VALUE_DOMAIN_SWEEP_STATE_2026-09-17.json` — machine-readable v0.31 completion state;
6. `docs/HONOR_CAMERA5_V031_INT32_VALUE_DOMAIN_SWEEP_BUILD_2026-09-17.md` — v0.31 build/protocol and failed-build provenance;
7. `docs/HONOR_CAMERA5_V030_VENDOR_ROUTE_MATRIX_COMPLETE_DEVICE_RESULT_2026-09-17.md` — completed predecessor matrix;
8. `state/CAMERA5_V030_VENDOR_ROUTE_FULL_FACTORIAL_MATRIX_COMPLETE_STATE_2026-09-17.json` — machine-readable v0.30 completion state;
9. `docs/HONOR_MAGIC8_PRO_TELE_200MP_FULL_RAW_V014_2026-09-17.md` — source-first acquisition authority;
10. `docs/HONOR_CAMERA5_TWO_DOOR_AIRLOCK_V017_2026-09-17.md` — request-side + post-HAL provenance architecture;
11. `docs/HONOR_MAGIC8_PRO_TELE_PAYLOAD_GEOMETRY_V020_2026-09-17.md` — completed v0.19/v0.20 payload result;
12. versioned route-control documents v0.21 onward;
13. `docs/handoff/TRUTHRAW_CAMERA5_RAW_ROUTE_V014_TO_V020_2026-09-17.md` — historical/reproduction map.

## C. Current Camera-5 bounded claim

Source/payload authority remains:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

Descriptive shorthand:

`APP_VISIBLE_4080x3072_BAYER_LIKE_RAW_PAYLOAD_EMBEDDED_IN_16320x12288_HAL_ENVELOPE`

The tested route delivers a `16320x12288` app-visible RAW_SENSOR/HardwareBuffer envelope while the tested captures populate an exact `25,067,520`-byte prefix. That prefix uniquely matches the runtime-advertised `4080x3072` RAW_SENSOR U16 byte count and is retained only as a bounded app-visible payload interpretation.

This does **not** prove untouched native ADC, native physical sensor geometry, exact binning/remosaic mechanism or 200 MP optical resolution.

## D. Trusted acquisition/content implementation lineage

Trusted acquisition/content chain:

- v0.14 source-first seal;
- v0.16 post-HAL HardwareBuffer envelope;
- v0.17 two-door request/delivery airlock;
- v0.19 full-raster write audit;
- v0.20 payload geometry decoder.

Historical/rejected/isolated:

- v0.15 direct-open physical Camera-5 route — rejected on this device as trusted acquisition lineage;
- v0.18 physical-focus probe — isolated parallel research, not parent of v0.19/v0.20.

Later route-control experiments do not replace v0.20 as source/payload authority unless new device evidence genuinely changes the bounded source/topology claim.

## E. Representation authorities and early single-factor results

Native representations resolved on-device:

- v0.23 `EnableIdealRAW`: BYTE / `0x801F0027`;
- v0.25 `RawCbSourceType`: INT32 / `0x801F0009`;
- v0.27 `EnableXCFAOptimization`: BYTE / `0x801F0036`;
- v0.29 `HALOutputBufferCombined`: INT32 / `0x801F0034`.

Accepted single-variable numeric-one interventions with no measured RAW-envelope/populated-payload topology change:

- v0.24 `EnableIdealRAW=BYTE(1)`;
- v0.26 `RawCbSourceType=INT32(1)`;
- v0.28 `EnableXCFAOptimization=BYTE(1)`.

Vendor names and numeric values remain non-semantic unless separately proven.

## F. Completed v0.30 interaction screen

The complete matrix screened all 16 UNSET-vs-numeric-one combinations across:

- A `EnableIdealRAW` / BYTE;
- B `RawCbSourceType` / INT32;
- C `EnableXCFAOptimization` / BYTE;
- D `HALOutputBufferCombined` / INT32.

All 16 unique profiles remain in the same measured topology class as v0.20:

- source envelope `401,080,320` bytes / `16320x12288`;
- first `768` declared rows populated;
- remaining `11,520` rows zero;
- populated/payload size `25,067,520` bytes;
- unique advertised standard RAW byte match `4080x3072`;
- no measured envelope/populated-prefix topology differential.

Bounded conclusion:

`WITHIN_THE_TESTED_BINARY_DESIGN__UNSET_VS_NUMERIC_ONE_AT_THE_RESOLVED_NATIVE_TYPES__NO_SINGLE_FACTOR_OR_COMBINATION_OF_ENABLEIDEALRAW_RAWCBSOURCETYPE_ENABLEXCFAOPTIMIZATION_HALOUTPUTBUFFERCOMBINED_CHANGED_THE_MEASURED_CAMERA5_RAW_ENVELOPE_OR_POPULATED_PREFIX_TOPOLOGY`

The v0.30 binary experiment is complete and should not be repeated.

## G. Completed v0.31 INT32 value-domain sweep

Experiment:

`CAMERA5_INT32_VALUE_DOMAIN_SWEEP_0_2_3`

Profiles:

- S01 `RawCbSourceType=0`;
- S02 `RawCbSourceType=2`;
- S03 `RawCbSourceType=3`;
- S04 `HALOutputBufferCombined=0`;
- S05 `HALOutputBufferCombined=2`;
- S06 `HALOutputBufferCombined=3`.

`EnableIdealRAW` and `EnableXCFAOptimization` remained UNSET in every v0.31 run. Exactly one unknown vendor key was written per run.

All six profiles passed builder set/readback, built-request readback and session-parameter attachment. All six also reproduced the v0.20 topology class:

- source envelope `401,080,320` bytes / `16320x12288`;
- first `768` rows populated and `11,520` rows zero;
- Stage 3.6 `ONLY_FIRST_768_ROWS_NONZERO__EXACT_12P5MP_BYTE_PAYLOAD_SIGNATURE`;
- Stage 3.7 payload `25,067,520` bytes / `12,533,760` U16 samples;
- unique advertised standard RAW byte match `4080x3072`;
- no measured RAW-envelope/populated-prefix topology differential.

The returned coarse route-side observations also remained invariant in class: HONOR `binningFactor=4`, AEC crop prefix `[11,8,4058,3055]`, full `allISPCropWindow`, `isInSensorZoom=0`, and Android `rawBinningFactorUsed=true`.

Combining v0.31 with prior UNSET/numeric-one evidence gives the bounded tested domain `{UNSET,0,1,2,3}` for both INT32 controls on the tested route, with no measured topology differential.

Bounded conclusion:

`DEVICE_SWEEP_COMPLETE_6_OF_6__RAWCB_AND_HALOUTPUTBUFFERCOMBINED_INT32_VALUES_0_2_3_ACCEPTED_AND_ATTACHED__NO_MEASURABLE_CAMERA5_RAW_ENVELOPE_OR_POPULATED_PREFIX_TOPOLOGY_DIFFERENTIAL`

This does not identify enum meanings, does not prove explicit zero equals UNSET semantically, and does not rule out values outside the tested domain or another route context.

## H. v0.31 build provenance

Branch:

`integration/truthraw-suite-v0-31-int32-value-domain-sweep`

Successful build:

- run `35274828567`;
- job `105382770948`;
- head `899e38abf87b2e4712cd60142cf2aeac9acd9e8b`;
- APK SHA-256 `3125ddab46232c46680cefc67ae2e729495be88ba17cb15d2681d2e5abaf3bd3`;
- artifact ID `10520386544`;
- artifact ZIP SHA-256 `1af039f1fb79640d65f4a696c8f8c52e8102945b8099b8b917d5c6c63f2b20c1`.

The first v0.31 run failed before compilation on a case-sensitive inherited bundle-version assertion. It produced no device/scientific result and remains provenance.

## I. Current open problem

The tested four-key binary plane and the small INT32 value domain for B/D are both negative for the measured topology response.

Current open problem:

`OPEN_NEEDS_ROUTE_DIFFERENTIAL_TO_ESTABLISH_WHETHER_A_DIFFERENT_HONOR_QTI_CONTROL_PATH_OR_CONTEXT_CAN_POPULATE_A_LARGER_NATIVE_OR_APP_VISIBLE_RAW_DOMAIN`

Independently:

`OPEN_NEEDS_CALIBRATION_AND_OPTICAL_EVIDENCE_BEFORE_ANY_NATIVE_ADC_OR_200MP_OPTICAL_PROMOTION`.

## J. Current next action

Do not blindly enumerate larger integers.

The next scientifically useful route work should either:

- run a **multi-key native-type oracle screen** over a new upstream vendor-route family without session/capture submission, then intervene only on type-resolved candidates; or
- test a **separately justified context/prerequisite** if evidence suggests an already-accepted key only acts under another route state.

Candidate names are clues only and never semantic authority.

## K. Camera-5 evidence interpretation rules

Current rules:

- file/buffer size is not evidence that every declared raster position is populated;
- DNG is auxiliary and cannot override Plane[0] source evidence;
- `.rawpayload` is an exact-prefix derived view, not a replacement source;
- diagnostic PNG is appearance-only;
- min/max codes alone do not prove calibrated black level or ADC bit depth;
- HONOR/QTI metadata is route evidence, not automatic semantic/calibration authority;
- vendor key names do not establish semantics;
- numeric values do not establish enum meaning merely by being accepted;
- explicit zero is not automatically semantically equivalent to UNSET;
- different capture hashes do not establish route differences;
- failed/rejected experiments remain provenance.

## L. Editing rule going forward

When new route evidence changes the current Camera-5 interpretation:

- update `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md` or create a dated successor when substantial;
- update `state/CURRENT_CAMERA5_RAW_ROUTE_STATE_2026-09-17.json` if source/payload authority changes;
- update `docs/CURRENT_CAMERA5_ROUTE_EXPERIMENT_2026-09-17.md`;
- create/update the versioned experiment result/state;
- preserve previous branches/builds/evidence rather than rewriting them away;
- update this index/bootstrap;
- never widen scientific authority beyond actual source and calibration evidence.
