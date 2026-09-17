# CURRENT Camera-5 route-control experiment — 2026-09-17

Status: **CURRENT EXPERIMENT TRACK; v0.20 REMAINS SOURCE/PAYLOAD AUTHORITY; v0.29 DEVICE TYPE RESULT COMPLETE; v0.30 FULL-FACTORIAL MATRIX BUILD SUCCESS / DEVICE MATRIX PENDING**

This document tracks upstream HONOR/QTI route-control experiments after the completed v0.20 payload-topology result. It does not replace `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md` for source authority.

## Control authority

TruthRaw v0.20 remains the untouched source/payload control:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

Control topology:

- app-visible envelope: `16320x12288`, `401,080,320` bytes
- populated source prefix: `25,067,520` bytes
- unique advertised standard RAW byte match: `4080x3072`
- source prefix behaves as a coherent Bayer-like full-frame raster under that interpretation.

Permanent boundary:

`APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF`

## Completed single-factor route-control sequence

The early route-control phase deliberately used one unknown vendor variable at a time so representation and immediate differential effects could be isolated.

Resolved native representations:

- v0.23 `EnableIdealRAW`: tag `0x801F0027`, native `BYTE`
- v0.25 `RawCbSourceType`: tag `0x801F0009`, native `INT32`
- v0.27 `EnableXCFAOptimization`: tag `0x801F0036`, native `BYTE`
- v0.29 `HALOutputBufferCombined`: tag `0x801F0034`, native `INT32`.

Completed single-variable interventions:

- v0.24 `EnableIdealRAW=BYTE(1)`
- v0.26 `RawCbSourceType=INT32(1)`
- v0.28 `EnableXCFAOptimization=BYTE(1)`.

All three were accepted and attached on the tested route, yet none produced a measurable differential in RAW envelope or populated-payload topology versus the v0.20 control. In each case the app-visible envelope remained `401,080,320` bytes and the populated prefix remained `25,067,520` bytes with the same first-768-row signature and unique `4080x3072` standard-RAW byte match.

These are bounded negative differentials for the tested values/routes. They do not prove the vendor controls are universally ineffective.

## v0.29 — HALOutputBufferCombined native type result

Device evidence:

`TRUTHRAW_CAM5_HAL_OUTPUT_BUFFER_COMBINED_NATIVE_TYPE_ORACLE_v029.json`

Classification:

`NATIVE_METADATA_TYPE_INT32__NO_SESSION_OR_CAPTURE_SUBMISSION`

Resolved device facts:

- key: `org.codeaurora.qcamera3.sessionParameters.HALOutputBufferCombined`
- tag: `0x801F0034`
- unsigned tag ID: `2149515316`
- accepted native type count: `1`
- resolved type: `INT32`
- INT32 set/get status: `0/0`
- accepted INT32 entry type/count: `1 / 1`
- BYTE/FLOAT/INT64/DOUBLE/RATIONAL rejected.

The numeric test value `1` was used strictly for metadata-type validation. v0.29 created disposable request templates only: no session, no session parameters, no capture, no vendor-modified request submission to HAL, no RAW access and no source mutation.

Representation conclusion:

`HAL_OUTPUT_BUFFER_COMBINED_IS_NATIVE_INT32_ON_TESTED_LOGICAL_CAMERA0_METADATA_SURFACE__VALUE_SEMANTICS_UNPROVEN`

## Why the experiment now changes from one-factor-at-a-time to a designed matrix

The earlier rule “do not combine unknown vendor controls” applied to the representation-discovery and first-intervention phase. That phase has now done its job: the four selected factors have device-resolved native representations, and three separate high-level interventions produced no topology differential.

The next scientific question is therefore interaction-sensitive:

`CAN_A_COMBINATION_OF_ALREADY_TYPE_RESOLVED_ROUTE_CONTROLS_CHANGE_THE_CAMERA5_RAW_ROUTE_OR_POPULATED_PAYLOAD_TOPOLOGY_WHEN_THE_SAME_CONTROLS_TESTED_IN_ISOLATION_DID_NOT?`

A single capture with all controls enabled would not answer that causally. v0.30 therefore uses a complete designed factorial screen while retaining one separately sealed RAW observation per run.

## v0.30 — Camera-5 vendor route full-factorial matrix

Branch:

`integration/truthraw-suite-v0-30-vendor-route-full-factorial-matrix`

