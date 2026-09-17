# TruthRaw document status index — 2026-09-17

Status: **CURRENT DOCUMENT GOVERNANCE INDEX** for the 2026-09-17 Camera-5 line through the v0.29 build.

This index supersedes `docs/DOCUMENT_STATUS_INDEX_2026-09-16.md` as the current navigation/governance index. Historical indexes remain preserved as historical state.

## A. Global current reading order

1. `README.md`
2. `START_HERE_NEW_CHAT.md`
3. `docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md`
4. `docs/CURRENT_SCENE_PHYSICS_RESTORATION_200MP_2026-09-16.md` — retained scientific background; older Camera-5 physical-gate wording is superseded below
5. `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md`
6. `state/CURRENT_CAMERA5_RAW_ROUTE_STATE_2026-09-17.json`
7. `docs/DOCUMENT_STATUS_INDEX_2026-09-17.md`
8. `docs/HONOR_CAMERA5_XCFA_V028_DEVICE_RESULT_2026-09-17.md`
9. `docs/HONOR_CAMERA5_HAL_OUTPUT_BUFFER_COMBINED_V029_NATIVE_TYPE_ORACLE_BUILD_2026-09-17.md`
10. `docs/handoff/TRUTHRAW_CAMERA5_RAW_ROUTE_V014_TO_V020_2026-09-17.md`
11. exact canonical/research/module documents relevant to the task.

## B. Current Camera-5 authority chain

Read Camera-5 work in this order:

1. `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md` — current bounded interpretation through v0.29 build
2. `state/CURRENT_CAMERA5_RAW_ROUTE_STATE_2026-09-17.json` — machine-readable current state
3. `docs/HONOR_MAGIC8_PRO_TELE_200MP_FULL_RAW_V014_2026-09-17.md` — proven acquisition route/source-first sealing authority
4. `docs/HONOR_CAMERA5_TWO_DOOR_AIRLOCK_V017_2026-09-17.md` — request-side + post-HAL provenance architecture
5. `docs/HONOR_MAGIC8_PRO_TELE_PAYLOAD_GEOMETRY_V020_2026-09-17.md` — completed v0.19/v0.20 payload result
6. versioned route-control result/build documents v0.21 onward
7. `docs/handoff/TRUTHRAW_CAMERA5_RAW_ROUTE_V014_TO_V020_2026-09-17.md` — exact historical/reproduction map for the acquisition/payload line.

## C. Current Camera-5 bounded claim

The current source/payload authority remains:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

Descriptive shorthand:

`APP_VISIBLE_4080x3072_BAYER_LIKE_RAW_PAYLOAD_EMBEDDED_IN_16320x12288_HAL_ENVELOPE`

The tested route delivers a `16320x12288` app-visible RAW_SENSOR/HardwareBuffer envelope, while only the first `25,067,520` bytes are populated. Those bytes are an exact source prefix, uniquely match the runtime-advertised standard `4080x3072` RAW_SENSOR byte count, render as a coherent full-frame scene under that interpretation, and exhibit Bayer-like 2x2 spatial structure.

This does **not** prove untouched native ADC, native physical sensor geometry, exact binning/remosaic mechanism or 200 MP optical resolution.

## D. Trusted acquisition/content implementation lineage

Trusted acquisition/content chain:

- v0.14 source-first seal
- v0.16 post-HAL HardwareBuffer envelope
- v0.17 two-door request/delivery airlock
- v0.19 full-raster write audit
- v0.20 payload geometry decoder.

Historical/rejected/isolated:

- v0.15 direct-open physical Camera-5 route — rejected on this device as trusted acquisition lineage
- v0.18 physical-focus probe — isolated parallel research, intentionally not parent of v0.19/v0.20.

Later v0.21+ builds are route-control experiments and do not replace v0.20 as source/payload authority unless new device evidence proves a better route.

## E. Route-control results through v0.28

The project now has three correctly typed, accepted single-variable interventions with no measured RAW-envelope/populated-payload topology change:

- v0.24: `EnableIdealRAW=BYTE(1)`
- v0.26: `RawCbSourceType=INT32(1)`
- v0.28: `EnableXCFAOptimization=BYTE(1)`.

Their representation authorities were established first by no-submit native type oracles:

- v0.23 `EnableIdealRAW`: tag `0x801F0027`, type `BYTE`
- v0.25 `RawCbSourceType`: tag `0x801F0009`, type `INT32`
- v0.27 `EnableXCFAOptimization`: tag `0x801F0036`, type `BYTE`.

The accepted intervention results do not prove those controls have no effect. They only show no measured topology change under the tested key/value/session route.

## F. v0.28 current device differential

v0.28 proves the XCFA session intervention was real: Gate A observed the previous value `0`, builder/request readback after the intervention was `1`, and the session parameter was attached as exactly one unknown vendor variable.

Yet the resulting topology remained:

- source envelope `401,080,320` bytes
- returned `SENSOR_PIXEL_MODE=0`
- `rawBinningFactorUsed=true`
- HONOR `binningFactor=4`
- HONOR `isInSensorZoom=0`
- Stage 3.6 only rows `0..767` populated
- populated prefix `25,067,520` bytes
- Stage 3.7 unique advertised standard RAW match `4080x3072`.

