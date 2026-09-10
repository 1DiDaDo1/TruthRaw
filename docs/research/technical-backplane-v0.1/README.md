# TruthRaw Technical Backplane v0.1

A fixed-size, format-neutral technical backside for one TruthRaw image/master lineage.

The backplane stores **identity and provenance bindings, not image pixels**. Its serialized representation is exactly 180 bytes and contains one field each for:

- source-evidence SHA-256 binding;
- scientific-master SHA-256 binding;
- zero-line SHA-256 binding;
- scene-scale SHA-256 binding;
- physical-frame and independent-evidence counts;
- 12 compact room-status bytes;
- one claim-status byte;
- forbidden scientific-mutation/evidence-promotion flags;
- reserved bytes for forward-compatible fail-closed parsing;
- CRC32 corruption detection over the first 176 bytes.

## Scientific contract

`physicalFrameCount = 1` and `independentEvidenceCount = 1` are mandatory. A state that claims the scientific master or zero-line was modified, or counts appearance/counterfactual data as evidence, cannot be serialized as a valid backplane.

The zero-line is therefore **one shared binding per lineage**, never a per-pixel field and never a tile buffer. Rooms may receive a handle/reference to this backplane; they do not receive private copies as scientific state.

CRC32 is used only for accidental corruption detection. It is not a cryptographic provenance primitive. The four SHA-256 fields are external identity bindings and this module does not itself prove that a supplied hash matches external file bytes; that remains Archivist responsibility.

## Container policy

This module is deliberately format-neutral. It does not hide data in image pixels and does not yet prescribe a DNG private-data carrier. DNG carriage remains OPEN until interoperability is separately validated.

Status: `RESEARCH_CANDIDATE_LOCAL_GCC_CLANG_ASAN_UBSAN_PASS`.
