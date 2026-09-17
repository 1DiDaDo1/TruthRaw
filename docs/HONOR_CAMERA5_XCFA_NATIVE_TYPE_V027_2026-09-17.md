# HONOR Camera-5 EnableXCFAOptimization native-type oracle v0.27

Date: 2026-09-17

Status: BUILD IN PROGRESS / DEVICE RESULT PENDING

## Why this candidate is next

TruthRaw v0.24 (`EnableIdealRAW=BYTE(1)`) and v0.26 (`RawCbSourceType=INT32(1)`) were both accepted as single vendor session-variable interventions, but neither changed the measured Camera-5 RAW-envelope or populated-payload topology versus the v0.20 control.

The v0.26 untouched Gate-B request-state observation exposes:

`org.codeaurora.qcamera3.sessionParameters.EnableXCFAOptimization`

as available on logical request, physical request, logical session and physical session surfaces. Its observed builder default/current representation is a one-element `byte[]` with preview `[0]`.

That observation narrows the representation question, but it is **not native metadata-type proof** and the key name is **not semantic authority**.

## v0.27 question

Resolve the actual native Camera2 metadata element type for `EnableXCFAOptimization` before any intervention.

The oracle tests independently:

- BYTE
- INT32
- FLOAT
- INT64
- DOUBLE
- RATIONAL

Each test uses a separate disposable NDK still-capture request metadata object with numeric value `1` strictly as a type-validation value. It requires set success plus readback type/count agreement.

## Safety boundary

v0.27 creates no capture session, attaches no session parameters, submits no capture or repeating request, accesses no RAW pixels and modifies no source.

The v0.20 source/payload authority remains unchanged:

`APP_VISIBLE_PHYSICAL5_16320x12288_RAW_SENSOR_ENVELOPE_WITH_EXACT_4080x3072_STANDARD_RAW_PREFIX_CANDIDATE_PROVEN`

Current control payload topology remains `25,067,520` populated bytes with the unique advertised standard RAW byte match `4080x3072`.

## Branch and workflow

Branch:
`integration/truthraw-suite-v0-27-xcfa-native-type-oracle`

Workflow:
`.github/workflows/android-truthraw-suite-v0-27-xcfa-native-type-oracle.yml`

Patch:
`tools/patch_fotograaf_v027_xcfa_native_type_oracle.py`

The native metadata validator is the same NDK setter/readback mechanism proven by v0.25, generalized so the queried vendor key name is supplied explicitly.

## Device protocol

Run **Step 1 only**.

Expected stop:
`STAGE 1.5 DIAGNOSTIC STOP`

Save:
`TRUTHRAW_CAM5_XCFA_NATIVE_TYPE_ORACLE_v027.json`

Do not proceed to any XCFA intervention until the device result resolves the native representation unambiguously.

## Authority boundary

A resolved native type establishes representation only. It does not prove what `XCFA`, value `0`, value `1`, or any other numeric value means in HONOR/QTI sensor, remosaic, binning or ISP terms.
