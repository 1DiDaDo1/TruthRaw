# TruthRaw Linear DNG Projection v0.1

Status: **IMPLEMENTED RESEARCH CANDIDATE — AWAITING EXECUTED HOST/ANDROID CI**

The first GitHub Actions attempts for this module and the Android integration ended before runner allocation (`runner_id=0`, zero executed steps). Fresh attempts on 2026-09-13, including host run `34770189394` and Android run `34770189340`, again ended without allocated runners or executed steps. Those runs are infrastructure/startup failures, not compiler/test evidence. This module is therefore not promoted to validated until GCC, Clang, ASan/UBSan and the Android arm64 build actually execute successfully.

## Purpose

Provide a standards-oriented DNG compatibility projection downstream of a finalized TruthRaw Scientific Preview lineage, while keeping the Scientific Master, source evidence, TruthRange gauge and authority state immutable.

The output is a 3-channel, uncompressed, unsigned 16-bit **LinearRaw DNG** in camera-native reconstructed RGB space. It is generated before `camera_to_xyz()`, appearance processing, tone mapping and sRGB conversion.

This is a **representation/export projection**, not a new measurement and not a new evidence root.

## Scientific boundary

Export is allowed only when the caller supplies a real `finalized_scientific_preview_release::v0_2::ReleaseResult` with:

- finalized preview authority;
- nonzero Scientific Master digest;
- `physicalFrameCount == 1`;
- `independentEvidenceCount == 1`;
- no appearance mutation of the Scientific Master;
- no counterfactual observation promoted as evidence.

The export does not change:

- source SHA-256 identity;
- Scientific Master digest;
- TruthRange zero-line/gauge;
- Technical Backplane;
- color-binding authority;
- physical frame/evidence counts.

`SOURCE_METADATA_BOUND` color remains source-bound. A Linear DNG export cannot promote it to independent or `FULL_PHYSICAL` calibration.

## Pixel semantics

The source for the exported pixels is the same bounded tile reconstruction contract used to construct the camera-native RGB Scientific Master identity:

1. source CFA tile read;
2. per-phase black subtraction;
3. optional residual row/column black correction;
4. white-level normalization;
5. source GainMap application exactly once when present;
6. measured-preserving camera-native RGB reconstruction;
7. compatibility quantization only at the export boundary.

The v0.1 compatibility output maps camera-native reconstructed samples to unsigned 16-bit `[0, 65535]`:

- finite values below `0` are clipped to `0`;
- finite values above `1` are clipped to `65535`;
- low/high clipping counts are reported;
- non-finite samples fail closed.

This clipping affects only the DNG compatibility representation. It does not modify the Scientific Master.

## Container contract

v0.1 writes classic TIFF/DNG with:

- `NewSubFileType = 0`;
- `PhotometricInterpretation = 34892 (LinearRaw)`;
- `SamplesPerPixel = 3`;
- `BitsPerSample = 16,16,16`;
- `SampleFormat = unsigned integer`;
- `Compression = 1` (uncompressed);
- chunky `PlanarConfiguration = 1`;
- one full-height strip;
- source Orientation;
- `BlackLevel = 0,0,0` with `BlackLevelRepeatDim = 1,1`;
- `WhiteLevel = 65535,65535,65535`;
- `LinearResponseLimit = 1/1`;
- full-frame ActiveArea and default crop;
- `DNGVersion = 1.4.0.0`;
- `DNGBackwardVersion = 1.2.0.0`.

The backward-version floor is deliberately conservative. The projection can carry DNG 1.2 camera-profile metadata such as `ForwardMatrix1/2`, `CameraCalibrationSignature` and `ProfileCalibrationSignature`; moreover DNG 1.2 defines the inverse-correlated-color-temperature interpolation semantics used for dual-illuminant calibration. Declaring 1.2.0.0 prevents the file from advertising a weaker reader contract than the intended downstream color semantics.

Tag `50734` is `LinearResponseLimit`; the explicit `1/1` value states that the bounded reconstructed LinearRaw encoding is treated as linear across its full compatibility range. It is not a BaselineExposure tag.

