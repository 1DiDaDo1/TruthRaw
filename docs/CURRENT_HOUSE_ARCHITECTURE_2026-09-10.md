# CURRENT HOUSE ARCHITECTURE — 2026-09-10

Status: **CURRENT ARCHITECTURE OVERVIEW**

This document is the current architectural map. It does not replace module-specific scientific evidence, frozen validation reports or the two core domain authorities for the sealed-house and zero-line/TruthRange semantics.

## 1. Foundation and evidence vault

The original Direct-CFA/source RAW and capture metadata are sealed evidence. They are never rewritten to make downstream execution easier.

The **original source container itself** is also part of that sealed provenance. A vendor RAW decoder may expose an exact or derived sample representation, but its output does not replace the original CR3/NEF/ARW/RAF/DNG/etc. file identity. Container recognition, decode certification and scientific admission are separate decisions. Filename extension or parser success alone never upgrades evidence authority.

Single-frame invariants:

- `physicalFrameCount = 1`
- `independentEvidenceCount = 1`
- counterfactual/virtual views are not evidence
- appearance is not evidence

### 1.1 Professional RAW ingress boundary

The Measurement Lab receives a borrowed decoder/provenance handle rather than assuming every RAW is the current strict DNG subset. A decoder adapter must expose at least container family, codec variant, source bit depth, measurement topology, compression semantics, decoder identity/build binding, byte-path verification state and bounded resource requirements.

Measurement topology is explicit. Bayer 2x2, X-Trans, monochrome, layered/Foveon, linear-RGB, multi-shot composite and computational RAW may not be flattened into one generic CFA label merely because a decoder can return pixels.

Only a verified single-frame/single-evidence, uncompressed or verified-lossless measurement whose downstream topology is independently certified may enter the current Direct-CFA single-frame scientific master. Computational and multi-shot products remain derived measurement classes. Unknown variants fail closed.

The active research contract is `docs/research/professional-raw-ingress-v0.1/README.md`. Its family registry is a compatibility target map, not a claim that all professional camera models or RAW modes are already implemented.

## 2. Scientific house

The Scene Master is reconstructed from the foundation and is separate from the source container. Its representation may exceed source RAW code range/container/display limits, while every claim remains bounded by evidence and uncertainty.

Measured/reconstructed/appearance/counterfactual state never collapse into one label.

## 3. TruthRange and zero-line

For positive physical light:

`T = log2(L/L0)`

The zero-line is a shared gauge binding. It is **not** a megapixel image and should not be copied into every tile. The renewed architecture treats zero-line identity as immutable shared state referenced through the Technical Backplane/corridor metadata.

Signed scene-linear processing remains a separate companion coordinate.

## 4. Floors and rooms

Truth floor is independent from computational size.

| Room | Primary role | Floor/domain | Typical memory behavior |
|---|---|---|---|
| Archivist | evidence/provenance binding | Foundation / Scientific | tiny immutable/shared |
| Measurement Lab | source measurement interpretation + decoder admission | Measurement / Scientific | streaming/tile-preferred; decoder lease explicit |
| Architect | measured-preserving reconstruction | Reconstruction / Scientific | heavy tiled |
| Restorer | uncertainty-aware reconstruction support | Reconstruction / Scientific | tiled/rebuildable |
| Scene Registry | Scene Master identity/registry | Scene / Scientific | handles + persistent master store |
| Surveyor | uncertainty/topology/covariance | Scene / Scientific | tiled/compact fields |
| Manifold Conditioning | exact numerical conditioning | Scene / Scientific | stateless/span; near-zero owned memory |
| Lighting Studio / CICM | counterfactual illumination/capture | Counterfactual | scalar/light state; geometry path remains bounded |
| Room Capsule | local relight geometry | Counterfactual | compact/downsampled + bounded tiles |
| Colorist | color transform/appearance preparation | Appearance | tile-local |
| Finisher | S-curve/detail/acutance appearance | Appearance | tile-local/disposable |
| Exporter | DNG/SDR/HDR projection | Projection | streaming sink preferred |

