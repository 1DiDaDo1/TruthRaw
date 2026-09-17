# CURRENT Camera-5 route-control experiment — 2026-09-17

Status: **v0.30 FULL-FACTORIAL DEVICE MATRIX COMPLETE; v0.20 REMAINS SOURCE/PAYLOAD AUTHORITY; NO MEASURABLE RAW-TOPOLOGY DIFFERENTIAL IN ANY OF 16 TESTED PROFILES**

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

## Representation authorities resolved before intervention

- v0.23 `EnableIdealRAW`: tag `0x801F0027`, native `BYTE`;
- v0.25 `RawCbSourceType`: tag `0x801F0009`, native `INT32`;
- v0.27 `EnableXCFAOptimization`: tag `0x801F0036`, native `BYTE`;
- v0.29 `HALOutputBufferCombined`: tag `0x801F0034`, native `INT32`.

The oracle builds established representation only. Vendor-key names and numeric values were not promoted to semantics.

## Pre-matrix single-factor interventions

- v0.24 `EnableIdealRAW=BYTE(1)` — accepted/attached, no measured topology differential;
- v0.26 `RawCbSourceType=INT32(1)` — accepted/attached, no measured topology differential;
- v0.28 `EnableXCFAOptimization=BYTE(1)` — accepted/attached, no measured topology differential.

These results motivated an interaction-sensitive complete factorial screen rather than continuing one-factor-at-a-time testing.

## v0.30 design

Experiment:

`CAMERA5_VENDOR_ROUTE_FULL_FACTORIAL_2_LEVEL_4_FACTOR`

Design:

`FULL_FACTORIAL_2_LEVEL_4_FACTOR_16_RUN_COMPLEMENT_PAIRED`

Factors (`ABCD`):

- A = `EnableIdealRAW`, native BYTE, tag `0x801F0027`;
- B = `RawCbSourceType`, native INT32, tag `0x801F0009`;
- C = `EnableXCFAOptimization`, native BYTE, tag `0x801F0036`;
- D = `HALOutputBufferCombined`, native INT32, tag `0x801F0034`.

Levels:

- `0` = `UNSET_NO_WRITE`;
- `1` = numeric `1` using the already resolved native representation.

Low is absence of a write, not explicit numeric zero. Numeric high has no promoted vendor semantic.

Run order:

`0000 -> 1111 -> 0101 -> 1010 -> 0011 -> 1100 -> 0110 -> 1001 -> 0001 -> 1110 -> 0010 -> 1101 -> 0100 -> 1011 -> 1000 -> 0111`.

## v0.30e recovery implementation

The original v0.30 UI and the scroll-based v0.30c/v0.30d variants were not reliably usable because long Stage-3 status output displaced essential controls. v0.30e fixed essential controls above the detail pane and added first-missing recovery.

v0.30e did **not** change the scientific matrix, acquisition ordering, source-seal ordering, Stage 3.6 or Stage 3.7.

Branch:
`integration/truthraw-suite-v0-30e-fixed-controls-matrix-recovery`

Build:

- head `a99861d3d4468849aeedbd8070805ffbdbd3d294`;
- run `35266284068`;
- APK SHA-256 `942b21518860af983be5383298b915d30f118684a83c19783b098decb313d112`;
- artifact ZIP SHA-256 `b2d1f9b306a52699b5dbbbb8eaad26a46a6a85f14a7a20ea66f880401b8ad55e`.

## Device matrix completion

All 16 unique profiles now have individually readable v0.30 evidence JSONs:

`R01 0000`
`R02 1111`
`R03 0101`
`R04 1010`
`R05 0011`
`R06 1100`
`R07 0110`
`R08 1001`
`R09 0001`
`R10 1110`
`R11 0010`
`R12 1101`
`R13 0100`
`R14 1011`
`R15 1000`
`R16 0111`.

A separate earlier R16 capture is retained as a structural replicate.

## Complete measured result

Every one of the 16 design points remains in the same measured app-visible topology class as v0.20:

- physical result Camera `5`;
- `16320x12288` app-visible RAW envelope;
- `401,080,320` source bytes;
- row stride `32,640`, pixel stride `2`;
- returned `SENSOR_PIXEL_MODE=0` in the tested captures;
- `rawBinningFactorUsed=true`;
- Stage 3.6 classification `ONLY_FIRST_768_ROWS_NONZERO__EXACT_12P5MP_BYTE_PAYLOAD_SIGNATURE`;
- first `768` declared rows populated;
- remaining `11,520` rows zero;
- exact populated/payload size `25,067,520` bytes;
- unique advertised standard RAW match `4080x3072`;
- no measured RAW-envelope/populated-prefix topology differential versus v0.20.

The complete complement-pair set also remains invariant:

`0000/1111`, `0101/1010`, `0011/1100`, `0110/1001`, `0001/1110`, `0010/1101`, `0100/1011`, `1000/0111`.

## Complete factorial interpretation

For the measured categorical response **app-visible RAW envelope + populated-prefix topology**, all 16 design points have the same response.

Therefore, within the tested binary domain

`A,B,C,D ∈ {UNSET, numeric 1 in the resolved native representation}`

there is no observed single-factor or interaction-level topology differential.

This includes all single-high states:

- A-only R15 `1000`;
- B-only R13 `0100`;
- C-only R11 `0010`;
- D-only R09 `0001`.

It also includes every two-factor state, every three-factor state, R01 all-UNSET and R02 all-four-high.

Bounded conclusion:

`WITHIN_THE_TESTED_BINARY_DESIGN__UNSET_VS_NUMERIC_ONE_AT_THE_RESOLVED_NATIVE_TYPES__NO_SINGLE_FACTOR_OR_COMBINATION_OF_ENABLEIDEALRAW_RAWCBSOURCETYPE_ENABLEXCFAOPTIMIZATION_HALOUTPUTBUFFERCOMBINED_CHANGED_THE_MEASURED_CAMERA5_RAW_ENVELOPE_OR_POPULATED_PREFIX_TOPOLOGY`

This closes the simple binary-interaction hypothesis for these four controls at the tested levels.

## What this does not prove

The completed matrix does not prove:

- that the four keys are globally ineffective;
- that other numeric values cannot change the route;
- that vendor names describe their real semantics;
- that numeric `1` means enabled/full/native/unbinned;
- that other vendor keys or operating modes cannot expose a different route;
- untouched/native photodiode ADC output;
- native physical sensor geometry;
- exact electrical binning/remosaic;
- 200 MP optical resolution.

Pixel-value differences are not interpreted causally from this matrix because scene/exposure/focus varied between physical captures. The completed result is a route/topology result.

## Current next research direction

Do not repeat this four-key 0/1 matrix. That experimental space is complete.

Next scientifically useful paths are:

1. discover the actual **value domain / enum semantics** of the INT32 controls, especially `RawCbSourceType` and `HALOutputBufferCombined`, before trying other numeric values; or
2. identify new upstream session/request controls and resolve native representation before intervention.

Detailed complete result:

`docs/HONOR_CAMERA5_V030_VENDOR_ROUTE_MATRIX_COMPLETE_DEVICE_RESULT_2026-09-17.md`

Machine-readable complete state:

`state/CAMERA5_V030_VENDOR_ROUTE_FULL_FACTORIAL_MATRIX_COMPLETE_STATE_2026-09-17.json`

v0.20 remains source/payload authority until a future experiment produces a genuine bounded route differential.
