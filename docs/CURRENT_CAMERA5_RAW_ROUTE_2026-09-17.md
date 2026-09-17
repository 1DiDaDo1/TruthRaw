# TruthRaw current Camera-5 RAW route — 2026-09-17

Status: **CURRENT CAMERA-5 ACQUISITION / PAYLOAD AUTHORITY = v0.20; v0.30 FOUR-FACTOR FULL-FACTORIAL ROUTE MATRIX COMPLETE WITH NO MEASURABLE TOPOLOGY DIFFERENTIAL**  
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
- HONOR `allISPCropWindow` beginning `[0,0,16320,12288,...]`;
- Android returned `SENSOR_PIXEL_MODE=0`;
- Android returned `rawBinningFactorUsed=true`;
- HONOR `isInSensorZoom=0` in tested route-control captures.

These are route observations. They do not prove electrical binning, remosaic behavior or native ADC geometry.

## 5. Route-control representation results

Native metadata representations were resolved before intervention:

- v0.23 `EnableIdealRAW`: BYTE, tag `0x801F0027`;
- v0.25 `RawCbSourceType`: INT32, tag `0x801F0009`;
- v0.27 `EnableXCFAOptimization`: BYTE, tag `0x801F0036`;
- v0.29 `HALOutputBufferCombined`: INT32, tag `0x801F0034`.

Vendor names and numeric value `1` are not semantic authority.

## 6. Single-factor route-control results

The first three accepted single-variable interventions were:

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

All 16 unique profiles now have individually readable device evidence JSONs:

`0000, 1111, 0101, 1010, 0011, 1100, 0110, 1001, 0001, 1110, 0010, 1101, 0100, 1011, 1000, 0111`.

All eight complement pairs were therefore observed.

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

This includes all four single-high states, every two-factor state, every three-factor state, the all-UNSET control and all-four-high state.

Bounded complete-matrix conclusion:

`WITHIN_THE_TESTED_BINARY_DESIGN__UNSET_VS_NUMERIC_ONE_AT_THE_RESOLVED_NATIVE_TYPES__NO_SINGLE_FACTOR_OR_COMBINATION_OF_ENABLEIDEALRAW_RAWCBSOURCETYPE_ENABLEXCFAOPTIMIZATION_HALOUTPUTBUFFERCOMBINED_CHANGED_THE_MEASURED_CAMERA5_RAW_ENVELOPE_OR_POPULATED_PREFIX_TOPOLOGY`

For the measured categorical topology response, the response is invariant at every design point. This does not claim pixel-value equality between captures.

## 9. What the complete matrix rules out

The simple hypothesis that one of these four controls, or an interaction among them, at the tested binary levels is sufficient to expose a larger populated app-visible RAW domain is not supported.

In particular:

- A-only, B-only, C-only and D-only each remain in the same topology class;
- all six two-factor combinations remain in the same topology class;
- all four three-factor combinations remain in the same topology class;
- all-four-high remains in the same topology class;
- all-UNSET reproduces the v0.20 topology within the matrix framework.

## 10. What remains open

The complete matrix does not prove:

- untouched/native photodiode ADC output;
- physical sensor geometry is `4080x3072`;
- exact electrical binning/remosaic mechanism;
- 200 MP optical resolution;
- vendor-key names describe true semantics;
- numeric `1` means enabled/full/native/unbinned;
- the four tested keys are globally ineffective;
- other numeric values cannot change the route;
- other vendor controls or operating modes cannot expose another route.

## 11. Current next research direction

Do not repeat this four-key 0/1 matrix. That parameter space is complete.

Next useful work should move to either:

- value-domain / enum-semantics discovery for the INT32 controls, especially `RawCbSourceType` and `HALOutputBufferCombined`, before testing other numeric values; or
- new upstream vendor-route candidates, with native-type oracle first and intervention only after representation is resolved.

Detailed complete matrix report:

`docs/HONOR_CAMERA5_V030_VENDOR_ROUTE_MATRIX_COMPLETE_DEVICE_RESULT_2026-09-17.md`

Machine-readable complete matrix state:

`state/CAMERA5_V030_VENDOR_ROUTE_FULL_FACTORIAL_MATRIX_COMPLETE_STATE_2026-09-17.json`

## 12. Position in TruthRaw architecture

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
