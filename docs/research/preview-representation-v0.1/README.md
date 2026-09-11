# TruthRaw Preview Representation v0.1

Status: RESEARCH DESIGN CANDIDATE — 2026-09-11

Base: `research/adaptive-ui-tile-preview-v0.2-2026-09-11` @ `ca5837e86dd75689227ff24db5c397272767b80f`

## Decision

TruthRaw MUST NOT choose one encoded image file format as the universal preview representation.

Preview is split into three roles with separate contracts:

1. **Runtime UI surface** — bounded decoded pixels used only inside the TruthRaw UI.
2. **Portable compatibility preview** — an encoded preview that ordinary Android/gallery/chat/file viewers can display reliably.
3. **Optional HDR preview** — a richer display-only derivative for capable Android devices.

The source RAW format and the requested export format MUST NOT dictate the runtime preview representation.

## v0.1 baseline

### A. Runtime UI surface — canonical baseline

- Pixel representation: `Bitmap.Config.ARGB_8888`.
- Color space: sRGB for the universal SDR path.
- No intermediate JPEG/PNG/WebP/HEIF/AVIF encode/decode step is required for the live UI.
- Preview dimensions remain bounded by the UI/runtime policy; the existing v0.2 path uses a 384 px UI cap and a 512 px native hard cap.
- The runtime preview is transient appearance data, never evidence and never a Scientific Scene Master.

Rationale: the current Android v0.2 path already transfers bounded native preview pixels directly into an `ARGB_8888` Bitmap. Encoding those pixels merely to decode them back into the same UI would add codec CPU, latency and temporary memory without adding scientific information.

### B. Portable compatibility preview — canonical default

Use **baseline JPEG, 8-bit sRGB** as the default externally visible preview artifact.

Normative v0.1 policy:

- MIME: `image/jpeg`.
- Color: explicitly rendered to sRGB.
- Orientation: pixels normalized to display orientation; metadata may additionally describe orientation but viewers must not need it for correct display.
- Alpha: not supported; previews are opaque photographic imagery.
- Suggested display-preview maximum edge: 2048 px. This is a presentation/export policy, not a scientific resolution limit.
- Suggested quality target: 92, subject to later visual/device validation. Quality is not a scientific parameter.
- JPEG bytes MUST NEVER be used for numeric validation, source evidence identity, CFA measurement reconstruction, scientific-master hashing, calibration, uncertainty calculation or promotion claims.

Why JPEG is the compatibility default:

- Android requires JPEG image support broadly and its platform image APIs directly support JPEG.
- It has the lowest interoperability risk across Android gallery/file/chat/browser surfaces.
- It is cheap to decode and appropriate for photographic previews.
- Unlike AVIF/HEIF, it does not make the basic preview dependent on newer codec/container support.
- Unlike PNG, it does not pay lossless photographic file-size cost when pixel-exact diagnostics are not required.

This default directly addresses the failure mode where a scientifically valid DNG/LinearRaw or special-format result exists but the user cannot see a convenient phone preview.

### C. Optional HDR preview — enhancement, never baseline

For capable Android devices, TruthRaw MAY additionally emit/display **Ultra HDR JPEG** as a display derivative.

Rules:

- The normal sRGB JPEG preview remains available as the compatibility baseline.
- Ultra HDR gain-map data is appearance/display data and MUST NOT become scene evidence.
- An Ultra HDR display gain map MUST NOT be confused with or substituted for a DNG/optical lens-shading GainMap or any scientific gain field.
- HDR display availability MUST NOT change Scientific Master bytes or truth authority.
- On unsupported SDR surfaces, the compatibility base image remains usable.

Android 14 introduced Ultra HDR JPEG support. Android 16 additionally supports Ultra HDR in HEIC, while AVIF Ultra HDR support is still described by Android documentation as future/work-in-progress. Therefore HEIC Ultra HDR and AVIF Ultra HDR are not the universal v0.1 preview contract.

## Diagnostic preview

Use **PNG** only where lossless preview pixels are materially useful, for example UI regression fixtures, pixel-exact appearance diagnostics, masks or screenshots with synthetic overlays.

PNG is not the default photographic preview because its lossless compression generally trades larger files for precision that the human-facing compatibility preview does not need.

A PNG diagnostic is still appearance/diagnostic data. It does not become measurement evidence merely because the encoding is lossless.

## Formats intentionally not chosen as the universal baseline

| Format | v0.1 role | Why not the single universal default |
|---|---|---|
| JPEG | **Portable compatibility default** | Lossy, so not for numeric/scientific validation. |
| PNG | Diagnostic/lossless preview | Larger photographic payload; no HDR gain-map baseline. |
| WebP | Optional future transport | Good compression, but no decisive project benefit over the far more conservative JPEG compatibility baseline. |
| HEIF/HEIC | Optional export/HDR path | Container/codec availability is less universal; Android 16 HEIC Ultra HDR is useful but newer. |
| AVIF | Optional export/future preview | Efficient and supported on modern Android, but not the safest compatibility artifact across all external viewers; Ultra HDR AVIF is not yet the Android baseline. |
| JPEG XL | Research only | Not part of the required Android v0.1 compatibility surface; do not make basic preview visibility depend on it. |
| DNG/LinearRaw | Scientific/RAW result, not human preview | A RAW container is not a reliable phone/chat preview surface. |
| TIFF | Export/research only | Too heavy/variable for a universal mobile preview path. |

