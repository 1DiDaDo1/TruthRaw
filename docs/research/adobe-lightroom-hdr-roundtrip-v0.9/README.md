# TruthRaw ↔ Adobe Lightroom HDR round-trip v0.9

**Status:** RESEARCH — NOT MAIN-PROMOTED  
**Date:** 2026-09-16  
**Source:** one physical HONOR BKQ-N49 tele DNG, no HDR merge, no synthetic exposure stack

## Empirical result

The first real Lightroom HDR round-trip has now been returned as three AVIF files made from the exact v0.8 source:

`IMG_BNC_TRUTHRAW20260907_094449_565.dng`

Source SHA-256:

`7930ba5db0fe1b7b9dcc09e9337efbca76b8877fb78a6dc4667761bf25159b67`

The user explicitly reported that the brightest export was created by moving only the tone/white response upward to fill the HDR range. The embedded Adobe XMP independently confirms the controlled difference: global Exposure stayed `0.00`; the unedited export has `Whites2012=0` and a `Linear` tone curve; the bright export has `Whites2012=+2`, `ToneCurveName2012=Custom`, and an extended HDR tone curve ending at `386,500`.

All three exports retain `HDREditMode=1` and `HDRMaxValue=+8.00`. Therefore the large brightness change occurred while the HDR Limit remained unchanged.

## Frozen exports

### A — unedited HDR sRGB / Rec.709

- file: `IMG_BNC_TRUTHRAW20260907_094449_565.avif`
- SHA-256: `4828cb3bcba031a063b170664edc7d9de8ad89572e983112229ee1a0c434592c`
- bytes: 1,856,625
- 4064x3056
- AV1 10-bit 4:4:4
- transfer: SMPTE ST 2084 (PQ)
- primaries: Rec.709
- `HDREditMode=1`
- `HDRMaxValue=+8.00`
- Exposure: 0.00
- Whites: 0
- Tone Curve: Linear
- Adobe content-color-volume max: 1404.717812 nits
- Adobe content-color-volume average: 18.191564 nits
- MaxCLL: 1383 nits
- AVIF contains a second stream titled `GMap` and declares the compatible brand `tmap`

### B — unedited HDR P3

- file: `IMG_BNC_TRUTHRAW20260907_094449_565 (1).avif`
- SHA-256: `e4ddf68cc4f286dcad979a01776f2755b8116235d375b569d7f3ed8b2cc39b31`
- bytes: 1,691,444
- same geometry / 10-bit 4:4:4 / PQ
- primaries: P3-D65
- same HDR Limit +8
- Exposure 0.00 / Whites 0 / Linear curve
- Adobe content-color-volume max: 1404.659249 nits
- Adobe content-color-volume average: 18.192041 nits
- MaxCLL: 1383 nits

The unedited Rec.709 and P3 exports have effectively identical luminance behavior. Their maximum-luminance metadata differs by only about 0.0042%, and MaxCLL is exactly the same. This is useful as a control: changing output gamut did not create the large brightness expansion seen in the edited file.

### C — tone-expanded HDR sRGB / Rec.709

- file: `IMG_BNC_TRUTHRAW20260907_094449_565 (2).avif`
- SHA-256: `60f8cd7a51f8b011f131ecb10932bbda28b03d3444e06d135c2828865fb77d8e`
- bytes: 9,973,759
- same 4064x3056, AV1 10-bit 4:4:4, PQ, Rec.709
- same `HDREditMode=1`
- same `HDRMaxValue=+8.00`
- Exposure: 0.00
- Whites: +2
- Tone Curve: Custom
- SDR curve points: `0,12`, `152,228`, `255,255`
- extended HDR curve points: `0,12`, `152,228`, `386,500`
- Adobe content-color-volume max: 5315.807728 nits
- Adobe content-color-volume average: 47.547779 nits
- MaxCLL: 5259 nits
- GMap/tmap presentation structure remains present

## Quantified presentation expansion

Relative to the unedited Rec.709 export, the tone-expanded export changes:

```text
maximum luminance ratio = 5315.807728 / 1404.717812
                        = 3.784253095x
                        = +1.920008581 EV

MaxCLL ratio           = 5259 / 1383
                        = 3.802603037x
                        = +1.926987340 EV

average luminance ratio = 47.547779 / 18.191564
                         = 2.613726835x
                         = +1.386108371 EV
```

This is the central v0.9 finding:

> Lightroom can use the same single physical RAW source and the same +8 EV HDR Limit while tone controls move the rendered output roughly 1.92 EV higher at the brightest end.

That extra rendered brightness is therefore **presentation expansion**, not newly measured sensor dynamic range.

## Scientific interpretation

The experiment cleanly separates three different things:

```text
source/evidence dynamic range
        !=
Lightroom HDR output window (+8 EV limit)
        !=
where tone controls place pixels inside that window
```

The +8 HDR Limit is a presentation ceiling/headroom control. It does not imply that the camera measured +8 EV beyond SDR white.

The custom tone curve and Whites +2 can populate more of that available output interval. They do not create photons, recover exact censored radiance, or increase the authority of the Scientific Master.

This empirically supports the TruthRaw rule:

**Representation may exceed the source; knowledge claims may not exceed the evidence.**

and the dynamic-range split:

1. open-ended TruthRange / Scene Master address space;
2. finite source/evidence-supported range;
3. finite reconstruction-supported range;
4. chosen presentation/output range.

## Censoring remains unchanged

The original source contains 217 WhiteLevel-censored CFA samples. Adobe may render those areas at different output brightnesses, but the v0.9 result does not turn those samples into exact recovered radiance. Their scientific meaning remains a lower bound.

## AVIF finding

The returned Lightroom AVIF files are not simple SDR images with a label. The primary image is 10-bit AV1 4:4:4 with PQ transfer, and the container also exposes a second stream named `GMap` plus the `tmap` compatible brand. Adobe's XMP additionally records display-referred HDR content-volume luminance values.

This is strong interoperability evidence for the future TruthRaw presentation route. It does **not** mean Adobe's gain/tone-map metadata becomes scientific evidence.

## Consequence for TruthRaw HDR architecture

A production TruthRaw HDR export should therefore not try to "fill +8 EV" merely because Adobe permits it. The correct policy is:

```text
Scientific Master
  -> authority/censoring-aware presentation projection
  -> chosen HDR output window
  -> PQ/AVIF or another validated HDR exchange
```

Tone expansion may be offered as an appearance control, but must be labelled as such. A neutral/faithful projection should occupy only the HDR range supported by the scene representation and the chosen rendering transform.

## Next gate

The next experiment should derive a **TruthRaw-owned HDR projection** from the Scene Master / Dynamic Authority Field and encode it to a real HDR exchange format, then compare it against these Adobe outputs.

The important comparison is no longer whether Lightroom can do HDR — it can. The next question is whether TruthRaw can preserve its own scene-linear authority, color transform, censoring and uncertainty while using Adobe-compatible HDR transport without allowing presentation tone curves to redefine scientific truth.
