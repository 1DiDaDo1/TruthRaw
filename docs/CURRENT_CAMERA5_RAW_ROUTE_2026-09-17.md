# TruthRaw current Camera-5 RAW route — 2026-09-17

Status: **CURRENT CAMERA-5 ACQUISITION / PAYLOAD AUTHORITY = v0.20; v0.30 BINARY MATRIX AND v0.31 INT32 VALUE SWEEP COMPLETE WITH NO MEASURABLE TOPOLOGY DIFFERENTIAL**  
Device: HONOR Magic 8 Pro / BKQ-N49  
Logical camera: `0`  
Physical tele camera: `5`

Permanent boundary:

`APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF`

## 1. Current source/payload authority

Trusted route:

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

- physical result Camera `5`;
- delivered app-visible Image `16320x12288`;
- source bytes `401,080,320`;
- row stride `32,640`;
- pixel stride `2`;
- exact Image/result timestamp identity;
- returned `SENSOR_PIXEL_MODE=0`;
- `rawBinningFactorUsed=true`;
- source-first seal before returned-pixel-mode interpretation.

Stage 3.6 proved that only rows `0..767` are populated in the declared 16320-wide interpretation. Rows `768..12287` are all zero, producing an exact populated prefix of `25,067,520` bytes.

Stage 3.7 found exactly one runtime-advertised standard RAW_SENSOR geometry with the same U16 byte count:

`4080 * 3072 * 2 = 25,067,520 bytes`.

The exact source prefix is copied without transform to `.rawpayload` derived evidence. Spatial diagnostics show a coherent full-frame scene and Bayer-like 2x2 structure when indexed as `4080x3072`.

v0.20 remains source/payload authority.

## 3. HAL / HardwareBuffer envelope

The app-visible delivery envelope remains:

- Java Image `16320x12288`, RAW_SENSOR format 32;
- one plane;
- pixel stride `2`;
- row stride `32640`;
- capacity `401080320` bytes;
- HardwareBuffer `16320x12288`, one layer, format 32;
- native stride `16320` pixels;
- envelope probe does not lock, map or write the buffer.

The 401-MB object remains the primary sealed evidence object even though tested captures populate only the first 25,067,520 bytes.

## 4. HONOR/QTI metadata observations retained

Repeated tested captures expose multiple coordinate/readout domains, including:

- HONOR `binningFactor=4`;
- HONOR `AECRealCropWindow` around `[11,8,4058,3055,...]`;
- HONOR `allISPCropWindow=[0,0,16320,12288,0,0,16320,12288]` in the v0.31 captures;
- Android returned `SENSOR_PIXEL_MODE=0`;
- Android returned `rawBinningFactorUsed=true`;
- HONOR `isInSensorZoom=0` in the tested route-control captures.

These are route observations. They do not prove electrical binning, remosaic behavior or native ADC geometry.

## 5. Route-control representation results

Native metadata representations were resolved before intervention:

- v0.23 `EnableIdealRAW`: BYTE, tag `0x801F0027`;
- v0.25 `RawCbSourceType`: INT32, tag `0x801F0009`;
- v0.27 `EnableXCFAOptimization`: BYTE, tag `0x801F0036`;
- v0.29 `HALOutputBufferCombined`: INT32, tag `0x801F0034`.

Vendor names and numeric values are not semantic authority.

## 6. Single-factor route-control results

The first three accepted single-variable numeric-one interventions were:

- v0.24 `EnableIdealRAW=BYTE(1)`;
- v0.26 `RawCbSourceType=INT32(1)`;
- v0.28 `EnableXCFAOptimization=BYTE(1)`.

Each was accepted and attached, yet each remained in the v0.20 structural class: `401,080,320`-byte envelope, first 768 rows populated, `25,067,520`-byte prefix and unique `4080x3072` standard RAW byte match.

These were bounded negative differentials only.

## 7. v0.30 complete four-factor matrix

v0.30 tested all `2^4 = 16` combinations of:

- A `EnableIdealRAW` / BYTE;
- B `RawCbSourceType` / INT32;
- C `EnableXCFAOptimization` / BYTE;
- D `HALOutputBufferCombined` / INT32.

Matrix `0` = UNSET / no write. Matrix `1` = numeric `1` in the resolved native representation.

All 16 unique profiles have individually readable device evidence JSONs and all eight complement pairs were observed.

## 8. Complete v0.30 measured result

All 16 profiles remain in the same measured app-visible topology class as v0.20:

- physical Camera `5`;
- `16320x12288` declared RAW envelope;
- `401,080,320` source bytes;
- row stride `32,640`, pixel stride `2`;
- returned `SENSOR_PIXEL_MODE=0` in the tested captures;
- `rawBinningFactorUsed=true`;
- Stage 3.6: only first `768` declared rows populated;
- remaining `11,520` rows zero;
- Stage 3.7: exact `25,067,520`-byte source prefix;
- unique advertised standard RAW match `4080x3072`;
- no measured RAW-envelope/populated-prefix topology differential.

Bounded complete-matrix conclusion:

`WITHIN_THE_TESTED_BINARY_DESIGN__UNSET_VS_NUMERIC_ONE_AT_THE_RESOLVED_NATIVE_TYPES__NO_SINGLE_FACTOR_OR_COMBINATION_OF_ENABLEIDEALRAW_RAWCBSOURCETYPE_ENABLEXCFAOPTIMIZATION_HALOUTPUTBUFFERCOMBINED_CHANGED_THE_MEASURED_CAMERA5_RAW_ENVELOPE_OR_POPULATED_PREFIX_TOPOLOGY`