Therefore v0.20 remains current source/payload authority.

## G. Current next route experiment — v0.29

The current device-pending experiment is:

`org.codeaurora.qcamera3.sessionParameters.HALOutputBufferCombined`

v0.29 is **representation oracle only**, not an intervention.

Branch:
`integration/truthraw-suite-v0-29-hal-output-buffer-combined-native-type-oracle`

Build status: **SUCCESS — DEVICE RESULT PENDING**

Expected evidence export:
`TRUTHRAW_CAM5_HAL_OUTPUT_BUFFER_COMBINED_NATIVE_TYPE_ORACLE_v029.json`

On-device protocol: **Step 1 only**. The build must stop at `STAGE 1.5 DIAGNOSTIC STOP` with preview/capture disabled.

It tests native `BYTE`, `INT32`, `FLOAT`, `INT64`, `DOUBLE`, and `RATIONAL` on separate disposable request metadata instances. No capture session is created; no session parameters are attached; no modified vendor request is submitted to HAL; no RAW is captured.

Build record:

- GitHub Actions run `35255819562`
- APK SHA-256 `deb775a8b19fcd02616904ace931c56297d3badbd6d3b679e308137949ba5552`
- artifact ID `10513220672`
- artifact ZIP SHA-256 `84675230ab4f40538d3fb71671ee71d4cde9b05cf430a542f01ef6030ab1d3b0`.

## H. Current open problem

The old 2026-09-16 open question “can we obtain a real 16320x12288 app-visible physical Camera-5 RAW delivery?” is closed by v0.14.

The current open problem is narrower:

`OPEN_NEEDS_ROUTE_DIFFERENTIAL_TO_ESTABLISH_WHETHER_A_DIFFERENT_HONOR_QTI_CONTROL_PATH_CAN_POPULATE_A_LARGER_NATIVE_OR_APP_VISIBLE_RAW_DOMAIN`

Independently:

`OPEN_NEEDS_CALIBRATION_AND_OPTICAL_EVIDENCE_BEFORE_ANY_NATIVE_ADC_OR_200MP_OPTICAL_PROMOTION`.

## I. Permanent scientific authorities retained

These remain active unless explicitly superseded in their own domain:

- `docs/CORE_VISION_SEALED_HOUSE_ARCHITECTURE.md`
- `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md`
- `docs/CORE_VISION_UNCERTAINTY_AWARE_APPEARANCE.md`
- `docs/CORE_VISION_VIRTUAL_OBSERVATION_MANIFOLD.md`
- `docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md`
- exact canonical module README/STATUS/VALIDATION documents
- version-local research contracts under `docs/research/...`.

Permanent global laws remain:

- source evidence immutable;
- representation may exceed source, claims may not exceed evidence;
- measured/reconstructed/censored/unknown/counterfactual/appearance remain distinct;
- single-frame provenance remains explicit;
- censoring is a bound, not a guessed value;
- uncertainty/support is locally bound;
- stage precision is evidence-driven;
- counterfactual/appearance/transport does not write back;
- compute resources do not increase truth authority;
- failed/rejected experiments remain provenance.

## J. Camera-5 evidence interpretation rules

Current rules:

- file/buffer size is not evidence that every declared raster position is populated;
- a non-zero-band test is insufficient to prove independent image content;
- DNG is auxiliary and cannot override Plane[0] source evidence;
- the 15/16 black region is present in the sealed source on tested routes;
- the `.rawpayload` is an exact-prefix derived view, not a replacement source;
- the diagnostic PNG is appearance-only;
- min/max codes alone do not prove calibrated black level or ADC bit depth;
- HONOR/QTI metadata is route evidence, not automatic semantic/calibration authority;
- vendor key names do not establish semantics;
- numeric value `1` does not by itself mean enable/full/native/unbinned;
- native-type resolution must precede unknown vendor-key intervention;
- route-control interventions remain one-variable-at-a-time.

## K. Multi-camera / focus status

The project has physical focus-result and QTI lens-position telemetry plus multicamera sidecars. This supports future controlled multi-focus/multi-camera research.

The v0.18 focus branch remains isolated until the Camera-5 RAW route is better understood.

Any future multi-camera Scientific Master work must seal each physical RAW separately and preserve physical-frame/evidence counts.

## L. Historical documents to preserve but not bootstrap from

Historical global snapshots include:

- `docs/DOCUMENT_STATUS_INDEX_2026-09-10.md`
- `docs/DOCUMENT_STATUS_INDEX_2026-09-16.md`
- `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md`
- older `state/CURRENT_CANONICAL_STATE_*`
- older handoffs/project audits.

Old Camera-5 capability-only/open-physical-gate wording is retained as history. Use the 2026-09-17 Camera-5 current documents for device-route conclusions.

## M. Editing rule going forward

When new route evidence changes the current Camera-5 interpretation:

- update `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md` or create a dated successor when substantial;
- update `state/CURRENT_CAMERA5_RAW_ROUTE_STATE_2026-09-17.json`;
- create/update the versioned experiment state/result document;
- preserve previous branches/builds/evidence rather than rewriting them away;
- update this index/bootstrap;
- never widen scientific authority beyond actual source and calibration evidence.
