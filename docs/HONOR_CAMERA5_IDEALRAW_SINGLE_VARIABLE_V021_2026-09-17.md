# HONOR Camera-5 IdealRAW single-variable experiment — v0.21 — 2026-09-17

Status: **ACTIVE EXPERIMENT / DEVICE RESULT PENDING**

Parent control: **TruthRaw v0.20**

Experimental branch:

`integration/truthraw-suite-v0-21-idealraw-single-variable`

## Purpose

v0.20 established that the trusted Camera-5 16320x12288 RAW_SENSOR route delivers a 401,080,320-byte app-visible HAL envelope whose populated prefix is 25,067,520 bytes. That exact prefix uniquely matches the advertised standard 4080x3072 RAW_SENSOR byte count and forms a coherent Bayer-like full-frame image when indexed at 4080x3072.

The missing 15/16 therefore exists in the sealed Camera2 source itself. The next test must act upstream of source delivery rather than attempting to repair the 401 MB buffer downstream.

v0.21 changes exactly one unknown vendor control:

`org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW = 1`

No physical meaning is assigned to the vendor name. The experiment asks only whether this one controlled request/session intervention changes the delivered Camera-5 RAW route.

## Preserved acquisition chain

v0.21 reconstructs v0.20 exactly before applying its intervention. It preserves:

`logical camera 0`
`-> physical output Camera 5`
`-> RAW_SENSOR 16320x12288 ImageReader`
`-> MAXIMUM_RESOLUTION OutputConfiguration declaration`
`-> physical-scoped still request`
`-> exact Image / physical Camera-5 SENSOR_TIMESTAMP identity`
`-> original Image.Plane[0] persistence and SHA-256 seal before metadata interpretation`
`-> read-only post-HAL HardwareBuffer envelope`
`-> camera/image close`
`-> read-only Stage 3.6 full-raster audit`
`-> read-only Stage 3.7 payload-geometry decoder`

DNG remains auxiliary.

## Intervention order

The request-side airlock is now:

`Gate A: record untouched app-visible session/request surface`
`-> v0.21: attempt one IdealRAW session-parameter intervention`
`-> session support query / session creation`
`-> Gate B: record the still-request state`
`-> HONOR/QTI vendor pipeline`
`-> Gate C: delivered Image/HardwareBuffer/result`

This preserves an explicit before/after intervention boundary.

## Fail-closed runtime type gate

The app does not set the vendor tag merely because its name sounds useful.

`Camera2IdealRawSessionProbe.kt` requires all of the following before the session can be submitted:

1. `EnableIdealRAW` is advertised in `logical.availableSessionKeys`;
2. the runtime Java value class can be discovered from the actual `CaptureRequest.Key` object;
3. that type is a one-byte representation accepted by the probe (`Byte` or `byte[1]`);
4. the builder accepts value `1`;
5. builder readback is exactly `1`;
6. `SessionConfiguration.setSessionParameters()` accepts the built request.

If any condition fails, v0.21 stops before capture and records a BLOCKED classification. It does not guess another vendor type/value.

## Single-variable contract

The v0.21 helper contains no setters for:

- `RawCbSourceType`;
- `EnableXCFAOptimization`;
- `HALOutputBufferCombined`;
- `EnableInsensorZoom`;
- `EnableSnapshotOnlyInsensorZoom`;
- `EnableMCXMasterCb`;
- any HONOR vendor metadata key.

The only intentional unknown vendor intervention is `EnableIdealRAW=1`.

The normal v0.14 physical `SENSOR_PIXEL_MODE` handling is part of the already proven acquisition topology and is not a new v0.21 variable.

## Evidence fields

The v0.21 evidence JSON adds:

- `controlledVendorInterventionAfterGateA = true`;
- `controlledVendorInterventionKeyCount = 1`;
- `idealRawSingleVariableExperiment`;
- `idealRawExperimentSemanticPromotionAllowed = false`.

The nested experiment record includes:

- key advertisement at logical/physical request/session scope;
- actual runtime value class;
- exact write representation;
- builder readback;
- session-parameter attachment result;
- explicit classification and authority boundary.

## Decision test against v0.20

The important output is not whether the app says the setter succeeded. The important test is whether the delivered source changes.

Compare v0.21 against the unchanged v0.20 control on at least:

- delivered Image/HardwareBuffer geometry;
- source byte length;
- Stage 3.6 populated-prefix bytes;
- non-zero row range;
- band hashes / zero bands;
- Stage 3.7 selected geometry;
- payload code domain and 2x2 phase statistics;
- returned `SENSOR_PIXEL_MODE`;
- returned `rawBinningFactorUsed`;
- HONOR `binningFactor`, AEC/ISP crops and sensor sidecar metadata;
- QTI route/multicamera metadata;
- full source/prefix SHA-256 identities.

Possible bounded outcomes include:

- no material difference from v0.20;
- same 16320x12288 envelope but a different populated prefix;
- 50 MP-like population;
- full 200 MP population;
- different code/CFA topology;
- session/capture rejection.

None of these outcomes alone proves untouched native ADC or exact vendor semantics.

## Authority boundary

Permanent boundary:

`APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF`

A successful change after setting `EnableIdealRAW=1` may support a causal statement limited to this device/software state:

> Changing this one app-visible vendor session parameter changed the delivered Camera2 RAW route.

It does not by itself establish what Qualcomm/HONOR means by “IdealRAW”, what sensor mode was physically selected, or whether the resulting samples are untouched photodiode ADC values.

## Repository locations

Implementation:

- `suite_android/app/src/main/java/com/truthraw/adaptiveui/Camera2IdealRawSessionProbe.kt`
- `tools/patch_fotograaf_v021_idealraw_single_variable.py`
- `.github/workflows/android-truthraw-suite-v0-21-idealraw-single-variable.yml`

Control / inherited analysis:

- `tools/patch_fotograaf_v020_payload_geometry_decoder.py`
- `suite_android/app/src/main/java/com/truthraw/adaptiveui/RawSensorRasterAudit.kt`
- `suite_android/app/src/main/java/com/truthraw/adaptiveui/RawPayloadGeometryDecoder.kt`
- `suite_android/app/src/main/java/com/truthraw/adaptiveui/Camera2PreHalGate.kt`
- `suite_android/app/src/main/java/com/truthraw/adaptiveui/Camera2EnvelopeProbe.kt`

Current Camera-5 authority remains in:

- `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md`
- `state/CURRENT_CAMERA5_RAW_ROUTE_STATE_2026-09-17.json`
