# TruthRaw ↔ Adobe Lightroom mobile HDR TIFF audit v1.0

**Status:** RESEARCH — NOT MAIN-PROMOTED  
**Source:** `IMG_BNC_TRUTHRAW20260907_094449_565.dng`  
**Observed TIFF:** `IMG_BNC_TRUTHRAW20260907_094449_565 (1).tif`

## Why this audit exists

Adobe documents TIFF as an HDR-capable Lightroom export format and recommends TIFF/PSD for workflows requiring further HDR editing. The actual Android export was therefore inspected byte-for-byte rather than assuming that every TIFF carries the same HDR transport semantics as the already observed AVIF/PQ export.

The TruthRaw rule remains:

> Representation may exceed the source. Knowledge claims may not exceed the evidence.

An Adobe rendering can be a valid HDR presentation without becoming new sensor evidence.

## Real file observations

The uploaded TIFF is:

- SHA-256: `03da35eb8c708eac09de55850e868a728378501ded5e6a91db0dbae7f74f8af9`
- 74,559,418 bytes
- 4064×3056 RGB
- 16 bits/channel, unsigned integer storage
- uncompressed TIFF
- ICC profile description: `Adobe RGB (1998)`
- software: `Adobe Lightroom 11.5.22 (Android)`
- embedded XMP: 19,878 bytes

The XMP proves that Lightroom exported the same tone-expanded HDR edit used in the bright AVIF comparison:

- `HDREditMode=1`
- `HDRMaxValue=+8.00`
- `Exposure2012=0.00`
- `Whites2012=+2`
- `ToneCurveName2012=Custom`
- extended HDR tone curve: `(0,12) -> (152,228) -> (386,500)`

So this is not an accidental SDR export with no HDR edit state.

## Important transport finding

A generic TIFF reader sees a conventional 16-bit unsigned RGB raster with an `Adobe RGB (1998)` ICC profile. The observed TIFF tag set does not independently expose the same explicit ST-2084/PQ transport that the companion AVIF exposes through its container/video color metadata.

Therefore TruthRaw does **not** classify the raw TIFF pixel codes themselves as PQ merely because Adobe calls the export HDR-capable.

Current classification:

`ADOBE_ECOSYSTEM_HDR_INTERCHANGE`

not yet:

`SELF_DESCRIBING_HDR_TRANSPORT`

This does not contradict Adobe's HDR workflow claim. It means that the exact mobile TIFF needs an Adobe re-import or a second independent HDR-aware decoder round-trip before TruthRaw treats its pixel transport as independently self-describing.

## Code-ceiling observation

In the standard 16-bit raster decode, at least one RGB channel reaches code 65535 in about **0.49269%** of pixels.

Per-channel code-ceiling fractions are approximately:

- R: 0.21820%
- G: 0.27921%
- B: 0.45762%

The matching tone-expanded AVIF, decoded as 10-bit Rec.709 PQ, does not hit its 10-bit code ceiling; its maximum decoded channel code is 958/1023.

This is a strong reason not to assume that a conventional generic TIFF decode is numerically equivalent to Adobe's explicit AVIF PQ transport. It does **not** by itself prove that Lightroom has lost all HDR information, because Adobe may use its own HDR/XMP interpretation when reopening the TIFF. That must be tested directly.

## Scientific authority boundary

This TIFF is a presentation/editing artifact. It cannot:

- create new measured sensor dynamic range;
- convert `CENSORED` source samples into exact radiance;
- convert `UNKNOWN` regions into evidence;
- write Adobe tone mapping back into the Scientific Master;
- upgrade Dynamic Authority Field classes.

Its HDR XMP state is useful interoperability metadata only.

## Adobe documentation context

Adobe's current mobile documentation states that TIFF exports preserve HDR data by default when the source was edited in HDR mode. Adobe also recommends TIFF/PSD for additional HDR work. On desktop, Adobe documents HDR color spaces and 32-bit HDR TIFF support. A November 2025 Adobe employee explanation additionally states that 10/16-bit HDR TIFF can use PQ while 32-bit floating TIFF uses linear encoding.

The empirical Android file inspected here is more specific than those generic rules: it carries `Adobe RGB (1998)` ICC plus HDR XMP state, so the exact mobile transport must be validated rather than inferred.

## Next gate

1. Re-import this exact TIFF into Lightroom mobile.
2. Confirm whether `Bewerken in HDR-modus` remains enabled without manually turning it on.
3. Record the resulting HDR histogram and HDR Limit.
4. Export the re-imported TIFF again as AVIF HDR without changing tone controls.
5. Compare that second-generation AVIF against the first-generation bright AVIF.
6. If the HDR histogram and PQ output survive within a defined tolerance, promote the TIFF route to independently validated Adobe HDR interchange.
7. Keep AVIF/JXL as the preferred standards-explicit public HDR exchange until that round-trip is closed.
