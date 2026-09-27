# Professional RAW Gatehouse Runtime v0.1

Status: RESEARCH_CANDIDATE — stacked on `research/libraw-fd-datastream-v0.1`.

## Purpose

The Gatehouse is an isolated decoder/admission runtime between sealed professional RAW sources and the TruthRaw Main House. It exists to keep external decoder state, large decode buffers, codec-specific scratch memory and failures outside the Main House.

It is **not** a second scientific pipeline and it does not create a zero-line, Scientific Master, extra physical frame or extra independent evidence.

## Routing

- `MainHouseDirect`: native-certified TileNative DNG bypasses the Gatehouse.
- `GatehouseRequired`: an externally decoded, lossless-certified, single-frame Bayer source must pass through the Gatehouse.
- `ResearchOnly`: derived/computational/multi-shot/uncertified cases do not enter the normal single-frame Direct-CFA scientific route.
- `FailClosed`: unsupported or invalid inputs stop.

A successful external path is:

`sealed source -> probe -> decoder admission -> decode -> sample/topology/evidence audits -> immutable handoff -> decoder detach -> Main House`

## Twelve Gatehouse rooms

1. Source Vestibule
2. Format Probe
3. Codec Resolver
4. Decode Chamber
5. Sample Audit Lab
6. Topology Lab
7. Metadata Semantics Lab
8. Frame/Evidence Lab
9. Resource Quarantine
10. Provenance Binder
11. Admission Inspector
12. Handoff Airlock

Only Decode Chamber and Sample Audit Lab are classified as heavy in v0.1.

## Low-resource policy

The Gatehouse uses the same principle as the Main House: resource power changes execution, never truth authority.

The Gatehouse budget is bounded by the minimum of:

- one eighth of app memory class;
- one half of currently available memory;
- an optional stricter working-set ceiling.

Below 4 MiB the plan fails closed.

Low/background/severe-thermal execution uses one worker, one compatible heavy room, 64 KiB I/O chunks and no retained rebuildable cache. Mid/high tiers may use more workers and larger I/O chunks, but `allowMainHouseHeavyOverlap` remains false in v0.1.

A decoder is admitted only when its declared `residentUpperBoundBytes + scratchUpperBoundBytes` fits the Gatehouse budget. There is no quality downgrade and no evidence downgrade to make a decoder fit.

## Isolation invariant

For external decode, Main House heavy processing may start only after:

- the handoff is sealed;
- original source remains sealed;
- source-evidence binding is verified;
- decoded representation is immutable and persisted or externally owned;
- decoder context is no longer live;
- no mutable decoder state is shared with Main House;
- Gatehouse created neither zero-line nor Scientific Master;
- `physicalFrameCount == 1`;
- `independentEvidenceCount == 1`;
- evidence class is `LosslessDecodedCertified`;
- topology is the currently certified downstream `Bayer2x2` path.

This allows a low-memory phone to complete and release a large external decode before the Main House acquires its heavy working set.

## Current scope

v0.1 proves routing, resource admission, state transitions, detach/handoff rules and invariants with synthetic adapter outputs. It does **not** yet prove real CR3/NEF/ARW/RAF/IIQ sample decoding, physical Android RSS/thermal behavior, or real-device end-to-end handoff.
