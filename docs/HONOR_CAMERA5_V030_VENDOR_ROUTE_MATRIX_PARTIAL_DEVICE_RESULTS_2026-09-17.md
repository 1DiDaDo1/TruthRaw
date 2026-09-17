# HONOR Camera-5 v0.30 vendor-route matrix — partial device results — 2026-09-17

## Status

**PARTIAL MATRIX: 8 / 16 unique profiles have individually readable evidence JSONs.**

Source/payload authority remains **TruthRaw v0.20**.

Permanent boundary:

`APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF`

No v0.30 capture is promoted to calibration authority or Scientific Master authority.

## Experimental design

Experiment:

`CAMERA5_VENDOR_ROUTE_FULL_FACTORIAL_2_LEVEL_4_FACTOR`

Four factors are screened in a complete 2-level, 4-factor design (`2^4 = 16` profiles):

| Factor | Vendor key | Native type | Native tag |
|---|---|---|---|
| A | `org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW` | BYTE | `0x801F0027` |
| B | `org.codeaurora.qcamera3.sessionParameters.RawCbSourceType` | INT32 | `0x801F0009` |
| C | `org.codeaurora.qcamera3.sessionParameters.EnableXCFAOptimization` | BYTE | `0x801F0036` |
| D | `org.codeaurora.qcamera3.sessionParameters.HALOutputBufferCombined` | INT32 | `0x801F0034` |

Matrix level `0` means **UNSET / NO WRITE**. Matrix level `1` means the already type-validated numeric value `1` is written with the resolved native type. Numeric value `1` has **no promoted vendor semantic**.

The matrix changes request/session control only. The proven v0.20 acquisition ordering remains: physical Camera 5 route, original Image.Plane source seal first, post-HAL envelope observation read-only, camera objects closed, then Stage 3.6 and Stage 3.7 read-only audits.

## Hard evidence inventory

Only individually readable v0.30 evidence JSON files are counted here. Uploaded ZIP bundles are retained as auxiliary provenance containers, but their contents are not promoted because archive inspection timed out in the current analysis environment.

| Run | ABCD | Keys written | Session params | Source SHA-256 | Payload SHA-256 | Topology differential vs v0.20 |
|---|---:|---:|---|---|---|---|
| R01 | `0000` | 0 | no | `1ef8bc365b791687f58c3cdf711f6f4919366c688f7fb4ce7f9586e7832736b7` | `dc9fec4e1e9c2af3c6314c936968cd2c2fa78d640911a8948f340b7f0cd5fd03` | no |
| R02 | `1111` | 4 | yes | `c2627863173e3d232bf70a409ce8fd0c468529526d651a1068f8065c9e09612b` | `c6d6778f457ee39d5a0d3c41a69115e40b2c30cc1056e6f5af33069e3151d080` | no |
| R04 | `1010` | 2 | yes | `0a427c7b68db4b181e223079eae4c62e8a8a15cd7abafb12716577c291ec534f` | `7d564bde34c1a57dc5d4dcd3795841bc09dc078b9a205356f5bace3182218545` | no |
| R11 | `0010` | 1 | yes | `2d288be2fb38667a034b69baa7bd28dcb111f44ddd9f66b40ebdf52c43663eaa` | `913c7f8a37dc69d516ac46294de714aa967a1568b8fd5ade40595df7ac1ed594` | no |
| R13 | `0100` | 1 | yes | `34f9612cbb8fcf0b952d955ca878a958697e666fa4601b742ede95b65ea7e666` | `615aed736da8b844dd99c411d3419a448759a132c00ed938a7c10e93058a2bd9` | no |
| R14 | `1011` | 3 | yes | `2635b2b2da48e336c63ca9383deccb67a5f62ecb111af418828030e73e2f863f` | `a9be07e6f0d8994cc921192eaf3bafa4986c34f77739a2bbaa8abf143ff9df22` | no |
| R15 | `1000` | 1 | yes | `fe128ee22a71a904c41003ff2dffbae904141ec9485295ccbd8108d5a6b1712a` | `f76960addab8c620812a4cd8185de2cb58ea5e450079fa44b5e54f53e757a821` | no |
| R16 | `0111` | 3 | yes | `6f4e1446f5ddd52ab79e03d163fde54c27c66b3ee49787febf4b6693b804fd99` | `a9b38dece95ba0ae97705549ba55b0bbef937d412f01f27bd6b9e13caacf5ee7` | no |

