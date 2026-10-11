# D.RAW 44489 — Workspace / Output / Free Raster v0.2

**Status:** binding product/UX and implementation contract — no scientific promotion  
**Date:** 2026-10-06  
**Continuation code:** `44489`  
**Implementation branch:** `feat/draw-workspace-free-raster-v01`  
**Frozen scientific/audit reference:** PR #130 / `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f`

This document binds the next Workspace round after the first real-device acceptance. It converts the observed product issues into an implementation contract without changing the scientific evidence law.

## 1. Product placement

Free Raster is a downstream **View / Output / Projection** surface. It is not the primary evidence ingress and should not dominate the initial workspace before a source/output exists.

Canonical product flow:

`Source / Camera -> Universal Intake -> Scientific Master -> PURE / ADVANCED / PRO -> Unified Output State -> Free Raster / Export`

The central Workspace remains the user-facing orchestration surface. Source selection and the shared PURE/ADVANCED/PRO workbench come first; Output / Free Raster follows downstream.

Free Raster may alter x/y placement, scale, crop/view framing and output-raster representation. These operations never create `MEASURED` samples and never increase scientific authority.

## 2. Two safe raster inputs

### 2.1 Internal D.RAW output

Free Raster should consume an already-rendered presentation/output state from the **existing** MainActivity / Unified Output / projection machinery.

It must not introduce:

- a second RAW/DNG decoder;
- a second Scientific Master;
- a second reconstruction route;
- a second independent output truth;
- authority inference from UI state.

The internal bridge must therefore be a consumer of existing output state, not a new renderer.

### 2.2 External presentation raster

A user may deliberately open JPEG, PNG or WebP as an external raster for viewing/projecting. Such input is always explicitly labelled:

`EXTERNAL_PRESENTATION_RASTER / PRESENTATION_ONLY`

An external raster:

- is not a sealed D.RAW observation merely because it is displayed;
- creates no CFA/sensor evidence;
- may not write Scientific Master state;
- may not grant `MEASURED`, `CALIBRATED_ESTIMATE` or `RECONSTRUCTED` authority;
- remains a presentation object unless separately admitted through another evidence contract.

RAW/DNG continues through the existing D.RAW workbench / Universal Intake route and is never locally decoded by Free Raster.

## 3. Android URI / raster loading repair

The real-device Workspace round exposed a product defect while opening an ordinary raster:

`Presentatie-raster kon niet worden gelezen: Geen leesbare inputstream`

The v0.2 loader must be provider-safe rather than path-based.

Required behavior:

1. use the Android Storage Access Framework URI as authority;
2. attempt `ContentResolver.openFileDescriptor(uri, "r")` and decode from its file descriptor;
3. fall back to a freshly opened `ContentResolver.openInputStream(uri)`;
4. reopen the provider separately for bounds and real decode — never assume one stream is reusable;
5. best-effort retain `FLAG_GRANT_READ_URI_PERMISSION` using `takePersistableUriPermission` when the provider supports it;
6. tolerate providers that only grant temporary read access;
7. validate known MIME types for JPEG/PNG/WebP where the provider supplies them;
8. report permission, provider, decode and memory failures separately;
9. create only a sampled display raster, currently bounded to approximately 2048 px on the long side, and label it presentation-only;
10. do not add RAW parsing or scientific inference to this loader.

EXIF/orientation handling may be added only through an already-admitted safe Android/image dependency or a separately reviewed deterministic parser. It must not silently become a second image-processing pipeline.

## 4. One shared output cable

The product cable is:

1. **Observation / Source** — sealed provenance and source identity.
2. **Scientific Core / Scientific Master** — measurement, calibration/reconstruction state, uncertainty and authority.
3. **Unified Output State** — shared downstream rendered/output state.
4. **Route View** — PURE / ADVANCED / PRO operate on the same upstream scientific observation.
5. **Free Raster / Export** — consume the shared output state.

There is one scientific core. Route selection is not evidence selection.

### PURE

Scientific-first view with minimal downstream appearance. PURE may expose evidence/authority and neutral display mapping, but being PURE does not create stronger evidence than the underlying Scientific Master.

### ADVANCED

Photographic appearance/restoration view. Adjustments are downstream only and may not write Scientific Master values.

### PRO

Professional/research/output view using the same source, Scientific Master and output cable. Diagnostics and additional controls do not grant stronger evidence.

## 5. Black, white, highlights and colour — appearance semantics

