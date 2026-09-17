# TruthRaw current Camera-5 RAW route — 2026-09-17

Status: **CURRENT CAMERA-5 ACQUISITION / PAYLOAD AUTHORITY = v0.20; ROUTE-CONTROL RESEARCH TRACKED THROUGH PARTIAL v0.30 DEVICE MATRIX**  
Device: HONOR Magic 8 Pro / BKQ-N49  
Logical camera: `0`  
Physical tele camera: `5`

Permanent boundary:

`APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF`

This document is the current bounded Camera-5 interpretation. It preserves v0.14 source-first acquisition authority, v0.16/v0.17 airlock provenance, v0.19 raster-population evidence, v0.20 payload-geometry evidence, the later single-variable route-control experiments, and the v0.30 interaction screen.

## 1. Current source/payload authority

The trusted route is:

`logical camera 0`
`-> physical output Camera 5`
`-> MAXIMUM_RESOLUTION output declaration`
`-> physical-scoped still request`
`-> app-visible Android Image/HardwareBuffer declared 16320x12288 RAW_SENSOR`
`-> original Image.Plane[0] sealed first`
`-> post-HAL envelope recorded read-only`
`-> Image/session/reader/camera closed`
`-> sealed file audited read-only`
`-> populated prefix matched against runtime-advertised standard RAW geometries`

Current formal classification:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

Descriptive shorthand:

`APP_VISIBLE_4080x3072_BAYER_LIKE_RAW_PAYLOAD_EMBEDDED_IN_16320x12288_HAL_ENVELOPE`

This remains weaker than proof of untouched ADC output or native physical sensor geometry.

## 2. v0.20 control result

Qualifying control evidence:
`TRUTHRAW_1789654132526_CAM5_200MP_EVIDENCE_v020.json`

Observed control facts include:

- physical result camera `5`;
- delivered app-visible Image `16320x12288`;
- source bytes `401,080,320`;
- row stride `32,640`;
- pixel stride `2`;
- exact Image/result timestamp identity;
- returned `SENSOR_PIXEL_MODE=0`;
- `rawBinningFactorUsed=true`;
- source-first seal before returned-pixel-mode interpretation.

Stage 3.6 proved that only rows `0..767` are populated in the declared 16320-wide interpretation. Rows `768..12287` are all zero. Therefore the populated prefix is exactly:

`16320 * 768 * 2 = 25,067,520 bytes`

Stage 3.7 found exactly one runtime-advertised standard RAW_SENSOR geometry with the same U16 byte count:

`4080 * 3072 * 2 = 25,067,520 bytes`

The exact source prefix was copied without transform to a `.rawpayload`; its SHA-256 matched Stage-3.6 band 0. Spatial diagnostics show a coherent full-frame scene and Bayer-like 2x2 structure when indexed as `4080x3072`.

v0.20 source/payload authority remains unchanged by all later route-control experiments so far.

## 3. HAL / HardwareBuffer envelope

The app-visible delivery envelope remains:

- Java Image: `16320x12288`, RAW_SENSOR format 32;
- one plane;
- pixel stride `2`;
- row stride `32640`;
- capacity `401080320` bytes;
- HardwareBuffer `16320x12288`, one layer, format 32;
- native stride `16320` pixels;
- Java/native HardwareBuffer identity observed;
- envelope probe does not lock, map or write the buffer.

The 401-MB object is therefore retained as the primary sealed evidence object even though the tested captures populate only the first 25,067,520 bytes.

## 4. HONOR/QTI metadata observations retained

Repeated tested captures continue to expose multiple coordinate/readout domains:

- HONOR `binningFactor=4`;
- HONOR `AECRealCropWindow` begins around `[11,8,4058,3055]`;
- HONOR `allISPCropWindow` reports `[0,0,16320,12288,...]`;
- Android returned `SENSOR_PIXEL_MODE=0`;
- Android returned `rawBinningFactorUsed=true`;
- HONOR `isInSensorZoom=0` in tested route-control captures;
- AF/QTI and multicamera sidecars remain observable.

These are observations. They do not prove the sensor's electrical binning, remosaic behavior or native ADC geometry.

