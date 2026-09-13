# Modern finalized Scientific Preview architecture — restoration source

Date: 2026-09-13

Status: **AUTHORITATIVE BRANCH-LOCAL RESTORATION SOURCE FOR PREVIEW/REPRESENTATION SEPARATION**

This note records the later TruthRaw preview design that supersedes the idea that one historical DNG IFD layout is itself the preview architecture.

The exact old chat phase label remembered as approximately `1.3` / `1.4` has not yet been recovered with enough confidence to assign it here. The architecture itself is present in the repository and is therefore recorded by its actual module names rather than by an uncertain phase number.

## 1. Leading architecture

The leading preview route is:

```text
sealed RAW/DNG evidence
        -> exact source binding
        -> source-bound DNG color producer
        -> Scientific Master reconstruction
        -> exact Scientific Master digest
        -> TruthRange self-gauge / zero-line binding
        -> Technical Backplane phase 2
        -> reconstructed/color/appearance streaming
        -> Finalized Scientific Preview
        -> bounded preview surface
             |-> runtime ARGB_8888 / sRGB
             |-> portable baseline JPEG / sRGB
             |-> optional HDR display derivative
             |-> embedded DNG JPEG compatibility preview
```

The DNG container is downstream of this preview architecture. It does not define the Scientific Preview.

## 2. Finalized Scientific Preview authority

`finalized-scientific-preview-release-v0.2` requires the release path to recompute and bind the real Scientific Master identity and exact TruthRange self-gauge before releasing preview pixels.

The release remains tied to:

- exact sealed source identity;
- source-bound color identity;
- Scientific Master SHA-256;
- TruthRange zero-line / gauge identity;
- scene-scale identity;
- Technical Backplane phase 2;
- `physicalFrameCount=1`;
- `independentEvidenceCount=1`;
- preview authority.

Changing preview representation may not change any of these identities.

## 3. Preview Representation v0.1

The 2026-09-11 Preview Representation design explicitly rejects one encoded image format as the universal preview representation.

It defines three distinct representation roles:

1. **Runtime UI surface**
   - bounded decoded pixels;
   - `Bitmap.Config.ARGB_8888`;
   - explicit sRGB SDR baseline;
   - live UI requires no encode/decode round trip.

2. **Portable compatibility preview**
   - baseline JPEG;
   - 8-bit sRGB;
   - display orientation normalized in pixels;
   - suggested maximum long edge `<=2048 px`;
   - suggested quality `92`;
   - presentation/export only.

3. **Optional HDR preview**
   - separate display derivative;
   - never replaces the SDR compatibility baseline by hardware capability alone;
   - display gain maps are never scientific/DNG lens gain maps.

The source RAW format and final export format must not dictate the live preview representation.

## 4. DNG embedded preview policy

`preview-representation-v0.1/DNG_EMBEDDED_PREVIEW_NOTE.md` defines the correct relationship:

```text
one scientific/raw result
        |
        +--> DNG/raw payload
        |
        +--> embedded JPEG preview (compatibility only)
        |
        +--> standalone JPEG preview (visibility/share only)
```

Therefore an embedded DNG preview should use the **same sRGB JPEG compatibility representation** as the portable preview path.

The embedded JPEG must never become:

- source evidence;
- calibration evidence;
- Scientific Master storage;
- CFA measurement;
- uncertainty input;
- numeric validation reference;
- authority/promotion evidence.

Removing, replacing or re-encoding the JPEG preview must leave the RGB LinearRaw payload and its evidence/scientific identity unchanged.

## 5. Android JPEG path already designed

The 2026-09-11 Android JPEG note defines the preferred Android route:

```text
bounded ARGB_8888 / sRGB Bitmap
        -> Bitmap.compress(JPEG, quality, caller-owned OutputStream)
        -> standalone preview.jpg / embedded compatibility preview
```

The design deliberately avoids JPEG encoding merely to display the live UI.

