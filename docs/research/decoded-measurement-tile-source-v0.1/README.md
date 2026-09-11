# Decoded Measurement Tile Source v0.1

Status: RESEARCH_CANDIDATE — stacked on Decoded Measurement Handoff v0.1.

## Purpose

Expose a verified, persisted decoded measurement handoff to the existing TruthRaw Main House through the existing `streaming_v0_1::IRawTileSource` interface.

The Main House therefore does not know about LibRaw, decoder contexts, Gatehouse rooms or full-frame decoder buffers. It sees a normal bounded RAW-tile source.

## Admission boundary

Binding requires all of the following:

- handoff store is open and payload integrity has already been verified;
- metadata is explicitly bound to the original sealed source;
- metadata semantics have been verified upstream;
- decoded store is bound to the original sealed source;
- Gatehouse is detached;
- handoff is single-frame `LosslessDecodedCertified` `Bayer2x2`;
- metadata dimensions and CFA pattern exactly match the handoff descriptor.

v0.1 refuses `hasGainField=true` and `hasResidualBlack=true` rather than silently discarding those measurement semantics. Those cases need explicit detached transport support first.

## Memory behavior

The bridge owns no RAW frame. It borrows the verified handoff Reader and keeps only a small copy of certified `DngMetadata`. `readRawTile()` converts the requested halo rectangle directly into a bounded `Reader::read_rect()` operation.

Row/column bias calls return zero only because v0.1 binding rejects sources that declare residual-black data.

## Scientific boundaries

This bridge creates no evidence, reconstruction, zero-line or Scientific Master. It only exposes an already admitted decoded measurement representation through the same source ABI used by native tile DNG.

Native-certified DNG continues to bypass Gatehouse/handoff entirely. External professional RAW follows:

`sealed source -> Gatehouse -> verified persisted handoff -> detach -> DecodedMeasurementTileSource -> Main House`

The source remains `physicalFrameCount=1` / `independentEvidenceCount=1`.

## Current scope

The candidate proves exact synthetic CFA tile reads through the `IRawTileSource` ABI and fail-closed metadata/admission gates. It does not yet prove full `StreamingTruthRawProcessor` end-to-end processing from this source, real professional-camera metadata extraction, GainMap/residual-black detached transport, or Android physical-device performance.
