# Linear DNG Compatibility Projection v0.1

Status: **CANDIDATE — CI VALIDATION PENDING**

## Purpose

Provide a standards-oriented DNG `LinearRaw` compatibility projection downstream of the already-finalized TruthRaw scientific lineage.

This module does **not** create new sensor measurements and does **not** replace the Scientific Master.

## Representation boundary

Input representation:

- the same camera-native reconstructed RGB produced by the reconstruction backend that underlies the Scientific Master;
- before `camera_to_xyz`, XYZ→sRGB, appearance, tone mapping or JPEG encoding.

Output representation:

- classic-TIFF DNG;
- `PhotometricInterpretation = LinearRaw`;
- three interleaved unsigned 16-bit camera-native channels;
- tiled storage;
- source-bound D50 color metadata derived from the exact finalized color binding;
- compatibility-range quantization to `[0,65535]`.

Samples below zero or above one are clipped **only in the compatibility projection** and are counted in the export result. Their existence does not alter or shrink the Scientific Master domain.

## Authority rules

The writer requires:

- a validated normalized scientific color binding;
- authority of `SourceMetadataBound`, `GatehouseCertifiedMetadata`, or `IndependentCalibration`;
- exact equality between the active TileNative source matrix and that binding;
- one physical frame and one independent evidence root.

`SourceMetadataBound` remains source-bound. Export never promotes it to independent or `FULL_PHYSICAL` color authority.

The Android bridge additionally re-runs the complete Finalized Scientific Preview v0.2 gate for the same sealed source before writing any DNG bytes and re-verifies the source SHA-256 after export.

## Resource contract

- tile-based Stage-2 + reconstruction;
- no full-frame reconstructed RGB allocation;
- no appearance buffer for the full frame;
- explicit caller memory budget;
- classic-TIFF <4 GiB bound;
- destination is written through a caller-provided seekable file descriptor.

## DNG tags in v0.1

The candidate writer currently emits the core TIFF/DNG tags needed for its tiled LinearRaw representation, including dimensions, 16-bit sample layout, `LinearRaw`, tile offsets/counts, DNG version, black/white level, active/crop area, source-bound `ColorMatrix1`, `ForwardMatrix1`, `AsShotNeutral`, and D50 calibration illuminant.

The camera model string is deliberately synthetic (`TruthRaw Reconstructed Linear v0.1`) so the output cannot be mistaken for the original measured camera CFA.

## Not claimed

Until CI and a real consumer test are complete, this module does **not** claim:

- Lightroom acceptance for every generated file;
- measured sensor-CFA status;
- reconstructed-CFA equivalence;
- preservation of arbitrary negative or >1 Scientific Master values inside the bounded 16-bit DNG projection;
- independent physical camera/lens calibration;
- universal DNG reader compatibility.

The canonical rule remains:

**Measured where measured. Reconstructed where necessary. Never invented.**
