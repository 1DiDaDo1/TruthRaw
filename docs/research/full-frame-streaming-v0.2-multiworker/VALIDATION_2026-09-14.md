# Full-Frame Streaming v0.2 Multi-Worker — Host + Android Compile Validation 2026-09-14

Status: **PASS — HOST RESEARCH CANDIDATE + ANDROID COMPILE ADMISSION; NOT YET DEVICE RUNTIME PROMOTION**

This record documents the bounded multi-worker streaming candidate built after recovering the canonical v4.7i multithreading history.

## Validated architecture

The tested execution route is:

`serialized IRawTileSource access`
`-> 2 or 4 compute workers`
`-> worker-private scratch`
`-> deterministic pass-1 integer reduction barrier`
`-> one global ExposurePlan`
`-> parallel pass-2 tile reconstruction/appearance/HDR work`
`-> bounded reorder window`
`-> canonical tile-index sink commit`

The source and sink are deliberately not required to be thread-safe. Source calls are serialized; sink writes are performed by one ordered commit path.

This is the concrete bounded-streaming form of the older v4.7i idea of several photographers working in one room.

## Reference

Reference implementation:

`docs/research/full-frame-streaming-v0.1`

with `workers=1`.

The candidate reuses the same byte-frozen canonical v4.7i reconstruction/appearance code and the same v0.1 streaming mathematics. It changes execution scheduling only.

## First synthetic scheduler regression

Synthetic frame:

- 258 x 194 BGGR;
- per-phase black levels;
- NoiseProfile present;
- gain field present;
- residual row/column black present;
- saturated samples present;
- HDR enabled;
- Stage-2 diagnostics enabled;
- tile core 32, halo 7.

Worker configurations:

- 2 workers;
- 4 workers;
- 4-worker case repeated three additional times.

Required equality against the v0.1 one-worker reference:

- ExposurePlan;
- stage2Over1Count;
- clippedCount;
- pass-1/pass-2 tile counts;
- complete SDR float bytes;
- complete halfLogGain float bytes;
- complete Stage-2 diagnostic float bytes;
- physicalFrameCount=1;
- independentEvidenceCount=1.

Scheduling/resource assertions:

- source concurrency peak = 1;
- sink commit order = canonical tile index;
- reorder occupancy never exceeds configured queue depth;
- queue depth <= 2 * worker count;
- RAW tile reads remain exactly two passes over the tile set.

The first full passing workflow was:

- run id: `34833317060`
- head: `28a13e905dd07600432c259c2ac957a5c809e5b1`
- GCC Release: PASS
- Clang Release: PASS
- Clang ASan/UBSan: PASS

## TileNativeDngSource integration gate

The next gate replaced the synthetic `FrameSource` at the ingress boundary with the real research `TileNativeDngSource` implementation.

Two tests are now part of the v0.2 target:

1. `test_tile_native_source_gate_v0_2.cpp`
   - opens an actual classic TIFF/DNG byte fixture through `TileNativeDngSource`;
   - drives two-pass tile reads from 2 and 4 worker threads behind one source gate;
   - requires exact `uint16_t` tile equality against serial reads;
   - requires source concurrency peak = 1;
   - requires exact TileNative audit read counts;
   - requires `fullRawMaterialized=false` and `fullFileMaterialized=false`.

2. `test_tile_native_multiworker_streaming_v0_2.cpp`
   - uses the actual `TileNativeDngSource` as the input to the reusable v0.2 scheduler;
   - compares a one-worker `StreamingTruthRawProcessor` reference against 2- and 4-worker v0.2 execution;
   - requires exact float-byte equality for SDR, HDR `halfLogGain`, and Stage-2 diagnostics;
   - requires identical global ExposurePlan/counters/tile counts;
   - requires serialized source access and single-frame evidence invariants;
   - verifies that the TileNative adapter still does not materialize the full RAW or full file.

The reusable candidate scheduler is now separated into:

