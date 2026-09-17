# TruthRaw document status index — 2026-09-17

Status: **CURRENT DOCUMENT GOVERNANCE INDEX** for the 2026-09-17 Camera-5 line through the partial v0.30 device matrix.

This index supersedes `docs/DOCUMENT_STATUS_INDEX_2026-09-16.md` as the current navigation/governance index. Historical indexes remain preserved as historical state.

## A. Global current reading order

1. `README.md`
2. `START_HERE_NEW_CHAT.md`
3. `docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md`
4. `docs/CURRENT_SCENE_PHYSICS_RESTORATION_200MP_2026-09-16.md`
5. `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md`
6. `state/CURRENT_CAMERA5_RAW_ROUTE_STATE_2026-09-17.json`
7. `docs/CURRENT_CAMERA5_ROUTE_EXPERIMENT_2026-09-17.md`
8. `docs/HONOR_CAMERA5_V030_VENDOR_ROUTE_MATRIX_PARTIAL_DEVICE_RESULTS_2026-09-17.md`
9. `state/CAMERA5_V030_PARTIAL_MATRIX_V030E_FIXED_CONTROLS_STATE_2026-09-17.json`
10. `docs/DOCUMENT_STATUS_INDEX_2026-09-17.md`
11. `docs/handoff/TRUTHRAW_CAMERA5_RAW_ROUTE_V014_TO_V020_2026-09-17.md`
12. exact canonical/research/module documents relevant to the task.

## B. Current Camera-5 authority chain

Read Camera-5 work in this order:

1. `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md` — current bounded interpretation through partial v0.30 device matrix;
2. `state/CURRENT_CAMERA5_RAW_ROUTE_STATE_2026-09-17.json` — machine-readable source/payload authority;
3. `docs/CURRENT_CAMERA5_ROUTE_EXPERIMENT_2026-09-17.md` — current route-control experiment state;
4. `docs/HONOR_CAMERA5_V030_VENDOR_ROUTE_MATRIX_PARTIAL_DEVICE_RESULTS_2026-09-17.md` — detailed partial matrix result;
5. `state/CAMERA5_V030_PARTIAL_MATRIX_V030E_FIXED_CONTROLS_STATE_2026-09-17.json` — machine-readable matrix completion/result state;
6. `docs/HONOR_MAGIC8_PRO_TELE_200MP_FULL_RAW_V014_2026-09-17.md` — proven acquisition route/source-first sealing authority;
7. `docs/HONOR_CAMERA5_TWO_DOOR_AIRLOCK_V017_2026-09-17.md` — request-side + post-HAL provenance architecture;
8. `docs/HONOR_MAGIC8_PRO_TELE_PAYLOAD_GEOMETRY_V020_2026-09-17.md` — completed v0.19/v0.20 payload result;
9. versioned route-control result/build documents v0.21 onward;
10. `docs/handoff/TRUTHRAW_CAMERA5_RAW_ROUTE_V014_TO_V020_2026-09-17.md` — exact historical/reproduction map.

## C. Current Camera-5 bounded claim

The current source/payload authority remains:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

Descriptive shorthand:

`APP_VISIBLE_4080x3072_BAYER_LIKE_RAW_PAYLOAD_EMBEDDED_IN_16320x12288_HAL_ENVELOPE`

The tested route delivers a `16320x12288` app-visible RAW_SENSOR/HardwareBuffer envelope, while only the first `25,067,520` bytes are populated. Those bytes are an exact source prefix, uniquely match the runtime-advertised standard `4080x3072` RAW_SENSOR byte count, render as a coherent full-frame scene under that interpretation, and exhibit Bayer-like 2x2 spatial structure.

This does **not** prove untouched native ADC, native physical sensor geometry, exact binning/remosaic mechanism or 200 MP optical resolution.

## D. Trusted acquisition/content implementation lineage

Trusted acquisition/content chain:

- v0.14 source-first seal;
- v0.16 post-HAL HardwareBuffer envelope;
- v0.17 two-door request/delivery airlock;
- v0.19 full-raster write audit;
- v0.20 payload geometry decoder.

Historical/rejected/isolated:

- v0.15 direct-open physical Camera-5 route — rejected on this device as trusted acquisition lineage;
- v0.18 physical-focus probe — isolated parallel research, intentionally not parent of v0.19/v0.20.

Later route-control experiments do not replace v0.20 as source/payload authority unless new device evidence genuinely changes the bounded source/topology claim.

## E. Route-control representation and single-factor results

Native representations resolved on-device:

- v0.23 `EnableIdealRAW`: tag `0x801F0027`, type `BYTE`;
- v0.25 `RawCbSourceType`: tag `0x801F0009`, type `INT32`;
- v0.27 `EnableXCFAOptimization`: tag `0x801F0036`, type `BYTE`;
- v0.29 `HALOutputBufferCombined`: tag `0x801F0034`, type `INT32`.

Accepted single-variable interventions with no measured RAW-envelope/populated-payload topology change:

- v0.24 `EnableIdealRAW=BYTE(1)`;
- v0.26 `RawCbSourceType=INT32(1)`;
- v0.28 `EnableXCFAOptimization=BYTE(1)`.

