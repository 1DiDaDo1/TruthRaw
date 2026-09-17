# CURRENT Camera-5 route-control experiment — 2026-09-17

Status: **CURRENT EXPERIMENT TRACK; v0.20 REMAINS SOURCE/PAYLOAD AUTHORITY; v0.25 DEVICE TYPE RESULT COMPLETE; v0.26 BUILD SUCCESS / DEVICE RESULT PENDING**

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

## v0.21 — fail-closed IdealRAW Java type attempt

Result: `BLOCKED_IDEALRAW_RUNTIME_TYPE_UNAVAILABLE`.

No vendor value was written, no session parameter attached and no capture submitted.

## v0.22 — IdealRAW app-side marshalling dry run

Result: `MULTIPLE_APP_SIDE_MARSHALLING_CANDIDATES__AMBIGUOUS_NO_HAL_SUBMISSION`.

Byte/byte[] and Int/int[] could be marshalled locally; no HAL/session submission occurred.

## v0.23 — IdealRAW native type oracle

Device result: `NATIVE_METADATA_TYPE_BYTE__NO_SESSION_OR_CAPTURE_SUBMISSION`.

Resolved:

- key `org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW`
- tag `0x801F0027`
- native type `BYTE`
- count `1`
- u8 accepted, i32 rejected.

No capture session or vendor-modified HAL submission occurred.

## v0.24 — IdealRAW BYTE(1) controlled intervention

Branch: `integration/truthraw-suite-v0-24-idealraw-byte-intervention`.

Exactly one unknown vendor variable was changed after untouched Gate A:

`EnableIdealRAW = BYTE(1)`

The value was set/read back and attached as session parameters. The trusted logical0 -> physical5 -> 16320x12288 MAX acquisition completed with source-first sealing and unchanged Stage 3.6/3.7 analysis.

Device differential versus v0.20:

- populated prefix remained `25,067,520` bytes
- only-first-768-rows signature remained
- unique payload geometry remained `4080x3072`
- app-visible envelope remained `16320x12288`
- returned `SENSOR_PIXEL_MODE = 0`
- raw binning flag remained active
- HONOR `binningFactor = 4`
- HONOR `isInSensorZoom = 0`.

Bounded conclusion: the intervention was accepted, but no measurable RAW-envelope or populated-payload-topology differential was observed on this tested route. This is not a universal no-effect claim.

## v0.25 — RawCbSourceType native type oracle

Device evidence: `TRUTHRAW_CAM5_RAWCB_SOURCE_TYPE_NATIVE_TYPE_ORACLE_v025.json`.

Classification:

`NATIVE_METADATA_TYPE_INT32__NO_SESSION_OR_CAPTURE_SUBMISSION`

Resolved device facts:

- key: `org.codeaurora.qcamera3.sessionParameters.RawCbSourceType`
- tag lookup available and successful
- tag: `0x801F0009`
- unsigned tag ID: `2149515273`
- accepted native type count: `1`
- resolved type: `INT32`
- accepted entry count: `1`
- INT32 set/get status: `0/0`
- BYTE/FLOAT/INT64/DOUBLE/RATIONAL rejected.

Numeric test value `1` was used strictly for metadata-type validation. Its vendor semantics are not known.

Safety/provenance:

- disposable request templates only
- no session created
- no session parameters attached
- no capture submitted
- no vendor-modified request submitted to HAL
- no RAW pixel access
- no source mutation
- no semantic promotion.

Representation conclusion: `RawCbSourceType` is native Camera2 metadata `INT32` on this tested device/route.

v0.25 build provenance:

- run `35246799407`
- job `105288947534`
- workflow head `f4a474da0b0635527a0c1ba8a9b15dfc64d3ce95`
- APK bytes `4,882,977`
- APK SHA-256 `25f31327aa6ab32a33e3e706370f30c506d2d4867ba769c7637a74f8fadabbd1`
- artifact ID `10508380597`
- artifact ZIP SHA-256 `376d151af2a521042f029b81f0c5c566c527ec10f165a164274242e5f733caf6`.

## v0.26 — RawCbSourceType INT32(1) single-variable intervention

Branch: `integration/truthraw-suite-v0-26-rawcb-int32-intervention`.

Build status: **SUCCESS; DEVICE RESULT PENDING**.

GitHub Actions provenance:

- run `35249255766`
- job `105297221061`
- workflow head `521691e3eeb9500b514c500da5a1280ddd828527`
- ordering and single-variable assertions: PASS
- APK bytes `4,899,361`
- APK SHA-256 `2f4df3f33a03279e467d2d630e87b85c75f82caac13fcd37cc32c982357a1bbd`
- artifact ID `10509261489`
- artifact ZIP bytes `1,596,602`
- artifact ZIP SHA-256 `a4bce47f2c689982e1dcdf5f73a79ef161af96fbe6d2bdabdb7fd6fbfb960832`.

Design:

1. reconstruct exact v0.20 acquisition/audit chain;
2. preserve untouched Gate A fingerprint;
3. set exactly one unknown vendor variable: `RawCbSourceType = INT32(1)`;
4. record pre-set builder readback where available;
5. require builder readback `1` after set;
6. require built-request readback `1`;
7. attach the request as session parameters;
8. touch no second unknown vendor key;
9. preserve logical0 -> physical5 topology;
10. preserve 16320x12288 MAX output/request route;
11. preserve original Plane[0] source-first seal;
12. preserve post-HAL HardwareBuffer envelope observation;
13. repeat Stage 3.6 full-raster audit unchanged;
14. repeat Stage 3.7 payload-geometry decoder unchanged.

The requested numeric value `1` is an A/B intervention value only. The project does not assign it a semantic label such as full-resolution, unbinned, native or sensor source.

## v0.26 device differential targets

Compare directly against v0.20:

- source-envelope byte count
- populated-prefix byte count
- first/last non-zero positions
- all 16 x 768-row band population states
- source/payload hashes
- selected advertised standard RAW geometry
- returned `SENSOR_PIXEL_MODE`
- `rawBinningFactorUsed`
- HONOR `binningFactor`
- HONOR `isInSensorZoom`
- AEC/ISP crop metadata.

A changed topology is route-differential evidence only; it remains app-visible Camera2/HAL output, not automatically untouched ADC or 200 MP optical proof.

## Authority rules

- v0.20 remains source/payload authority until a newer validated device result changes it.
- v0.24 is valid negative differential evidence for its tested intervention, not a universal no-effect proof.
- v0.25 resolves representation/type only.
- a vendor-key name is not semantic authority.
- successful setter/readback/attachment is intervention provenance, not proof of sensor mode.
- do not combine unknown vendor controls in one experiment.
- source bytes remain sealed before result/vendor interpretation.
