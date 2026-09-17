# HONOR Camera-5 v0.30 vendor-route matrix — complete device result — 2026-09-17

## Status

**COMPLETE MATRIX: 16 / 16 unique profiles have individually readable evidence JSONs.**

TruthRaw v0.20 remains source/payload authority.

Permanent boundary:

`APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF`

No v0.30 capture is promoted to calibration authority or Scientific Master authority.

## Experimental design

Experiment:

`CAMERA5_VENDOR_ROUTE_FULL_FACTORIAL_2_LEVEL_4_FACTOR`

Design:

`FULL_FACTORIAL_2_LEVEL_4_FACTOR_16_RUN_COMPLEMENT_PAIRED`

Factors:

| Factor | Vendor key | Native type | Native tag |
|---|---|---|---|
| A | `org.codeaurora.qcamera3.sessionParameters.EnableIdealRAW` | BYTE | `0x801F0027` |
| B | `org.codeaurora.qcamera3.sessionParameters.RawCbSourceType` | INT32 | `0x801F0009` |
| C | `org.codeaurora.qcamera3.sessionParameters.EnableXCFAOptimization` | BYTE | `0x801F0036` |
| D | `org.codeaurora.qcamera3.sessionParameters.HALOutputBufferCombined` | INT32 | `0x801F0034` |

Level `0` means `UNSET_NO_WRITE`. Level `1` means numeric `1` written using the already device-resolved native representation. The matrix does not assign semantic meaning to either level.

## Complete hard-evidence inventory

| Run | ABCD | Keys written | Session params attached | Source SHA-256 | Payload SHA-256 | Topology differential vs v0.20 |
|---|---:|---:|---|---|---|---|
| R01 | `0000` | 0 | no | `1ef8bc365b791687f58c3cdf711f6f4919366c688f7fb4ce7f9586e7832736b7` | `dc9fec4e1e9c2af3c6314c936968cd2c2fa78d640911a8948f340b7f0cd5fd03` | no |
| R02 | `1111` | 4 | yes | `c2627863173e3d232bf70a409ce8fd0c468529526d651a1068f8065c9e09612b` | `c6d6778f457ee39d5a0d3c41a69115e40b2c30cc1056e6f5af33069e3151d080` | no |
| R03 | `0101` | 2 | yes | `e00f376b551ad861dd4baacad39bae1e510522140b74c36f2ff93ce7a6068203` | `8563b525ede5bd15ab1b340f9f56988441df533a9f7d3a2e3400fb1411b5b539` | no |
| R04 | `1010` | 2 | yes | `0a427c7b68db4b181e223079eae4c62e8a8a15cd7abafb12716577c291ec534f` | `7d564bde34c1a57dc5d4dcd3795841bc09dc078b9a205356f5bace3182218545` | no |
| R05 | `0011` | 2 | yes | `c757ed23328a760495b5d8c122b5bc14dfce343b7b91cb19d548e482f7027a04` | `9cdb3c287047f273de54d959fe762f7b623af45dbb65452a96c55986956fb99f` | no |
| R06 | `1100` | 2 | yes | `1aa50fefc88dd63651ebae077e1b1ff34d8c601b809ad645dab4fe52aa4f0610` | `1e9c4f5d63af6042bf03a6db9d8fb4c3eb558861185441de0e1f92163c00c935` | no |
| R07 | `0110` | 2 | yes | `278d041b5669006e95d64b47d9af10c02601916e05757749021521edf28e65dd` | `d6a68954675a48573595880bc7aadf443560f36eebddc3405f2bd7cd1f70b48c` | no |
| R08 | `1001` | 2 | yes | `f810df10485ae8533fbd75954163fe667d634215606eb87247bee0f45b978646` | `abd86e1e9553292c91f6dc46193378217a4db8894c476b4713154892827287d6` | no |
| R09 | `0001` | 1 | yes | `ea01cb9f040b05cf78921a51e89aca81772fae8b3f580a91f5f588289d818b9b` | `027f89bd608d281cf97262c17d3baaa98b4a181ecb2131690c79e4ec105baa8a` | no |
| R10 | `1110` | 3 | yes | `fdd604f4ea503a3220f5860c1a9ca76da784650018e62b0edc10d71c23373cc9` | `ddaaeba9e22ad8846bb0550234ac63ff1f8a45455af32d979727b11bc585524f` | no |
| R11 | `0010` | 1 | yes | `2d288be2fb38667a034b69baa7bd28dcb111f44ddd9f66b40ebdf52c43663eaa` | `913c7f8a37dc69d516ac46294de714aa967a1568b8fd5ade40595df7ac1ed594` | no |
| R12 | `1101` | 3 | yes | `56104557ff0f98b278a08e797cb3a61bac71b27d893acb944783cd54f5108239` | `fa1a67cda9a7ac992be72936b0486efd1e5c51728671dc874e77cb13368f86fe` | no |
| R13 | `0100` | 1 | yes | `34f9612cbb8fcf0b952d955ca878a958697e666fa4601b742ede95b65ea7e666` | `615aed736da8b844dd99c411d3419a448759a132c00ed938a7c10e93058a2bd9` | no |
| R14 | `1011` | 3 | yes | `2635b2b2da48e336c63ca9383deccb67a5f62ecb111af418828030e73e2f863f` | `a9be07e6f0d8994cc921192eaf3bafa4986c34f77739a2bbaa8abf143ff9df22` | no |
| R15 | `1000` | 1 | yes | `fe128ee22a71a904c41003ff2dffbae904141ec9485295ccbd8108d5a6b1712a` | `f76960addab8c620812a4cd8185de2cb58ea5e450079fa44b5e54f53e757a821` | no |
| R16 | `0111` | 3 | yes | `6f4e1446f5ddd52ab79e03d163fde54c27c66b3ee49787febf4b6693b804fd99` | `a9b38dece95ba0ae97705549ba55b0bbef937d412f01f27bd6b9e13caacf5ee7` | no |

