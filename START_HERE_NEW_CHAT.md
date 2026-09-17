# START HERE — TruthRaw current bootstrap

This is the living bootstrap entry point for the consolidated TruthRaw research state, updated through the completed HONOR Camera-5 v0.30 full-factorial route matrix and the **v0.31 INT32 value-domain sweep build** on 2026-09-17.

It is a navigation/current-state document. It does not rewrite frozen historical evidence, dated handoffs, rejected experiments or canonical bytes.

## Mandatory current reading order

1. `docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md`
2. `docs/CURRENT_SCENE_PHYSICS_RESTORATION_200MP_2026-09-16.md`
3. `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md`
4. `state/CURRENT_CAMERA5_RAW_ROUTE_STATE_2026-09-17.json`
5. `docs/CURRENT_CAMERA5_ROUTE_EXPERIMENT_2026-09-17.md`
6. `docs/HONOR_CAMERA5_V031_INT32_VALUE_DOMAIN_SWEEP_BUILD_2026-09-17.md`
7. `state/CAMERA5_V031_INT32_VALUE_DOMAIN_SWEEP_STATE_2026-09-17.json`
8. `docs/HONOR_CAMERA5_V030_VENDOR_ROUTE_MATRIX_COMPLETE_DEVICE_RESULT_2026-09-17.md`
9. `state/CAMERA5_V030_VENDOR_ROUTE_FULL_FACTORIAL_MATRIX_COMPLETE_STATE_2026-09-17.json`
10. `docs/DOCUMENT_STATUS_INDEX_2026-09-17.md`
11. `docs/handoff/TRUTHRAW_CAMERA5_RAW_ROUTE_V014_TO_V020_2026-09-17.md`
12. only then exact canonical/research/module documents relevant to the task.

Historical partial-matrix state/result documents remain preserved as provenance and are no longer the current matrix completion authority.

## One-sentence current definition

**TruthRaw seals app-visible RAW/CFA observations as immutable source evidence, reconstructs a separate uncertainty- and authority-aware Scientific Master in Free Scientific Space, and permits open-world/counterfactual/restoration/appearance projections only without upgrading what the physical evidence actually measured.**

## Permanent scientific laws

1. Source evidence is immutable.
2. Representation may exceed the source; claims may not exceed evidence.
3. Measured/reconstructed/censored/unknown/counterfactual/appearance remain distinct.
4. Evidence count remains physical and explicit.
5. Censoring is a bound, not a guessed exact value.
6. Uncertainty/support is local and fail-closed.
7. Precision is stage-specific: exact integer/packed evidence first; F64 where branch-sensitive science requires it; F32 only when validated safe.
8. Counterfactual state never becomes capture evidence.
9. Appearance/transport never writes back into science.
10. Restoration never overpaints valid measured support in the Scientific Master.
11. Compute resources never increase truth authority.
12. Vendor metadata and vendor-key names are observations, not semantic/calibration authority by themselves.
13. Failed/rejected experiments remain provenance.

## Current Camera-5 source/payload authority

Trusted acquisition/content lineage:

`v0.14 source-first seal`
`-> v0.16 post-HAL HardwareBuffer envelope`
`-> v0.17 two-door request/delivery airlock`
`-> v0.19 full-raster write audit`
`-> v0.20 payload geometry decoder`.

Trusted route:

`logical 0 -> physical Camera 5 -> MAXIMUM_RESOLUTION declaration -> physical-scoped request -> app-visible 16320x12288 RAW_SENSOR envelope -> original Plane[0] sealed first -> read-only envelope/audit -> payload geometry interpretation`.

Current bounded classification:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

Descriptive shorthand:

`APP_VISIBLE_4080x3072_BAYER_LIKE_RAW_PAYLOAD_EMBEDDED_IN_16320x12288_HAL_ENVELOPE`.

This does not prove untouched ADC, native sensor geometry, exact electrical binning/remosaic or 200 MP optical resolution.

## v0.20 control facts

The tested source envelope is `401,080,320` bytes with `rowStride=32640`, `pixelStride=2` and app-visible geometry `16320x12288`.

Stage 3.6 established that only rows `0..767` are populated; rows `768..12287` are zero:

`16320 * 768 * 2 = 25,067,520 bytes`.

Stage 3.7 found exactly one runtime-advertised standard Camera-5 RAW_SENSOR geometry with the same U16 byte count:

`4080 * 3072 * 2 = 25,067,520 bytes`.

The exact prefix copy forms a coherent full-frame scene under the `4080x3072` interpretation and shows Bayer-like 2x2 structure. The `.rawpayload` remains derived evidence; the sealed 401-MB source remains primary.

## Route-control representation authorities

- v0.23 `EnableIdealRAW`: BYTE, tag `0x801F0027`;
- v0.25 `RawCbSourceType`: INT32, tag `0x801F0009`;
- v0.27 `EnableXCFAOptimization`: BYTE, tag `0x801F0036`;
- v0.29 `HALOutputBufferCombined`: INT32, tag `0x801F0034`.

