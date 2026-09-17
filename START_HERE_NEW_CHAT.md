# START HERE — TruthRaw current bootstrap

This is the living bootstrap entry point for the consolidated TruthRaw research state, updated through the HONOR Camera-5 v0.29 build on 2026-09-17.

It is a navigation/current-state document. It does not rewrite frozen historical evidence, dated handoffs, rejected experiments or canonical bytes.

## Mandatory current reading order

Read in this order:

1. `docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md`
2. `docs/CURRENT_SCENE_PHYSICS_RESTORATION_200MP_2026-09-16.md` — scientific background; older Camera-5 physical-gate wording is superseded by the 2026-09-17 Camera-5 documents
3. `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md`
4. `state/CURRENT_CAMERA5_RAW_ROUTE_STATE_2026-09-17.json`
5. `docs/DOCUMENT_STATUS_INDEX_2026-09-17.md`
6. `state/CAMERA5_V029_HAL_OUTPUT_BUFFER_COMBINED_NATIVE_TYPE_ORACLE_STATE_2026-09-17.json`
7. `docs/HONOR_CAMERA5_HAL_OUTPUT_BUFFER_COMBINED_V029_NATIVE_TYPE_ORACLE_BUILD_2026-09-17.md`
8. `docs/HONOR_CAMERA5_XCFA_V028_DEVICE_RESULT_2026-09-17.md`
9. `docs/handoff/TRUTHRAW_CAMERA5_RAW_ROUTE_V014_TO_V020_2026-09-17.md`
10. only then the exact canonical/research/module documents relevant to the task.

Historical architecture documents remain background authorities in their own domain:

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

The trusted acquisition/content lineage remains:

`v0.14 source-first seal`
`-> v0.16 post-HAL HardwareBuffer envelope`
`-> v0.17 two-door request/delivery airlock`
`-> v0.19 full-raster write audit`
`-> v0.20 payload geometry decoder`.

The trusted route is:

`logical 0 -> physical Camera 5 -> MAXIMUM_RESOLUTION declaration -> physical-scoped request -> app-visible 16320x12288 RAW_SENSOR envelope -> original Plane[0] sealed first -> read-only envelope/audit -> payload geometry interpretation`.

Current bounded classification:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

Descriptive shorthand:

`APP_VISIBLE_4080x3072_BAYER_LIKE_RAW_PAYLOAD_EMBEDDED_IN_16320x12288_HAL_ENVELOPE`.

This does not prove untouched ADC, native sensor geometry, exact electrical binning/remosaic or 200 MP optical resolution.

## v0.20 control facts

The tested source envelope is `401,080,320` bytes with `rowStride=32640`, `pixelStride=2` and app-visible geometry `16320x12288`.

Stage 3.6 established that only rows `0..767` are populated; rows `768..12287` are zero. Therefore:

`16320 * 768 * 2 = 25,067,520 bytes`.

Stage 3.7 found exactly one runtime-advertised standard Camera-5 RAW_SENSOR geometry with the same U16 byte count:

`4080 * 3072 * 2 = 25,067,520 bytes`.

The exact prefix copy forms a coherent full-frame scene under the `4080x3072` interpretation and shows Bayer-like 2x2 structure. The `.rawpayload` remains derived evidence; the sealed 401-MB source remains primary.

## Route-control research through v0.28

Unknown vendor keys are investigated in two phases:

1. resolve native `camera_metadata` representation using disposable request metadata only, with no session/capture/HAL submission;
2. only after representation is known, test one vendor key/value at a time while preserving the v0.20 acquisition/audit chain.

Numeric value `1` is an experimental control value, not proven vendor semantics.

Resolved native representations:

- v0.23 `EnableIdealRAW`: tag `0x801F0027`, native `BYTE`
- v0.25 `RawCbSourceType`: tag `0x801F0009`, native `INT32`
- v0.27 `EnableXCFAOptimization`: tag `0x801F0036`, native `BYTE`.

Accepted single-variable interventions with **no measured RAW-topology change**:

- v0.24 `EnableIdealRAW=BYTE(1)`
- v0.26 `RawCbSourceType=INT32(1)`
- v0.28 `EnableXCFAOptimization=BYTE(1)`.