Build status: **SUCCESS — DEVICE MATRIX PENDING**.

Design:

`FULL_FACTORIAL_2_LEVEL_4_FACTOR_16_RUN_COMPLEMENT_PAIRED`

Factors (`ABCD`):

- A = `EnableIdealRAW`, native `BYTE`, tag `0x801F0027`
- B = `RawCbSourceType`, native `INT32`, tag `0x801F0009`
- C = `EnableXCFAOptimization`, native `BYTE`, tag `0x801F0036`
- D = `HALOutputBufferCombined`, native `INT32`, tag `0x801F0034`.

Factor levels are deliberately representation-level controls, not semantic claims:

- low = `UNSET_NO_WRITE`
- high = numeric `1` written using the already device-resolved native representation.

All `2^4 = 16` combinations occur exactly once. The order is complement-paired to reduce monotonic scene/time drift confounding:

`0000 -> 1111 -> 0101 -> 1010 -> 0011 -> 1100 -> 0110 -> 1001 -> 0001 -> 1110 -> 0010 -> 1101 -> 0100 -> 1011 -> 1000 -> 0111`.

Run `R01_ABCD_0000` is the untouched matrix control. Crucially, `0` in the profile bitstring means **UNSET**, not an explicit vendor value zero.

## v0.30 provenance and safety

The first v0.30 workflow attempt is intentionally retained as failed-build provenance. It failed before compilation because the initial patch used an overly specific UI text anchor. No scientific or acquisition result came from that attempt.

The corrected patch is:

`tools/patch_fotograaf_v030b_vendor_route_full_factorial_matrix.py`

Corrected build provenance:

- GitHub Actions run `35260779194`
- job `105335638334`
- workflow head `f234055ca102177ced64105214c9c3357c312f83`
- matrix completeness/order assertions: PASS
- source-first ordering assertions: PASS
- Gradle build: SUCCESS
- APK bytes: `4,915,745`
- APK SHA-256: `d697ea34f1b21f973f174b3dacba951510dc157eceb491f187bee33b59d27c46`
- artifact ID: `10515027009`
- artifact ZIP bytes: `1,604,556`
- artifact ZIP SHA-256: `648e22e3df73f97b9c6f87223ecb8748c657be39157bfa64dd4b5f1b7da908af`.

Every run preserves the v0.20 acquisition/audit order:

`Gate A untouched route observation`
`-> selected matrix session parameters`
`-> physical-5/MAX session`
`-> physical-scoped still capture`
`-> original Plane[0] sealed first`
`-> read-only post-HAL envelope observation`
`-> closed/sealed Stage 3.6 full-raster audit`
`-> Stage 3.7 payload-geometry decoder`.

Each successful run advances to the next profile. A blocked/failed profile does not advance automatically. Every capture is independently sealed and identified by its matrix profile in filenames/evidence.

## v0.30 primary differential targets

The matrix is a route/topology screen. For every run compare against v0.20 and against its complement partner:

- source-envelope bytes and geometry
- populated-prefix bytes
- first/last non-zero byte offsets
- 16-band population signature
- selected payload geometry
- returned `SENSOR_PIXEL_MODE`
- `rawBinningFactorUsed`
- HONOR `binningFactor`
- HONOR `isInSensorZoom`
- AEC/ISP crop metadata
- HardwareBuffer envelope
- source/payload hashes as identity/provenance, not cross-scene equality expectations.

## Authority rules

- v0.20 remains source/payload authority until newer device evidence genuinely changes the bounded source/topology claim.
- v0.23/v0.25/v0.27/v0.29 resolve metadata representation only.
- v0.24/v0.26/v0.28 are bounded single-factor negative differential results for tested value `1`.
- v0.30 is a designed interaction screen; a profile hit establishes a combination-level differential first, not immediate semantics for any individual key.
- vendor-key names and numeric value `1` are not semantic authority.
- low matrix state means UNSET, not explicit zero.
- source bytes remain sealed before result/vendor interpretation.
- failed/rejected experiments remain provenance.

## Immediate continuation

Run v0.30 beginning with `R01_ABCD_0000`, keep scene/framing as stable as practical, complete each successful capture through Stage 3.7 and save the evidence JSON for every run. The first priority is detecting any structural topology change; only after a matrix hit should the corresponding interaction region be narrowed with follow-up experiments.
