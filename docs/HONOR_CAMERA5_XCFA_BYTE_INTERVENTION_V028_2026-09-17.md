# HONOR Camera-5 EnableXCFAOptimization BYTE intervention v0.28

Date: 2026-09-17

Status: **BUILD SUCCESS / DEVICE RESULT PENDING**

## Representation authority

v0.27 device evidence resolved:

- key: `org.codeaurora.qcamera3.sessionParameters.EnableXCFAOptimization`
- vendor tag: `0x801F0036`
- native Camera2 metadata type: `BYTE`
- count: `1`
- BYTE accepted; INT32/FLOAT/INT64/DOUBLE/RATIONAL rejected.

The v0.27 oracle created disposable request templates only and did not submit a modified request to HAL.

## v0.28 controlled intervention

v0.28 changes exactly one unknown vendor session variable after untouched Gate A:

`EnableXCFAOptimization = BYTE(1)`

Numeric value `1` is an A/B intervention value only. TruthRaw does not assign it semantic meaning such as XCFA enabled, remosaic enabled, native, unbinned or full-resolution.

The intervention helper requires:

- key advertised as a logical session key
- `CaptureRequest.Key<Byte>` representation
- successful builder set
- builder readback exactly `1`
- built-request readback exactly `1`
- successful `SessionConfiguration.setSessionParameters(request)`.

If any requirement fails, capture is blocked before the experimental session proceeds.

## Preserved acquisition/audit chain

v0.28 reconstructs the exact v0.20 control chain and preserves:

1. untouched Gate-A route/session fingerprint before intervention;
2. logical camera 0 -> physical output camera 5;
3. 16320x12288 MAXIMUM_RESOLUTION route declaration/request;
4. exact Image/physical-result timestamp requirement;
5. original Plane[0] source-first persistence + SHA-256 seal;
6. read-only post-HAL HardwareBuffer envelope observation;
7. Image/session/reader/camera closure before source-file audit;
8. Stage 3.6 complete raster population/hash audit;
9. Stage 3.7 exact populated-prefix geometry decoder;
10. no second unknown vendor-key intervention.

## Control

TruthRaw v0.20 remains source/payload authority until a newer validated device result changes it:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

Control measurements:

- envelope: `401,080,320` bytes
- populated prefix: `25,067,520` bytes
- non-zero rows: `0..767`
- unique advertised standard RAW byte match: `4080x3072`.

v0.24 (`EnableIdealRAW=BYTE(1)`) and v0.26 (`RawCbSourceType=INT32(1)`) were both accepted interventions with no measurable source-envelope/populated-payload topology differential versus this control.

## Differential targets

After a v0.28 device capture compare:

- source-envelope bytes
- source SHA-256
- populated-prefix bytes
- first/last non-zero byte/sample/row
- all 16 x 768-row band population states
- selected advertised standard RAW geometry
- payload SHA-256 and Stage-3.6 first-band identity
- returned `SENSOR_PIXEL_MODE`
- `rawBinningFactorUsed`
- HONOR `binningFactor`
- HONOR `isInSensorZoom`
- AEC/ISP crop metadata.

A changed topology would be route-differential evidence only. It would not automatically prove untouched ADC geometry or optical resolution.

## Build provenance

Branch:
`integration/truthraw-suite-v0-28-xcfa-byte-intervention`

Patch:
`tools/patch_fotograaf_v028_xcfa_byte_intervention.py`

Helper:
`suite_android/app/src/main/java/com/truthraw/adaptiveui/Camera2XcfaByteSessionProbe.kt`

Workflow:
`.github/workflows/android-truthraw-suite-v0-28-xcfa-byte-intervention.yml`

GitHub Actions:

- run: `35252818559`
- job: `105309042996`
- workflow head: `86f709582442c0cdbaafad9880715c6887e1b1bf`
- conclusion: `success`
- ordering/single-variable assertions: PASS
- APK bytes: `4,880,841`
- APK SHA-256: `1035c5dbaffbdbbad1048e12653e6a32b6441a0f67ebb1d9449075b33666c0ac`
- artifact ID: `10512015658`
- artifact ZIP bytes: `1,589,573`
- artifact ZIP SHA-256: `bbc1eea89014b806d2933e75382a5fe7d07133f29ea929a8e0edef5e09a4aa82`.

## Device protocol

Run Step 1 -> Step 2 -> Step 3.

If the intervention attaches successfully, allow the capture to finish Stage 3.6 and Stage 3.7. Save the v0.28 evidence JSON and, if offered, the exact `.rawpayload` and geometry diagnostic PNG.
