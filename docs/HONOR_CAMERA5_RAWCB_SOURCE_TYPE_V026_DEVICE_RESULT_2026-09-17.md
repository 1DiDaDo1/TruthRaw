# HONOR Camera-5 RawCbSourceType v0.26 device result — 2026-09-17

Status: **DEVICE RESULT COMPLETE; NO RAW/PAYLOAD TOPOLOGY DIFFERENTIAL OBSERVED**

## Intervention

v0.25 resolved `org.codeaurora.qcamera3.sessionParameters.RawCbSourceType` as native `INT32`, vendor tag `0x801F0009`.

v0.26 changed exactly one unknown vendor session variable after untouched Gate A:

`RawCbSourceType = INT32(1)`

The numeric value `1` is not assigned vendor semantics by TruthRaw.

Device evidence confirms:

- builder set PASS;
- builder readback `1`;
- built-request readback `1`;
- session parameters attached;
- one controlled vendor intervention only;
- no semantic promotion.

## Capture route

The trusted route remained:

`logical 0 -> physical 5 -> MAXIMUM_RESOLUTION output declaration -> 16320x12288 RAW_SENSOR Image`

Observed:

- physical result camera 5;
- exact Image/result timestamp identity;
- returned SENSOR_PIXEL_MODE `0`;
- `rawBinningFactorUsed=true`;
- source bytes `401,080,320`;
- rowStride `32,640`;
- pixelStride `2`;
- source SHA-256 `99cdc14c882110a28e771362b83240fada2b78b3f7fdf312f0d9b28771fc7040`.

## Stage 3.6 full-raster audit

Result:

`ONLY_FIRST_768_ROWS_NONZERO__EXACT_12P5MP_BYTE_PAYLOAD_SIGNATURE`

Observed:

- `12,288` declared rows;
- only rows `0..767` are non-zero;
- `11,520` rows are all zero;
- only band 0 is non-zero;
- bands 1..15 are all zero;
- populated first-band bytes `25,067,520`;
- sealed-source SHA identity PASS;
- source not modified.

This is the same primary population topology class as the v0.20 control.

## Stage 3.7 payload geometry

Result:

`UNIQUE_ADVERTISED_STANDARD_RAW_BYTE_MATCH_DECODED`

Observed:

- populated prefix: `25,067,520` bytes;
- first non-zero byte offset: `0`;
- last non-zero byte offset: `25,067,518`;
- exactly one advertised standard RAW byte match: `4080x3072`;
- payload file: `TRUTHRAW_1789664349878_CAM5_PAYLOAD_STANDARD_RAW_BYTE_MATCH_v026.rawpayload`;
- payload SHA-256: `11a8d4b4c32b0dd6ecd04896095885c3507225419f18d6f671adde5cb0c96d97`;
- payload hash equals Stage-3.6 first-band hash;
- no byte transform: exact source prefix copy;
- candidate code range for this scene: `60..674`;
- distance-2 spatial correlation remains very high (`dx2 ~= 0.99091`, `dy2 ~= 0.99053`), consistent with the same Bayer-like 2x2 spatial structure.

The diagnostic PNG is appearance-only and not source evidence.

## HONOR route metadata

Still observed:

- `com.hihonor.capture.metadata.binningFactor = 4`;
- `com.hihonor.capture.metadata.isInSensorZoom = 0`;
- AEC real crop begins `[11,8,4058,3055,...]`;
- ISP crop remains `[0,0,16320,12288,0,0,16320,12288]`.

## Differential conclusion against v0.20

`RawCbSourceType=INT32(1)` was accepted and attached as the sole controlled unknown vendor session variable, but **no measurable differential was observed in the RAW envelope or populated payload topology** relative to the v0.20 control.

Unchanged structural class:

- 401,080,320-byte app-visible RAW_SENSOR envelope;
- 25,067,520-byte populated prefix;
- only first 768 declared rows populated;
- unique 4080x3072 standard RAW byte match;
- returned SENSOR_PIXEL_MODE remains 0;
- raw binning flag remains true;
- HONOR binningFactor remains 4;
- HONOR isInSensorZoom remains 0.

This does **not** prove that `RawCbSourceType` is universally ineffective, nor that numeric value `1` has any named vendor meaning. It is a bounded negative differential for this tested Camera-5 route and value.

## Authority boundary

v0.20 remains source/payload control authority. v0.26 adds valid route-control differential evidence only.

Permanent boundary:

`APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF`