## Preview stage taxonomy

A preview MUST identify which upstream state it visualizes:

- `SOURCE_PROXY` — quick source/CFA diagnostic; may be grayscale and explicitly non-color-authoritative.
- `SCIENTIFIC_SCENE_PREVIEW` — display transform of a valid Scientific Scene Master; never the master itself.
- `APPEARANCE_PREVIEW` — Natural/Detailed/Soft/etc. display result.
- `COUNTERFACTUAL_PREVIEW` — clearly labeled hypothetical illumination/capture world.
- `EXPORT_PREVIEW` — downsampled representation of a selected output policy.

A preview stage label may never silently upgrade a source proxy into a Scientific Scene Master claim.

## One input format-independent path

Conceptual contract:

```text
DNG / CR3 / NEF / ARW / RAF / IIQ / other admitted RAW
        |
        v
validated source / Gatehouse handoff
        |
        v
scientific or diagnostic stage selected
        |
        v
Preview Surface (bounded pixels)
        |                    |
        |                    +--> optional HDR display derivative
        v
runtime ARGB_8888/sRGB
        |
        +--> optional portable JPEG/sRGB artifact
```

The same preview policy applies to every admitted input family. Adding another RAW decoder MUST NOT require another UI preview format.

Likewise, selecting DNG, JPEG, HEIF, AVIF, TIFF or another final export MUST NOT require TruthRaw to rerun scientific reconstruction merely to create its UI preview.

## Resource contract

- UI preview pixels remain bounded independently of source megapixel count.
- No UI-owned full RAW payload.
- No full-resolution decoded image is allocated solely to create a screen-sized preview when tile/streaming access can produce the requested preview.
- Low-end and flagship devices may differ in preview edge, cache retention or scheduling, but not in scientific authority.
- Portable preview encoding occurs after the preview pixels exist; it is not a prerequisite for live rendering.

## Color contract

The universal SDR preview is explicitly sRGB, because implicit/untagged color is unacceptable for TruthRaw.

Future wide-gamut/HDR UI may use a separate representation such as `RGBA_F16` with an explicit RGB ColorSpace on capable surfaces. That is an enhancement path, not the v0.1 compatibility baseline. Android's `ImageDecoder` can target explicit RGB color spaces and associates `ARGB_8888` naturally with sRGB while `RGBA_F16` can use extended sRGB or another requested RGB space.

Color display transforms belong to preview/appearance. They MUST NOT alter the scene-referred Scientific Master.

## Current v0.2 relationship

The existing Adaptive UI Tile Preview v0.2 remains scientifically conservative:

- it displays a grayscale CFA source proxy;
- the Kotlin side creates an `ARGB_8888` Bitmap directly from the bounded JNI packet;
- it has no demosaic, white balance or scientific color-matrix authority;
- it is capped at 384 px in the UI and 512 px natively;
- it remains a diagnostic `SOURCE_PROXY`, not the final TruthRaw color preview.

Preview Representation v0.1 does not relabel that proxy. It defines the representation contract into which a later reconstructed/color-managed preview can be inserted.

## Required next proof

Before promotion beyond research candidate:

1. Produce a real reconstructed color preview from a valid TruthRaw scientific/appearance stage.
2. Render it directly as bounded `ARGB_8888` sRGB in the Android UI.
3. Encode the same bounded preview as baseline JPEG/sRGB and prove it displays on the target Honor device and in the intended external share/view surfaces.
4. Compare the runtime Bitmap and decoded JPEG using a presentation-tolerance metric; do not demand pixel equality from lossy JPEG.
5. Confirm no scientific/master/provenance bytes change when the preview codec changes.
6. Measure preview peak RSS, latency and thermal behavior on the physical target device.
7. Add optional Ultra HDR JPEG only after the SDR path is proven and keep the SDR base/fallback available.

## External platform references reviewed 2026-09-11

- Android `ImageDecoder`: JPEG/PNG/WebP/GIF/HEIF decode to Bitmap/Drawable; explicit target RGB color spaces are supported.
  https://developer.android.com/reference/android/graphics/ImageDecoder
- Android bitmap memory guidance: `ARGB_8888` is 4 bytes/pixel; `RGBA_F16` is 8 bytes/pixel.
  https://developer.android.com/topic/performance/memory/guide/bitmaps
- Android 12: AVIF image support introduced.
  https://developer.android.com/about/versions/12/features
- Android 14: Ultra HDR JPEG introduced with SDR-compatible base image and gain map.
  https://developer.android.com/about/versions/14/features
- Android Ultra HDR display/edit documentation.
  https://developer.android.com/media/grow/ultra-hdr/display
  https://developer.android.com/media/grow/ultra-hdr/edit
- Android 16: HEIC Ultra HDR support added; AVIF Ultra HDR described as future work.
  https://developer.android.com/about/versions/16/features

## Non-claim

This document chooses preview/display representations. It does **not** prove production JPEG/HEIF/AVIF encoding quality, physical-device performance, a final color transform, HDR correctness, DNG embedded-preview compatibility, or broad third-party viewer behavior. Those require their own tests.