# TruthRaw active research and validation state — 2026-09-14

Status: **ACTIVE IMPLEMENTATION / VALIDATION LEDGER**

This file preserves the concrete active state around PR #23, Honor physical evidence, PURE float32 DNG validation, calibration research, and remaining blockers. It is a snapshot document: always re-fetch the live branch head and workflows before making a fresh CI claim.

## 1. Active repository work

Repository:

`1DiDaDo1/TruthRaw`

Branch:

`research/ui-output-modes-certificate-v0.1-2026-09-14`

Draft PR:

`#23 — Integrate four-mode output policy + TRUTHRAW PURE float32 DNG`

Base:

`research/project-factcheck-optimization-v0.1-2026-09-13`

PR remains intentionally draft. It must not be merged merely because documentation is complete.

## 2. Product contract on PR #23

Public modes:

1. `JPG`
2. `JPG XL`
3. `TRUTHRAW PURE`
4. `TRUTHRAW ADVANCED`

Appearance controls:

- `Colourful`
- `Detailed`
- `Soft`
- `HDR`

Rules:

- appearance toggles apply to JPG/JPG XL/ADVANCED only;
- PURE is neutral;
- missing/unknown mode fails safe to PURE;
- JPEG XL remains fail-closed until a real encoder is validated;
- appearance intent does not modify reconstruction, master identity, zero-line, evidence counts or Backplane authority.

Android prototype identity:

`0.7-four-mode-certificate`

Languages currently represented:

- English fallback
- Dutch
- German
- French

## 3. TRUTHRAW PURE route

Current intended scientific export chain:

`sealed source -> source-bound color -> Scientific Master v0.2 -> Technical Backplane phase 2 -> canonical 64x64 Scientific Master replay -> exact master digest gate -> cameraToXyzD50 -> float32 XYZ-D50 LinearRaw DNG`

Writer properties:

- IEEE float32;
- 3 samples/pixel;
- `PhotometricInterpretation=34892` (`LinearRaw`);
- classic TIFF/DNG path in current writer;
- 64x64 tile projection;
- negative values preserved;
- values above 1 preserved;
- no appearance/tone/gamut mapping;
- exact master identity gate;
- one physical frame / one independent evidence root.

Older 16-bit DNG routes remain compatibility projections.

## 4. TruthRaw Certificate v0.1

Canonical record marker:

`TRCERT01`

Current certificate class for PURE:

- projection class: TruthRawPureFloat32Dng (`4` in current v0.1 schema);
- claim class: Reconstructed (`3`);
- signature state: `UNSIGNED DEVELOPMENT`;
- algorithm: NONE until a trusted issuer scheme is provisioned;
- evidence counts: `1/1`.

Certificate binds source/master/zero-line/scene-scale/Backplane/build identity under the current schema.

Important limitation:

Certificate v0.1 does not independently serialize every internal exporter boolean, including the internal `appearanceApplied` and `counterfactualObservationCreated` states. The artifact verifier must report that rather than overclaiming file-byte proof.

Backplane CRC32 is an integrity field, not collision-resistant cryptographic identity.

## 5. Formal five-file physical regression set

These are test evidence, not automatically calibration evidence.

1. `IMG_260816_134122_304_005.dng`
   - SHA-256 `04068eabc681f241b9fa102e46b27b964a636be13361e1d8bacabe0db60471e5`
2. `IMG_260816_134204_911_008.dng`
   - SHA-256 `cf8ccfd48afe3cff6214f7cfb8707221c3722afa3e5d332464cd9d7f485d3521`
3. `IMG_260816_143716_085_041.dng`
   - SHA-256 `8d8d1b1d84189bb8836990a09a7ee2676545a3ba3af89d02fe5100efe8c9dfef`
4. `IMG_260830_143012_297_014.dng`
   - SHA-256 `fac842110cfd5b527aae2fa46237824d444aa5701017c2b68021962ebce47da3`
5. `IMG_260908_193845_662_027.dng`
   - SHA-256 `d7981684ef6238f9d036aea0c587e15c600d690c2622dd33657d7625314ec070`

Common known properties:

- 4080x3072;
- BGGR;
- WhiteLevel 1023;
- Honor BKQ-N49 / MotionCam-domain sources;
- one frame/evidence root each.

The formal gate remains open until actual Android-produced PURE outputs for the full set are independently extracted and verified end-to-end.

## 6. Supplemental exact-source Honor PURE validation

This is supplemental and must not be counted as one of the formal five.

Source:

`IMG_260908_193753_205_015.dng`

- bytes `25,899,870`
- dimensions `4080x3072`
- SHA-256 `757f6aaa0b45a0e68270ee8073d338895d2df284d8ae57bca5683a5ca2ba2573`