## 5. Route-control methodology

Unknown vendor keys are never assigned meaning from their names alone.

The early route-control sequence deliberately split representation discovery and intervention:

1. **representation oracle:** resolve the real native `camera_metadata` element type with disposable request metadata only; no session, no capture and no modified HAL submit;
2. **single-variable intervention:** only after type resolution, set exactly one vendor key/value, preserve the v0.20 acquisition/audit chain, and compare topology against v0.20.

After four representations had been resolved and the first three single-variable interventions yielded no topology differential, v0.30 moved to a designed full-factorial interaction screen.

Numeric value `1` remains an experimental control value, not a proven vendor semantic.

## 6. v0.21–v0.24 — EnableIdealRAW

v0.21 stopped safely because the Java runtime type was unavailable; nothing was submitted.

v0.22 showed multiple app-side marshalling candidates and therefore remained ambiguous; nothing was submitted.

v0.23 used the native metadata oracle and resolved:

- key: `org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW`
- native tag: `0x801F0027`
- native type: `BYTE`
- accepted native type count: `1`
- no session/capture/HAL submission.

v0.24 then applied exactly:

`EnableIdealRAW = BYTE(1)`

The intervention was accepted, builder/request readback succeeded, and the request was attached as session parameters. The resulting RAW topology nevertheless remained in the v0.20 structural class: 401,080,320-byte envelope, 25,067,520 populated bytes, first 768 rows populated, unique `4080x3072` standard RAW byte match.

Bounded result:

`IDEALRAW_BYTE_ONE_ACCEPTED_AND_ATTACHED_BUT_NO_MEASURABLE_RAW_ENVELOPE_OR_POPULATED_PAYLOAD_TOPOLOGY_DIFFERENTIAL_ON_TESTED_CAMERA5_ROUTE`

## 7. v0.25–v0.26 — RawCbSourceType

v0.25 resolved:

- key: `org.codeaurora.qcamera3.sessionParameters.RawCbSourceType`
- native tag: `0x801F0009`
- native type: `INT32`
- accepted native type count: `1`
- no session/capture/HAL submission.

v0.26 applied exactly:

`RawCbSourceType = INT32(1)`

The intervention was accepted and attached, but the measured RAW topology again remained unchanged from the v0.20 structural class.

Bounded result:

`RAWCB_SOURCE_TYPE_INT32_ONE_ACCEPTED_AND_ATTACHED_BUT_NO_MEASURABLE_RAW_ENVELOPE_OR_POPULATED_PAYLOAD_TOPOLOGY_DIFFERENTIAL_ON_TESTED_CAMERA5_ROUTE`

## 8. v0.27–v0.28 — EnableXCFAOptimization

v0.27 resolved:

- key: `org.codeaurora.qcamera3.sessionParameters.EnableXCFAOptimization`
- native tag: `0x801F0036`
- native type: `BYTE`
- accepted native type count: `1`
- no session/capture/HAL submission.

v0.28 applied exactly:

`EnableXCFAOptimization = BYTE(1)`

The intervention was accepted and attached and again remained in the same measured topology class: 401,080,320-byte envelope, 25,067,520-byte populated prefix, first 768 rows populated and a unique `4080x3072` standard RAW byte match.

Bounded result:

`XCFA_BYTE_ONE_ACCEPTED_AND_ATTACHED_BUT_NO_MEASURABLE_RAW_ENVELOPE_OR_POPULATED_PAYLOAD_TOPOLOGY_DIFFERENTIAL_ON_TESTED_CAMERA5_ROUTE`

## 9. v0.29 — HALOutputBufferCombined representation result

Device oracle resolved:

- key: `org.codeaurora.qcamera3.sessionParameters.HALOutputBufferCombined`;
- native tag: `0x801F0034`;
- native type: `INT32`;
- accepted native type count: `1`;
- no session, no capture, no modified HAL submit.

Classification:

`NATIVE_METADATA_TYPE_INT32__NO_SESSION_OR_CAPTURE_SUBMISSION`

This resolved representation only. Numeric value semantics remain unproven.

## 10. v0.30 — designed four-factor interaction screen

