# CURRENT Camera-5 route-control experiment — 2026-09-17

Status: **CURRENT EXPERIMENT TRACK; v0.20 REMAINS SOURCE/PAYLOAD AUTHORITY; v0.27 DEVICE TYPE RESULT COMPLETE; v0.28 BUILD IN PROGRESS / DEVICE RESULT PENDING**

This document tracks upstream HONOR/QTI route-control experiments after the completed v0.20 payload-topology result. It does not replace `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md` for source authority.

## Control authority

TruthRaw v0.20 remains the untouched source/payload control:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

Control topology:

- app-visible envelope: `16320x12288`, `401,080,320` bytes
- populated source prefix: `25,067,520` bytes
- unique advertised standard RAW byte match: `4080x3072`
- source prefix behaves as a coherent Bayer-like full-frame raster under that interpretation.

The app-visible result is not untouched photodiode/ADC proof.

## v0.21–v0.24 — EnableIdealRAW track

v0.21 failed closed because runtime Java type was unavailable. v0.22 proved multiple app-side marshalling candidates without HAL submission. v0.23 then resolved the real native type:

- key `org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW`
- tag `0x801F0027`
- native type `BYTE`, count one.

v0.24 changed exactly one vendor session variable after untouched Gate A:

`EnableIdealRAW = BYTE(1)`

The intervention was accepted and attached, but Stage 3.6/3.7 remained topologically identical to v0.20: same `401,080,320`-byte envelope, same `25,067,520` populated bytes, same first-768-rows signature and same unique `4080x3072` standard-RAW byte match. This is a bounded negative differential for that tested value/route, not a universal no-effect claim.

## v0.25–v0.26 — RawCbSourceType track

v0.25 device result:

`NATIVE_METADATA_TYPE_INT32__NO_SESSION_OR_CAPTURE_SUBMISSION`

Resolved:

- key `org.codeaurora.qcamera3.sessionParameters.RawCbSourceType`
- tag `0x801F0009`
- native type `INT32`, count one.

v0.26 changed exactly one vendor session variable:

`RawCbSourceType = INT32(1)`

The intervention was accepted, builder and built-request readback both returned `1`, and session parameters were attached. The complete source-first capture and audit then showed no measurable envelope or populated-payload topology differential versus v0.20:

- envelope remained `16320x12288`, `401,080,320` bytes
- populated prefix remained `25,067,520` bytes
- non-zero rows remained exactly `0..767`
- bands `1..15` remained all zero
- unique standard RAW byte match remained `4080x3072`
- returned `SENSOR_PIXEL_MODE` remained class `0`
- raw binning flag remained active
- HONOR `binningFactor = 4`
- HONOR `isInSensorZoom = 0`
- AEC crop remained around the `4080x3072` domain while ISP crop remained `16320x12288`.

Bounded conclusion:
`RAWCB_SOURCE_TYPE_INT32_ONE_ACCEPTED_AND_ATTACHED_BUT_NO_MEASURABLE_RAW_ENVELOPE_OR_POPULATED_PAYLOAD_TOPOLOGY_DIFFERENTIAL_ON_TESTED_CAMERA5_ROUTE`

## v0.27 — EnableXCFAOptimization native type oracle

Device evidence:
`TRUTHRAW_CAM5_XCFA_NATIVE_TYPE_ORACLE_v027.json`

Classification:
`NATIVE_METADATA_TYPE_BYTE__NO_SESSION_OR_CAPTURE_SUBMISSION`

Resolved device facts:

- key: `org.codeaurora.qcamera3.sessionParameters.EnableXCFAOptimization`
- tag lookup available and successful
- tag: `0x801F0036`
- unsigned tag ID: `2149515318`
- accepted native type count: `1`
- resolved type: `BYTE`
- accepted entry type: `0`
- accepted entry count: `1`
- BYTE set/get status: `0/0`
- INT32/FLOAT/INT64/DOUBLE/RATIONAL rejected.

Numeric test value `1` was used strictly for metadata-type validation. No session was created, no session parameters attached, no capture submitted, no vendor-modified request submitted to HAL, no RAW pixel access occurred and no source was mutated.

Representation conclusion: `EnableXCFAOptimization` is native Camera2 metadata `BYTE` on this tested device/route. The key name and value do not establish remosaic/binning/sensor semantics.

v0.27 build provenance:

- run `35251736680`
- job `105305482639`
- workflow head `db850876ab4a31603e04dca39e8ece3ea087c3c3`
- APK bytes `4,899,361`
- APK SHA-256 `75cdf0bf708e85167934f947268c0e6e925232048c45dd548390d9c507f2fbae`
- artifact ID `10509896067`
- artifact ZIP bytes `1,595,318`
- artifact ZIP SHA-256 `8909bcbe085bc926e540e17b3d1d8d7ecc907c955221094edd001dd60d8da0b0`.

## v0.28 — EnableXCFAOptimization BYTE(1) single-variable intervention

Branch:
`integration/truthraw-suite-v0-28-xcfa-byte-intervention`

Current build run:
`35252818559`

Design:

1. reconstruct the exact v0.20 acquisition/audit chain;
2. preserve untouched Gate A before intervention;
3. set exactly one unknown vendor variable: `EnableXCFAOptimization = BYTE(1)`;
4. record pre-set builder value where available;
5. require builder readback `1` after set;
6. require built-request readback `1`;
7. attach that request as session parameters;
8. touch no second unknown vendor key;
9. preserve logical0 -> physical5 topology;
10. preserve 16320x12288 MAX route;
11. seal original Plane[0] before result/vendor interpretation;
12. preserve read-only post-HAL HardwareBuffer envelope observation;
13. repeat Stage 3.6 full-raster audit unchanged;
14. repeat Stage 3.7 payload-geometry decoder unchanged.

`BYTE(1)` is an A/B intervention value only. The project does not assign it semantic labels such as XCFA enabled, remosaic enabled, native, unbinned or full-resolution.

## v0.28 differential targets

Compare directly with v0.20 and the negative v0.24/v0.26 interventions:

- source-envelope bytes
- populated-prefix bytes
- first/last non-zero positions
- 16-band population signature
- selected payload geometry
- returned `SENSOR_PIXEL_MODE`
- `rawBinningFactorUsed`
- HONOR `binningFactor`
- HONOR `isInSensorZoom`
- AEC/ISP crop metadata
- source and payload hashes.

## Authority rules

- v0.20 remains source/payload authority until a newer validated device capture changes that evidence.
- v0.24 and v0.26 are bounded negative differential results for their tested values/routes.
- v0.23, v0.25 and v0.27 resolve representation/type only.
- vendor-key names are not semantic authority.
- successful setter/readback/attachment is intervention provenance, not sensor-mode proof.
- do not combine unknown vendor controls in one experiment.
- source bytes remain sealed before result/vendor interpretation.