A separate earlier R16 capture is retained as a structural replicate with source SHA-256 `de6f94b0da4a6798691c8b6dba138f67012aa61e2adb33411f8b2be7b7724cec` and payload SHA-256 `3d17dff7d87de5cc49aa681b347583b4128e79423fe5743263d36e9fb88c95db`.

## Common measured topology across all 16 profiles

Every profile lands in the same measured app-visible topology class:

- physical result Camera `5`;
- app-visible RAW envelope `16320x12288`;
- source bytes `401,080,320`;
- row stride `32,640`;
- pixel stride `2`;
- returned `SENSOR_PIXEL_MODE=0` in the tested captures;
- `rawBinningFactorUsed=true`;
- Stage 3.6 classification `ONLY_FIRST_768_ROWS_NONZERO__EXACT_12P5MP_BYTE_PAYLOAD_SIGNATURE`;
- only rows `0..767` populated in the declared-width raster interpretation;
- rows `768..12287` all zero;
- populated/payload byte count `25,067,520`;
- one advertised standard RAW geometry with the same U16 byte count: `4080x3072`;
- Stage 3.7 payload is the exact source prefix, not a transformed reconstruction;
- no measured RAW-envelope/populated-prefix topology differential versus v0.20.

Different captures have different source and payload hashes because the physical observations, exposure and focus are not byte-identical. Hash differences are not treated as route differentials.

## Complement-pair result

Every designed complement pair remains in the same topology class:

- `0000 / 1111` — R01 / R02;
- `0101 / 1010` — R03 / R04;
- `0011 / 1100` — R05 / R06;
- `0110 / 1001` — R07 / R08;
- `0001 / 1110` — R09 / R10;
- `0010 / 1101` — R11 / R12;
- `0100 / 1011` — R13 / R14;
- `1000 / 0111` — R15 / R16.

## What the complete matrix establishes

For the measured response **app-visible RAW envelope + populated-prefix topology**, the response is invariant across the entire tested binary design.

Within this exact experimental domain:

`A,B,C,D ∈ {UNSET, numeric 1 using the resolved native representation}`

there is no observed single-factor or combination-level topology differential.

This includes all four single-high states:

- A-only R15 `1000`;
- B-only R13 `0100`;
- C-only R11 `0010`;
- D-only R09 `0001`.

It also includes every two-factor state, every three-factor state, the all-UNSET control and the all-four-high state.

Because the topology response is identical at all 16 design points, every factorial contrast for this categorical topology response is observationally zero in this matrix. This statement is restricted to the measured topology response; it is not a claim that RAW pixel values, image appearance or other internal pipeline behavior are identical.

## Strongest bounded conclusion

`WITHIN_THE_TESTED_BINARY_DESIGN__UNSET_VS_NUMERIC_ONE_AT_THE_RESOLVED_NATIVE_TYPES__NO_SINGLE_FACTOR_OR_COMBINATION_OF_ENABLEIDEALRAW_RAWCBSOURCETYPE_ENABLEXCFAOPTIMIZATION_HALOUTPUTBUFFERCOMBINED_CHANGED_THE_MEASURED_CAMERA5_RAW_ENVELOPE_OR_POPULATED_PREFIX_TOPOLOGY`

This closes the simple hypothesis that one of these four controls, or an interaction among them, at the tested numeric-one level is sufficient to open a larger populated app-visible RAW domain.

## What remains open

The complete matrix does **not** prove:

- that these four keys are globally ineffective;
- that other numeric values have no effect;
- that vendor names describe real semantics;
- that numeric `1` means enabled/full/native/unbinned;
- that another vendor key or operating mode cannot alter the route;
- untouched/native photodiode ADC output;
- native physical sensor geometry;
- exact binning/remosaic mechanism;
- 200 MP optical resolution.

## Next research direction

Do not repeat the same four-key binary 0/1 matrix. That experimental space is complete.

The next scientifically useful work is either:

1. resolve the **value domain / enum semantics** of the INT32 controls, especially `RawCbSourceType` and `HALOutputBufferCombined`, before testing any other numeric value; or
2. identify new upstream route candidates, resolve their native metadata representation first, and only then intervene.

v0.20 remains the source/payload authority until a future experiment produces a genuine bounded route differential.