The matrix factors are:

- A `EnableIdealRAW` / BYTE / `0x801F0027`;
- B `RawCbSourceType` / INT32 / `0x801F0009`;
- C `EnableXCFAOptimization` / BYTE / `0x801F0036`;
- D `HALOutputBufferCombined` / INT32 / `0x801F0034`.

Matrix level `0` means `UNSET_NO_WRITE`. Matrix level `1` means numeric `1` using the already resolved native representation. The full design contains all `16` combinations.

The v0.30e fixed-controls recovery build preserves the scientific matrix and v0.20 source-first ordering while keeping essential controls permanently reachable and selecting the first missing cached run.

## 11. Current v0.30 device evidence

Individually readable hard-evidence JSONs currently exist for eight unique profiles:

- R01 `0000`;
- R02 `1111`;
- R04 `1010`;
- R11 `0010`;
- R13 `0100`;
- R14 `1011`;
- R15 `1000`;
- R16 `0111`.

A second independent R16 `0111` capture also exists as a structural replicate.

Every one of these hard-evidenced profiles remains in the same measured app-visible topology class as v0.20:

- `16320x12288` declared RAW envelope;
- `401,080,320` source bytes;
- `768` populated rows and `11,520` all-zero rows;
- `25,067,520` populated/payload bytes;
- unique advertised standard RAW match `4080x3072`;
- no measured envelope/populated-prefix topology differential.

R01 `0000` is especially important because it is the contemporaneous all-UNSET control inside the matrix framework. It reproduces v0.20 topology without any matrix vendor writes.

R02 `1111` is the strongest current combined negative result: writing numeric `1` to all four currently type-resolved factors simultaneously is not sufficient to change the measured app-visible RAW envelope/populated-prefix topology.

Single-high hard evidence currently includes:

- R11 `0010`: C only — no topology differential;
- R13 `0100`: B only — no topology differential;
- R15 `1000`: A only — no topology differential.

D-only R09 `0001` is still missing as individually readable hard evidence.

The remaining hard-evidence profiles are:

`R03, R05, R06, R07, R08, R09, R10, R12`

with ABCD values:

`0101, 0011, 1100, 0110, 1001, 0001, 1110, 1101`.

Detailed report:

`docs/HONOR_CAMERA5_V030_VENDOR_ROUTE_MATRIX_PARTIAL_DEVICE_RESULTS_2026-09-17.md`

Machine-readable state:

`state/CAMERA5_V030_PARTIAL_MATRIX_V030E_FIXED_CONTROLS_STATE_2026-09-17.json`

## 12. Bundle handling boundary

Multiple v0.30 evidence bundle ZIPs were uploaded during recovery. They are preserved as auxiliary provenance. Archive inspection timed out in the current analysis environment, so no run is promoted from ZIP filename, bundle size, UI progression or recollection alone.

Until bundle contents are independently parsed, matrix-completion authority comes from individually readable evidence JSONs.

## 13. Claims that remain forbidden

Do not promote the current result to:

- untouched/native photodiode ADC output;
- proof that the physical sensor itself is `4080x3072`;
- proof that all payload samples are independent photodiode ADC measurements;
- proof of exact sensor binning/remosaic mechanism;
- proof of 200 MP optical resolution;
- proof of calibrated black level from a single capture minimum;
- proof of 10-bit ADC from max code `1023` alone;
- proof that a vendor key name describes its semantics;
- proof that numeric value `1` means enabled/full/native/unbinned;
- proof that the four tested vendor controls are globally ineffective;
- proof that the eight still-missing matrix profiles cannot differ.

## 14. Position in TruthRaw architecture

This work remains acquisition/provenance/sample-topology research:

`request/session intent`
`-> HONOR/QTI vendor pipeline`
`-> app-visible Image/HardwareBuffer`
`-> immutable source seal`
`-> integer-exact raster/payload audit`
`-> only then calibration/de-ISP`
`-> Scientific Master`
`-> Dynamic Authority / uncertainty`
`-> TruthRange / zero-line`
`-> appearance/export`

F64/FP32 and downstream reconstruction cannot increase the authority of the original Camera2 evidence.
