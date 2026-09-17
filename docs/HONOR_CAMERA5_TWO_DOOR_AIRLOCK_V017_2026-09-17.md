# HONOR Camera-5 two-door airlock — v0.17 — 2026-09-17

Status: **EXPERIMENTAL ACQUISITION / PROVENANCE ARCHITECTURE**

## Goal

TruthRaw has now observed that the HONOR/QTI Camera2 path exposes two different boundaries around the same 200 MP capture:

1. a request/session boundary before the vendor pipeline executes; and
2. a HardwareBuffer/Image/CaptureResult boundary after the vendor pipeline delivers the RAW_SENSOR output to Android.

The v0.17 design treats these as a two-door airlock.

## Door A/B — request side before vendor execution

The application freezes the exact route intent before session creation and again immediately before capture request build/submit.

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

## Next experiment

Run the same airlock on the three distinct Camera-5 readout/sample domains when supported:

- 4080x3072;
- 8160x6144;
- 16320x12288.

Compare only like-for-like fields. A vendor field may be assigned semantics only if repeated differential evidence supports that interpretation. In particular, test whether HONOR `binningFactor`, `sensorCustomMetaData`, crop windows and candidate QTI raw-route keys change systematically with the readout domain.

## Precision and zero-line

This layer is acquisition/provenance only.

- RAW source bytes remain exact integer evidence.
- F64 may later be used for differential/covariance/calibration analysis.
- FP32 may only be used in demonstrated-safe downstream operations.
- TruthRange `T = log2(L/L0)` and the zero-line are not used to interpret request/session/vendor routing metadata.
