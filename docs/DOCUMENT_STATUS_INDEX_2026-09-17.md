# TruthRaw document status index — 2026-09-17

Status: **CURRENT DOCUMENT GOVERNANCE INDEX** for the 2026-09-17 Camera-5 line through the completed v0.30 full-factorial device matrix.

This index supersedes `docs/DOCUMENT_STATUS_INDEX_2026-09-16.md` as the current navigation/governance index. Historical indexes remain preserved as historical state.

## A. Global current reading order

1. `README.md`
2. `START_HERE_NEW_CHAT.md`
3. `docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md`
4. `docs/CURRENT_SCENE_PHYSICS_RESTORATION_200MP_2026-09-16.md`
5. `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md`
6. `state/CURRENT_CAMERA5_RAW_ROUTE_STATE_2026-09-17.json`
7. `docs/CURRENT_CAMERA5_ROUTE_EXPERIMENT_2026-09-17.md`
8. `docs/HONOR_CAMERA5_V030_VENDOR_ROUTE_MATRIX_COMPLETE_DEVICE_RESULT_2026-09-17.md`
9. `state/CAMERA5_V030_VENDOR_ROUTE_FULL_FACTORIAL_MATRIX_COMPLETE_STATE_2026-09-17.json`
10. `docs/DOCUMENT_STATUS_INDEX_2026-09-17.md`
11. `docs/handoff/TRUTHRAW_CAMERA5_RAW_ROUTE_V014_TO_V020_2026-09-17.md`
12. exact canonical/research/module documents relevant to the task.

Historical partial v0.30 matrix documents remain provenance, not current completion authority.

## B. Current Camera-5 authority chain

Read Camera-5 work in this order:

1. `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md` — current bounded interpretation through completed v0.30 matrix;
2. `state/CURRENT_CAMERA5_RAW_ROUTE_STATE_2026-09-17.json` — machine-readable current source/payload and route-control state;
3. `docs/CURRENT_CAMERA5_ROUTE_EXPERIMENT_2026-09-17.md` — current route-control experiment conclusion;
4. `docs/HONOR_CAMERA5_V030_VENDOR_ROUTE_MATRIX_COMPLETE_DEVICE_RESULT_2026-09-17.md` — complete 16-profile device result;
5. `state/CAMERA5_V030_VENDOR_ROUTE_FULL_FACTORIAL_MATRIX_COMPLETE_STATE_2026-09-17.json` — machine-readable complete matrix state;
6. `docs/HONOR_MAGIC8_PRO_TELE_200MP_FULL_RAW_V014_2026-09-17.md` — source-first acquisition authority;
7. `docs/HONOR_CAMERA5_TWO_DOOR_AIRLOCK_V017_2026-09-17.md` — request-side + post-HAL provenance architecture;
8. `docs/HONOR_MAGIC8_PRO_TELE_PAYLOAD_GEOMETRY_V020_2026-09-17.md` — completed v0.19/v0.20 payload result;
9. versioned route-control documents v0.21 onward;
10. `docs/handoff/TRUTHRAW_CAMERA5_RAW_ROUTE_V014_TO_V020_2026-09-17.md` — historical/reproduction map.

## C. Current Camera-5 bounded claim

Source/payload authority remains:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

Descriptive shorthand:

`APP_VISIBLE_4080x3072_BAYER_LIKE_RAW_PAYLOAD_EMBEDDED_IN_16320x12288_HAL_ENVELOPE`

The tested route delivers a `16320x12288` app-visible RAW_SENSOR/HardwareBuffer envelope, while only the first `25,067,520` bytes are populated. Those bytes are an exact source prefix, uniquely match the runtime-advertised standard `4080x3072` RAW_SENSOR byte count, render as a coherent full-frame scene under that interpretation, and exhibit Bayer-like 2x2 spatial structure.

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

## E. Route-control representation and single-factor results

Native representations resolved on-device:

- v0.23 `EnableIdealRAW`: BYTE / `0x801F0027`;
- v0.25 `RawCbSourceType`: INT32 / `0x801F0009`;
- v0.27 `EnableXCFAOptimization`: BYTE / `0x801F0036`;
- v0.29 `HALOutputBufferCombined`: INT32 / `0x801F0034`.

Accepted single-variable interventions with no measured RAW-envelope/populated-payload topology change:

- v0.24 `EnableIdealRAW=BYTE(1)`;
- v0.26 `RawCbSourceType=INT32(1)`;
- v0.28 `EnableXCFAOptimization=BYTE(1)`.

Numeric `1` remains a type-valid experimental control value, not proven vendor semantics.

## F. v0.30 complete interaction screen

The complete matrix screened all 16 combinations of four factors:

- A `EnableIdealRAW` / BYTE;
- B `RawCbSourceType` / INT32;
- C `EnableXCFAOptimization` / BYTE;
- D `HALOutputBufferCombined` / INT32.