Device/app observation:

- HONOR BKQ-N49;
- Android 16;
- package `com.truthraw.adaptiveui`;
- version `0.7-four-mode-certificate`;
- exact tested APK SHA-256 `1c1acd4855a77c2547efdc3e540ebc96a9455f396ff0ddf8eb69fa8a519392f6`;
- source pre/post hash stable;
- tile reads `7,680`;
- no full RAW materialization observed in the empirical harness;
- scientific claim remained false for source metadata color because independent physical color calibration was not proven.

Saved PURE artifact:

`IMG_260908_193753_205_015_truthraw_pure_float32_v0_1.dng`

- bytes `151,021,352`
- SHA-256 `a80390deb51e315e73973782f151555521eed819383ebe45a2906a9b321dc788`
- 4080x3072;
- 32/32/32 float;
- SampleFormat 3/3/3;
- LinearRaw 34892;
- 3 samples/pixel;
- 64x64 tiles;
- DNG 1.4;
- finite real serialized components `37,601,280`;
- negative components `288,739`;
- components `>1`: `218,566`;
- observed real range approximately `-0.0091582136 .. 1.45196819`;
- tile padding components checked zero: `147,456`.

Embedded source/master identity:

- source SHA: `757f6aaa0b45a0e68270ee8073d338895d2df284d8ae57bca5683a5ca2ba2573`;
- Scientific Master SHA: `dc747bc3bc9d7ab24614560ead67008455742c6732faf6f1a3167d56670eae97`;
- zero-line SHA: `4a494991c163986b98dd3795bc1e6a27d0674fea4c3c3d1a070bcebb5292ee95`;
- scene-scale SHA: `90cca2a7932116f9059174e50028817a0ecc66df182d107eda92b1955cb6fdaf`;
- Backplane CRC: `2144df1c`;
- build identity: `e8ed38cc9b92378760f60ae17d90dc6b3149f7e4405cc97eb7a9f36166cc8e55`.

Result:

**PASS — exact-source artifact/projection integrity + device/app cross-binding.**

Does not prove:

- all five formal sources;
- Adobe/Lightroom interoperability;
- independent physical color calibration;
- trusted signing;
- file-byte serialization of every internal appearance/counterfactual boolean.

## 7. Seven-file Honor ISO ladder

Files:

- `IMG_20260908_194910.dng` — ISO200 — SHA `d6cdae0877f02b3e1575c6d9a3cfb528fe141708334aa2204b5c35e16be88daa` — exposure ~0.09940137 s
- `IMG_20260908_194916.dng` — ISO400 — SHA `06be2865d149404d96e49a221728528701180f8eadbb1e9f80fb18b7544f1369` — exposure ~0.09940137 s
- `IMG_20260908_194924.dng` — ISO800 — SHA `b7c47c3f82efe3a7f4652d604b8a4994b049bfa9d880651754fd6558ce660c12` — exposure ~0.049925934 s
- `IMG_20260908_194929.dng` — ISO1600 — SHA `69875efa0aa17b611ea89c4c5141c29b990d31194a47752539054545e6aa8895` — exposure ~0.026520332 s
- `IMG_20260908_194939.dng` — ISO3200 — SHA `275db1de6bfc43e5e6b7b226f58cf32a8df7d9fb60694407f4a8065805b4db23` — exposure ~0.013260166 s
- `IMG_20260908_194947.dng` — ISO6400 — SHA `60d7eb1570e809dc3e138c8714a441a79a3dba130f7989d8c03b9e39143f4804` — exposure ~0.006630083 s
- `IMG_20260908_194951.dng` — ISO12800 — SHA `dd26e86d20ce9f864538928fbc2c183262045468a42c53ab0908c92ca7163788` — exposure ~0.003315042 s

Common properties:

- bytes `25,097,544` each;
- 4080x3072;
- HONOR / BKQ-N49;
- one direct-CFA IFD;
- uint16 container;
- CFA photometric interpretation;
- BGGR (`02 01 01 00`);
- WhiteLevel 1023;
- BlackLevel around 64 DN;
- f/2.6;
- focal length 22.48 mm;
- Orientation 1;
- DNG 1.4;
- dual ColorMatrix/ForwardMatrix/CameraCalibration metadata;
- NoiseProfile present.

Exposure-product observation:

- ISO200 x time ~19.88;
- ISO400 x time ~39.76;
- ISO800 x time ~39.94;
- ISO1600+ x time ~42.43.

Interpretation: ISO400-12800 is roughly a matched-brightness/gain ladder; ISO200 sits roughly one stop below ISO400.

Very little clipping was observed even at high ISO.

