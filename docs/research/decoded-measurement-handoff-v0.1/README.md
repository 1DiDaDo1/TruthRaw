# Decoded Measurement Handoff v0.1

Status: RESEARCH_CANDIDATE — stacked on Professional RAW Gatehouse Runtime v0.1.

## Purpose

This module lets the Professional RAW Gatehouse persist a certified decoded CFA representation, verify it, destroy the external decoder context, and only then let the TruthRaw Main House consume the same measurements through bounded reads.

It exists primarily to prevent a large LibRaw/external-decoder working set from overlapping the Main House on memory-constrained phones.

## Lifecycle

`sealed RAW -> Gatehouse decode -> bounded handoff write -> seal -> integrity verify -> decoder destroy -> Gatehouse detach -> Main House bounded reads`

Native-certified TileNative DNG still bypasses this transport and may enter the Main House directly.

## Transport

v0.1 uses a caller-owned seekable POSIX file descriptor and a fixed 256-byte `TRDMH001` header followed by canonical little-endian 16-bit CFA sample storage.

Writer and Reader borrow the descriptor; neither owns or closes it.

The module has no full-frame in-memory handoff buffer. Writes and integrity verification use fixed 4 KiB chunks. Rectangular Main House reads also use bounded 4 KiB chunks.

## Scientific boundaries

The persisted decoded representation is **not new evidence**. It is another representation of the same sealed capture:

- `physicalFrameCount == 1`
- `independentEvidenceCount == 1`
- v0.1 handoff topology must be `Bayer2x2`
- evidence class must be `LosslessDecodedCertified`
- source compression must have been uncompressed or lossless verified
- source evidence binding and decoder audit binding are carried explicitly
- no zero-line is created here
- no Scientific Master is created here
- no reconstruction, color rendering, appearance processing or counterfactual work occurs here

The Gatehouse additionally requires `decodedRepresentationIntegrityVerified` before a handoff may be sealed/admitted to the Main House.

## Integrity semantics

The payload CRC32 detects accidental transport/storage corruption. **CRC32 is not cryptographic evidence identity and does not replace the sealed source SHA-256 or decoder audit hash.**

`sourceEvidenceHash` and `decoderAuditHash` are opaque upstream 256-bit bindings. v0.1 verifies that they are present/nonzero but does not manufacture or reinterpret them.

The Reader refuses pixel/rect reads until the entire stored payload has been streamed once through `verify_payload_integrity()` successfully. This verification is bounded-memory, not full-frame materialization.

## Low-memory behavior

The handoff layer itself has a small fixed resident bound (object state + 4 KiB I/O chunk + 256-byte header). A decoded image may be hundreds of megabytes on storage without becoming a hundreds-of-megabytes handoff RAM allocation.

This enables the intended low-end sequence:

1. Gatehouse admits the external decoder only if its own peak fits the Gatehouse budget.
2. Decoder emits samples into the bounded writer.
3. Store is sealed and integrity-verified.
4. External decoder/full-frame working set is destroyed.
5. Gatehouse becomes `Detached`.
6. Main House starts its own heavy resource leases and reads only required rectangles/tiles from the persisted representation.

v0.1 intentionally forbids heavy Gatehouse/Main-House overlap.

## Current scope

This is a research transport, not a production DNG/JPEG/HEIF/AVIF container. It currently proves deterministic synthetic Bayer sample persistence, bounded reads, corruption detection and Gatehouse detach/admission semantics. It does not yet prove a real CR3/NEF/ARW/RAF/IIQ decode, physical Android storage/RSS performance, crash recovery, encrypted storage, or production lifecycle cleanup.
