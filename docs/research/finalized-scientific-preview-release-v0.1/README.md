# Finalized Scientific Preview Release v0.1

Status: **RESEARCH IMPLEMENTATION CANDIDATE — CI PROOF REQUIRED**

Parent validated streaming Scientific Master head:
`5af2d3474539dc3911d8e3420c08b3e17149b987`

## Purpose

This module connects the already-designed bounded color preview to the now-validated Scientific Master / TruthRange / Technical Backplane phase-2 lineage.

It does **not** redesign color, appearance, preview sampling, or Android output.

The release path is:

`prepared exact source + source-bound/independent color binding`
`-> serialized 180-byte Technical Backplane candidate`
`-> CRC/format validation`
`-> opened IRawTileSource source/color identity check`
`-> recompute bounded Scientific Master + exact TruthRange self-gauge from same source`
`-> deterministic Technical Backplane phase-2 rebuild`
`-> require byte-for-byte equality with supplied Backplane`
`-> existing phase-2 Scientific Preview admission`
`-> existing Full-Frame Streaming processor`
`-> existing BoundedSrgbPreviewSink`
`-> finalized bounded sRGB Scientific Preview surface`

The preview remains presentation/appearance. It does not become the Scientific Master and does not create evidence.

## Why the release gate recomputes scientific identity

A CRC-valid Backplane alone is not sufficient authority. CRC proves record integrity, not that a caller supplied the true Scientific Master/zero-line/scene-scale hashes for the currently opened source.

Therefore this module independently recomputes, from the same `IRawTileSource` and reconstruction backend:
- Scientific Master SHA-256;
- TruthRange v0.2 self-gauge `L0`;
- zero-line identity;
- scene-scale identity.

It then runs Technical Backplane Phase 2 again using the room/claim statuses from the supplied record. The resulting canonical 180 bytes must be **exactly identical** to the supplied Backplane.

A caller cannot open the Scientific Preview gate merely by constructing non-zero hashes and recalculating CRC.

## Source and color binding

Before scientific work begins, the opened source must satisfy:
- `metadata.sourceId == prepared.source.sourceEvidenceId`;
- `metadata.cameraToXyzD50 == prepared.color.cameraToXyzD50`;
- non-empty prepared color binding ID.

After phase-2 rebuild, those identities are checked again against the actual finalized admission.

This blocks accidental reuse of a Backplane from another photo or another color binding.

## Preview authority labels

The release result is explicitly one of:
- `FINALIZED_SOURCE_BOUND_SCIENTIFIC_PREVIEW`;
- `FINALIZED_INDEPENDENTLY_CALIBRATED_SCIENTIFIC_PREVIEW`.

For current DNG metadata color on Android, the expected role is the first one.

`SOURCE_METADATA_BOUND` color remains source-bound. Finalizing the lineage does not convert it into independent physical camera/lens calibration or `FULL_PHYSICAL` color truth.

## Existing preview path reused unchanged

After authorization, the module uses the existing:
- `StreamingTruthRawProcessor`;
- v4.7i reconstruction backend;
- selected appearance backend;
- camera RGB -> XYZ D50 -> linear sRGB conversion already present in Full-Frame Streaming;
- `BoundedSrgbPreviewSink`;
- standard sRGB OETF and ARGB8888 bounded presentation surface.

No preview pixels participate in the Scientific Master digest or TruthRange gauge.

## Fail-closed postconditions

Even after admission, the produced preview is rejected if streaming reports:
- frame/evidence counts other than 1/1;
- appearance modifying Scientific Master;
- counterfactual observation creation;
- GainMap not applied exactly once;
- adapter-owned full RAW/SDR/half-gain/diagnostic frames;
- incomplete bounded preview pixel ownership.

## Validation targets

The CI candidate must prove:
- valid phase-2 lineage opens a non-empty non-gray bounded sRGB preview;
- the recomputed Scientific Master equals the Backplane master identity;
- source-bound color authority remains source-bound;
- a valid-CRC Backplane with a fabricated Scientific Master hash is rejected **before preview pixels are allocated**;
- a wrong source identity is rejected before preview;
- a wrong color matrix is rejected before preview;
- a corrupted serialized Backplane is rejected before preview;
- physical frame/evidence counts remain 1/1;
- GCC Release, Clang Release and Clang ASan/UBSan all pass.

## Android implication

Once this host releasegate is validated, the current Android `SOURCE_BOUND_APPEARANCE_PREVIEW` path can be upgraded without redesigning its Bitmap/JPEG presentation layer:

`PFD exact source seal`
`-> source-bound DNG color binding`
`-> TileNativeDngSource`
`-> bounded Scientific Master/TruthRange identity`
`-> Technical Backplane phase 2`
`-> this release gate`
`-> existing BoundedSrgbPreviewSink`
`-> Android ARGB_8888 sRGB Bitmap`
`-> optional presentation-only JPEG`.

That Android integration will still require a new arm64 APK CI proof and then physical Honor/MotionCam execution.

## Non-claims

This module does not yet prove:
- physical Honor/MotionCam execution;
- real selected user DNG completion of the full finalized preview path;
- on-device RSS/thermal/latency/frame time;
- independent camera/lens color calibration;
- dual-illuminant interpolation;
- FULL_PHYSICAL color truth;
- Lightroom-readable TruthRaw compatibility DNG.

**Measured where measured. Reconstructed where necessary. Never invented.**
