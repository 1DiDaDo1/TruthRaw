# Android JPEG preview path

TruthRaw's portable JPEG preview is downstream of the bounded runtime preview surface.

Preferred Android flow:

```text
bounded ARGB_8888 / sRGB Bitmap
        |
        v
Bitmap.compress(JPEG, quality, caller-owned OutputStream)
        |
        v
standalone preview.jpg / embedded compatibility preview
```

Rules:

- Do not encode JPEG merely to display the live TruthRaw UI; the UI already owns bounded pixels.
- Do not collect the complete JPEG into a second UI-owned `ByteArray` when a destination `OutputStream` is available.
- JPEG encoding may not mutate source evidence, Scientific Master, zero-line, scene state, uncertainty, or reconstruction settings.
- JPEG is appearance/export transport only.
- Initial quality target is 92; physical-device visual/size/latency validation remains required.
- The first reconstructed-color implementation must not reuse the v0.2 preview-only source/color sentinel. Real source-evidence and accepted color binding are prerequisites for real-file scientific color.