The graph is forward-only across truth floors. Scientific rooms cannot read appearance/counterfactual output back as scientific evidence.

## 5. Corridors

A corridor transports identity and permission, not another copy of the photograph.

Preferred corridor payload:

- artifact/master handle
- provenance fingerprint/hash
- source/master/scene-scale/zero-line binding
- decoder provenance handle when source decoding is involved
- tile rectangle/format when relevant
- truth floor / claim status
- evidence counters

Large pixel data remain behind external source/master/sink handles.

## 6. Technical Backplane

The Technical Backplane is the compact “digital backside” of the house.

It is intended to bind, once per image/master:

- source evidence hash
- scientific master hash
- zero-line binding
- scene-scale binding
- single-frame evidence invariants
- compact room/status provenance

It is not a second image, not extra evidence, not a hidden generative payload and not a replacement for standard DNG metadata.

Current research prototype: fixed 180-byte format-neutral serialization with corruption detection and one zero-line binding. DNG embedding/export carriage remains OPEN until interoperability is independently tested.

Professional decoder identity/version/codec metadata remains an external immutable provenance binding. It must not inflate or duplicate this fixed record per tile/room.

## 7. Memory ownership classes

Every runtime datum must be classified into exactly one execution ownership class:

### A — Immutable shared state
One copy, referenced everywhere. Examples: source identity, scientific-master identity, zero-line binding, scene-scale binding, immutable capture metadata, decoder provenance binding.

### B — Persistent scientific artifact
Per-pixel information that must survive room boundaries, but should live behind a persistent artifact/master handle rather than duplicated in RAM.

### C — Tile-local transient
Working pixels valid only for one active tile/halo. Release immediately after downstream consumption.

### D — Rebuildable derived cache
Deterministically reproducible data: compact geometry, preview caches, some derived confidence/appearance intermediates. May be evicted under pressure.

### E — Streamed output
Projection/export bytes written incrementally to a sink, avoiding a second full-frame output buffer.

## 8. Cheap versus powerful phones

Scientific semantics are invariant across device classes.

Low-resource execution:
- smallest admissible tiles;
- one heavy room at a time;
- CPU correctness baseline;
- no speculative prefetch;
- minimal cache retention;
- aggressive release between rooms/tiles.

High-resource execution:
- larger tiles when the room benefits;
- multiple dependency-compatible rooms/lanes;
- larger but bounded rebuildable caches;
- optional acceleration backend;
- prefetch only when memory/thermal state permits.

A stronger phone changes throughput/latency, never evidence confidence or claim authority.

Professional RAW decoder adapters obey the same rule. A full-frame compatibility decoder may be inadmissible on a weak phone because its declared resource lease does not fit; this changes execution availability only. It does not permit a flagship to assign stronger evidence semantics. Long-term device parity therefore favors bounded tile/streaming decoders.

## 9. Current memory gap

Canonical reconstruction v4.7i already processes reconstruction/appearance internally by tiles, but its public ownership model still includes full-frame vectors such as:

- `DecodedDngFrame.raw`
- `ProcessResult.sdrRgb`
- optional full-frame `halfLogGain`
- optional full-frame `stage2Diagnostic`

These are the highest-priority remaining RAM bottlenecks.

Target migration:

`sealed source handle -> certified decoder/tile source -> bounded tile/halo workspace -> persistent Scene Master/artifact store -> downstream room -> StreamedSink`

Canonical v4.7i must remain byte-stable until a research streaming adapter proves equivalence, ordering, halo correctness, error propagation and bounded peak memory.

## 10. Documentation authority rule

Use `docs/DOCUMENT_STATUS_INDEX_2026-09-10.md`.

Historical documents remain preserved because failure/rejection/provenance is part of the scientific record. “Old” does not mean “delete”; it means “do not mistake for current global state”.