Accepted single-variable interventions v0.24/v0.26/v0.28 produced no measured RAW envelope/populated-payload topology differential. Numeric `1` remains an experimental control value, not proven vendor semantics.

## Completed v0.30 full-factorial matrix

Design:

`FULL_FACTORIAL_2_LEVEL_4_FACTOR_16_RUN_COMPLEMENT_PAIRED`

Factors (`ABCD`):

- A `EnableIdealRAW` — BYTE;
- B `RawCbSourceType` — INT32;
- C `EnableXCFAOptimization` — BYTE;
- D `HALOutputBufferCombined` — INT32.

Levels:

- low = `UNSET_NO_WRITE`;
- high = numeric `1` using the device-resolved native type.

All 16 unique profiles have individually readable evidence JSONs. Every profile remains in the same measured topology class as v0.20:

- `16320x12288` / `401,080,320`-byte app-visible RAW envelope;
- first `768` rows populated;
- remaining `11,520` rows zero;
- exact populated/payload size `25,067,520` bytes;
- unique advertised standard RAW byte match `4080x3072`;
- no measured envelope/populated-prefix topology differential.

Current bounded matrix conclusion:

`WITHIN_THE_TESTED_BINARY_DESIGN__UNSET_VS_NUMERIC_ONE_AT_THE_RESOLVED_NATIVE_TYPES__NO_SINGLE_FACTOR_OR_COMBINATION_OF_ENABLEIDEALRAW_RAWCBSOURCETYPE_ENABLEXCFAOPTIMIZATION_HALOUTPUTBUFFERCOMBINED_CHANGED_THE_MEASURED_CAMERA5_RAW_ENVELOPE_OR_POPULATED_PREFIX_TOPOLOGY`

The four-key 0/1 experimental space is complete and should not be repeated.

## Current experiment — v0.31 INT32 value-domain sweep

v0.31 changes the tested numeric domain instead of repeating binary combinations.

Experiment:

`CAMERA5_INT32_VALUE_DOMAIN_SWEEP_0_2_3`

Only the two INT32 controls are exercised, one unknown vendor key per run:

- S01 `RawCbSourceType=0`;
- S02 `RawCbSourceType=2`;
- S03 `RawCbSourceType=3`;
- S04 `HALOutputBufferCombined=0`;
- S05 `HALOutputBufferCombined=2`;
- S06 `HALOutputBufferCombined=3`.

A=`EnableIdealRAW` and C=`EnableXCFAOptimization` remain UNSET throughout. Numeric `1` and UNSET are not repeated because they were already screened by v0.30.

Values `0`, `2`, and `3` are experimental stimuli only. No vendor enum or semantic meaning is assumed.

Build status: **SUCCESS — DEVICE RESULTS PENDING**.

Branch:

`integration/truthraw-suite-v0-31-int32-value-domain-sweep`

Successful build provenance:

- run `35274828567`;
- job `105382770948`;
- head `899e38abf87b2e4712cd60142cf2aeac9acd9e8b`;
- APK SHA-256 `3125ddab46232c46680cefc67ae2e729495be88ba17cb15d2681d2e5abaf3bd3`;
- artifact ID `10520386544`;
- artifact ZIP SHA-256 `1af039f1fb79640d65f4a696c8f8c52e8102945b8099b8b917d5c6c63f2b20c1`.

The first v0.31 build attempt is preserved as failed-build provenance; it failed before compilation on an uppercase bundle-version assertion and produced no device/scientific result.

The v0.31 fixed-control UI retains first-missing recovery and the source-first v0.20 acquisition/audit ordering.

## Immediate device action

Run S01 through S06 and save each evidence JSON individually. If a value produces a topology differential, stop broad sweeping and narrow that region. If a value is rejected/blocked, preserve the failure rather than inferring semantics.

## Current open problem

`OPEN_NEEDS_ROUTE_DIFFERENTIAL_TO_ESTABLISH_WHETHER_A_DIFFERENT_HONOR_QTI_CONTROL_PATH_OR_VALUE_DOMAIN_CAN_POPULATE_A_LARGER_NATIVE_OR_APP_VISIBLE_RAW_DOMAIN`

Independently:

`OPEN_NEEDS_CALIBRATION_AND_OPTICAL_EVIDENCE_BEFORE_ANY_NATIVE_ADC_OR_200MP_OPTICAL_PROMOTION`.

## Precision and governance continuity

Current precision direction remains:

`exact RAW integer/packed evidence -> integer-exact topology/content audit -> F64 branch-sensitive science -> controlled F32 only where validated`.

TruthRange/zero-line remains downstream of acquisition and calibration. RAW code zero, BlackLevel, display black and the TruthRange zero-line are different concepts.

Governance remains unchanged: preserve old Camera-5 documents as provenance, keep v0.15 direct-open rejection and v0.18 focus work isolated, do not let GCam/computational RAW determine TruthRaw evidence authority, make no silent license changes, and keep `canonical/ptc/v1.1` meaning **Pure Truth Certificate**.
