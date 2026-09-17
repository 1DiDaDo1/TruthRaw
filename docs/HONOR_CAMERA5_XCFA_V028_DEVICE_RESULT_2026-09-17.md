# HONOR Camera-5 EnableXCFAOptimization BYTE intervention v0.28 — device result

Date: 2026-09-17

Status: **DEVICE RESULT COMPLETE — INTERVENTION ACCEPTED, NO MEASURABLE RAW TOPOLOGY DIFFERENTIAL**

## Experiment

v0.27 resolved:

- key: `org.codeaurora.qcamera3.sessionParameters.EnableXCFAOptimization`
- vendor tag: `0x801F0036`
- native metadata type: `BYTE`
- accepted native type count: `1`
- no session or capture submission during type resolution.

v0.28 therefore changes exactly one unknown vendor session variable after untouched Gate A:

`EnableXCFAOptimization = BYTE(1)`

No semantic meaning is inferred from the key name or numeric value.

## Intervention proof

The v0.28 evidence records:

- single unknown vendor variable: true
- builder value before set: `0`
- builder set: pass
- builder readback: `1`
- request readback: `1`
- session parameters attached: true
- intervention classification: `XCFA_BYTE_ONE_ATTACHED_AS_SINGLE_VENDOR_SESSION_VARIABLE__SEMANTICS_UNPROVEN`

This establishes that the Camera2/session layer accepted the intervention representation and attached it to the tested session configuration.

## Capture route

Evidence file:

`TRUTHRAW_1789666565218_CAM5_200MP_EVIDENCE_v028.json`

Observed route:

`logical 0 -> physical 5 -> MAXIMUM_RESOLUTION output declaration -> 16320x12288 RAW_SENSOR Image`

Observed capture facts:

- physical result camera: `5`
- exact Image / sensor timestamp identity: pass
- source envelope: `401,080,320` bytes
- row stride: `32,640`
- pixel stride: `2`
- source SHA-256: `219b527630f27a4aa959f1523ca34d6cfd379e7b862998aa60ea0c1ea18dca4c`
- returned `SENSOR_PIXEL_MODE = 0`
- `rawBinningFactorUsed = true`
- HONOR `binningFactor = 4`
- HONOR `isInSensorZoom = 0`
- HONOR `AECRealCropWindow` begins `[11, 8, 4058, 3055]`
- HONOR `allISPCropWindow` begins `[0, 0, 16320, 12288]`.

## Stage 3.6

The sealed-source audit remains in the same topology class as the v0.20 control:

- only declared rows `0..767` are non-zero
- declared rows `768..12287` are all zero
- only band `0` is populated
- bands `1..15` are all zero
- populated prefix: `25,067,520` bytes
- populated samples: `12,533,760` U16
- classification: `ONLY_FIRST_768_ROWS_NONZERO__EXACT_12P5MP_BYTE_PAYLOAD_SIGNATURE`.

The re-read SHA-256 matches the original sealed-source SHA, so the audit remains read-only.

## Stage 3.7

The populated prefix again has exactly one runtime-advertised standard RAW byte-count match:

`4080 x 3072 x 2 = 25,067,520 bytes`

Selected candidate:

- geometry: `4080x3072`
- exact prefix-copy file: `TRUTHRAW_1789666565218_CAM5_PAYLOAD_STANDARD_RAW_BYTE_MATCH_v028.rawpayload`
- payload SHA-256: `323ac8fe2acac81af06e7fddc6464da63f73bdf004e72a91cf730d4d98193646`
- payload SHA equals Stage-3.6 first-band SHA: true
- copy transform: none
- min code in this scene: `62`
- max code: `1023`.

Same-phase distance-2 spatial correlations remain very high:

- horizontal dx=2: `0.9865536814664547`
- vertical dy=2: `0.9867309386181549`.

The diagnostic PNG forms a coherent scene when the prefix is indexed as `4080x3072`; it remains appearance-only and is not scientific evidence.

## Differential conclusion

Against the unchanged v0.20 control, v0.28 shows no measurable change in:

- declared 401 MB RAW envelope
- populated-prefix byte count
- 16-band population signature
- selected 4080x3072 standard RAW candidate
- returned SENSOR_PIXEL_MODE
- HONOR binningFactor.

Bounded conclusion:

`XCFA_BYTE_ONE_ACCEPTED_AND_ATTACHED_BUT_NO_MEASURABLE_RAW_ENVELOPE_OR_POPULATED_PAYLOAD_TOPOLOGY_DIFFERENTIAL_ON_TESTED_CAMERA5_ROUTE`

This **does not** prove that XCFA optimization has no effect. It only shows that this single-variable `BYTE(1)` session intervention did not change the measured source-envelope / populated-payload topology in this tested route.

## Current authority

v0.20 remains the current source/payload interpretation authority:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

v0.28 is route-control differential evidence only.

## Next safe question

Before changing another unknown vendor variable, resolve its native camera-metadata representation without HAL submission.

Next candidate:

`org.codeaurora.qcamera3.sessionParameters.HALOutputBufferCombined`

The name is not treated as semantic authority. The next step should be a native-type oracle only; no session parameter intervention until the device resolves the representation unambiguously.
