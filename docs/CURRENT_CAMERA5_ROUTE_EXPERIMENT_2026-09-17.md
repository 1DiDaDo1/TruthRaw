# CURRENT Camera-5 route-control experiment — 2026-09-17

Status: **v0.30 BINARY MATRIX COMPLETE; v0.31 INT32 VALUE-DOMAIN SWEEP BUILD SUCCESS / DEVICE RESULTS PENDING; v0.20 REMAINS SOURCE/PAYLOAD AUTHORITY**

This document tracks upstream HONOR/QTI route-control research after the completed v0.20 payload-topology result. It does not replace `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md` for source authority.

Permanent boundary:

`APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF`

## Control authority

TruthRaw v0.20 remains the source/payload authority:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

Measured v0.20 topology:

- app-visible envelope `16320x12288`, `401,080,320` bytes;
- row stride `32,640`, pixel stride `2`;
- first `768` declared rows populated;
- remaining `11,520` rows zero;
- populated source prefix `25,067,520` bytes;
- unique advertised standard RAW U16 byte match `4080x3072`.

## Representation authorities

- v0.23 `EnableIdealRAW`: tag `0x801F0027`, native `BYTE`;
- v0.25 `RawCbSourceType`: tag `0x801F0009`, native `INT32`;
- v0.27 `EnableXCFAOptimization`: tag `0x801F0036`, native `BYTE`;
- v0.29 `HALOutputBufferCombined`: tag `0x801F0034`, native `INT32`.

The oracle builds establish representation only. Vendor-key names and numeric values are not promoted to semantics.

## Completed v0.30 binary interaction screen

Experiment:

`CAMERA5_VENDOR_ROUTE_FULL_FACTORIAL_2_LEVEL_4_FACTOR`

All 16 combinations of UNSET versus numeric `1` across the four type-resolved controls were captured with individually readable evidence JSONs.

Every profile remained in the same measured route/topology class as v0.20:

- physical Camera `5`;
- `16320x12288` app-visible envelope;
- `401,080,320` source bytes;
- first `768` rows populated;
- `11,520` rows zero;
- `25,067,520` payload bytes;
- unique advertised standard RAW byte match `4080x3072`;
- no measured RAW-envelope/populated-prefix topology differential.

Bounded v0.30 conclusion:

`WITHIN_THE_TESTED_BINARY_DESIGN__UNSET_VS_NUMERIC_ONE_AT_THE_RESOLVED_NATIVE_TYPES__NO_SINGLE_FACTOR_OR_COMBINATION_OF_ENABLEIDEALRAW_RAWCBSOURCETYPE_ENABLEXCFAOPTIMIZATION_HALOUTPUTBUFFERCOMBINED_CHANGED_THE_MEASURED_CAMERA5_RAW_ENVELOPE_OR_POPULATED_PREFIX_TOPOLOGY`

The 0/1 matrix is therefore closed and should not be repeated.

Detailed result:

`docs/HONOR_CAMERA5_V030_VENDOR_ROUTE_MATRIX_COMPLETE_DEVICE_RESULT_2026-09-17.md`

Machine state:

`state/CAMERA5_V030_VENDOR_ROUTE_FULL_FACTORIAL_MATRIX_COMPLETE_STATE_2026-09-17.json`

## Current experiment — v0.31 INT32 value-domain sweep

Because v0.30 found no topology differential anywhere in the binary interaction plane, v0.31 changes the independent variable rather than firing more 0/1 combinations.

Experiment:

`CAMERA5_INT32_VALUE_DOMAIN_SWEEP_0_2_3`

Only the two INT32 controls are exercised:

- B = `RawCbSourceType`, native INT32, tag `0x801F0009`;
- D = `HALOutputBufferCombined`, native INT32, tag `0x801F0034`.

The BYTE factors A=`EnableIdealRAW` and C=`EnableXCFAOptimization` remain UNSET in every v0.31 run.

v0.31 tests six profiles:

