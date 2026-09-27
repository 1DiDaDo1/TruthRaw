# Fact check — Professional RAW Ingress / Decoder ABI v0.1

## Proven by this candidate implementation/tests

- Format/container family, decoder certification, measurement topology and scientific admission are independent fields.
- Unknown inputs fail closed.
- A decoder that merely exists but whose byte path is not verified cannot be promoted above `RESEARCH_ONLY`.
- A certified external lossless Bayer decoder is distinguishable from the native-certified DNG route.
- A decoded X-Trans source does not gain Bayer/Direct-CFA authority when the downstream topology is not certified.
- Computational RAW and multi-shot composite RAW are explicitly blocked from single-frame Direct-CFA admission.
- Decoder resource declarations are orthogonal to evidence class and include explicit tile/full-frame semantics.
- The original source is always marked as requiring sealed preservation.

## Repository facts this module depends on

- Tile-Native DNG Source v0.1 is deliberately a strict classic-TIFF/DNG subset and blocks compressed DNG, BigTIFF, packed non-16-bit samples, non-RGB/non-2x2 CFA and unsupported OpcodeList2 semantics.
- Room ABI v0.1 explicitly does not yet own Tile-Native DNG Source or Full-Frame Streaming handles; those are v0.2 integration targets.
- Technical Backplane v0.1 binds one source identity and one single-frame lineage; it is not a codec metadata container.

## Deliberate non-claims

- This module does not decode CR3, NEF, ARW, RAF, RW2, ORF, PEF, 3FR, IIQ, X3F or any other vendor format by itself.
- It does not claim universal camera support.
- It does not certify any third-party decoder/library version.
- A family name in the target registry does not certify every camera, firmware, bit packing, compression or RAW mode in that family.
- Successful lossless decoding does not by itself prove spectral/color calibration, electrons/DN, read noise, full well, PSF/MTF, illuminant or scene truth.
- `LOSSLESS_DECODED_CERTIFIED` describes the verified byte-to-sample route plus admitted measurement topology; it is not the same label as `DIRECT_NATIVE_CERTIFIED`.
- Computational or multi-shot data may be valuable but do not become a second independent observation of the original single-frame TruthRaw lineage.

## Architectural decision

Broad professional-camera compatibility must be added through explicit decoder adapters and topology-aware admission. The existing low-memory DNG reader stays as a strict native path. Compatibility must never be obtained by silently flattening all RAW families into an assumed 2x2 Bayer buffer.
