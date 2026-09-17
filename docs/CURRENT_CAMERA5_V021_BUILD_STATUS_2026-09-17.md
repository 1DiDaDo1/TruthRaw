# TruthRaw Camera-5 v0.21 build status — 2026-09-17

Status: **BUILD SUCCESS / DEVICE RESULT PENDING**

This document records implementation/build status only. It does not supersede the completed v0.20 device evidence and does not promote any v0.21 scientific claim before an on-device result exists.

## Build identity

- branch: `integration/truthraw-suite-v0-21-idealraw-single-variable`
- workflow: `TruthRaw FotoGraaf v0.21 IdealRAW Single Variable`
- GitHub Actions run: `35237852510`
- workflow build commit: `10e809d8d60f7073efd6d468e877633e9bcce839`
- result: `success`
- APK bytes: `4,848,073`
- APK SHA-256: `acc047f8fcd44591a9624cfa59ac8b65c5d09e23e3f67e8833fb6a57e6007fbc`
- artifact id: `10504271250`
- artifact ZIP bytes: `1,580,110`
- artifact ZIP SHA-256: `cdf686af1968f9f8f68d41b88524d2af05027168437d298f250c94c6c5853350`

CI confirmed the patch chain reconstructs v0.20 first, applies exactly one `Camera2IdealRawSessionProbe.applyExperiment(...)`, retains source-first sealing, retains the post-HAL envelope, runs Stage 3.6 before Stage 3.7, contains no HardwareBuffer lock/map call, and successfully compiles/packages the arm64 APK.

## Intervention identity

Only the following unknown vendor control is intentionally changed:

`org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW = 1`

The intervention is attached as a logical-session parameter after Gate A records the unmodified app-visible control surface and before the session support query/session creation.

The probe fails closed unless the actual runtime key is advertised as a logical session key, its Java type is discoverable as a one-byte representation (`Byte` or `byte[]`), the builder accepts value 1, the builder reads exactly 1 back, and `SessionConfiguration.setSessionParameters()` succeeds.

No fallback guesses are allowed. In particular, v0.21 does not modify `RawCbSourceType`, XCFA controls, `HALOutputBufferCombined`, in-sensor zoom, MCX controls or HONOR vendor-result metadata.

## Control and scientific boundary

The unchanged device-evidence control remains v0.20:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

The v0.21 build itself proves only that the controlled intervention implementation compiles and satisfies its static/order guards. It does not prove that HONOR accepts the intervention at runtime or that the delivered RAW changes.

Permanent boundary:

`APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF`

## Required on-device comparison

After a v0.21 capture, compare against v0.20 at minimum:

- runtime IdealRAW key type, write representation and builder readback;
- whether session parameters are attached and the session/capture succeeds;
- Image/HardwareBuffer geometry and source byte count;
- Stage 3.6 populated-prefix byte count/non-zero rows/band hashes;
- Stage 3.7 selected payload geometry and exact-prefix SHA;
- code range, CFA phase statistics and neighbor correlations;
- returned `SENSOR_PIXEL_MODE` and `rawBinningFactorUsed`;
- HONOR `binningFactor`, AEC/ISP crop domains and `sensorCustomMetaData`;
- QTI route/multicamera/AF sidecars.

A changed payload after this single controlled intervention can support a causal route-differential statement for this device/software state. It still cannot establish the physical semantics of the vendor tag or untouched native sensor/ADC truth by itself.
