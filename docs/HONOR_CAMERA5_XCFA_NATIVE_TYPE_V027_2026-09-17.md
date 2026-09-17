# HONOR Camera-5 EnableXCFAOptimization native-type oracle v0.27

Date: 2026-09-17

Status: **DEVICE RESULT COMPLETE — NATIVE TYPE BYTE**

## Why this candidate was selected

TruthRaw v0.24 (`EnableIdealRAW=BYTE(1)`) and v0.26 (`RawCbSourceType=INT32(1)`) were both accepted as single vendor session-variable interventions, but neither changed the measured Camera-5 RAW-envelope or populated-payload topology versus the v0.20 control.

The v0.26 untouched Gate-B request-state observation exposed:

`org.codeaurora.qcamera3.sessionParameters.EnableXCFAOptimization`

on logical/physical request and session surfaces with an observed current/default Java representation of a one-element `byte[]` preview `[0]`. That observation narrowed the representation question but was not native metadata-type proof.

## Device evidence

Evidence file:
`TRUTHRAW_CAM5_XCFA_NATIVE_TYPE_ORACLE_v027.json`

Device classification:
`NATIVE_METADATA_TYPE_BYTE__NO_SESSION_OR_CAPTURE_SUBMISSION`

Resolved facts:

- camera ID: `0`
- vendor key: `org.codeaurora.qcamera3.sessionParameters.EnableXCFAOptimization`
- vendor tag lookup available: true
- tag lookup status: `0`
- vendor tag: `0x801F0036`
- unsigned tag ID: `2149515318`
- camera open status: `0`
- accepted native type count: `1`
- resolved native type: `BYTE`
- BYTE setter status: `0`
- BYTE readback status: `0`
- accepted entry type: `0` (`BYTE`)
- accepted entry count: `1`
- INT32/FLOAT/INT64/DOUBLE/RATIONAL rejected.

The test value `1` was used strictly for metadata-type validation. Its vendor semantics remain unproven.

## Safety result

The oracle created disposable request templates only. It created no capture session, attached no session parameters, submitted no capture or repeating request, accessed no RAW pixels and modified no source.

Therefore the v0.20 source/payload authority remains unchanged:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

Current control payload topology remains `25,067,520` populated bytes with the unique advertised standard RAW byte match `4080x3072`.

## Representation conclusion

On this tested device/route, `EnableXCFAOptimization` is native Camera2 metadata `BYTE`, count one.

This is representation authority only. It does not prove what `XCFA`, numeric value `0`, numeric value `1`, remosaic, binning, sensor readout or ISP behavior means.

## Build provenance

Branch:
`integration/truthraw-suite-v0-27-xcfa-native-type-oracle`

Workflow:
`.github/workflows/android-truthraw-suite-v0-27-xcfa-native-type-oracle.yml`

Patch:
`tools/patch_fotograaf_v027_xcfa_native_type_oracle.py`

GitHub Actions:
- run `35251736680`
- job `105305482639`
- workflow head `db850876ab4a31603e04dca39e8ece3ea087c3c3`
- build conclusion `success`
- APK bytes `4,899,361`
- APK SHA-256 `75cdf0bf708e85167934f947268c0e6e925232048c45dd548390d9c507f2fbae`
- artifact ID `10509896067`
- artifact ZIP bytes `1,595,318`
- artifact ZIP SHA-256 `8909bcbe085bc926e540e17b3d1d8d7ecc907c955221094edd001dd60d8da0b0`.

## Next controlled experiment

v0.28 may test exactly one vendor variable:

`EnableXCFAOptimization = BYTE(1)`

The value remains an A/B intervention value only. v0.28 must preserve untouched Gate A, logical0 -> physical5 MAX topology, source-first sealing, post-HAL envelope observation, Stage 3.6 full-raster audit and Stage 3.7 payload-geometry decoder. No second unknown vendor key may be combined.
