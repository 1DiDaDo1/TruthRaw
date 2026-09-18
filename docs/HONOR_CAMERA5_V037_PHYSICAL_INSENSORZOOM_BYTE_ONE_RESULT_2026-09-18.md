# HONOR Camera-5 v0.37 physical-only inSensorZoomEnable BYTE(1) result — 2026-09-18

Status: **DEVICE RESULT COMPLETE — ATTACHMENT PASS, CAPTURE PASS, NO RAW TOPOLOGY DIFFERENTIAL**

## Scope

v0.37 preserves v0.20 as source/payload authority and changes exactly one vendor variable after Gate A:

`org.codeaurora.qcamera3.sessionParameters.inSensorZoomEnable = BYTE(1)`

Selection was based on v0.36 physical-only availability plus unique BYTE representation within the four-candidate set. The vendor name and numeric value remain experimental stimuli only.

## Intervention evidence

The uploaded v0.37 evidence reports:

- native tag from v0.36: `0x801F0029`
- native type: `BYTE`
- logical session advertised: false
- physical session advertised: true
- logical request advertised: false
- physical request advertised: true
- physical override advertised: false
- physical-only availability topology pass: true
- builder set pass
- builder readback = 1
- built request readback = 1
- session parameters attached = true
- capture permitted = true
- exactly one vendor key written
- no other v0.36 candidate written
- semantic promotion disabled

This is a positive framework/session-attachment feasibility result. It does not prove vendor semantics or that the HAL acted on the control.

## Source evidence

The capture completed through the unchanged logical-0 -> physical-5 MAX source-first chain.

- app-visible RAW_SENSOR envelope: 16320x12288
- bytes: 401,080,320
- row stride: 32,640
- pixel stride: 2
- source SHA-256:
  `b1c808c36fcb125307e279720303b071d4360f64b8c80db3d4727d17d9b9d85b`
- physical result Camera ID: 5
- timestamp identity: PASS
- focal length: 22.48 mm
- `rawBinningFactorUsed = true`
- returned SENSOR_PIXEL_MODE = 0 remains an independent observation and mismatch is preserved

## Stage 3.6

Read-only sealed-file audit remains:

- 768 non-zero rows
- 11,520 zero rows
- first non-zero row 0
- last non-zero row 767
- exact populated prefix: 25,067,520 bytes
- classification:
  `ONLY_FIRST_768_ROWS_NONZERO__EXACT_12P5MP_BYTE_PAYLOAD_SIGNATURE`

The independently uploaded .rawsensor SHA-256 matches the evidence JSON.

## Stage 3.7

The decoder again finds exactly one advertised standard RAW geometry matching the populated byte count:

- 4080x3072
- 25,067,520 bytes
- exact source prefix [0, 25,067,520)
- no transform
- candidate payload SHA-256:
  `df177751937e9dc2db7e8475024725e4d75276356e470c3b2cbb00a9bb1604a2`

The independently uploaded .rawpayload is byte-for-byte identical to that prefix and its SHA-256 matches the JSON.

Classification remains:

`EXACT_PREFIX_BYTES_MATCH_ONE_ADVERTISED_STANDARD_RAW_GEOMETRY__GEOMETRY_INTERPRETATION_CANDIDATE_NOT_SENSOR_PROOF`

## Coarse result-side observations

The result remains in the previously observed route class:

- HONOR `binningFactor = 4`
- HONOR `isInSensorZoom = 0`
- `AECRealCropWindow` begins `[11,8,4058,3055]`
- `allISPCropWindow = [0,0,16320,12288,0,0,16320,12288]`

These are observations only. Similar names do not establish causal linkage to the written key.

## Bounded conclusion

v0.37 establishes:

1. a physical-only advertised vendor key can be represented as its v0.36-resolved native BYTE type;
2. the resulting request can be attached as SessionConfiguration session parameters despite the key not appearing on the logical session/request advertised surfaces;
3. the session and physical Camera-5 MAX capture can complete;
4. the app-visible RAW envelope/populated-prefix topology still does not change.

Therefore the v0.37 outcome is:

`ATTACHMENT_PASS__CAPTURE_PASS__NO_MEASURABLE_RAW_TOPOLOGY_DIFFERENTIAL`

It does not prove that BYTE(1) means enabled, that the HAL consumed the control semantically, or that the key is ineffective in other contexts.

## Next decision

Do not enumerate more BYTE values.

The remaining v0.36 physical-only candidates I/J/K are one evidence-equivalent INT32 cohort:

- I: `EnableVSR`, tag `0x801F000B`
- J: `ExtendedMaxZoom`, tag `0x801F000A`
- K: `enableQLL`, tag `0x801F000D`

The next experiment should screen this cohort one key at a time at the already established numeric stimulus 1, with:

- fresh source-first capture for each run;
- exactly one vendor key written per run;
- no combinations;
- no enum/value interpretation;
- immediate stop if any run leaves the v0.20 topology class.

This tests the remaining evidence-qualified key dimension without returning to blind value enumeration.
