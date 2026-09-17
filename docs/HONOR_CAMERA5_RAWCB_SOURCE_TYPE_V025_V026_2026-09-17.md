# HONOR Camera-5 RawCbSourceType v0.25/v0.26 — 2026-09-17

Status: **CURRENT ROUTE-CONTROL RESEARCH; v0.20 REMAINS SOURCE/PAYLOAD AUTHORITY**

## Why this exists

TruthRaw v0.24 proved that `EnableIdealRAW=BYTE(1)` could be attached as the sole controlled vendor session variable, yet no measurable RAW-envelope or populated-payload-topology differential was observed versus v0.20. The next candidate is `org.codeaurora.qcamera3.sessionParameters.RawCbSourceType`.

Vendor-key names are not semantic authority. The representation/type question is resolved first; value semantics remain unproven.

## v0.25 native metadata type oracle — device result complete

Device evidence:

`TRUTHRAW_CAM5_RAWCB_SOURCE_TYPE_NATIVE_TYPE_ORACLE_v025.json`

Observed:

- key: `org.codeaurora.qcamera3.sessionParameters.RawCbSourceType`
- tag lookup available and successful
- vendor tag: `0x801F0009`
- tag unsigned: `2149515273`
- native metadata type: `INT32`
- accepted type count: `1`
- accepted entry count: `1`
- test value: numeric `1`, used for **type validation only**, not vendor-value semantics.

Native type matrix:

- BYTE: rejected
- INT32: accepted, set status `0`, get status `0`, entry type `1`, count `1`
- FLOAT: rejected
- INT64: rejected
- DOUBLE: rejected
- RATIONAL: rejected.

Classification:

`NATIVE_METADATA_TYPE_INT32__NO_SESSION_OR_CAPTURE_SUBMISSION`

Safety/provenance:

- disposable request templates only
- no session created
- no session parameters attached
- no capture submitted
- no vendor-modified request submitted to HAL
- no RAW pixel access
- no source mutation
- no semantic promotion.

Bounded conclusion:

`RawCbSourceType` is represented as native Camera2 metadata `INT32` on this device/route. This does **not** prove what numeric value `1` means.

## v0.26 controlled intervention

Branch:

`integration/truthraw-suite-v0-26-rawcb-int32-intervention`

Design:

1. Reconstruct exact v0.20 acquisition/audit control.
2. Capture untouched Gate A request/session fingerprint.
3. Change exactly one unknown vendor variable:
   `org.codeaurora.qcamera3.sessionParameters.RawCbSourceType = INT32(1)`.
4. Record pre-set builder readback where available.
5. Require builder readback `1` after set.
6. Require built-request readback `1`.
7. Attach that request as session parameters.
8. Touch no second unknown vendor key.
9. Preserve logical camera 0 -> physical camera 5 topology.
10. Preserve 16320x12288 MAX output/request route.
11. Seal original `Image.Plane[0]` before metadata interpretation.
12. Preserve post-HAL HardwareBuffer envelope observation.
13. Repeat unchanged Stage 3.6 full-raster audit.
14. Repeat unchanged Stage 3.7 payload-geometry decoder.

The requested numeric value `1` is an experimental A/B value only. Its vendor semantics are explicitly unproven.

## Differential targets against v0.20

Primary comparison quantities:

- app-visible source-envelope byte count
- populated-prefix byte count
- first/last non-zero positions
- 16 x 768-row band population signature
- selected advertised standard RAW geometry
- source and payload hashes
- returned `SENSOR_PIXEL_MODE`
- `rawBinningFactorUsed`
- HONOR `binningFactor`
- HONOR `isInSensorZoom`
- AEC and ISP crop metadata.

A changed topology would be route-differential evidence only. It would still not establish untouched ADC or native optical resolution.

## Source/payload authority remains v0.20

Until a newer device capture produces a validated differential, the authority remains:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

with the tested v0.20 populated prefix of `25,067,520` bytes and unique advertised standard RAW byte match `4080x3072`.