For v0.28 specifically, Gate A saw XCFA value `0`; builder/readback after set was `1`; built request readback was `1`; session parameters were attached. The final capture still produced the same structural class: 401,080,320-byte envelope, first 25,067,520 bytes populated, only rows 0..767 non-zero, and unique `4080x3072` standard RAW byte match.

Therefore v0.20 remains source/payload authority.

## Current experiment — v0.29 HALOutputBufferCombined native type oracle

Current candidate:

`org.codeaurora.qcamera3.sessionParameters.HALOutputBufferCombined`

The key is exposed on logical and physical request/session surfaces. Its name is potentially relevant to the unusual large-envelope/small-populated-prefix observation, but the name alone grants no semantics.

v0.29 is **type resolution only**. It tests native `BYTE`, `INT32`, `FLOAT`, `INT64`, `DOUBLE` and `RATIONAL` on separate disposable request metadata instances, then validates setter/readback/type/count.

It deliberately creates no capture session, attaches no session parameters, submits no capture and sends no vendor-modified request to HAL.

Branch:
`integration/truthraw-suite-v0-29-hal-output-buffer-combined-native-type-oracle`

Build status: **SUCCESS — DEVICE RESULT PENDING**

GitHub Actions run: `35255819562`

APK:
- bytes `4,899,361`
- SHA-256 `deb775a8b19fcd02616904ace931c56297d3badbd6d3b679e308137949ba5552`

Artifact:
- ID `10513220672`
- ZIP bytes `1,595,399`
- ZIP SHA-256 `84675230ab4f40538d3fb71671ee71d4cde9b05cf430a542f01ef6030ab1d3b0`.

On-device protocol: **Step 1 only**. Expected stop:

`STAGE 1.5 DIAGNOSTIC STOP`

Expected export:

`TRUTHRAW_CAM5_HAL_OUTPUT_BUFFER_COMBINED_NATIVE_TYPE_ORACLE_v029.json`

No preview or RAW capture should be performed by this build.

## Current open problem

The old question “can Camera 5 deliver an app-visible physical 16320x12288 RAW_SENSOR object?” is closed by v0.14.

The current open route question is:

`OPEN_NEEDS_ROUTE_DIFFERENTIAL_TO_ESTABLISH_WHETHER_A_DIFFERENT_HONOR_QTI_CONTROL_PATH_CAN_POPULATE_A_LARGER_NATIVE_OR_APP_VISIBLE_RAW_DOMAIN`.

Independently, calibration/optical proof is still required before any native-ADC or 200-MP optical promotion.

## Multi-camera / focus status

Camera 5 exposes Android focus-distance results plus QTI AF lens-position/phase-detect telemetry and multicamera sidecars.

The v0.18 focus branch remains isolated until the RAW route is better understood. Future multi-camera/multi-focus work must seal each physical RAW separately and keep physical-frame/evidence counts explicit.

## Precision and downstream continuity

Current precision direction remains:

`exact RAW integer/packed evidence -> integer-exact topology/content audit -> F64 branch-sensitive science -> controlled F32 only where validated`.

TruthRange/zero-line remains downstream of acquisition and calibration. RAW code zero, BlackLevel, display black and the TruthRange zero-line are different concepts.

## Repository/governance rules

- Use `docs/DOCUMENT_STATUS_INDEX_2026-09-17.md` for current-vs-historical interpretation.
- Preserve older Camera-5 documents as historical provenance; do not rewrite them to pretend later results were known earlier.
- v0.15 direct-open Camera-5 remains rejected-route provenance.
- v0.18 focus remains isolated parallel research.
- APK/GCam/computational RAW does not determine TruthRaw evidence authority or calibration.
- No silent license changes.
- `canonical/ptc/v1.1` means **Pure Truth Certificate**.

## Immediate continuation

1. run v0.29 Step 1 only and export its oracle JSON;
2. if exactly one native type is accepted, record that representation without assigning semantics;
3. only then decide whether a separate single-variable HALOutputBufferCombined intervention is justified;
4. preserve v0.20 as immutable source/payload control;
5. continue independent calibration/optical work separately from route-control experiments.
