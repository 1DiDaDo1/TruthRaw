# TruthRaw current Camera-5 RAW route — 2026-09-17

Status: **CURRENT CAMERA-5 ACQUISITION / PAYLOAD AUTHORITY = v0.20; ROUTE-CONTROL RESEARCH TRACKED THROUGH v0.29 BUILD**  
Device: HONOR Magic 8 Pro / BKQ-N49  
Logical camera: `0`  
Physical tele camera: `5`

Permanent boundary:

`APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF`

This document is the current bounded Camera-5 interpretation. It preserves v0.14 source-first acquisition authority, v0.16/v0.17 airlock provenance, v0.19 raster-population evidence, v0.20 payload-geometry evidence, and the later single-variable route-control experiments.

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

The route-control sequence is deliberately split into two steps:

1. **representation oracle:** resolve the real native `camera_metadata` element type with disposable request metadata only; no session, no capture and no modified HAL submit;
2. **single-variable intervention:** only after type resolution, set exactly one vendor key/value, preserve the v0.20 acquisition/audit chain, and compare the resulting topology against v0.20.

Numeric value `1` is an experimental control value, not a proven vendor semantic.

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

Gate B had already observed a current/default byte value `[0]`; that observation was a clue, not native-type proof.

v0.28 applied exactly:

`EnableXCFAOptimization = BYTE(1)`

The v0.28 evidence proves:

- one controlled unknown vendor key only;
- builder readback before set: `0`;
- builder readback after set: `1`;
- built request readback: `1`;
- session parameters attached;
- source-first capture completed through Stage 3.6 and Stage 3.7.

v0.28 device result remained:

- app-visible source envelope: `401,080,320` bytes;
- returned `SENSOR_PIXEL_MODE=0`;
- `rawBinningFactorUsed=true`;
- HONOR `binningFactor=4`;
- HONOR `isInSensorZoom=0`;
- `AECRealCropWindow` begins `[11,8,4058,3055]`;
- `allISPCropWindow` begins `[0,0,16320,12288]`;
- Stage 3.6: only band 0 / rows 0..767 populated;
- Stage 3.6 populated prefix: `25,067,520` bytes;
- Stage 3.7 unique advertised standard RAW match: `4080x3072`;
- v0.28 exact payload SHA-256: `323ac8fe2acac81af06e7fddc6464da63f73bdf004e72a91cf730d4d98193646`.

Bounded result:

`XCFA_BYTE_ONE_ACCEPTED_AND_ATTACHED_BUT_NO_MEASURABLE_RAW_ENVELOPE_OR_POPULATED_PAYLOAD_TOPOLOGY_DIFFERENTIAL_ON_TESTED_CAMERA5_ROUTE`

This does not prove XCFA has no effect; only that this intervention caused no measured topology change in the tested route.

## 9. Current cumulative interpretation

Three different, correctly typed, accepted single-variable interventions have now completed without changing the measured RAW envelope/population topology:

- `EnableIdealRAW=BYTE(1)`;
- `RawCbSourceType=INT32(1)`;
- `EnableXCFAOptimization=BYTE(1)`.

This narrows the search. It still does not establish that those keys are ineffective, that value `1` has a particular meaning, or that no multi-condition vendor route exists.

## 10. v0.29 — HALOutputBufferCombined native type oracle

The next candidate is:

`org.codeaurora.qcamera3.sessionParameters.HALOutputBufferCombined`

Reason for selection: it is exposed on logical and physical request/session surfaces, remains untested, and its name is potentially relevant to the unusual large-envelope/small-populated-prefix observation. The name itself grants **no** semantic authority.

v0.29 is therefore an oracle only. It does not intervene.

Branch:
`integration/truthraw-suite-v0-29-hal-output-buffer-combined-native-type-oracle`

Build status: **SUCCESS; DEVICE RESULT PENDING**

GitHub Actions run: `35255819562`

APK:
- bytes: `4,899,361`
- SHA-256: `deb775a8b19fcd02616904ace931c56297d3badbd6d3b679e308137949ba5552`

Artifact:
- ID: `10513220672`
- ZIP bytes: `1,595,399`
- ZIP SHA-256: `84675230ab4f40538d3fb71671ee71d4cde9b05cf430a542f01ef6030ab1d3b0`

v0.29 tests disposable native request metadata as `BYTE`, `INT32`, `FLOAT`, `INT64`, `DOUBLE` and `RATIONAL`, followed by readback/type/count validation. It creates no capture session, attaches no session parameters, submits no capture and never sends a vendor-modified request to HAL.

Required on-device action: **Step 1 only**. Expected stop:

`STAGE 1.5 DIAGNOSTIC STOP`

Expected export:

`TRUTHRAW_CAM5_HAL_OUTPUT_BUFFER_COMBINED_NATIVE_TYPE_ORACLE_v029.json`

## 11. Claims that remain forbidden

Do not promote the current result to:

- untouched/native photodiode ADC output;
- proof that the physical sensor itself is `4080x3072`;
- proof that all payload samples are independent photodiode ADC measurements;
- proof of exact sensor binning/remosaic mechanism;
- proof of 200 MP optical resolution;
- proof of calibrated black level from a single capture minimum;
- proof of 10-bit ADC from max code `1023` alone;
- proof that a vendor key name describes its semantics;
- proof that numeric value `1` means enabled/full/native/unbinned.

## 12. Position in TruthRaw architecture

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