The source DNG's camera-space color metadata is copied byte-for-byte where present for these tags:

- ColorMatrix1/2;
- CameraCalibration1/2;
- AnalogBalance;
- AsShotNeutral;
- CalibrationIlluminant1/2;
- CameraCalibrationSignature;
- ProfileCalibrationSignature;
- ForwardMatrix1/2.

Opcode lists are deliberately not copied because Stage-2 corrections handled by TruthRaw must not be applied again by the downstream DNG reader.

## Standards audit — Adobe DNG 1.7.1.0

The v0.1 container contract was re-audited against Adobe Digital Negative Specification 1.7.1.0 before physical export promotion. Relevant conclusions used by this module are:

- `LinearRaw` (`PhotometricInterpretation = 34892`) is valid for a raw IFD and may represent CFA data that has already been demosaiced;
- Orientation is required and is written explicitly;
- `BlackLevel` cardinality is `BlackLevelRepeatRows × BlackLevelRepeatCols × SamplesPerPixel`, therefore `1 × 1 × 3 = 3` here;
- `WhiteLevel` cardinality is `SamplesPerPixel`, therefore 3 here;
- `LinearResponseLimit` is tag 50734, type RATIONAL, default 1.0;
- DNG 1.2 formalized camera profiles and the inverse-CCT interpolation rule for multiple color calibrations;
- ForwardMatrix1/2 and the calibration-signature tags belong to the DNG 1.2 camera-profile feature set.

This audit strengthens only compatibility metadata. It does not add evidence or alter reconstructed samples.

## Bounded-memory execution

The writer traverses the image in the same 64×64 canonical core grid used by the Scientific Master binding and writes row segments directly to the destination file. It does not materialize the full Scientific Master or full output pixel frame in RAM.

The current v0.1 Android sink uses `ftruncate` + `pwrite`; therefore the Android document provider must expose a seekable/read-write file descriptor. Providers that do not support random access must fail closed rather than falling back to an unbounded in-memory file.

## Android route

The Android button is **Linear DNG opslaan**. The flow is:

`finalized preview already visible -> ACTION_CREATE_DOCUMENT -> worker thread -> source SHA seal -> source-bound color producer v0.2 -> finalized Scientific Preview release v0.2 -> source reverification -> Linear DNG projection -> source reverification -> output metrics`

The export reruns the finalized gate; the UI cannot authorize DNG creation with a loose Boolean.

## Falsification tests

`tests/test_linear_dng_projection_v0_1.cpp` verifies on a synthetic source:

- `LinearRaw` PhotometricInterpretation;
- 3-channel 16-bit payload sizing;
- DNG version tag;
- `DNGBackwardVersion = 1.2.0.0`;
- `BlackLevel` type/count for three LinearRaw samples;
- `WhiteLevel` type/count for three LinearRaw samples;
- `LinearResponseLimit = 1/1`;
- byte-exact copied ColorMatrix payload;
- AsShotNeutral presence;
- coherent strip offset/byte count;
- bounded tile traversal;
- no full Scientific Master materialization;
- frame/evidence remains `1/1`;
- export without finalized authority is rejected.

CI must pass GCC Release, Clang Release and Clang ASan/UBSan before research validation is claimed. Android arm64 JNI/Kotlin/APK build must separately pass before a phone-test APK is released.

## Deliberate exclusions from v0.1

Not yet claimed or implemented here:

- reconstructed CFA/remosaic DNG;
- direct-CFA evidence repack DNG;
- lossless compression;
- tiled output storage;
- floating-point LinearRaw preservation of values outside `[0,1]`;
- Adobe `dng_validate` pass;
- physical Lightroom/Camera Raw compatibility;
- independent physical camera/lens color calibration.

A reconstructed CFA output, when added, must be labeled **RECONSTRUCTED_CFA_PROJECTION**. It must never be represented as directly measured sensor CFA.

## DNG technology notice

This product includes DNG technology under license by Adobe.

TruthRaw canonical rule:

**Measured where measured. Reconstructed where necessary. Never invented.**