The next appearance refinement must preserve the multidisciplinary distinctions already bound by the D.RAW scientific foundation.

### Black

`display black point != sensor BlackLevel != noise floor != Zero-Line != scene radiance zero`

A photographic black-point adjustment is `APPEARANCE_ONLY`. It may improve perceptual anchoring but may not relabel or rewrite scientific black/zero references.

### White

`display white point != diffuse reference white != adapted white != sensor WhiteLevel / saturation`

A photographic white-point adjustment is `APPEARANCE_ONLY` and may not redefine sensor clipping or TruthRange ceiling.

### Highlights

Use a downstream highlight knee / roll-off / compression control rather than hard clipping. The goal is to retain visible texture in bright fur, skin, clouds and other near-white detail while keeping sensor censoring/clipping semantics unchanged.

### Shadows and midtones

Shadow lift and midtone contrast are display/appearance operators. They must not turn unknown or censored values into measured values.

### Colour

Keep separate concepts for:

- warmth / colour temperature appearance;
- tint;
- saturation;
- vibrance / colourfulness;
- neutral protection.

White balance remains distinct from spectral calibration. Three-channel RGB does not prove the complete spectrum.

### Texture / detail

Allow subtle local texture/detail or edge-aware appearance sharpening without halo-driven authority claims. Optical resolution remains determined by admitted optical support, not by display sharpening or output raster density.

## 6. Reference appearance direction from the supplied black/white dog image

The supplied JPEG is a useful **appearance reference**, not scientific calibration evidence. For that class of image, the desired downstream behavior is:

- black fur may be visually deeper without crushing low-level texture;
- white fur must retain highlight texture instead of flattening/clipping;
- highlight compression should be modest and smooth;
- midtones should separate fur strands without excessive local-contrast halos;
- colour may be slightly warmer/more colourful where the current export is dull/cool, while neutral fur remains protected;
- sharpening/detail should remain subtle.

No numeric calibration constants may be learned from this single JPEG.

## 7. UI wording and hierarchy

Prefer the downstream section name:

**`Output / Vrije Raster`**

When no internal output exists, communicate that directly:

- `Geen D.RAW-output-raster beschikbaar`
- `Open externe JPG / PNG / WebP`
- `Ga naar D.RAW werkbank`
- `Projectie / output uitvoeren`

For external raster input show:

`EXTERNAL_PRESENTATION_RASTER / PRESENTATION_ONLY`

`Preview 1:1` remains the correct wording for one display pixel per decoded preview pixel. It must never imply one display pixel equals one sensor or Scientific-Master sample.

## 8. v0.2 implementation order

1. harden external JPG/PNG/WebP URI loading;
2. move Free Raster downstream in Workspace;
3. preserve RAW/DNG redirect to the existing workbench;
4. bind internal Free Raster consumption to the existing Unified Output state only after the exact existing Ready/result object is traced;
5. extend the existing ADVANCED appearance cable rather than creating duplicate controls/rendering;
6. expose black point, white point and highlight roll-off with explicit `APPEARANCE_ONLY` semantics;
7. separate colourfulness/vibrance from raw saturation where the existing renderer allows it;
8. retain one shared PURE/ADVANCED/PRO source/scientific/output cable;
9. build and verify an APK on the exact runtime head;
10. run a real-device follow-up with a normal JPEG plus a D.RAW observation.

## 9. v0.2 acceptance criteria

The round is complete only when all applicable items are proven:

- ordinary JPG/PNG/WebP can be opened through Android document-provider URIs without the prior inputstream failure;
- Free Raster is presented downstream under Output/Projection rather than as a primary scientific ingress;
- external raster state is visibly `PRESENTATION_ONLY`;
- RAW/DNG still goes through Universal Intake / existing D.RAW workbench;
- no second RAW decoder or Scientific Master exists;
- internal Free Raster output consumes the existing Unified Output cable where attached;
- PURE, ADVANCED and PRO share the same upstream observation/scientific core;
- black/white/highlight/colour controls remain appearance-only;
- no candidate promotion, source mutation or scientific writeback is introduced;
- Android compile/verify is green on the exact runtime SHA;
- final APK identity records exact SHA, artifact ID, bytes, SHA-256 and signing certificate;
- real-device screenshot/output follow-up is recorded without converting product acceptance into scientific promotion.

## 10. Permanent boundary

Nothing in this v0.2 product refinement changes the evidence law:

`MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`

**One Free World. Many sealed observations. One evidence law.**
