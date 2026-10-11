# D.RAW 44489 — Warm Illuminant Retention v0.1 — Real-Device PASS — 2026-10-09

## Status

This document records the physical acceptance of the downstream warm-illuminant appearance layer on PR #131.

Accepted runtime head:

`8ad695bad5be4fbc799fa781d401124e466ca48e`

Build provenance:

- workflow: `D.RAW Free Raster v0.3 Finish APK`
- run: `37851925752`
- artifact: `11582322599`
- APK bytes: `8,858,859`
- APK SHA-256: `2449505201762a78911663f8b6198086226b978fd3edc4b793d03712296b7031`

PR #131 remains open, draft and unmerged.

## Real-device evidence

User bundles:

- `DRAWwarmtestADV2.zip`
- `DRAWwarmtestPRO.zip`

Accepted ADVANCED JPEG:

`DRAW_CAPTURE_1791498014821_tele_4080x3072_draw_advanced_fullres.jpg`

SHA-256:

`89b14f498e03c3b3c42c393cd435ab479e039421ee2d39f7acfbabf842cb0d17`

Accepted PRO JPEG:

`DRAW_CAPTURE_1791498365845_tele_4080x3072_draw_pro_fullres.jpg`

SHA-256:

`19d30fa75f296b47a55d3a5f39f46e2cf15f4a8160d978b72e376fb06077987f`

Observed result:

- source-white warmth retention is visibly active in ADVANCED and PRO when Natural Light is enabled;
- lamp shade, lit wall and curtains retain a credible warm illuminant impression instead of being over-neutralized toward D50;
- the previous near-white magenta/purple highlight defect does not return;
- PURE remains isolated from the new warm-illuminant appearance stage;
- the Q100 / true-4:4:4 JPEG contract remains intact.

Classification:

- **PASS — ADVANCED warm-illuminant retention real-device**
- **PASS — PRO warm-illuminant retention real-device**
- **PASS — near-white highlight guard retained**
- **PASS — PURE isolation retained**
- **PASS — Q100 / true-4:4:4 output contract retained**

The HONOR stock-camera image used during this round is an **appearance reference only**. It is not scientific truth, calibration evidence, a vendor profile, or a target that may create authority.

## Scientific / authority boundary

The accepted warm-illuminant stage remains strictly downstream `APPEARANCE_ONLY / DERIVED_PRESENTATION_OUTPUT`.

It may not:

- mutate sealed source/CFA evidence;
- rewrite Scientific Master;
- change BlackLevel, WhiteLevel, clipping/censoring or source calibration;
- infer a source spectrum or classify a scene-light kind from CCT;
- create MEASURED evidence or scientific authority;
- perform scientific writeback.

The stage uses only the already authority-checked source-metadata-bound white-point state and applies a bounded partial inverse chromatic adaptation in presentation linear RGB. Its maximum warm-source retention remains 18%; this physical round gives no reason to strengthen it.

## Next accepted direction

Do **not** increase warm-illuminant retention based on these images. The remaining visible difference is primarily tone / local luminous-field appearance: the stock-camera reference presents illuminated wall regions with somewhat more local brightness impression.

The next candidate therefore belongs downstream in Natural Light View/Appearance, not in source white balance or Scientific Master:

`ADVANCED/PRO Natural Light -> local luminous-field appearance cue -> warm-illuminant retention -> highlight guards -> gamut/output`

Any first implementation must be labelled as an image-space appearance approximation, **not physical light-transport reconstruction**, unless independent geometry/material/illumination evidence later authorizes a stronger claim.