It also deliberately prefers a caller-owned `OutputStream` rather than collecting a second complete JPEG `ByteArray` in the UI heap when streaming is possible.

## 6. Current branch implementation already follows most of the modern design

The current Android route already calls:

`NativeTilePreviewBridge.buildFinalizedScientificColorPreview(...)`

That JNI path performs:

- source SHA-256 seal;
- DNG color producer v0.2;
- prepared source-bound preview admission;
- source re-verification;
- TileNative DNG source;
- Scientific Master Streaming Binding v0.2;
- Technical Backplane phase 2;
- finalized Scientific Preview release;
- `BoundedSrgbPreviewSink`;
- post-source re-verification.

`TilePreviewLoader` then converts the returned finalized ARGB values to an explicit sRGB `Bitmap` using `PortablePreviewEncoder.createSrgbBitmap(...)`.

The UI labels this state as `Finalized Scientific Preview` and only enables JPEG / Linear DNG save actions after the ready state exists.

`PortablePreviewEncoder.encodeJpeg(...)` already enforces:

- `ARGB_8888` source bitmap;
- explicit sRGB color space;
- baseline JPEG;
- default quality `92`;
- caller-owned `OutputStream`.

Thus the modern preview architecture was not lost entirely. The missing integration is principally that the DNG writer does not yet carry the portable JPEG representation generated from that finalized preview.

## 7. Current limitation: UI edge is not the portable-preview target

The current Android UI path uses:

- Kotlin UI cap: `MAX_PREVIEW_EDGE = 384`;
- native hard cap in `source_bound_color_preview_bridge.cpp`: `512`.

That is appropriate for a low-memory live UI preview but smaller than the Preview Representation policy's suggested portable-preview limit of `<=2048 px`.

Do not automatically treat the current 384 px UI bitmap as the final DNG compatibility preview just because it is scientifically finalized.

The next representation step must preserve the same finalized-preview authority while selecting a bounded portable-preview size under an explicit memory/runtime policy.

## 8. Relationship to the old v0.7 multi-IFD fix

The historical v0.7 corrected layout:

- IFD0 reduced thumbnail;
- full RGB LinearRaw SubIFD;
- JPEG preview SubIFD;

remains valuable **interoperability evidence**. It proved that a JPEG DNG preview could coexist with an unchanged full LinearRaw payload and pass the exact historical DNG validator gates.

It is not the final TruthRaw preview architecture.

Its role is now:

- evidence about practical TIFF/DNG packaging and Android preview discovery;
- a regression reference for keeping raw bytes independent from preview bytes;
- one candidate container arrangement to compare against the modern representation policy.

The modern design determines what preview pixels/authority mean. The container only transports that representation.

## 9. Required restoration order

1. Keep the current Scientific Master / finalized release path unchanged.
2. Keep v0.2 RGB LinearRaw finite-headroom repair unchanged.
3. Define a portable finalized preview surface derived from the **same finalized preview pipeline**, with bounded memory independent of source megapixels.
4. Encode that surface to baseline JPEG/sRGB quality 92 using the established Android representation contract.
5. Allow the same JPEG representation to be written both standalone and embedded in DNG.
6. Ensure DNG embedding does not rerun or redefine scientific reconstruction merely for UI/preview reasons.
7. Prove preview codec/container changes do not change Scientific Master hash, TruthRange gauge, Backplane, RGB LinearRaw payload identity, frame/evidence counts or color authority.
8. Test the exact resulting DNG on Honor/Android preview discovery, Lightroom/ACR, LibRaw and DNG SDK.

## 10. Promotion boundary

No claim is currently made that the exact best DNG IFD topology for the modern preview has been revalidated on the new v0.2 RGB writer.

Do not promote a particular historical IFD arrangement merely because it worked before. Reuse its proven lessons, then validate the exact modern container bytes.

The phase label `1.3` / `1.4` remains unassigned until an exact historical source explicitly establishes it. Do not invent that numbering.
