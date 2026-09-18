# HONOR Camera-5 v0.35 MCXMasterCb isolated route result — 2026-09-18

Status: **DEVICE RESULT COMPLETE** for the third v0.32 upstream candidate.

## Scope

v0.35 preserves v0.20 as source/payload authority. It changes exactly one vendor session variable after Gate A:

`org.codeaurora.qcamera3.sessionParameters.EnableMCXMasterCb = INT32(1)`

The key name and numeric value remain experimental stimuli only. No vendor semantics are promoted.

## Representation / intervention evidence

The v0.32 oracle had resolved:

- tag: `0x801F0005`
- native metadata type: `INT32`
- logical session/request advertised: true
- physical session/request advertised: false

The v0.35 device evidence reports:

- builder set succeeded
- builder readback = 1
- built-request readback = 1
- session parameters attached
- `vendorKeysWritten = 1`
- `EnableInsensorZoom` not written
- `EnableSnapshotOnlyInsensorZoom` not written
- semantic promotion disabled

## Measured result

The source-first chain remained intact:

`Gate A -> exactly one MCXMasterCb intervention -> logical 0 / physical 5 MAX capture -> original Plane[0] seal -> HAL envelope observation -> Stage 3.6 -> Stage 3.7`

Observed source:

- app-visible RAW_SENSOR envelope: 16320x12288
- bytes: 401,080,320
- row stride: 32,640
- pixel stride: 2
- source SHA-256 reported by device JSON:
  `91732c19a81bdea3393b8033b4148588ff00d64428aeae80ecf28ff63d04fa86`
- timestamp identity: PASS
- focal length: 22.48 mm
- `rawBinningFactorUsed = true`

Stage 3.6 remained:

- only rows 0..767 non-zero
- rows 768..12287 zero
- exact populated prefix: 25,067,520 bytes
- classification:
  `ONLY_FIRST_768_ROWS_NONZERO__EXACT_12P5MP_BYTE_PAYLOAD_SIGNATURE`

Stage 3.7 remained:

- payload byte count: 25,067,520
- one exact advertised standard RAW match: 4080x3072
- candidate payload is exact source prefix with no transform
- candidate payload SHA-256 reported by device JSON:
  `59f4161b34634a08cde08335d669934df1672d06fbc7f4ac644cdd0e3f07b004`
- classification:
  `EXACT_PREFIX_BYTES_MATCH_ONE_ADVERTISED_STANDARD_RAW_GEOMETRY__GEOMETRY_INTERPRETATION_CANDIDATE_NOT_SENSOR_PROOF`

The diagnostic PNG is appearance-only and was reported as SHA-256:
`32cbd3f99a266a31c04983a467b7c54e74eca7380defedd09330e3bc3d2b434e`.

Coarse result-side observations also remained in the previously seen class:

- HONOR `binningFactor = 4`
- HONOR `isInSensorZoom = 0`
- `allISPCropWindow = [0,0,16320,12288,0,0,16320,12288]`
- AEC real-crop prefix begins `[11,8,4058,3055]`

These names remain observations, not semantic proof.

## Bounded conclusion

Within this tested logical-0 -> physical-5 MAX context, native-type-correct isolated
`EnableMCXMasterCb = INT32(1)` produced **no measurable app-visible RAW envelope or populated-prefix topology differential**.

Together with v0.33 and v0.34, all three v0.32 candidates have now been isolated at numeric value 1 without a topology differential:

1. `EnableInsensorZoom`
2. `EnableSnapshotOnlyInsensorZoom`
3. `EnableMCXMasterCb`

This does not prove numeric 1 means enabled, global ineffectiveness, native ADC geometry, or 200 MP optical resolution.

## Next research decision

Do not continue by blindly enumerating more values or by assigning meaning from vendor names.

The v0.35 evidence exposes a stronger **availability-topology** clue: several QTI session-parameter keys are advertised on physical Camera 5 but not on logical Camera 0. The clearest route candidate already present in the bounded route inventory is:

`org.codeaurora.qcamera3.sessionParameters.inSensorZoomEnable`

For v0.36, use a representation-only physical-route native-type oracle. Because the earlier v0.15 direct-open physical-5 route was rejected, the oracle should not rely solely on opening Camera 5 directly. It should separate:

- vendor-tag lookup authority from physical Camera-5 characteristics; and
- disposable request-template type testing on logical Camera 0;

with no session creation, no session-parameter attachment, no capture submit and no RAW access.

Only uniquely resolved representation evidence may justify a later isolated intervention.
