# TruthRaw document status index — 2026-09-17

Status: **CURRENT DOCUMENT GOVERNANCE INDEX** for the 2026-09-17 Camera-5/v0.20 integration line.

This index supersedes `docs/DOCUMENT_STATUS_INDEX_2026-09-16.md` as the current navigation/governance index. The 2026-09-16 index remains historical and must not be rewritten to pretend it already knew the v0.14-v0.20 physical findings.

## A. Global current reading order

1. `README.md`
2. `START_HERE_NEW_CHAT.md`
3. `docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md`
4. `docs/CURRENT_SCENE_PHYSICS_RESTORATION_200MP_2026-09-16.md` — retained scientific background; its Camera-5 physical-gate subsection is superseded by the newer Camera-5 documents below
5. `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md`
6. `state/CURRENT_CAMERA5_RAW_ROUTE_STATE_2026-09-17.json`
7. `docs/DOCUMENT_STATUS_INDEX_2026-09-17.md`
8. `docs/PROJECT_HISTORY_AND_CHANGES_2026-09-16.md`
9. `docs/handoff/TRUTHRAW_CONSOLIDATED_HANDOFF_2026-09-16.md`
10. `docs/handoff/TRUTHRAW_DETAILED_HANDOFF_2026-09-16.md`
11. `docs/handoff/TRUTHRAW_CAMERA5_RAW_ROUTE_V014_TO_V020_2026-09-17.md`
12. exact canonical/research/module documents relevant to the task.

## B. Current Camera-5 authority chain

Read Camera-5 work in this order:

1. `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md` — current bounded interpretation
2. `docs/HONOR_MAGIC8_PRO_TELE_200MP_FULL_RAW_V014_2026-09-17.md` — proven acquisition route/source-first sealing authority
3. `docs/HONOR_CAMERA5_TWO_DOOR_AIRLOCK_V017_2026-09-17.md` — request-side + post-HAL provenance architecture
4. `docs/HONOR_MAGIC8_PRO_TELE_PAYLOAD_GEOMETRY_V020_2026-09-17.md` — completed v0.19/v0.20 payload result
5. `docs/handoff/TRUTHRAW_CAMERA5_RAW_ROUTE_V014_TO_V020_2026-09-17.md` — exact historical/reproduction map
6. `state/CURRENT_CAMERA5_RAW_ROUTE_STATE_2026-09-17.json` — machine-readable current state.

## C. Current Camera-5 bounded claim

The project may now state:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

Descriptive shorthand:

`APP_VISIBLE_4080x3072_BAYER_LIKE_RAW_PAYLOAD_EMBEDDED_IN_16320x12288_HAL_ENVELOPE`

This means that the tested route delivers a `16320x12288` app-visible RAW_SENSOR/HardwareBuffer envelope, while only the first `25,067,520` bytes are populated; those bytes are an exact source prefix, uniquely match the runtime-advertised standard `4080x3072` RAW_SENSOR byte count, render as a coherent full-frame scene under that geometry and exhibit Bayer-like 2x2 spatial structure.

It does **not** prove untouched native ADC, native physical sensor geometry, exact binning/remosaic mechanism or 200 MP optical resolution.

## D. Current trusted implementation lineage

Trusted acquisition/content chain:

- v0.14 source-first seal
- v0.16 post-HAL HardwareBuffer envelope
- v0.17 two-door request/delivery airlock
- v0.19 full-raster write audit
- v0.20 payload geometry decoder.

Historical/rejected/isolated:

- v0.15 direct-open physical Camera-5 route — rejected on this device as a trusted route; useful historical technique only
- v0.18 physical-focus probe — isolated parallel research, intentionally not parent of v0.19/v0.20.

## E. Current physical source facts that supersede the old open gate

The old 2026-09-16 statement that a qualifying real `16320x12288` payload still had to be obtained is no longer current.

A qualifying app-visible physical-Camera-5 `16320x12288` delivery with exact timestamp binding and original Plane[0] source sealing was proven at v0.14.

