# HONOR Camera-5 two-door airlock — v0.17 — 2026-09-17

Status: **EXPERIMENTAL ACQUISITION / PROVENANCE ARCHITECTURE**

## Goal

TruthRaw has now observed that the HONOR/QTI Camera2 path exposes two different boundaries around the same 200 MP capture:

1. a request/session boundary before the vendor pipeline executes; and
2. a HardwareBuffer/Image/CaptureResult boundary after the vendor pipeline delivers the RAW_SENSOR output to Android.

The v0.17 design treats these as a two-door airlock.

## Door A/B — request side before vendor execution

The application freezes the exact route intent before the physical-5/MAX RAW session is created and again immediately before capture request build/submit.

Recorded facts include:

- logical camera 0 / physical camera 5 intent;
- RAW_SENSOR 16320x12288 target;
- physical output binding;
- MAXIMUM_RESOLUTION output declaration;
- whether global and physical SENSOR_PIXEL_MODE were written;
- available session keys;
- available physical-override keys;
- presence and builder-default/current values of selected HONOR/QTI vendor request keys.

The probe is observation-only. Unknown vendor keys are never set merely because their names sound useful. In particular, names such as EnableIdealRAW, RawCbSourceType, EnableXCFAOptimization, HALOutputBufferCombined or inSensorZoomEnable do not by themselves establish semantics.

This request-side gate is not a pre-HAL pixel interceptor. With ordinary Camera2 application authority, sensor bytes are not delivered to the application before the vendor HAL. The gate instead captures the control state before HONOR/QTI selects/executes its internal route.

## Door C — post-HAL delivery boundary

The already proven v0.16 envelope remains unchanged:

- original Image.Plane[0] is persisted and SHA-256 sealed first;
- the live HardwareBuffer is then described without lock/map/write;
- Image, HardwareBuffer, native AHardwareBuffer descriptor and physical CaptureResult metadata are recorded;
- DNG remains secondary.

## Evidence law

The two-door airlock does not create a second physical observation.

`physicalFrameCount = 1`

`independentEvidenceCount = 1`

The pre-HAL request fingerprint is capture provenance. The post-HAL envelope is delivery provenance. Neither becomes new sensor evidence.

## Scientific boundary

The v0.17 airlock must not promote an app-visible 16320x12288 RAW_SENSOR buffer to untouched native ADC or independent 200 MP photodiode truth.

The currently observed vendor metadata (for example HONOR binningFactor, sensorCustomMetaData, AEC/ISP crop windows and QTI route keys) remains `OBSERVED_VENDOR_METADATA_UNKNOWN_SEMANTICS` until differential captures or authoritative documentation establish meaning.

## Why this helps TruthRaw

The airlock provides a machine-readable mapping:

`what TruthRaw asked for`

`-> what HONOR/QTI reported before/through the request surface`

`-> what buffer Android actually received`

`-> what physical CaptureResult and vendor metadata accompanied that buffer`

This can distinguish three classes of discrepancy without changing the source:

- request/session intent mismatch;
- vendor/HAL route transformation or metadata contradiction;
- downstream container/presentation issues.

## Differential route-fingerprint protocol

The next authority-building step is not to guess a vendor value, but to vary one controlled acquisition domain at a time and compare the entire airlock.

Preferred same-scene matrix when each route can be obtained without weakening its own correctness conditions:

- Camera-5 RAW_SENSOR 4080x3072;
- Camera-5 RAW_SENSOR 8160x6144;
- Camera-5 RAW_SENSOR 16320x12288.

For every capture preserve exposure, framing, physical camera identity and other controls as closely as the route allows, but never forge unsupported request state merely to make two runs look alike.

Compare:

- Gate A session-key availability and physical-override availability;
- Gate B builder default/current state for route-candidate keys;
- global and physical SENSOR_PIXEL_MODE request state;
- Image / HardwareBuffer width, height, format, stride and usage;
- physical CaptureResult SENSOR_PIXEL_MODE and SENSOR_RAW_BINNING_FACTOR_USED;
- HONOR `binningFactor`, `slaveBinningFactor`, `isInSensorZoom`, `sensorCustomMetaData`, `sensorStages`, crop windows and sensor identifiers;
- QTI multicamera/stream-route metadata;
- raw byte payload identity and later Stage-4 topology statistics.

A field that changes with readout domain is correlation evidence, not yet causation. A field that remains constant is useful negative evidence, but does not prove irrelevance.

The repository tool `tools/compare_camera5_airlock_evidence.py` performs this comparison without rewriting source evidence. It fingerprints the full visible vendor-result set, compares the selected route fields, and byte-diffs `sensorCustomMetaData`; aligned 32-bit little-endian words are exposed only as numeric structure and are deliberately not assigned names or meanings.

## Current v0.16 clue set to test

The first post-HAL capture produced several high-value observations that become hypotheses for the differential protocol rather than conclusions:

- app-visible RAW_SENSOR / HardwareBuffer domain: 16320x12288;
- HONOR `binningFactor = 4`;
- AEC-oriented crop metadata close to the 4080x3072 domain;
- ISP/active-array metadata in the 16320x12288 domain;
- `isInSensorZoom = 0`;
- a 40-byte `sensorCustomMetaData` payload;
- request-surface names including `EnableIdealRAW`, `RawCbSourceType`, `EnableXCFAOptimization`, `HALOutputBufferCombined` and in-sensor-zoom controls.

None of those names or numeric coincidences are promoted to physical meaning until differential evidence supports it.

## Safe promotion ladder

The route investigation now follows this order:

`request/session fingerprint`
`-> post-HAL buffer envelope`
`-> exact integer RAW seal`
`-> cross-domain differential metadata`
`-> Stage-4 RAW topology/content analysis`
`-> only then sensor/readout interpretation`
`-> calibration/de-ISP`
`-> Scientific Master`
`-> TruthRange / zero-line`

This prevents vendor metadata from silently becoming calibration authority.

## Precision and zero-line

This layer is acquisition/provenance only.

- RAW source bytes remain exact integer evidence.
- metadata byte/word differencing is integer-exact where possible;
- F64 may later be used for differential/covariance/calibration analysis;
- FP32 may only be used in demonstrated-safe downstream operations;
- TruthRange `T = log2(L/L0)` and the zero-line are not used to interpret request/session/vendor routing metadata.
