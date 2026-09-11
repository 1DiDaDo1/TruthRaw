# TruthRaw Professional RAW Decoder Adapter v0.1

Status: **RESEARCH_CANDIDATE_REPOSITORY_CI_PASS — DECODER ADAPTER CONTRACT; REAL VENDOR CODECS NOT YET IMPLEMENTED**

This module connects the format-neutral Professional RAW Ingress classification to concrete decoder implementations without allowing “file can be opened” to become “scientific evidence is certified”.

## Core rule

A decoder adapter must report, separately:

- source/container family;
- measurement topology;
- compression semantics;
- decoder certification/provenance;
- whether decoded samples are proven equivalent to stored measurements;
- whether original source bytes remain sealed and bound;
- physical-frame/evidence interpretation;
- decoder resident + scratch upper bounds;
- whether it requires full-frame materialization.

File extension and camera brand are never evidence.

## Adapter #1 — native strict DNG

The existing `TileNativeDngSource v0.1` is the first native adapter target. It already uses borrowed random-access bytes, reports a resident upper bound and audits whether full file/full RAW were materialized. The bridge in this module can describe an already-open source as `NativeCertified` only when:

- adapter identity is non-empty;
- full file and full RAW were not materialized;
- the Archivist says the source-evidence binding is verified;
- color/topology authority is verified.

This does not broaden TileNativeDngSource format support.

## External decoders

CR3, CR2, NEF/NRW, ARW, RAF, RW2, ORF/ORI, PEF, 3FR/FFF, IIQ, MOS/MEF and X3F can later implement the same adapter contract.

A verified lossless external Bayer decoder can be classified `LosslessDecodedCertified`, not `DirectNativeCertified`.

If decoded sample equivalence is not independently verified, scientific admission fails closed even if the codec library successfully produced pixels.

X-Trans, Foveon/layered, linear RGB, computational RAW and multi-shot composite topologies are never disguised as Bayer.

## Memory authority

Decoder memory is part of resource admission. Full-frame decoders are not scientifically “worse” merely because they use more RAM; they may simply be inadmissible on a low-resource device. A stronger device may admit the exact same decoder without changing its evidence class.

This preserves:

`resource authority != truth authority`

## Non-claims

This module does not yet ship LibRaw, RawSpeed, Adobe DNG SDK or vendor SDK code. It does not claim CR3/NEF/ARW/RAF/IIQ files are currently decodable by TruthRaw. It defines the contract those decoders must satisfy.