Subsequent v0.19/v0.20 content analysis established that the tested `401,080,320`-byte envelope is not fully populated: only the first `25,067,520` bytes carry non-zero source codes.

Therefore old wording such as:

`OPEN_NEEDS_REAL_16320x12288_RAW_PAYLOAD_AND_TOTALCAPTURERESULT_BINDING`

is historical, not current.

The open problem has moved upstream/downstream in a more precise form:

`OPEN_NEEDS_ROUTE_DIFFERENTIAL_TO_ESTABLISH_WHETHER_A_DIFFERENT_HONOR_QTI_CONTROL_PATH_CAN_POPULATE_A_LARGER_NATIVE_OR_APP_VISIBLE_RAW_DOMAIN`

and independently:

`OPEN_NEEDS_CALIBRATION_AND_OPTICAL_EVIDENCE_BEFORE_ANY_NATIVE_ADC_OR_200MP_OPTICAL_PROMOTION`.

## F. Permanent scientific authorities retained

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
- one source frame remains one physical frame and one evidence source unless a new physical modality is explicitly admitted;
- censoring is a bound, not a guessed value;
- uncertainty/support is locally bound;
- stage precision is evidence-driven;
- counterfactual/appearance/transport does not write back;
- compute resources do not increase truth authority;
- failed/rejected experiments remain provenance.

## G. Camera-5 evidence interpretation rules

Current rules:

- file/buffer size is not evidence that every declared raster position is populated;
- a non-zero-band test is insufficient to prove independent image content;
- DNG is auxiliary and cannot override Plane[0] source evidence;
- the 15/16 black region is present in the sealed source on tested v0.19/v0.20 routes, so it is not a gallery/DngCreator-only problem;
- the `.rawpayload` is an exact-prefix derived view, not a replacement source;
- the diagnostic PNG is appearance-only;
- min code 64 / max code 1023 are clues, not yet black-level/ADC-bit-depth proof;
- HONOR `binningFactor=4`, AEC/ISP crop domains and QTI/HONOR vendor tags are route observations, not calibration authority;
- vendor key names do not establish semantics.

## H. Current next route experiment

Use v0.20 unchanged as the control.

First candidate single-variable differential:

`org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW`

Only proceed after runtime key lookup and value-type verification on this device. The first experimental build must change no second unknown vendor route key.

The same v0.20 source-first seal, airlock, Stage 3.6 and Stage 3.7 audits must be reused so the differential is interpretable.

Other candidates such as `RawCbSourceType`, XCFA, HALOutputBufferCombined and in-sensor-zoom controls are later separate experiments, not simultaneous toggles.

## I. Multi-camera / focus status

The project has discovered physical focus-result and QTI lens-position telemetry, plus multicamera sidecars. This supports future controlled multi-focus/multi-camera research.

The v0.18 focus branch remains isolated until the Camera-5 RAW route is better understood.

Any future multi-camera Scientific Master work must seal each physical RAW separately and preserve physical-frame/evidence counts. Simultaneous capture does not automatically imply independent evidence.

## J. Historical documents to preserve but not bootstrap from

Historical global snapshots include:

- `docs/DOCUMENT_STATUS_INDEX_2026-09-10.md`
- `docs/DOCUMENT_STATUS_INDEX_2026-09-16.md`
- `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md`
- older `state/CURRENT_CANONICAL_STATE_*`
- older handoffs/project audits.

The old Camera-5 capability-only/open-physical-gate wording inside 2026-09-16 documents is retained as history. Use the 2026-09-17 Camera-5 documents for current device-route conclusions.

## K. Editing rule going forward

When new route evidence changes the current Camera-5 interpretation:

- update `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md` or create a dated successor when the change is substantial;
- update `state/CURRENT_CAMERA5_RAW_ROUTE_STATE_2026-09-17.json` or create a versioned successor;
- add the experiment to the detailed Camera-5 handoff/history;
- preserve the previous branch/build/evidence rather than rewriting it away;
- update the current document-status index/bootstrap;
- never widen scientific authority beyond the actual source and calibration evidence.
