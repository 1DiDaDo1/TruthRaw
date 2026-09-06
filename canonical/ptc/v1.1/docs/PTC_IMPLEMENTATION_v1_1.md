# PTC v1.1 implementation

PTC is implemented as a fail-closed certification boundary around TruthRaw export.

## Pipeline hook

`source RAW hash -> admission gates -> exact reconstruction backend hash -> scientific-master hash -> appearance/export -> media-payload hash -> PTC evaluate -> metadata embed -> optional external full-file hash`

The certificate cannot turn a failed research output into a certified output. It only reports the evidence state.

## Hash-cycle fix

A whole-file SHA-256 cannot be embedded into the same final file without changing that file. PTC therefore embeds `MediaPayloadSHA256`, a digest over image/coding payload with metadata segments excluded. The complete final container SHA-256 belongs in an external manifest, release ledger, or signature registry.

For JPEG the reference implementation hashes every coding/image-semantic JPEG segment while excluding APP0–APP15 and COM metadata. Injecting PTC XMP therefore leaves the media-payload digest unchanged.

For DNG, production integration must use the DNG SDK/exporter. The certificate supplies the TIFF Copyright value plus an XMP packet for TIFF tag 700. The Python reference deliberately does **not** rewrite production DNGs because a generic TIFF rewrite could discard private/opcode/provenance tags and would violate PTC's fail-closed rule.

## FULL_PHYSICAL end-state blockers

FULL_PHYSICAL requires, in addition to core integrity:
- source-bound noise model;
- backend-bound calibrated uncertainty;
- per-lens color calibration;
- illuminant calibration;
- electron/PTC calibration;
- optical calibration.

Missing items return `PURE_TRUTH_DERIVED`, not a false PASS.

## Signature

The Python reference supports optional Ed25519 signatures. A signature proves integrity relative to a key; it does not prove the key belongs to TruthRaw unless the public key is separately published/trusted.