A prior independent R16 capture is also retained:

- source SHA-256: `de6f94b0da4a6798691c8b6dba138f67012aa61e2adb33411f8b2be7b7724cec`
- payload SHA-256: `3d17dff7d87de5cc49aa681b347583b4128e79423fe5743263d36e9fb88c95db`

The two R16 captures have different byte hashes because they are different physical captures, but both land in the same measured topology class. This is a structural replicate, not a byte-identity replicate.

## Common measured topology across all currently hard-evidenced profiles

Every hard-evidenced profile above remains in the same measured app-visible topology class:

- physical result Camera `5`;
- app-visible RAW envelope `16320x12288`;
- source bytes `401,080,320`;
- row stride `32,640`;
- pixel stride `2`;
- only rows `0..767` populated in the declared-width raster audit;
- rows `768..12287` all zero;
- populated payload byte count `25,067,520`;
- exactly one advertised standard RAW geometry with the same U16 byte count: `4080x3072`;
- Stage 3.7 payload is an exact prefix copy, not a transformed reconstruction;
- no measured envelope/populated-prefix topology differential versus v0.20.

Small numbers of zero-valued code samples can occur inside the populated prefix. Those sample values do not alter the row/population geometry and are not treated as route changes.

## Current negative results

### R01 — contemporaneous all-UNSET matrix control

R01 (`0000`) writes none of the four vendor keys. It reproduces the same measured topology class as v0.20. This shows that the v0.30 matrix framework itself did not create the 401-MB-envelope / 25-MB-populated-prefix observation.

### R02 — all four tested high levels together

R02 (`1111`) writes all four type-resolved numeric-one values together. It remains in the same topology class. Therefore the simple tested hypothesis that **all four numeric-one states together are sufficient** to unlock a different app-visible RAW envelope/populated-prefix route is not supported.

### Single-high profiles already hard-evidenced

- R11 (`0010`): C high only — no topology differential.
- R13 (`0100`): B high only — no topology differential.
- R15 (`1000`): A high only — no topology differential.

D-only (`0001`, R09) is not yet present as individually readable hard evidence.

### Multi-factor profiles already hard-evidenced

- R04 (`1010`) — no topology differential.
- R14 (`1011`) — no topology differential.
- R16 (`0111`) — no topology differential in two independent captures.

These results narrow the tested parameter space but do **not** establish that the vendor keys are ineffective. The numeric semantics remain unknown, and eight matrix profiles still lack individually readable evidence JSONs.

## Missing hard-evidence profiles

The missing run numbers are:

`R03, R05, R06, R07, R08, R09, R10, R12`

Equivalent ABCD profiles:

- R03 `0101`
- R05 `0011`
- R06 `1100`
- R07 `0110`
- R08 `1001`
- R09 `0001`
- R10 `1110`
- R12 `1101`

Do not count a run as complete from a screenshot, ZIP filename, bundle size, UI progression or recollection alone. For matrix completion, require the corresponding readable evidence JSON.

## Bundle handling

Several `TRUTHRAW_CAM5_V030_MATRIX_EVIDENCE_BUNDLE_*.zip` files were uploaded during recovery. They are preserved as auxiliary provenance. Current archive inspection repeatedly timed out, so their internal member lists have not been independently verified here.

Consequently:

- no run is promoted from ZIP metadata alone;
- individually readable JSONs define the hard completion set;
- future bundle parsing may recover additional runs, but that would be a later evidence update, not a retroactive assumption.

## Next device action

Continue v0.30e using its first-missing recovery selector. Save each produced evidence JSON individually. Bundle export is useful as redundancy but is not the sole authority.

Current hard missing set:

`R03, R05, R06, R07, R08, R09, R10, R12`

## Forbidden promotions

These partial matrix results do not prove:

- untouched photodiode ADC output;
- native physical sensor geometry;
- exact sensor binning/remosaic mechanism;
- 200 MP optical resolution;
- that vendor-key names describe their real semantics;
- that numeric `1` means enabled/full/native/unbinned;
- that the four tested keys are globally ineffective;
- that untested matrix profiles cannot differ.

The experiment remains acquisition/provenance/sample-topology research only.