Numeric `1` remains a type-valid experimental control value, not proven vendor semantics.

## F. v0.30 designed interaction screen

The project moved from one-factor-at-a-time screening to a complete 2-level, 4-factor interaction screen after the four native representations were resolved.

Factors:

- A = `EnableIdealRAW` / BYTE;
- B = `RawCbSourceType` / INT32;
- C = `EnableXCFAOptimization` / BYTE;
- D = `HALOutputBufferCombined` / INT32.

Matrix low = `UNSET_NO_WRITE`. Matrix high = numeric `1` in the resolved native representation. Low is not explicit zero and neither level has promoted vendor semantics.

The complete design contains 16 profiles.

## G. Current partial v0.30 device result

Individually readable evidence JSONs currently prove eight unique profiles:

`R01 0000`
`R02 1111`
`R04 1010`
`R11 0010`
`R13 0100`
`R14 1011`
`R15 1000`
`R16 0111`.

A second independent R16 capture is retained as a structural replicate.

All currently hard-evidenced profiles preserve the same measured topology class as v0.20:

- source envelope `401,080,320` bytes / `16320x12288`;
- first `768` declared rows populated;
- remaining `11,520` rows zero;
- populated/payload size `25,067,520` bytes;
- unique advertised standard RAW byte match `4080x3072`;
- no measured envelope/populated-prefix topology differential.

R01 `0000` is the contemporaneous all-UNSET control and reproduces v0.20 topology without matrix vendor writes.

R02 `1111` shows that setting numeric `1` on all four currently type-resolved factors simultaneously is not sufficient to change the measured topology on the tested route.

A-only (`R15`), B-only (`R13`) and C-only (`R11`) are individually hard-evidenced without topology differential. D-only (`R09`) remains missing.

Current missing individually readable profiles:

`R03, R05, R06, R07, R08, R09, R10, R12`.

## H. v0.30e recovery build and UI boundary

Original v0.30 and the scroll-based v0.30c/v0.30d approaches were not reliably usable on the device because long Stage-3 output displaced essential controls.

v0.30e fixes essential controls above the detail pane and adds first-missing recovery. It does not change the scientific matrix, acquisition order, source-seal order, Stage 3.6 or Stage 3.7.

Branch:

`integration/truthraw-suite-v0-30e-fixed-controls-matrix-recovery`

Build status: **SUCCESS**.

## I. Evidence-bundle boundary

Multiple `TRUTHRAW_CAM5_V030_MATRIX_EVIDENCE_BUNDLE_*.zip` files were uploaded as recovery bundles. They remain auxiliary provenance containers.

Archive inspection repeatedly timed out in the current analysis environment. Therefore no run is promoted from ZIP filename, bundle size, UI progression or recollection alone. Matrix completion is currently defined by individually readable evidence JSON files.

## J. Current open problem

The old question “can we obtain a real `16320x12288` app-visible physical Camera-5 RAW delivery?” is closed by the source-first Camera-5 route.

The current open problem is narrower:

`OPEN_NEEDS_ROUTE_DIFFERENTIAL_TO_ESTABLISH_WHETHER_A_DIFFERENT_HONOR_QTI_CONTROL_PATH_CAN_POPULATE_A_LARGER_NATIVE_OR_APP_VISIBLE_RAW_DOMAIN`

Independently:

`OPEN_NEEDS_CALIBRATION_AND_OPTICAL_EVIDENCE_BEFORE_ANY_NATIVE_ADC_OR_200MP_OPTICAL_PROMOTION`.

## K. Permanent scientific authorities retained

These remain active unless explicitly superseded in their own domain:

- `docs/CORE_VISION_SEALED_HOUSE_ARCHITECTURE.md`;
- `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md`;
- `docs/CORE_VISION_UNCERTAINTY_AWARE_APPEARANCE.md`;
- `docs/CORE_VISION_VIRTUAL_OBSERVATION_MANIFOLD.md`;
- `docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md`;
- exact canonical module README/STATUS/VALIDATION documents;
- version-local research contracts under `docs/research/...`.

Permanent global laws remain: source evidence immutable; representation may exceed source but claims may not exceed evidence; measured/reconstructed/censored/unknown/counterfactual/appearance remain distinct; evidence count remains physical; uncertainty/support stays local; counterfactual/appearance never writes back; failed/rejected experiments remain provenance.

## L. Camera-5 evidence interpretation rules

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
- matrix low means UNSET, not explicit zero;
- a matrix hit establishes a combination-level differential first;
- bundle metadata alone does not complete a run.

## M. Editing rule going forward

When new route evidence changes the current Camera-5 interpretation:

- update `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md` or create a dated successor when substantial;
- update `state/CURRENT_CAMERA5_RAW_ROUTE_STATE_2026-09-17.json` if source/payload authority changes;
- update `docs/CURRENT_CAMERA5_ROUTE_EXPERIMENT_2026-09-17.md`;
- update `state/CAMERA5_V030_PARTIAL_MATRIX_V030E_FIXED_CONTROLS_STATE_2026-09-17.json`;
- preserve previous branches/builds/evidence rather than rewriting them away;
- update this index/bootstrap;
- never widen scientific authority beyond actual source and calibration evidence.
