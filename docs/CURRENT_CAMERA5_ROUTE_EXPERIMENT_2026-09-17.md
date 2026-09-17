# CURRENT Camera-5 route-control experiment — 2026-09-17

Status: **CURRENT EXPERIMENT TRACK; v0.20 REMAINS SOURCE/PAYLOAD AUTHORITY**

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

Design:

- reconstruct exact v0.20 acquisition/audit chain;
- preserve Gate A before intervention;
- verify the key remains advertised as a logical session key;
- construct the key explicitly as `CaptureRequest.Key<Byte>` using Kotlin `Byte::class.javaObjectType`;
- set exactly one value: `EnableIdealRAW = BYTE(1)`;
- require builder and built-request readback `1`;
- attach that request as the SessionConfiguration session parameters;
- touch no second unknown vendor key;
- preserve logical0 -> physical5 -> 16320x12288 MAX topology;
- seal the original Plane[0] before interpreting result metadata;
- repeat the same post-HAL envelope, Stage 3.6 full-raster audit and Stage 3.7 payload-geometry decoder.

Build status:

- GitHub Actions run: `35243866129`
- workflow head: `429063ceb11087cb16132fa910c740a170b2f6d5`
- result: **SUCCESS**
- order/single-variable assertions: PASS
- APK bytes: `4,880,841`
- APK SHA-256: `e8f898b041a63b9f68a246878aadd3427e24d321731042a49a02e0318f5e5c15`
- artifact ID: `10506437816`
- artifact ZIP bytes: `1,587,319`
- artifact ZIP SHA-256: `04b3a576d01ccbec8340cd1a5353c494cf77b245bc60163ca7fde28f9ef5053b`
- device result: pending.

Integration provenance: the first v0.24 build attempt failed only at Kotlin compile time because the custom BYTE key used an incompatible Java/Kotlin generic class expression. No APK/device intervention occurred. The implementation was corrected to `Byte::class.javaObjectType`; the scientific experiment design was unchanged and the subsequent build passed.

A successful session/capture is only an intervention/acceptance fact. A route effect requires a measurable differential against v0.20.

## Primary v0.24 decision variables

Compare v0.24 directly with v0.20 on:

1. session support/configuration acceptance;
2. still-capture acceptance;
3. physical Camera-5 result/timestamp binding;
4. delivered Image/HardwareBuffer geometry, format, stride and capacity;
5. populated source byte count and first/last non-zero positions;
6. 768-row band population and row repetition;
7. Stage-3.7 advertised-size byte match;
8. derived exact-prefix geometry and SHA identities;
9. returned `SENSOR_PIXEL_MODE` and raw-binning result;
10. HONOR/QTI binning/crop/vendor fingerprints before and after HAL delivery.

## Authority rules

- `EnableIdealRAW` remains an unknown-semantics vendor control until an effect is measured.
- A vendor-key name is not semantic authority.
- A successful setter is not proof of sensor mode.
- A changed source topology is evidence of a route differential but is still app-visible Camera2/HAL output, not untouched ADC proof.
- v0.20 remains the control and must not be rewritten.
- No other vendor control is to be toggled in v0.24.
