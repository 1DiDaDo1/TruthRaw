# Fact check — Tile-Native DNG Source v0.1

## PASS — architectural claims supported by implementation/tests

- The production source implements the existing `IRawTileSource` API.
- No member contains a full-frame RAW or full-file byte vector.
- Strip/tile locator arrays are retained as TIFF tag references, not materialized vectors.
- Uncompressed 16-bit row spans are random-access read directly into caller-owned tile memory.
- Little- and big-endian fixtures pass.
- Multi-row strips and TIFF tile storage pass.
- A requested rectangle crossing strip/tile boundaries matches known CFA samples.
- 2x2 BlackLevel and scalar WhiteLevel bind into `DngMetadata`.
- six-double NoiseProfile binds only when RGB CFAPlaneColor ordering is explicit.
- four phase-specific GainMap opcodes produce the expected phase gains while RAW remains unchanged.
- residual black is not invented; `hasResidualBlack=false` and zero-bias access is only a neutral interface response.
- missing source/color bindings fail closed.
- unsupported compression, packed 12-bit storage and BigTIFF fail closed.
- malformed lazy strip offset fails at first use rather than causing an out-of-bounds read.
- ambiguous CFA IFD selection fails unless `explicitRawIfdOffset` identifies one supported CFA IFD.
- metadata/OpcodeList/resident caps are enforced.
- POSIX `pread()` source path passes locally.

## Synthetic large-frame result

Fixture dimensions: 16320 x 12288 = 200,540,160 CFA samples.

The virtual file exposes one uncompressed strip representing the entire frame without allocating its ~401 MB RAW payload in the test process. After open, source resident state is a few KiB and metadata reads are a few hundred bytes. Reading a 64x64 rectangle transfers exactly 8192 RAW payload bytes. Exact numbers are recorded in `evidence/LOCAL_TEST_METRICS_v0_1.txt`.

This proves the implementation's steady-state ownership strategy for the synthetic fixture. It does not prove that `open()` peaks at the same few-KiB value: IFD discovery and OpcodeList2 parsing have explicit bounded transients, but v0.1 does not yet report their peak. It also does not prove device RSS, filesystem cache behavior, flash latency, Android thermal behavior, or universal DNG compatibility.

## OPEN / deliberately not claimed

- compressed DNG support;
- packed 10/12/14-bit TIFF sample decoding;
- BigTIFF;
- universal DNG color resolution from ColorMatrix/CameraCalibration/ForwardMatrix/illuminant metadata;
- arbitrary OpcodeList2 processing beyond the strict GainMap subset;
- real Honor/MotionCam DNG validation on this new source;
- Android device RSS/latency/thermal benchmarks;
- multi-worker concurrent reads;
- explicit `open()` transient peak reporting (inputs are capped, peak telemetry remains open);
- cryptographic verification that the supplied `sourceEvidenceId` matches file bytes (Archivist responsibility).

## External-standard cross-check

Adobe currently publishes DNG Specification 1.7.1.0 and DNG SDK 1.7.1 build 2724. DNG remains TIFF-based and supports private metadata. LibTIFF documentation confirms that encoded strip/tile interfaces generally operate at strip/tile decoded-buffer granularity. These facts support the strict uncompressed direct-range baseline, but external documentation is not TruthRaw measurement evidence.