For the measured categorical topology response, the response is invariant at every v0.30 design point.

## 9. v0.31 INT32 value-domain sweep

v0.31 changed the numeric domain for the two INT32 controls instead of repeating the completed binary matrix.

Profiles:

- S01 `RawCbSourceType=0`;
- S02 `RawCbSourceType=2`;
- S03 `RawCbSourceType=3`;
- S04 `HALOutputBufferCombined=0`;
- S05 `HALOutputBufferCombined=2`;
- S06 `HALOutputBufferCombined=3`.

`EnableIdealRAW` and `EnableXCFAOptimization` remained UNSET in every v0.31 run. Exactly one unknown vendor key was written per capture.

All six values were accepted by the builder, matched builder readback, matched built-request readback and attached as session parameters. No enum meaning is inferred from this acceptance.

## 10. Complete v0.31 measured result

All six v0.31 captures again remain in the same topology class as v0.20:

- physical result Camera `5`;
- declared `16320x12288` app-visible RAW envelope;
- `401,080,320` source bytes;
- row stride `32,640`, pixel stride `2`;
- Stage 3.6 classification `ONLY_FIRST_768_ROWS_NONZERO__EXACT_12P5MP_BYTE_PAYLOAD_SIGNATURE`;
- first `768` declared rows populated;
- remaining `11,520` rows zero;
- payload bytes `25,067,520`;
- Stage 3.7 status `UNIQUE_ADVERTISED_STANDARD_RAW_BYTE_MATCH_DECODED`;
- unique advertised standard RAW match `4080x3072`;
- exact-prefix derived payload with no source replacement;
- no measured RAW-envelope/populated-prefix topology differential.

The returned coarse route-side metadata also remained in the same class across all six captures:

- HONOR `binningFactor=4`;
- HONOR `AECRealCropWindow` prefix `[11,8,4058,3055]`;
- HONOR `allISPCropWindow=[0,0,16320,12288,0,0,16320,12288]`;
- HONOR `isInSensorZoom=0`;
- Android `rawBinningFactorUsed=true`.

Source and payload hashes differ across captures, and scene/exposure/focus varied, so pixel-value differences are not interpreted causally.

Bounded v0.31 conclusion:

`DEVICE_SWEEP_COMPLETE_6_OF_6__RAWCB_AND_HALOUTPUTBUFFERCOMBINED_INT32_VALUES_0_2_3_ACCEPTED_AND_ATTACHED__NO_MEASURABLE_CAMERA5_RAW_ENVELOPE_OR_POPULATED_PREFIX_TOPOLOGY_DIFFERENTIAL`

Combining v0.31 with prior evidence gives a tested bounded value set of

`{UNSET, 0, 1, 2, 3}`

for both `RawCbSourceType` and `HALOutputBufferCombined` on this route. Within that tested domain neither control changed the measured app-visible RAW topology.

This does not prove explicit zero is semantically equivalent to UNSET or that values outside this domain cannot matter.

## 11. What the route-control program has ruled out so far

The available evidence does not support the simple hypothesis that the tested four-key binary combinations or the tested small INT32 values for B/D are sufficient to expose a larger populated app-visible Camera-5 RAW domain.

Specifically:

- all 16 v0.30 binary profiles are topology-invariant;
- explicit `RawCbSourceType` values `0,1,2,3` are accepted/attached without topology change;
- explicit `HALOutputBufferCombined` values `0,1,2,3` are accepted/attached without topology change;
- the tested route continues to report HONOR binning/crop/in-sensor-zoom observations consistent with the existing 4080x3072 payload interpretation.

## 12. What remains open

The completed route-control work does not prove:

- untouched/native photodiode ADC output;
- physical sensor geometry is `4080x3072`;
- exact electrical binning/remosaic mechanism;
- 200 MP optical resolution;
- vendor-key names describe true semantics;
- numeric values encode any specific enum meaning;
- explicit zero equals UNSET semantically;
- values outside the tested domain are ineffective;
- the tested keys are globally ineffective in other route contexts;
- other vendor controls or operating modes cannot expose another route.

## 13. Current next research direction

Do not repeat the v0.30 binary matrix and do not blindly enumerate larger arbitrary INT32 values.

The next useful route work should move to either:

- a new upstream vendor-route family, with native-type oracle first and intervention only after representation is resolved; or
- a separately justified prerequisite/context experiment if evidence suggests the accepted INT32 controls only act under another route state.

Detailed v0.31 result:

`docs/HONOR_CAMERA5_V031_INT32_VALUE_DOMAIN_SWEEP_DEVICE_RESULT_2026-09-17.md`

Machine-readable v0.31 state:

`state/CAMERA5_V031_INT32_VALUE_DOMAIN_SWEEP_STATE_2026-09-17.json`

Detailed v0.30 result:

`docs/HONOR_CAMERA5_V030_VENDOR_ROUTE_MATRIX_COMPLETE_DEVICE_RESULT_2026-09-17.md`

## 14. Position in TruthRaw architecture

This remains acquisition/provenance/sample-topology research:

`request/session intent`
`-> HONOR/QTI vendor pipeline`
`-> app-visible Image/HardwareBuffer`
`-> immutable source seal`
`-> integer-exact raster/payload audit`
`-> only then calibration/de-ISP`
`-> Scientific Master`
`-> Dynamic Authority / uncertainty`
`-> TruthRange / zero-line`
`-> appearance/export`.

F64/FP32 and downstream reconstruction cannot increase the authority of the original Camera2 evidence.
