# TruthRaw — Adobe Lightroom HDR DNG rewrite audit v1.1

**Status:** RESEARCH — NOT MAIN-PROMOTED  
**Scope:** exact CFA identity through an Adobe Lightroom HDR DNG rewrite, while keeping metadata authority separate from pixel authority.

## Real observation

The original HONOR tele source and the Lightroom-written DNG have different container hashes and sizes:

- original: `7930ba5d...59b67`, 25,106,120 bytes;
- Lightroom-written: `a69e434c...ede1e`, 7,839,986 bytes.

The Lightroom file is therefore **not** the immutable source file.

However, the rewritten DNG contains a 4080×3072 BGGR raw SubIFD encoded as tiled lossless JPEG (SOF3), 256×256 tiles, 192 tile segments. We decoded all 192 segments and compared the complete 12,533,760-sample CFA raster against the immutable original.

Result:

```text
mismatch_count = 0
max_abs_difference = 0
original_raw_sample_sha256  = 883cbe13fd2a5afe7e9f3f4147df3a8ed3be681340a3c0422933f42d0f4d719c
rewritten_raw_sample_sha256 = 883cbe13fd2a5afe7e9f3f4147df3a8ed3be681340a3c0422933f42d0f4d719c
```

Therefore this exact Adobe rewrite is **CFA sample-exact**.

## What changed

Sample identity does not imply metadata identity. Lightroom rewrote the DNG container and embedded its own HDR edit state. The file reports Lightroom 11.5.22 Android, `HDREditMode=1`, `HDRMaxValue=+8.00`, `Exposure2012=0.00`, `Whites2012=+2` and a custom extended tone curve.

Several source-bound fields remain equal or numerically equal:

- Make/model/unique camera model;
- 4080×3072 BGGR CFA geometry;
- WhiteLevel = 1023;
- BlackLevel = 64 for all Bayer phases;
- NoiseProfile exactly equal;
- OpcodeList2 byte-exact (`43f87e51...56e5c`);
- OpcodeList3 byte-exact;
- ActiveArea equal.

But the DNG is not metadata-source-exact:

- Orientation changed from 3 to 1;
- rational encodings were canonicalized/rounded;
- AsShotNeutral changed by a maximum of ~3.125e-7;
- ForwardMatrix values changed by up to ~5e-5;
- CameraCalibration by ~1.25e-5;
- ColorMatrix1 changed by up to ~0.02985;
- ColorMatrix2 changed by up to ~0.20949.

The rewritten file also has `NewRawImageDigest=6faea350add53e0841d80a210d9cf3a0`, but does not carry `OriginalRawFileDigest` or `OriginalRawFileName`.

## Scientific classification

The exact result is:

`ADOBE_HDR_DNG_REWRITE_CFA_SAMPLE_EXACT_METADATA_NOT_SOURCE_EXACT`

Consequences:

1. The immutable original DNG remains the archive/source authority.
2. The Adobe DNG may carry the **same measured CFA samples** only because full decoded-raster equality was proved against that original.
3. Adobe/XMP HDR edit settings are presentation state, not sensor evidence.
4. Rewritten calibration/color metadata may not replace original calibration authority without field-specific validation.
5. A future Adobe DNG rewrite must fail closed: if even one CFA sample differs, measured-CFA authority transfer is revoked.
6. No Adobe edit or rewritten metadata may write authority back into the Scientific Master.

## Why this matters for HDR

This gives TruthRaw a useful interoperability pattern:

```text
immutable original CFA source
       ↓ exact sample identity proof
Adobe-derived DNG carrier
       + HDR edit metadata
       + previews/rendered pyramids
```

The carrier can be useful for Lightroom interoperability, but it is never allowed to replace the immutable source. Representation may expand; evidence authority remains bound to the exact original CFA.

## Next gate

Build a TruthRaw-owned DNG/HDR sidecar strategy that can preserve:

- source SHA-256;
- decoded CFA sample SHA-256;
- Scientific Master hash;
- Dynamic Authority Field hash;
- explicit Adobe presentation settings;
- field-by-field metadata authority status.

The goal is a reversible derived carrier whose HDR editing can be changed freely while the original CFA and scientific authority remain cryptographically anchored.
