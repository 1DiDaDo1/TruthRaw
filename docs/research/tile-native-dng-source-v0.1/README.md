# TruthRaw Tile-Native DNG Source v0.1

Status: **RESEARCH CANDIDATE — LOCAL SOURCE TESTS PASS; REAL REPOSITORY INTEGRATION CI REQUIRED**

This module is the low-memory front door for the renewed TruthRaw house. It implements the existing Full-Frame Streaming v0.1 `IRawTileSource` contract without first materializing a complete `DecodedDngFrame.raw` or the complete DNG file in RAM.

## Scientific boundary

The source returns original stored CFA sample values plus separately bound metadata. It does not demosaic, denoise, reconstruct, relight, tone-map, or create evidence. GainMap samples are returned separately so the existing streaming Stage-2 path remains the single place that applies GainMap exactly once.

`sourceEvidenceId` is supplied by the Archivist/provenance layer. The source does not reinterpret each tile as a new measurement; physical frame count and independent evidence count remain one.

Color is deliberately not guessed from a partial DNG color-tag interpretation. `cameraToXyzD50` and a non-empty color binding ID must be supplied explicitly by the color/calibration authority. This avoids turning a reader convenience into a second color truth.

## Supported strict v0.1 profile

- Classic TIFF/DNG magic 42, little- or big-endian.
- one uniquely selected 2x2 RGB CFA IFD, or an explicit raw IFD offset when more than one CFA IFD exists;
- `PhotometricInterpretation = 32803` (CFA);
- `SamplesPerPixel = 1`;
- unsigned 16-bit TIFF sample storage;
- `Compression = 1` only;
- strip storage with arbitrary positive `RowsPerStrip`, or TIFF tile storage;
- 1-value or 2x2/4-value BlackLevel;
- scalar WhiteLevel;
- supported Orientation 1/3/6/8;
- optional six-double NoiseProfile with explicit RGB CFAPlaneColor order;
- optional OpcodeList2 containing only GainMap opcodes, with exactly one phase-covering GainMap per CFA phase.

This profile covers RAW values with a 10-bit scientific range stored in 16-bit TIFF samples. It does **not** claim support for packed 10/12/14-bit TIFF storage.

## Explicitly blocked in v0.1

BigTIFF, compressed DNG, packed non-16-bit samples, non-RGB or non-2x2 CFA, unsupported OpcodeList2 entries, ambiguous CFA IFD selection, and malformed/over-budget metadata fail closed. There is no hidden LibTIFF full-strip/full-tile fallback.

## Memory model

The IFD itself is parsed into a small tag-reference index. `StripOffsets`/`StripByteCounts` and tile offset/count arrays remain on disk and are addressed lazily per requested storage unit. Their values are validated when used. The full file and full RAW are never copied by this module.

For uncompressed strips, a requested row segment is read directly into the caller-owned `uint16_t` tile buffer using random access. Even a single strip containing a 200 MP image therefore does not require a strip-sized buffer. TIFF-tiled storage is handled by reading only intersecting row segments from each storage tile.

A POSIX `pread()` byte-source is supplied for Android/Linux-style file descriptors. The file descriptor remains caller-owned.

The synthetic 16320x12288 (~200.5 MP) one-strip fixture reports 4989 bytes of **steady-state source state** after open and reads exactly 8192 RAW payload bytes for a 64x64 request. This is a synthetic memory-contract test, not a measured Android benchmark. It is also not a claim that `open()` itself peaks at 4989 bytes: IFD discovery and OpcodeList2 parsing use bounded transient allocations. v0.1 caps those inputs (`maxIfdCount`, `maxIfdEntries`, `maxOpcodeListBytes`, `maxResidentBytes`) but does not yet expose a measured/open-peak counter.

## Why not use encoded LibTIFF reads as the low-end baseline?

LibTIFF's encoded strip/tile API normally operates on a decoded strip/tile buffer. That is valuable for compatibility and future compressed DNG support, but storage-unit size could then dominate peak RAM. v0.1 instead has a strict direct-range path for uncompressed 16-bit CFA data. A later compressed-source room may use LibTIFF/DNG SDK with an explicit storage-unit memory lease rather than weakening this baseline.

## Integration target

`DNG/file descriptor -> TileNativeDngSource -> Full-Frame Streaming v0.1 -> StreamedSink`

Canonical v4.7i is byte-unchanged. Full-Frame Streaming v0.1 is also treated as an upstream bound dependency. End-to-end repository CI must compare the tile-native route with canonical `processFrame` on the same known CFA samples before promotion.