- `multiworker_streaming_v0_2.h`
- `multiworker_streaming_v0_2.cpp`

so it is no longer only test-local scheduling code.

## Current successful host CI

Latest fully passing multi-worker workflow for the TileNative-integrated candidate:

- workflow: `Full Frame Streaming v0.2 Multi-Worker`
- run id: `34834874747`
- tested head: `8de8aab8a4f0fe57e29b1377cd02b3591822e618`
- overall conclusion: `success`

Passing jobs:

- GCC Release: **PASS**
- Clang Release: **PASS**
- Clang ASan/UBSan: **PASS**

That run includes the original synthetic scheduler equivalence test, the TileNative source gate, and the TileNative end-to-end multi-worker streaming equivalence test.

## Android compile admission

The reusable `multiworker_streaming_v0_2.cpp` candidate is now compiled into the Android native bridge as **compile-only research admission**.

It is deliberately **not selected by the production/runtime path yet**. Existing Android scientific/preview calls remain on their validated v0.1 route until real-device 1/2/4-worker evidence is available.

On the same tested head `8de8aab8a4f0fe57e29b1377cd02b3591822e618`, Android workflow:

- `Adaptive UI + Ingress v0.1 APK Build`
- run id: `34834874785`
- native/Gradle APK assembly: **PASS**

This closes the NDK/Android compilation-admission question for the reusable scheduler. It does not close runtime/device performance or artifact-equivalence gates.

## Preserved failure history

Two implementation failures are intentionally preserved.

### C++ most-vexing-parse

An early candidate failed Clang compilation because:

`std::vector<Workspace> p2Workspace(std::size_t(workerCount));`

was parsed as a function declaration.

It was corrected to list initialization and the full matrix was rerun. No warning was disabled to hide it.

### Strict integration-test unused helper

After adding the TileNative end-to-end test, GCC `-Werror` correctly rejected the translation unit because the shared test support's `max_abs_diff()` helper was unused.

The warning policy was not weakened. The exact-byte comparison diagnostic was improved to call `max_abs_diff()` only when a mismatch occurs, preserving strict `-Werror`. The subsequent GCC/Clang/ASan-UBSan matrix passed.

These are implementation defects in candidate/test code, not scientific failures of the multi-worker model.

## What PASS now supports

For the tested host route, PASS supports:

- multiple compute workers behind one serialized real `TileNativeDngSource` reader;
- expensive reconstruction/appearance/HDR tile work overlapping while source access remains single-reader;
- one deterministic integer reduction barrier before the global ExposurePlan;
- one bounded reorder window followed by canonical sink commit;
- 2/4-worker outputs reproducing the one-worker streaming reference byte-for-byte for all tested authoritative float vectors;
- no full RAW/file materialization introduced by worker count;
- HDR remaining downstream of the one global scene/exposure plan, without a second ISO/gain normalization;
- worker count not changing evidence count or scientific authority;
- reusable v0.2 scheduler code compiling successfully in the Android/NDK application target.

## What remains OPEN

This is **not** Android runtime promotion yet.

Still required:

- bind an Android scientific/preview runtime entry point to v0.2 behind an explicit research-only switch;
- run exact 1/2/4-worker comparisons on physical Honor BKQ-N49 DNG input;
- verify real saved output/Scientific Master identity remains exact where required;
- measure wall time, worker CPU time, PSS, queue stalls and thermal state;
- stress repeated runs for scheduling/race stability on device;
- establish explicit concurrent-safety certification for any reconstruction/appearance backend admitted to more than one worker;
- determine device-specific worker admission from CPU capacity, memory and thermal budget;
- only after those gates consider replacing the v0.1 production route.

No canonical v4.7i bytes and no full-frame-streaming-v0.1 correctness bytes were changed by this candidate.

Permanent interpretation:

**One real RAW reader can feed several photographers, and several photographers can feed one deterministic writer. More photographers may change speed, memory and heat; they may never change truth.**
