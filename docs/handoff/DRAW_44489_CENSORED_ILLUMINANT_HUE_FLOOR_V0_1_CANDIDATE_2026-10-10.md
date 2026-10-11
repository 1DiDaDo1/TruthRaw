# D.RAW 44489 — Censored Illuminant Hue Floor v0.1 Candidate — 2026-10-10

Status: **CI-GREEN APPEARANCE CANDIDATE / REAL-DEVICE WARM-HIGHLIGHT VALIDATION PENDING**.

This checkpoint follows Deep-Censor Chroma Guard v0.2. It does not change sealed source/CFA evidence, Scientific Master, calibration authority, reconstruction promotion or scientific writeback.

## Real-device motivation

User bundle:

`DRAWJPEGtelelamptest.zip`

- bytes: `26,237,955`;
- SHA-256: `fc41536dfc1d384ceae852b314afbe91b766ddfb3d1625ea7f6745ef7b181f06`.

Contained PRO JPEG:

`DRAW_CAPTURE_1791652795293_tele_4080x3072_draw_pro_fullres.jpg`

- bytes: `23,324,624`;
- SHA-256: `f984ac88ff72ed3dc5276ac20ad06f49673bc6aff1f80d1a1059321f700fa2e5`;
- geometry: `4080 x 3072`;
- route: PRO tele.

User judgement: after the successful purple/deep-censor work, the brightest white/highlight part of the lamp still lacks a darker yellow/amber hue. The target is therefore not more highlight desaturation and not lower exposure; it is a small source-white-consistent warm hue floor in nearly neutral, deeply censored bright output.

## Candidate design

Runtime file:

`suite_android/app/src/main/cpp/presentation_censored_illuminant_hue_floor_v0_1.h`

Ordering:

`Near-Censor Shoulder -> accepted Censored Chroma Fallback -> Deep-Censor Chroma Guard v0.2 -> Warm Illuminant Retention v0.1 -> Censored Illuminant Hue Floor v0.1 -> historical highlight observer -> gamut fit`

Hard gates:

- ADVANCED/PRO downstream Appearance only; PURE bypass by caller;
- valid existing source-bound `SourceWhitePoint`;
- existing Warm Illuminant CCT gate must admit warmth;
- CENSOR fraction starts at `0.50`, full at `0.80`;
- linear Rec.709 luminance starts at `0.72`, full at `0.95`;
- full neutral eligibility through relative chroma `0.015`, fades to zero at `0.090`;
- maximum introduced relative chroma floor `0.060`;
- chroma direction comes from the admitted source-white xy in the current D50 linear-sRGB basis;
- no hard-coded yellow/orange hue;
- no hue/object/semantic detector, vendor/camera identity, sharpening, blur or resampling;
- clearly chromatic output is exact no-op;
- strong purple regression sample is exact no-op;
- Rec.709 luminance is preserved by construction;
- final gamut fit stays downstream;
- no source mutation, no Scientific-Master writeback, no new evidence and no recovered-scene-colour claim.

## Exact code / CI lineage

Initial candidate commit:

`a02e12133a8aa8b7777d6ef65ce20e7abc0c21a6`

Exact CI-generated runtime wiring:

`2f4375e4371ace6770b6ae25690f025ee7191208`

Workflow:

- `D.RAW Censored Illuminant Hue Floor v0.1`;
- run `38073401245`;
- job `114275277204`;
- conclusion **SUCCESS**.

Green checks include the new hue-floor regression, Deep-Censor v0.2, Warm Illuminant, accepted Censored Chroma Fallback, Near-Censor Shoulder, Natural Light, gamut fit, PURE headroom, sealed Full-Frame Streaming integrity, strict High-Fidelity Q100/true-4:4:4 JPEG regression, Android unit tests/build and APK-native verification.

## Candidate APK

Artifact:

- id `11677282649`;
- name `draw-censored-illuminant-hue-floor-v01-apk`;
- archive digest SHA-256 `efdbf8c3fe4534690877f713690bd9396dcc91092021158dcde451bfb86b0cc8`.

Extracted APK:

- member `app-debug.apk`;
- bytes `8,891,987`;
- SHA-256 `abec7589b9da32a1fbc2ff0674c8f83db93440274a00d2ffc3777cc92e68e85b`.

Classification remains **CI-GREEN APPEARANCE CANDIDATE / REAL-DEVICE WARM-HIGHLIGHT VALIDATION PENDING**.

PASS requires the white/highlight lamp region to retain its brightness/detail while gaining a more natural darker yellow/amber source-white impression, with purple suppression at least as good as Deep-Censor v0.2, no halo/seam/ringing, no global yellow cast, no legitimate-colour washout and valid Q100/true-4:4:4 output.
