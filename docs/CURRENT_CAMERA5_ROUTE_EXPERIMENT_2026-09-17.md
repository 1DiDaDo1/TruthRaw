# CURRENT Camera-5 route-control experiment — 2026-09-17

Status: **v0.30 BINARY MATRIX COMPLETE; v0.31 INT32 VALUE-DOMAIN SWEEP COMPLETE 6/6; NO MEASURABLE RAW-TOPOLOGY DIFFERENTIAL; v0.20 REMAINS SOURCE/PAYLOAD AUTHORITY**

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

The 0/1 matrix is closed and should not be repeated.

Detailed result:

`docs/HONOR_CAMERA5_V030_VENDOR_ROUTE_MATRIX_COMPLETE_DEVICE_RESULT_2026-09-17.md`

Machine state:

`state/CAMERA5_V030_VENDOR_ROUTE_FULL_FACTORIAL_MATRIX_COMPLETE_STATE_2026-09-17.json`

## Completed v0.31 INT32 value-domain sweep

Experiment:

`CAMERA5_INT32_VALUE_DOMAIN_SWEEP_0_2_3`

Only the two INT32 controls were exercised:

- B = `RawCbSourceType`, native INT32, tag `0x801F0009`;
- D = `HALOutputBufferCombined`, native INT32, tag `0x801F0034`.

The BYTE factors A=`EnableIdealRAW` and C=`EnableXCFAOptimization` remained UNSET in every v0.31 run.

Six device profiles were captured:

- S01 `RawCbSourceType=0`;
- S02 `RawCbSourceType=2`;
- S03 `RawCbSourceType=3`;
- S04 `HALOutputBufferCombined=0`;
- S05 `HALOutputBufferCombined=2`;
- S06 `HALOutputBufferCombined=3`.

Numeric `1` and UNSET were already represented by the completed v0.30/v0.26 line. Values `0`, `2`, and `3` were treated only as numeric stimuli.

## v0.31 intervention result

All six v0.31 profiles passed the intervention path:

- target key advertised as logical/physical request and session key;
- exactly one unknown vendor key written per run;
- no other unknown vendor factor written;
- builder readback before set was null;
- requested value `0`, `2`, or `3` was accepted;
- builder readback matched;
- built-request readback matched;
- session parameters attached successfully;
- semantic promotion remained disabled.

Therefore both tested INT32 controls are proven to accept numeric values `0`, `1`, `2`, and `3` on this route, with UNSET separately represented by v0.30. This is value acceptance, not enum decoding.

## v0.31 measured topology

All six device captures again remained in the exact v0.20 structural class:

- physical result Camera `5`;
- app-visible RAW_SENSOR `16320x12288`;
- source bytes `401,080,320`;
- row stride `32,640`;
- pixel stride `2`;
- returned `SENSOR_PIXEL_MODE=0`;
- `rawBinningFactorUsed=true`;
- Stage 3.6 `ONLY_FIRST_768_ROWS_NONZERO__EXACT_12P5MP_BYTE_PAYLOAD_SIGNATURE`;
- first `768` declared rows populated;
- remaining `11,520` rows zero;
- payload bytes `25,067,520`;
- Stage 3.7 `UNIQUE_ADVERTISED_STANDARD_RAW_BYTE_MATCH_DECODED`;
- exactly one advertised standard RAW U16 byte match `4080x3072`;
- derived payload remains an exact prefix copy and matches Stage-3.6 first-band SHA-256.

No v0.31 profile produced a measurable RAW-envelope or populated-prefix topology differential.

## v0.31 returned route-side metadata

Across all six captures the coarse route observations also remained in the same class:

- HONOR `binningFactor=4`;
- HONOR `AECRealCropWindow` begins `[11,8,4058,3055,...]`;
- HONOR `allISPCropWindow=[0,0,16320,12288,0,0,16320,12288]`;
- HONOR `isInSensorZoom=0`;
- Android `rawBinningFactorUsed=true`.

Scene/exposure/focus varied between captures and source/payload hashes are distinct, so pixel-value differences are not interpreted as causal effects of the INT32 value.

## Bounded v0.31 conclusion

`DEVICE_SWEEP_COMPLETE_6_OF_6__RAWCB_AND_HALOUTPUTBUFFERCOMBINED_INT32_VALUES_0_2_3_ACCEPTED_AND_ATTACHED__NO_MEASURABLE_CAMERA5_RAW_ENVELOPE_OR_POPULATED_PREFIX_TOPOLOGY_DIFFERENTIAL`

Combining v0.31 with earlier references gives the bounded tested value set

`{UNSET, 0, 1, 2, 3}`

for both `RawCbSourceType` and `HALOutputBufferCombined` on the tested route. Within that domain, neither control changed the measured app-visible RAW envelope/populated-prefix topology.

This does **not** mean explicit zero equals UNSET semantically, nor that values outside this domain are ineffective.

Detailed v0.31 result:

`docs/HONOR_CAMERA5_V031_INT32_VALUE_DOMAIN_SWEEP_DEVICE_RESULT_2026-09-17.md`

Machine state:

`state/CAMERA5_V031_INT32_VALUE_DOMAIN_SWEEP_STATE_2026-09-17.json`

## v0.31 build provenance

Branch:

`integration/truthraw-suite-v0-31-int32-value-domain-sweep`

The first workflow run is retained as failed-build provenance:

- run `35274679023`;
- head `84f4b320abbf57550dc5d4672d3c675bbcd93758`;
- failed before compilation because inherited bundle naming retained uppercase `V030` after the first patch;
- no scientific/device result was produced.

Corrected successful build:

- run `35274828567`;
- job `105382770948`;
- head `899e38abf87b2e4712cd60142cf2aeac9acd9e8b`;
- APK bytes `4,932,129`;
- APK SHA-256 `3125ddab46232c46680cefc67ae2e729495be88ba17cb15d2681d2e5abaf3bd3`;
- artifact ID `10520386544`;
- artifact ZIP SHA-256 `1af039f1fb79640d65f4a696c8f8c52e8102945b8099b8b917d5c6c63f2b20c1`.

## Current research consequence

Do not continue blindly enumerating arbitrary larger integer values. The simple small-value domain for both INT32 controls is now negative for the route/topology response.

The next useful route experiment should move to one of two evidence-led directions:

1. resolve a **new upstream vendor-route family** before intervention; or
2. test a **separately justified prerequisite/context condition** if evidence suggests these accepted INT32 controls only act in another route state.

Candidate names alone remain insufficient. Any new key must have native representation resolved before mutation, and failed/blocked probes remain provenance.

## Authority boundary

v0.31 does not prove:

- vendor enum meanings;
- that value `0` means off/default;
- that explicit zero is semantically equivalent to UNSET;
- that values outside `0..3` cannot change another route;
- global ineffectiveness of either INT32 key;
- untouched/native ADC output;
- native sensor geometry;
- exact electrical binning/remosaic;
- 200 MP optical resolution.

v0.20 remains source/payload authority until a future experiment produces a genuine bounded route differential.
