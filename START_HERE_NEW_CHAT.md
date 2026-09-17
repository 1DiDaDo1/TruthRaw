# START HERE — TruthRaw current bootstrap

This is the living bootstrap entry point for the consolidated TruthRaw research state, updated through the HONOR Camera-5 v0.30 full-factorial route-matrix **partial device result** on 2026-09-17.

It is a navigation/current-state document. It does not rewrite frozen historical evidence, dated handoffs, rejected experiments or canonical bytes.

## Mandatory current reading order

1. `docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md`
2. `docs/CURRENT_SCENE_PHYSICS_RESTORATION_200MP_2026-09-16.md`
3. `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md`
4. `state/CURRENT_CAMERA5_RAW_ROUTE_STATE_2026-09-17.json`
5. `docs/CURRENT_CAMERA5_ROUTE_EXPERIMENT_2026-09-17.md`
6. `docs/HONOR_CAMERA5_V030_VENDOR_ROUTE_MATRIX_PARTIAL_DEVICE_RESULTS_2026-09-17.md`
7. `state/CAMERA5_V030_PARTIAL_MATRIX_V030E_FIXED_CONTROLS_STATE_2026-09-17.json`
8. `state/CAMERA5_V029_HAL_OUTPUT_BUFFER_COMBINED_NATIVE_TYPE_ORACLE_STATE_2026-09-17.json`
9. `state/CAMERA5_V030_VENDOR_ROUTE_FULL_FACTORIAL_MATRIX_STATE_2026-09-17.json`
10. `docs/DOCUMENT_STATUS_INDEX_2026-09-17.md`
11. `docs/handoff/TRUTHRAW_CAMERA5_RAW_ROUTE_V014_TO_V020_2026-09-17.md`
12. only then the exact canonical/research/module documents relevant to the task.

Historical vision documents remain background authorities in their own domain:

- `docs/CORE_VISION_SEALED_HOUSE_ARCHITECTURE.md`
- `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md`
- `docs/CORE_VISION_UNCERTAINTY_AWARE_APPEARANCE.md`
- `docs/CORE_VISION_VIRTUAL_OBSERVATION_MANIFOLD.md`.

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

## Route-control research through v0.29

Native representations resolved on-device:

- v0.23 `EnableIdealRAW`: `BYTE`, tag `0x801F0027`
- v0.25 `RawCbSourceType`: `INT32`, tag `0x801F0009`
- v0.27 `EnableXCFAOptimization`: `BYTE`, tag `0x801F0036`
- v0.29 `HALOutputBufferCombined`: `INT32`, tag `0x801F0034`.

Accepted single-variable interventions with no measured RAW envelope/populated-payload topology differential:

- v0.24 `EnableIdealRAW=BYTE(1)`
- v0.26 `RawCbSourceType=INT32(1)`
- v0.28 `EnableXCFAOptimization=BYTE(1)`.

Numeric `1` remains an experimental control value, not proven vendor semantics.

## Current experiment — v0.30 full-factorial vendor route matrix

Design:

`FULL_FACTORIAL_2_LEVEL_4_FACTOR_16_RUN_COMPLEMENT_PAIRED`

Factors (`ABCD`):

- A `EnableIdealRAW` — BYTE
- B `RawCbSourceType` — INT32
- C `EnableXCFAOptimization` — BYTE
- D `HALOutputBufferCombined` — INT32.

Levels:

- low = `UNSET_NO_WRITE`
- high = numeric `1` using the device-resolved native type.

Low is absence of a write, not explicit numeric zero. Numeric high has no promoted vendor meaning.

All 16 binary combinations are present exactly once in the designed matrix.

## Current v0.30 device result

The current hard-evidence set is defined only by individually readable evidence JSON files.

Eight unique matrix profiles are currently proven available:

`R01 0000`
`R02 1111`
`R04 1010`
`R11 0010`
`R13 0100`
`R14 1011`
`R15 1000`
`R16 0111`.

A second independent R16 `0111` capture is retained as a structural replicate.

Every one of those profiles remains in the same measured topology class as v0.20:

- source envelope `401,080,320` bytes / `16320x12288`;
- first `768` rows populated;
- remaining `11,520` rows zero;
- populated/payload bytes `25,067,520`;
- unique advertised standard RAW byte match `4080x3072`;
- no measured envelope/populated-prefix topology differential.

R01 `0000` is the contemporaneous all-UNSET control and reproduces v0.20 topology with no matrix vendor writes.

R02 `1111` proves the simple tested hypothesis “all four current numeric-one controls together are sufficient to change the app-visible RAW topology” is not supported.

Single-high hard results currently include A-only (`R15`), B-only (`R13`) and C-only (`R11`), all without topology differential. D-only (`R09`) is still missing.

Current missing individually readable matrix profiles:

`R03, R05, R06, R07, R08, R09, R10, R12`.

## Bundle boundary

Multiple `TRUTHRAW_CAM5_V030_MATRIX_EVIDENCE_BUNDLE_*.zip` files were uploaded during recovery. They remain auxiliary provenance containers. Archive inspection timed out in the current analysis environment, so no run is counted from ZIP filename, bundle size, UI progression or recollection alone.

Until those archives are independently parsed, individually readable evidence JSONs define matrix-completion authority.

## v0.30e recovery build

Branch:

`integration/truthraw-suite-v0-30e-fixed-controls-matrix-recovery`

Build status: **SUCCESS**.

- head `a99861d3d4468849aeedbd8070805ffbdbd3d294`
- run `35266284068`
- APK SHA-256 `942b21518860af983be5383298b915d30f118684a83c19783b098decb313d112`
- artifact ZIP SHA-256 `b2d1f9b306a52699b5dbbbb8eaad26a46a6a85f14a7a20ea66f880401b8ad55e`.

v0.30e changes UI/recovery only; it does not change the scientific matrix, acquisition ordering, source-seal ordering, Stage 3.6 or Stage 3.7.

## Immediate device continuation

Use v0.30e's first-missing recovery selector and save each new evidence JSON individually.

Hard missing set:

`R03, R05, R06, R07, R08, R09, R10, R12`.

Bundle export remains useful redundancy but is not the sole evidence authority.

## Current open problem

`OPEN_NEEDS_ROUTE_DIFFERENTIAL_TO_ESTABLISH_WHETHER_A_DIFFERENT_HONOR_QTI_CONTROL_PATH_CAN_POPULATE_A_LARGER_NATIVE_OR_APP_VISIBLE_RAW_DOMAIN`

Independently, calibration/optical proof is still required before any native-ADC or 200-MP optical promotion.

## Precision and governance continuity

Current precision direction remains:

`exact RAW integer/packed evidence -> integer-exact topology/content audit -> F64 branch-sensitive science -> controlled F32 only where validated`.

TruthRange/zero-line remains downstream of acquisition and calibration. RAW code zero, BlackLevel, display black and the TruthRange zero-line are different concepts.

Governance remains unchanged: preserve old Camera-5 documents as provenance, keep v0.15 direct-open rejection and v0.18 focus work isolated, do not let GCam/computational RAW determine TruthRaw evidence authority, make no silent license changes, and keep `canonical/ptc/v1.1` meaning **Pure Truth Certificate**.