Matrix low = `UNSET_NO_WRITE`. Matrix high = numeric `1` in the resolved native representation.

All 16 unique profiles now have individually readable device evidence JSONs. All eight complement pairs are represented.

## G. Complete v0.30 device result

All 16 profiles remain in the same measured topology class as v0.20:

- source envelope `401,080,320` bytes / `16320x12288`;
- first `768` declared rows populated;
- remaining `11,520` rows zero;
- populated/payload size `25,067,520` bytes;
- unique advertised standard RAW byte match `4080x3072`;
- no measured envelope/populated-prefix topology differential.

This includes:

- all four single-high profiles, including D-only R09 `0001`;
- all six two-factor profiles;
- all four three-factor profiles;
- all-UNSET R01 `0000`;
- all-four-high R02 `1111`.

For the measured categorical topology response, the complete binary design is invariant. Therefore no single-factor or interaction-level topology differential is observed within the tested domain.

Bounded conclusion:

`WITHIN_THE_TESTED_BINARY_DESIGN__UNSET_VS_NUMERIC_ONE_AT_THE_RESOLVED_NATIVE_TYPES__NO_SINGLE_FACTOR_OR_COMBINATION_OF_ENABLEIDEALRAW_RAWCBSOURCETYPE_ENABLEXCFAOPTIMIZATION_HALOUTPUTBUFFERCOMBINED_CHANGED_THE_MEASURED_CAMERA5_RAW_ENVELOPE_OR_POPULATED_PREFIX_TOPOLOGY`

## H. v0.30e recovery/UI provenance

The original v0.30 UI and scroll-based v0.30c/v0.30d variants were not reliably usable because long Stage-3 output displaced essential controls.

v0.30e fixed essential controls above the detail pane and added first-missing recovery without changing the scientific matrix, acquisition order, source-seal order, Stage 3.6 or Stage 3.7.

The complete matrix was ultimately recovered and completed with individually readable evidence JSONs.

## I. Historical partial-matrix records

These remain preserved as provenance:

- `docs/HONOR_CAMERA5_V030_VENDOR_ROUTE_MATRIX_PARTIAL_DEVICE_RESULTS_2026-09-17.md`;
- `state/CAMERA5_V030_PARTIAL_MATRIX_V030E_FIXED_CONTROLS_STATE_2026-09-17.json`.

They are superseded for matrix completion status by the complete result/state documents.

## J. Current open problem

The binary 0/1 interaction space for these four selected vendor controls is now complete and produced no measured route/topology differential.

The current open problem becomes:

`OPEN_NEEDS_VALUE_DOMAIN_SEMANTICS_OR_NEW_UPSTREAM_ROUTE_CONTROL_TO_ESTABLISH_WHETHER_A_DIFFERENT_HONOR_QTI_PATH_CAN_POPULATE_A_LARGER_NATIVE_OR_APP_VISIBLE_RAW_DOMAIN`

Independently:

`OPEN_NEEDS_CALIBRATION_AND_OPTICAL_EVIDENCE_BEFORE_ANY_NATIVE_ADC_OR_200MP_OPTICAL_PROMOTION`.

## K. Current next research direction

Do not repeat the same four-key binary matrix.

Next scientifically useful work should either:

- resolve the actual value domain / enum semantics of the INT32 controls `RawCbSourceType` and `HALOutputBufferCombined` before trying other numeric values; or
- identify new upstream route candidates, resolve native representation first, then intervene.

## L. Camera-5 evidence interpretation rules

Current rules:

- file/buffer size is not evidence that every declared raster position is populated;
- DNG is auxiliary and cannot override Plane[0] source evidence;
- `.rawpayload` is an exact-prefix derived view, not a replacement source;
- diagnostic PNG is appearance-only;
- min/max codes alone do not prove calibrated black level or ADC bit depth;
- HONOR/QTI metadata is route evidence, not automatic semantic/calibration authority;
- vendor key names do not establish semantics;
- numeric value `1` does not by itself mean enable/full/native/unbinned;
- matrix low means UNSET, not explicit zero;
- a matrix hit would establish a combination-level differential first;
- different capture hashes do not establish route differences;
- failed/rejected experiments remain provenance.

## M. Editing rule going forward

When new route evidence changes the current Camera-5 interpretation:

- update `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md` or create a dated successor when substantial;
- update `state/CURRENT_CAMERA5_RAW_ROUTE_STATE_2026-09-17.json` if source/payload authority changes;
- update `docs/CURRENT_CAMERA5_ROUTE_EXPERIMENT_2026-09-17.md`;
- create/update the versioned experiment result/state;
- preserve previous branches/builds/evidence rather than rewriting them away;
- update this index/bootstrap;
- never widen scientific authority beyond actual source and calibration evidence.