Metadata `NoiseProfile` grows monotonically with ISO. This is useful source evidence but not by itself independent calibration.

Important limitation:

Direct unregistered pixel subtraction across these scene frames is invalid as a temporal-noise estimator because framing/scene alignment was not fixed strongly enough. Low raw-pixel correlation confirms that caution. Use registered/uniform patches or controlled tripod/dark/flat repeats for calibration.

Suggested research label:

`HONOR_TELE_ISO_SERIES`

This set remains separate from the formal five-file PR artifact gate.

## 8. Real-scene response/repeatability findings

Observed same-exposure ISO100 -> ISO200 aligned signal ratios in one follow-up campaign:

- B `1.9087x`
- G1 `1.9679x`
- G2 `1.9907x`
- R `1.9994x`

Expected one-stop ratio: `2.000x`.

Duplicated ISO800 captures reproduced after alignment at approximately:

- G1/G2/R `~1.000x`;
- B `~0.9905x`.

Interpretation:

**PASS as source-bound gain response/repeatability evidence.**

Not sufficient for full temporal calibration because framing/focus/scene control was not strong enough.

## 9. Covered dark-frame evidence

One covered/no-scene ladder used ISO sequence:

`100, 100, 400, 400, 800, 1600, 3200, 6397, 12522`

with exposure approximately `0.0327347964 s`.

Direct same-condition pair estimate `(A-B)/sqrt(2)`:

- ISO100 temporal sigma ~`0.789-0.790 DN` across CFA channels;
- ISO400 temporal sigma ~`0.867-0.870 DN` across CFA channels.

Repeat-frame correlation was approximately zero at ISO100 and only ~`0.004-0.005` at ISO400 after extreme-outlier exclusion.

The dark frames contain genuine below-BlackLevel samples. This physically reinforces keeping signed post-black state.

The embedded middle NoiseProfile pair appeared to underpredict measured low-ISO green dark variation under the ordinary assumed mapping, but exact MotionCam/device NoiseProfile semantics are not independently certified. Therefore this remains OPEN, not permission to rewrite metadata.

## 10. Exact-ISO8192-associated capture/sample domain

First unusual exact-ISO8192 dark file:

`IMG_260914_104952_817_033.dng`

- SHA-256 `06bf9afdf6a71d4c0b3995dec641f33a8d205cd022139440960131b348650ec3`;
- trimmed dark sigma ~`54-55 DN`;
- raw-zero censoring ~`7.1204%`.

A later same-exposure series used:

`8192, 10244, 8184, 6500, 3200, 1600, 800, 400, 200, 100`

with exposure approximately `0.01648959145 s`.

Second exact-ISO8192 file:

`IMG_260914_110550_363_001.dng`

- SHA-256 `2dae19d2e9d144c4d257808432e58a7246764e1259d6a5033de2e1eff76c644e`;
- reproduced ~`54-55 DN` dark sigma;
- reproduced ~`7.13%` raw-zero censoring;
- identical BlackLevel values and six-value NoiseProfile to the first unusual exact-8192 observation.

At the same exposure:

- ISO8184 remained ordinary with `0%` raw-zero censoring and ~`2.6 DN` sigma;
- ISO10244 remained ordinary with `0%` raw-zero censoring and ~`3.2 DN` sigma.

Current conclusion:

- simple monotonic `ISO >= 8192` threshold: **REJECTED**;
- reproducible discrete exact-ISO8192-associated sample domain: **PASS AS OBSERVATION**;
- physical/software cause: **OPEN**.

Forbidden causal labels until proven:

- DCG switch;
- analog-gain switch;
- sensor defect;
- decoder bug.

New rule:

**Noise calibration must be keyed to exact capture/sample-domain identity, not ISO magnitude alone.**

## 11. CalibrationPack research state

The current FotoGraaf calibration architecture contains machine-readable contracts for:

- calibration acquisition protocol;
- dataset manifest;
- CalibrationPack validation;
- calibrated claim records;
- scene admission;
- measurement-model shadow comparison.

Key scope/admission facts:

- exact camera/lens/capture/sample-domain scope is required;
- runtime shutter/ISO/f-number/gain-readout state must remain in the validated domain;
- temperature becomes mandatory only when the pack claims calibrated temperature dependence;
- operational extrapolation is not silently allowed;
- ISO alone may not select a domain;
- worker count cannot change calibration authority;
- calibration does not change the scene's `1/1` evidence counts.

## 12. Measurement model binding improvement

A model name alone is insufficient. A scene binding must ultimately be tied to the exact model/protocol bytes or their cryptographic identities.

Current research direction therefore binds at least:

- sourceEvidenceSha256;
- packId;
- dataset manifest identity;
- calibration scope identity;
- quantity;
- modelId;
- uncertaintyModelId;
- model SHA-256;
- protocol SHA-256;
- valid-domain identity;
- validation/uncertainty report identities;
- acceptance protocol identity.

A numerical change such as changing a response scale while retaining the same human-readable model name must invalidate the exact binding.

## 13. Shadow MeasurementLab adapter

Research-only module:

`docs/research/fotograaf-scene-metrology-v0.1/shadow-measurement-model-v0.1/`

Purpose:

compare current source-bound interpretation with an admitted calibration-assisted interpretation before production promotion.

Source arithmetic is intentionally matched to canonical v4.7i Stage-2:

`((raw - phaseBlack) / max(whiteLevel - phaseBlack, 1)) * sourceGainField`

Research calibrated parameters may include only explicitly bound/allowed values such as:

- phase black;
- noise model;
- scalar response scale;
- saturation code;
- dark-SNR threshold.

Current enforced invariants:

- source and binding SHA match;
- exact measurement model binding required;
- one physical frame / one independent scene evidence root;
- existing GainMap must be exactly once;
- no second GainMap/lens-shading correction;
- negative post-black values remain signed;
- calibration cannot remove source WhiteLevel censoring;
- noise-only calibration cannot move the source signal coordinate;
- worker count is execution only.

The module is deliberately blocked from canonical/Android production routes while in shadow status.

## 14. Preserved CI failure and correction

An early shadow-model CI matrix run failed in GCC Release, Clang Release and Clang ASan/UBSan because the test program used ordinary `assert()` while Release set `NDEBUG`.

The asserts were compiled out. With `-Wall -Wextra -Wpedantic -Werror`, variables/functions used only inside those asserts became unused and compilation failed.

This was treated as a useful real failure rather than hidden.

Correction:

- keep strict warning/error policy;
- use an always-active test `require()` mechanism;
- ensure Release builds actually execute scientific invariant checks.

This failure history should remain preserved.

## 15. Current CI interpretation rule

Do not claim `all current CI green` from an older head.

Every documentation/code commit moves the PR head and schedules a new workflow set. Fresh status must be fetched for the exact head being discussed.

A previous scene-admission workflow was observed PASS after the exact-scope/fail-closed implementation. Shadow-model workflow history includes the Release-assert failure described above followed by fixes. The latest exact-head matrix must be re-fetched before declaring final green status.

## 16. Existing-app integration state

TruthRaw can be embedded into another Android app most cleanly as a sealed processing module.

Recommended division:

- host app: UI, file picker, storage, user workflow;
- TruthRaw: source sealing, decode/measurement interpretation, reconstruction, Scientific Master, provenance, export policy;
- interface: narrow request/status/result contract through Kotlin/JNI/native boundary.

Host preprocessing must not alter the source before TruthRaw. Direct Camera2 capture integration requires stronger evidence plumbing than file import because camera ID, mode, stride, timestamp, metadata and payload all become source identity.

## 17. Still-open production gates

### OPEN / BLOCKED

- full five-source on-device PURE artifact gate;
- Adobe Camera Raw / Lightroom interoperability;
- trusted issuer signing key and external verifier trust distribution;
- JPEG XL encoder validation;
- final render semantics for Colourful/Detailed/Soft/HDR;
- independent physical color calibration;
- independent NoiseProfile semantics/calibration;
- complete PTC/conversion-gain/read-noise characterization across sample domains;
- physical cause of exact-ISO8192-associated domain;
- calibrated optics/lens inverse;
- production promotion of FotoGraaf shadow model;
- full physical light/material/spectral authority.

### REJECTED

- silently treating current test/scene frames as calibration evidence;
- selecting empirical calibration regime by ISO magnitude alone;
- allowing counterfactual/HDR appearance to create physical authority;
- treating constructed/re-Bayer RAW as original evidence;
- using GCam/APK/computational RAW as TruthRaw scientific authority.

## 18. Immediate scientifically justified continuation

When continuing this research line:

1. re-fetch the exact PR head and workflow state;
2. keep current Scientific Master/PURE writer frozen unless a regression or explicit scientific reason requires change;
3. complete/green the shadow-model GCC/Clang/sanitizer matrix on the exact latest head;
4. preserve the shadow route as non-production until an actual validated CalibrationPack exists;
5. acquire controlled dark/flat/linearity repeats per exact capture/sample domain;
6. independently determine NoiseProfile plane/coordinate semantics;
7. test calibration effect in parallel with the source-bound path before promotion;
8. keep every multi-frame calibration dataset separate from later single-frame scene evidence;
9. continue the formal five-source PURE artifact gate and Adobe interoperability as independent validation streams.
