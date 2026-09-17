# HONOR Camera-5 v0.31 INT32 value-domain sweep — build record — 2026-09-17

## Status

**BUILD SUCCESS — DEVICE RESULTS PENDING**

Source/payload authority remains **TruthRaw v0.20**.

Permanent boundary:

`APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF`

v0.31 begins only after the complete v0.30 16/16 factorial matrix produced no measurable differential in the app-visible RAW envelope/populated-prefix topology.

## Why v0.31 exists

v0.30 closed the tested binary interaction space for four type-resolved vendor controls:

- `EnableIdealRAW` / BYTE / tag `0x801F0027`;
- `RawCbSourceType` / INT32 / tag `0x801F0009`;
- `EnableXCFAOptimization` / BYTE / tag `0x801F0036`;
- `HALOutputBufferCombined` / INT32 / tag `0x801F0034`.

Across all `2^4 = 16` combinations of UNSET versus numeric `1`, no profile changed the measured Camera-5 RAW envelope/populated-prefix topology. Therefore v0.31 does **not** repeat the 0/1 matrix.

The next bounded question is whether small alternate INT32 values on the two INT32 controls can select a different route.

## Experimental design

Experiment:

`CAMERA5_INT32_VALUE_DOMAIN_SWEEP_0_2_3`

Only one unknown vendor key is written per capture.

A (`EnableIdealRAW`) and C (`EnableXCFAOptimization`) remain UNSET in every v0.31 run.

The six profiles are:

| Run | Target | Native type | Numeric value |
|---|---|---|---:|
| S01 | `RawCbSourceType` | INT32 | 0 |
| S02 | `RawCbSourceType` | INT32 | 2 |
| S03 | `RawCbSourceType` | INT32 | 3 |
| S04 | `HALOutputBufferCombined` | INT32 | 0 |
| S05 | `HALOutputBufferCombined` | INT32 | 2 |
| S06 | `HALOutputBufferCombined` | INT32 | 3 |

Numeric `1` and UNSET are intentionally not repeated because they were already screened in v0.30.

The values `0`, `2`, and `3` are **experimental stimuli only**. No enum name, enable/disable meaning, route meaning or sensor meaning is assigned to them.

## Safety / authority rules

For every run:

- exactly one unknown vendor key may be written;
- native representation must remain INT32 as proven by the earlier no-submit oracles;
- builder readback must exactly equal the requested numeric value;
- built-request readback must exactly equal the requested numeric value;
- session-parameter attachment must succeed before the capture route can proceed;
- Gate A remains before the controlled intervention;
- `isSessionConfigurationSupported(config)` remains after intervention attachment;
- original `Image.Plane[0]` is sealed before post-HAL envelope interpretation;
- Stage 3.6 and Stage 3.7 remain unchanged and read-only;
- vendor semantics are not promoted from acceptance/readback;
- a blocked/failed run must not silently advance.

The response of interest remains **route/topology**, not cross-scene pixel-value equality.

## Implementation

Experiment branch:

`integration/truthraw-suite-v0-31-int32-value-domain-sweep`

New helper:

`suite_android/app/src/main/java/com/truthraw/adaptiveui/Camera2Int32ValueDomainSweep.kt`

Primary patch:

`tools/patch_fotograaf_v031_int32_value_domain_sweep.py`

Corrected provenance patch:

`tools/patch_fotograaf_v031b_int32_value_domain_sweep.py`

The patch reconstructs the v0.30e device-usable source-first chain and changes the controlled intervention scheduler/evidence identity only. v0.20 acquisition/payload authority, RAW sealing order, Stage 3.6 and Stage 3.7 are retained.

## Failed first build provenance

The first v0.31 workflow run is retained as failed-build provenance:

- run ID `35274679023`;
- head SHA `84f4b320abbf57550dc5d4672d3c675bbcd93758`;
- failure occurred before compilation;
- cause: the v0.31 patch correctly transformed the scientific path but its final assertion failed because the inherited bundle filename used uppercase `V030`, while the first replacement handled lowercase `v030` only;
- no device experiment or scientific result came from this failed build.

The corrected `v031b` patch injects only the deterministic uppercase `V030 -> V031` correction and preserves the first patch unchanged as provenance.

## Successful build provenance

GitHub Actions:

- run ID `35274828567`;
- job ID `105382770948`;
- workflow head SHA `899e38abf87b2e4712cd60142cf2aeac9acd9e8b`;
- result: **SUCCESS**.

The workflow verified:

- exactly six profiles;
- RawCb values `0,2,3`;
- HALOutputBufferCombined values `0,2,3`;
- exactly one unknown vendor key written per profile;
- INT32 `CaptureRequest.Key` representation;
- exact builder/request readback path;
- session-parameter attachment;
- no `EnableIdealRAW` or `EnableXCFAOptimization` writes in the v0.31 helper;
- Gate A before intervention;
- intervention before session-support check;
- source seal before post-HAL envelope observation;
- Stage 3.6 before Stage 3.7;
- arm64 APK build and native bridge verification.

APK:

- bytes: `4,932,129`;
- SHA-256: `3125ddab46232c46680cefc67ae2e729495be88ba17cb15d2681d2e5abaf3bd3`.

Artifact:

- ID: `10520386544`;
- name: `truthraw-suite-main-plus-fotograaf-v0-31-int32-value-domain-sweep-debug-arm64`;
- ZIP bytes: `1,611,249`;
- ZIP SHA-256: `1af039f1fb79640d65f4a696c8f8c52e8102945b8099b8b917d5c6c63f2b20c1`.

## Device protocol

Install the v0.31 APK over the current app if preservation of existing app data is desired.

The fixed-control UI remains: essential controls are above the detail pane and do not require scrolling.

For each v0.31 run:

`1 · Routes -> 2 · Live 3.7× -> 3 · Sweep capture`

Save the individual evidence JSON after each successful run. The selector advances using the same first-missing recovery concept as v0.30e.

Expected profile order:

`S01 RAWCB=0`
`S02 RAWCB=2`
`S03 RAWCB=3`
`S04 HALCOMBINED=0`
`S05 HALCOMBINED=2`
`S06 HALCOMBINED=3`.

Bundle ZIP export is redundancy only; individually readable evidence JSONs remain the preferred completion authority.

## Interpretation boundary

A value that attaches and captures successfully proves only that the tested app/session path accepted that numeric stimulus under the tested route. It does not prove the vendor's intended enum meaning.

A topology differential, if observed, would establish a **value-specific route differential first**. Semantic decoding would still require follow-up evidence.

If all six profiles reproduce the v0.20 topology class, the bounded conclusion will be limited to tested values `0,1,2,3` plus UNSET for the two INT32 controls; it still would not prove global ineffectiveness for other INT32 values or other vendor controls.
