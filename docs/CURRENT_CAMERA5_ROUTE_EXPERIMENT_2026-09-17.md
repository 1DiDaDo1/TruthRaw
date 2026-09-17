# CURRENT Camera-5 route-control experiment — 2026-09-17

Status: **CURRENT EXPERIMENT TRACK; v0.20 REMAINS SOURCE/PAYLOAD AUTHORITY; v0.24 DEVICE DIFFERENTIAL COMPLETE**

This document tracks upstream HONOR/QTI route-control experiments after the completed v0.20 payload-topology result. It does not replace `docs/CURRENT_CAMERA5_RAW_ROUTE_2026-09-17.md` for source authority.

## Control

TruthRaw v0.20 remains the untouched control:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

The tested control has a `401,080,320`-byte app-visible envelope with a populated `25,067,520`-byte prefix that uniquely matches the advertised standard `4080x3072` RAW_SENSOR byte count and behaves as a Bayer-like full-frame raster under that interpretation.

## Route-control sequence

### v0.21 — fail-closed Java runtime-type attempt

Result:

`BLOCKED_IDEALRAW_RUNTIME_TYPE_UNAVAILABLE`

No vendor value was written, no session parameter attached and no capture submitted.

### v0.22 — app-side marshalling dry run

Result:

`MULTIPLE_APP_SIDE_MARSHALLING_CANDIDATES__AMBIGUOUS_NO_HAL_SUBMISSION`

Passing Java representations:

- `Byte`
- `byte[]`
- `Int`
- `int[]`.

No HAL/session submission occurred. This narrowed the candidate family but did not resolve the native metadata type.

### v0.23 — native type oracle

Device evidence:

`TRUTHRAW_CAM5_IDEALRAW_NATIVE_TYPE_ORACLE_v023.json`

Result:

`NATIVE_METADATA_TYPE_BYTE__NO_SESSION_OR_CAPTURE_SUBMISSION`

The device resolved:

- key: `org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW`
- tag: `0x801F0027`
- native type: `BYTE`
- native count for test value: `1`
- `u8(1)` set/get: accepted
- `i32(1)` set: rejected.

v0.23 created request templates only. It created no capture session and submitted no vendor-modified request to HAL.

This is a representation/type fact, not proof that the vendor-key name or value `1` has a particular physical effect.

### v0.24 — first controlled BYTE intervention

Branch:

`integration/truthraw-suite-v0-24-idealraw-byte-intervention`

Build:

- GitHub Actions run: `35243866129`
- workflow head: `429063ceb11087cb16132fa910c740a170b2f6d5`
- result: **SUCCESS**
- APK bytes: `4,880,841`
- APK SHA-256: `e8f898b041a63b9f68a246878aadd3427e24d321731042a49a02e0318f5e5c15`
- artifact ID: `10506437816`
- artifact ZIP SHA-256: `04b3a576d01ccbec8340cd1a5353c494cf77b245bc60163ca7fde28f9ef5053b`.

Design:

- reconstruct exact v0.20 acquisition/audit chain;
- preserve Gate A before intervention;
- construct `org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW` as BYTE using the v0.23 native-type result;
- set exactly one unknown vendor value: `EnableIdealRAW = BYTE(1)`;
- require builder and built-request readback `1`;
- attach that request as SessionConfiguration session parameters;
- touch no second unknown vendor key;
- preserve logical0 -> physical5 -> 16320x12288 MAX topology;
- seal the original Plane[0] before interpreting result metadata;
- repeat the same post-HAL envelope, Stage 3.6 full-raster audit and Stage 3.7 payload-geometry decoder.

Device evidence:

`TRUTHRAW_1789661201607_CAM5_200MP_EVIDENCE_v024.json`

Intervention acceptance:

- `controlledVendorInterventionKeyCount = 1`
- `EnableIdealRAW` native type source = v0.23 BYTE oracle
- requested numeric value = `1`
- builder set = PASS
- builder readback = `1`
- built-request readback = `1`
- session parameters attached = true
- semantic promotion = false.

Capture still followed the trusted route:

- opened logical camera `0`
- physical result camera `5`
- delivered `16320x12288`
- exact Image/result timestamp equality
- original source bytes `401,080,320`
- pixel stride `2`
- row stride `32,640`
- returned `SENSOR_PIXEL_MODE = 0`
- `rawBinningFactorUsed = true`.

Stage 3.6 result:

`ONLY_FIRST_768_ROWS_NONZERO__EXACT_12P5MP_BYTE_PAYLOAD_SIGNATURE`

Observed:

- only band `0` is non-zero;
- bands `1..15` are all zero;
- populated prefix = `25,067,520` bytes;
- last non-zero byte offset = `25,067,518`.

Stage 3.7 result:

`UNIQUE_ADVERTISED_STANDARD_RAW_BYTE_MATCH_DECODED`

- unique advertised standard RAW byte match = `4080x3072`
- candidate payload = exact source prefix, no transform
- candidate payload SHA-256 = `66e2bd49a4414326dbed0f69cf3e2e474931d2893a5113b22c5b67826c661bc2`
- min/max codes for this scene = `60..1023`
- same Bayer-like distance-2 correlation structure remains present.

Relevant HONOR/QTI route observations stayed in the same structural class:

- `com.hihonor.capture.metadata.binningFactor = 4`
- `com.hihonor.capture.metadata.isInSensorZoom = 0`
- AEC real crop begins `[11,8,4058,3055,...]`
- ISP crop remains `16320x12288`.

### v0.24 differential conclusion

Against v0.20, the following primary topology quantities did **not** change:

- populated prefix bytes: `25,067,520`
- only-first-768-rows population signature
- selected payload geometry: `4080x3072`
- declared Image/HardwareBuffer envelope: `16320x12288`
- returned `SENSOR_PIXEL_MODE = 0`
- raw binning flag
- HONOR `binningFactor = 4`
- HONOR `isInSensorZoom = 0`.

Current bounded conclusion:

`EnableIdealRAW=BYTE(1)` was accepted and attached as the sole controlled vendor session variable, but **no measurable RAW-envelope or populated-payload-topology differential was observed on this tested Camera-5 route versus v0.20**.

This does not prove that the key has no effect in every route or mode. It may be ignored for this stream, already equivalent to the active internal state, relevant to another route, or require another condition. Those possibilities remain unproven and must not be selected by name alone.

## Next controlled question

The next candidate is:

`org.codeaurora.qcamera3.sessionParameters.RawCbSourceType`

The next step is **not** a capture intervention. First resolve its actual native vendor tag/type with an oracle-only probe, with:

- no session parameters attached;
- no capture session created for the modified request;
- no capture submitted;
- no second vendor key changed.

Only after the representation is established should a separate single-variable intervention be considered.

## Authority rules

- v0.20 remains the source/payload control authority.
- v0.24 is valid negative differential evidence for the tested intervention, not a universal no-effect proof.
- A vendor-key name is not semantic authority.
- A successful setter/readback is not proof of sensor mode.
- A changed source topology, if later observed, would be route-differential evidence but still app-visible Camera2/HAL output, not untouched ADC proof.
- Do not combine unknown vendor controls in one experiment.
