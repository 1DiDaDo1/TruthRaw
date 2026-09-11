# Preview Representation v0.1 — implementation next

The first implementation target is a bounded `IStreamingSink` consuming the existing v4.7i/full-frame-streaming SDR tiles.

The sink must:

- consume post-reconstruction/post-appearance **linear sRGB** tile values from `StreamingTruthRawProcessor`;
- apply the sRGB OETF only for the display surface;
- normalize orientation in the preview surface;
- allocate memory proportional to requested preview dimensions, never source megapixel count;
- store only an opaque 8-bit ARGB preview plus one-byte write ownership map;
- accept and discard the separate half-log HDR gain stream in SDR v0.1 rather than confusing it with display pixels;
- never become evidence or Scientific Master storage;
- preserve `physicalFrameCount=1` and `independentEvidenceCount=1`;
- permit changing preview edge without changing the processor's scientific exposure/provenance result.

Android JPEG encoding is downstream of this surface. The preferred Android encoder path is `Bitmap.compress(Bitmap.CompressFormat.JPEG, quality, OutputStream)` so the encoded JPEG can stream to a caller-owned destination instead of requiring a second full JPEG byte array in the UI heap.

A real-file Android route remains blocked until the selected source has a real source-evidence binding and accepted camera/lens color binding. The existing preview-only sentinel binding MUST NOT be reused for reconstructed scientific color.