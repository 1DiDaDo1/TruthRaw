# TruthRaw v0.74 — bit-exact stripe performance — 2026-09-20

## Status

Active integration branch:

`integration/truthraw-suite-v0-74-bitexact-stripe-performance`

Android app version:

`0.40-v0.74-bitexact-stripe-performance`

v0.74 is a performance-only integration on top of the real-device validated v0.73 camera/Main-House/PURE line.

It does **not** change:
- source admission;
- Stage-2 math;
- v4.7i reconstruction;
- Scientific Master pixel values;
- Scientific Master digest semantics;
- TruthRange/Zero-Line/L0;
- scene-scale;
- Technical Backplane;
- Dynamic Authority;
- color authority;
- PURE v0.63;
- TruthNegative;
- Restoration;
- Open Scene;
- frame/evidence counts.

## Physical motivation

The real-device v0.73 HONOR BKQ-N49 camera run measured:

- 4080x3072 admitted camera-origin DNG;
- finalized Scientific Preview wall time ~87.96 s;
- worker CPU ~87.70 s;
- logical RAW tile reads: 7,680;
- no full RAW materialization;
- PSS ~112-115 MiB;
- no thermal throttling;
- UI p95 ~16.6 ms.

This indicates a CPU/execution-granularity bottleneck rather than a memory, thermal or UI bottleneck.

## Existing v0.2 schedule

For 4080x3072:

Scientific Master / exact self-gauge:
- canonical 64x64 cells: 3,072;
- v0.2 pass 1: 3,072 source requests;
- v0.2 pass 2: 3,072 source requests;
- scientific subtotal: 6,144.

Existing finalized-preview streaming:
- 128x128 pass 1: 768;
- 128x128 pass 2: 768;
- preview subtotal: 1,536.

Measured total: 7,680 logical tile requests.

## v0.3 stripe binding

The Scientific Master digest remains canonically 64x64.

Runtime execution changes only its source-call granularity.

Digest/reconstruction pass:
- runtime core: 1024x64;
- v4.7i required halo preserved;
- each runtime stripe is split by the existing digest accumulator into the exact same canonical 64x64 leaf cells.

Exact median refinement:
- runtime core: 1024x256;
- zero reconstruction halo because this pass consumes only the current RAW/Stage-2 sample for radix classification;
- exact positive Float32 bit ordering and v0.2 radix16 algorithm remain unchanged.

For 4080x3072:

- digest pass: 4 x 48 = 192 source requests;
- median refinement: 4 x 12 = 48 source requests;
- scientific subtotal: 240.

Preview remains 1,536 requests.

Expected finalized-preview total after Android integration:

`240 + 1536 = 1776`

versus the v0.73 measured `7680`.

This is a 76.875% reduction in logical source-call count.

It is **not yet a physical latency claim**.

## Host falsification

Research branch:

`research/scientific-master-streaming-binding-v0-3-stripes-2026-09-20`

Scientific binding v0.3 was compared directly with v0.2 on:
- pseudo-random signal;
- constant signal;
- split median;
- sparse eligible evidence;
- GainMap;
- residual black;
- GainMap + residual black;
- multi-stripe geometry.

Required exact identities passed:
- Scientific Master SHA-256;
- L0 binary64 bits;
- Zero-Line mode/id/flags;
- scene-scale;
- eligible sample count;
- canonical master tile count;
- physical frame / independent evidence = 1/1.

GCC Release: PASS.
Clang Release: PASS.
Clang ASan/UBSan: PASS.

## Finalized release v0.3

A separate `finalized-scientific-preview-release-v0.3` keeps v0.2 intact and changes only the scientific binding implementation from v0.2 to v0.3.

End-to-end v0.2↔v0.3 tests require exact equality of:
- Scientific Master identity;
- L0 bits;
- 180-byte Technical Backplane;
- finalized preview ARGB pixels;
- preview authority;
- frame/evidence state;
- persisted v0.2 Backplane acceptance.

No existing v0.2 research artifact is rewritten.

## Android scope

Only:

`NativeTilePreviewBridge.buildFinalizedScientificColorPreview`

is routed through finalized release v0.3.

The following stay on their prior bindings in this step:
- PURE Float32 DNG;
- TruthNegative;
- Advanced;
- full-resolution Restoration;
- Restoration DNG/TIFF/EXR.

This isolates physical performance measurement to the exact path that measured ~87.96 s.

## Real-device success gate

On the same 4080x3072 Camera-5/Main-House sample class:

1. finalized preview releases successfully;
2. source ancestry remains camera-origin;
3. source SHA remains stable;
4. frame/evidence remains 1/1;
5. ForwardMatrix/CameraCalibration state remains unchanged;
6. Scientific Master digest equals the v0.73 result for the same source;
7. exact L0 bits equal v0.73 for the same source;
8. Technical Backplane identity remains equal;
9. tile-read count is reduced;
10. wall/CPU time is measured, not inferred.

Only then can v0.74 claim a physical performance improvement.
