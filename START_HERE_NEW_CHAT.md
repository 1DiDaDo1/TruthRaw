# START HERE — TruthRaw current bootstrap

This is the living bootstrap entry point for the consolidated TruthRaw research state, updated through the HONOR Camera-5 v0.30 full-factorial route-matrix build on 2026-09-17.

It is a navigation/current-state document. It does not rewrite frozen historical evidence, dated handoffs, rejected experiments or canonical bytes.

## Mandatory current reading order

1. `docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md`
2. `docs/CURRENT_SCENE_PHYSICS_RESTORATION_200MP_2026-09-16.md`
3. `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md`
4. `state/CURRENT_CAMERA5_RAW_ROUTE_STATE_2026-09-17.json`
5. `docs/CURRENT_CAMERA5_ROUTE_EXPERIMENT_2026-09-17.md`
6. `state/CAMERA5_V029_HAL_OUTPUT_BUFFER_COMBINED_NATIVE_TYPE_ORACLE_STATE_2026-09-17.json`
7. `state/CAMERA5_V030_VENDOR_ROUTE_FULL_FACTORIAL_MATRIX_STATE_2026-09-17.json`
8. `docs/DOCUMENT_STATUS_INDEX_2026-09-17.md`
9. `docs/handoff/TRUTHRAW_CAMERA5_RAW_ROUTE_V014_TO_V020_2026-09-17.md`
10. only then the exact canonical/research/module documents relevant to the task.

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

v0.29 device result:

`NATIVE_METADATA_TYPE_INT32__NO_SESSION_OR_CAPTURE_SUBMISSION`

for `org.codeaurora.qcamera3.sessionParameters.HALOutputBufferCombined`, tag `0x801F0034`, accepted type count `1`. No session/capture/HAL submission occurred in the oracle.

## Current experiment — v0.30 full-factorial vendor route matrix

After representation discovery and three negative single-factor topology differentials, the experiment moves from one-factor-at-a-time screening to an interaction-sensitive designed matrix.

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

This does **not** assign semantics to either level. Low is absence of a write, not explicit numeric zero.

All 16 binary combinations are present exactly once, ordered in complement pairs:

`0000/1111, 0101/1010, 0011/1100, 0110/1001, 0001/1110, 0010/1101, 0100/1011, 1000/0111`.

`R01_ABCD_0000` is the untouched matrix control.

Every run preserves the v0.20 source-first chain and independently seals its own RAW before post-HAL interpretation. Stage 3.6 and Stage 3.7 remain unchanged.

Branch:

`integration/truthraw-suite-v0-30-vendor-route-full-factorial-matrix`

Corrected build status: **SUCCESS — DEVICE MATRIX PENDING**.

Build provenance:

- run `35260779194`
- job `105335638334`
- head `f234055ca102177ced64105214c9c3357c312f83`
- APK bytes `4,915,745`
- APK SHA-256 `d697ea34f1b21f973f174b3dacba951510dc157eceb491f187bee33b59d27c46`
- artifact ID `10515027009`
- artifact ZIP bytes `1,604,556`
- artifact ZIP SHA-256 `648e22e3df73f97b9c6f87223ecb8748c657be39157bfa64dd4b5f1b7da908af`.

The first v0.30 attempt is retained as failed-build provenance; it stopped before compilation on an overly specific UI patch anchor. The corrected `v030b` patch built successfully.

## Device protocol for v0.30

Start with the displayed `R01_ABCD_0000` profile. Keep framing/scene as stable as practical. Complete a run through Stage 3.7 and save its evidence JSON before moving on. A successful run advances the selector to the next profile; a blocked/failed run remains selected so it can be diagnosed rather than silently skipped.

Primary screening targets are envelope bytes/geometry, populated-prefix bytes, non-zero extent, 16-band signature, selected payload geometry, returned pixel mode, raw-binning flag, HONOR binning factor, in-sensor-zoom state, AEC/ISP crops and HardwareBuffer envelope.

A matrix hit establishes a **combination-level route differential first**. It does not immediately identify which key or semantic caused it; follow-up narrowing experiments are required.

## Current open problem

`OPEN_NEEDS_ROUTE_DIFFERENTIAL_TO_ESTABLISH_WHETHER_A_DIFFERENT_HONOR_QTI_CONTROL_PATH_CAN_POPULATE_A_LARGER_NATIVE_OR_APP_VISIBLE_RAW_DOMAIN`

Independently, calibration/optical proof is still required before any native-ADC or 200-MP optical promotion.

## Precision and governance continuity

Current precision direction remains:

`exact RAW integer/packed evidence -> integer-exact topology/content audit -> F64 branch-sensitive science -> controlled F32 only where validated`.

TruthRange/zero-line remains downstream of acquisition and calibration. RAW code zero, BlackLevel, display black and the TruthRange zero-line are different concepts.

Governance remains unchanged: preserve old Camera-5 documents as provenance, keep v0.15 direct-open rejection and v0.18 focus work isolated, do not let GCam/computational RAW determine TruthRaw evidence authority, make no silent license changes, and keep `canonical/ptc/v1.1` meaning **Pure Truth Certificate**.
