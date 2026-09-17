# HONOR Camera-5 HALOutputBufferCombined v0.29 native type oracle — build state

Date: 2026-09-17

Status: **BUILD SUCCESS — DEVICE RESULT PENDING**

Branch:
`integration/truthraw-suite-v0-29-hal-output-buffer-combined-native-type-oracle`

## Why this experiment exists

TruthRaw v0.28 proved that `EnableXCFAOptimization=BYTE(1)` can be accepted and attached as a single session variable, yet the observed Camera-5 RAW topology remained unchanged: 401,080,320-byte 16320x12288 app-visible envelope, only the first 25,067,520 bytes populated, and the same unique 4080x3072 standard RAW byte match.

Therefore v0.29 does not guess another vendor value. It first resolves the native `camera_metadata` representation of:

`org.codeaurora.qcamera3.sessionParameters.HALOutputBufferCombined`

The key is advertised on logical and physical request/session surfaces, while Gate B in v0.28 observed no current/default value. The key name alone does not grant semantic authority.

## Safety design

v0.29 is Stage-1-only diagnostics. It reuses the disposable NDK metadata validator first established for earlier route-control type oracles.

It tests independent disposable request metadata instances with these native element types:

- BYTE / u8
- INT32
- FLOAT
- INT64
- DOUBLE
- RATIONAL

For every candidate the native setter is followed by readback/type/count validation.

v0.29 deliberately does **not**:

- create a capture session;
- attach session parameters;
- submit a capture;
- submit a vendor-modified request to HAL;
- access RAW pixels;
- alter or replace the v0.20 source authority;
- assign semantics to numeric value `1`.

Expected device stop:
`STAGE 1.5 DIAGNOSTIC STOP`

Expected evidence file:
`TRUTHRAW_CAM5_HAL_OUTPUT_BUFFER_COMBINED_NATIVE_TYPE_ORACLE_v029.json`

Only Step 1 is required on-device.

## Build authority

GitHub Actions run: `35255819562`

Job: `105319078638`

Workflow head: `d8e8961c196cf698d6ee5bb9194c90aee8ac3689`

Build conclusion: `success`

APK:
- bytes: `4,899,361`
- SHA-256: `deb775a8b19fcd02616904ace931c56297d3badbd6d3b679e308137949ba5552`

Artifact:
- ID: `10513220672`
- ZIP bytes: `1,595,399`
- ZIP SHA-256: `84675230ab4f40538d3fb71671ee71d4cde9b05cf430a542f01ef6030ab1d3b0`

## Authority boundary

The current Camera-5 source/payload authority remains v0.20:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

Permanent boundary:

`APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF`

v0.29 can resolve representation only. It cannot by itself prove what `HALOutputBufferCombined` means, whether any value changes the physical readout, or whether the name refers to the observed 401-MB envelope/prefix relationship.
