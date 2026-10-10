# D.RAW 44489 — Natural Light Local Field Tone v0.1 — Candidate

Date: 2026-10-09

## Purpose

Start the next Natural Light appearance improvement after the physically accepted warm-illuminant retention v0.1. The target is the remaining local brightness / luminous-field impression visible around illuminated surfaces, without changing the already accepted source-white colour response.

This candidate is **not** a physical light-transport reconstruction. It is an image-space View/Appearance operator derived from the already rendered single observation. It may improve perceptual tone under localized illumination, but it cannot establish lamp intensity, geometry, reflectance, spectrum, bounce-light paths or world-space irradiance.

## Placement

Intended downstream order:

`Scientific/output render -> existing Natural Light exposure/shadow appearance -> output acutance/HDR/color fullness -> Natural Light local field tone -> warm-illuminant retention -> highlight shoulder -> near-white chroma guard -> gamut -> RGB24/JPEG/Free Raster`

PURE remains outside this stage.

## v0.1 contract

The candidate:

- runs only when Natural Light is enabled in ADVANCED/PRO;
- is explicitly excluded from the PURE extended-linear headroom branch;
- derives a deterministic local luminance field from already rendered RGB;
- excludes censored source samples from the local-field statistic;
- uses a resolution-scaled local radius, bounded to 12..32 source pixels;
- applies only a common RGB gain, preserving channel ratios;
- protects deep blacks;
- protects near-white highlights so the accepted headroom/chroma guards remain authoritative;
- caps the additional local appearance lift at **+0.14 EV**;
- does not use semantic segmentation, vendor profiles, HONOR-specific fitting, AI/ML or generative inference;
- does not mutate source evidence or Scientific Master and creates no scientific authority.

## Interpretation

The local field is an appearance cue, not evidence of true light transport. A bright neighbourhood can justify a modest display-referred brightness impression; it cannot justify reconstruction of missing or censored radiance.

This is consistent with the project evidence law and with colour/appearance literature in which perceived appearance depends on viewing/adaptation conditions and local/background luminance. Those principles motivate a conservative appearance model only; they do not promote the derived field into measured scene radiometry.

## Acceptance gates

Before real-device evaluation:

1. deterministic apply tool must be idempotent;
2. dedicated host regression must prove disabled/censored/deep-black/near-white protection, RGB-ratio preservation and the +0.14 EV cap;
3. runtime placement must be after the PURE/ADVANCED-PRO branch split and before warm-illuminant/highlight/gamut guards;
4. existing warm-illuminant, highlight, PURE headroom, JPEG, sealed Full-Frame Streaming and Android tests must remain green;
5. APK must build and verify.

Real-device acceptance then compares the same warm-lamp observation in ADVANCED/PRO. Success means improved local brightness impression without washed blacks, halo/seam artefacts, reintroduced magenta highlights, colour-ratio drift, or changes to PURE.

Until that physical round succeeds, this remains **CANDIDATE / NOT PHYSICALLY ACCEPTED**.