- S01 `RawCbSourceType=0`;
- S02 `RawCbSourceType=2`;
- S03 `RawCbSourceType=3`;
- S04 `HALOutputBufferCombined=0`;
- S05 `HALOutputBufferCombined=2`;
- S06 `HALOutputBufferCombined=3`.

Numeric `1` and UNSET are not repeated because v0.30 already screened them. Values `0`, `2`, and `3` are stimuli only; no enum or semantic interpretation is assigned.

Exactly one unknown vendor key is written per v0.31 run. Builder readback, built-request readback and session-parameter attachment must all match the requested value before the capture route proceeds.

## v0.31 source/provenance invariants

v0.31 preserves the source-first research chain:

`Gate A untouched route observation`
`-> one controlled INT32 vendor stimulus`
`-> session support check`
`-> physical-5/MAX session`
`-> physical-scoped still capture`
`-> original Plane[0] sealed first`
`-> read-only post-HAL envelope observation`
`-> closed/sealed Stage 3.6 raster audit`
`-> Stage 3.7 payload decoder`.

The response of interest remains route/topology, not cross-scene pixel-value equality.

## v0.31 build provenance

Branch:

`integration/truthraw-suite-v0-31-int32-value-domain-sweep`

New helper:

`suite_android/app/src/main/java/com/truthraw/adaptiveui/Camera2Int32ValueDomainSweep.kt`

The first v0.31 workflow run is retained as failed-build provenance:

- run `35274679023`;
- head `84f4b320abbf57550dc5d4672d3c675bbcd93758`;
- failed before compilation because the inherited bundle filename used uppercase `V030`, while the first patch's version replacement handled lowercase `v030` only;
- no scientific/device result was produced.

Corrected patch:

`tools/patch_fotograaf_v031b_int32_value_domain_sweep.py`

Successful build:

- run `35274828567`;
- job `105382770948`;
- head `899e38abf87b2e4712cd60142cf2aeac9acd9e8b`;
- result **SUCCESS**;
- APK bytes `4,932,129`;
- APK SHA-256 `3125ddab46232c46680cefc67ae2e729495be88ba17cb15d2681d2e5abaf3bd3`;
- artifact ID `10520386544`;
- artifact ZIP bytes `1,611,249`;
- artifact ZIP SHA-256 `1af039f1fb79640d65f4a696c8f8c52e8102945b8099b8b917d5c6c63f2b20c1`.

Workflow assertions verified the six exact profiles, one unknown vendor key per run, INT32 representation, readback/attachment path, no A/C writes, source sealing before envelope observation, and unchanged Stage 3.6 -> Stage 3.7 ordering.

Detailed build record:

`docs/HONOR_CAMERA5_V031_INT32_VALUE_DOMAIN_SWEEP_BUILD_2026-09-17.md`

Machine state:

`state/CAMERA5_V031_INT32_VALUE_DOMAIN_SWEEP_STATE_2026-09-17.json`

## Immediate device action

Run v0.31 S01 through S06. Save each evidence JSON individually. The fixed-control UI and first-missing recovery behavior are retained so essential capture/export controls do not depend on scrolling.

If a profile is blocked or fails session/capture, preserve that failure and do not promote the numeric value to a semantic conclusion.

If a profile produces a topology differential, stop broad sweeping and narrow that value/route region with a follow-up experiment.

If all six profiles reproduce v0.20 topology, the bounded negative domain expands to tested INT32 values `0,1,2,3` plus UNSET for B and D, but still does not prove other INT32 values or other vendor controls are ineffective.

## Authority boundary

v0.31 does not prove:

- vendor enum meanings;
- that value `0` means off/default;
- that values `2` or `3` select named modes;
- global ineffectiveness of either INT32 key;
- untouched/native ADC output;
- native sensor geometry;
- exact electrical binning/remosaic;
- 200 MP optical resolution.

v0.20 remains source/payload authority until a future experiment produces a genuine bounded route differential.
