# Full-Frame Streaming v0.2 Multi-Worker — Host Validation 2026-09-14

Status: **PASS — HOST RESEARCH CANDIDATE**

This record documents the first passing bounded multi-worker streaming candidate built after recovering the canonical v4.7i multithreading history.

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

## Reference

Reference implementation:

`docs/research/full-frame-streaming-v0.1`

with `workers=1`.

The candidate reuses the same byte-frozen canonical v4.7i reconstruction/appearance code and the same v0.1 streaming mathematics. It changes execution scheduling only.

## Exact regression conditions

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

## CI result

Workflow:

`Full Frame Streaming v0.2 Multi-Worker`

Successful run:

- run id: `34833317060`
- head: `28a13e905dd07600432c259c2ac957a5c809e5b1`
- overall conclusion: `success`

Passing jobs:

- GCC Release: PASS
- Clang Release: PASS
- Clang ASan/UBSan: PASS

The sanitizer job completed the same equivalence test with no reported sanitizer failure.

## Preserved failure history

An earlier candidate run failed during Clang compilation because

`std::vector<Workspace> p2Workspace(std::size_t(workerCount));`

was parsed as a function declaration (the C++ most-vexing-parse case). The failure was not hidden or weakened with a compiler flag. The source was corrected to list initialization:

`std::vector<Workspace> p2Workspace{std::size_t(workerCount)};`

and the full GCC/Clang/sanitizer matrix was rerun successfully.

This compiler failure is an implementation defect in the first candidate, not a scientific failure of multi-worker scheduling.

## What PASS means

PASS supports these claims for the tested host candidate:

- multiple compute workers can be placed behind a serialized source reader;
- expensive reconstruction/appearance/HDR tile work can overlap;
- a bounded reorder window can preserve a single ordered sink;
- 2/4-worker scheduling can reproduce the one-worker streaming reference byte-for-byte for all tested scientific/output float vectors;
- HDR remains downstream of the one global scene/exposure plan and does not require repeating ISO/gain interpretation;
- worker count does not change evidence count or scientific authority.

## What remains OPEN

This is not yet Android production promotion.

Still required:

- integrate the scheduler with TileNativeDngSource/JNI rather than the synthetic FrameSource;
- verify the real DNG source adapter remains single-reader under 2/4 compute workers;
- bind a real streaming DNG/preview sink to the ordered-commit path;
- run exact 1/2/4-worker comparisons on Honor BKQ-N49 physical DNG input;
- measure wall time, CPU time, PSS, queue stalls and thermal state;
- determine device-specific worker admission from memory and thermal budget;
- verify PURE Scientific Master/artifact identity is unchanged where exact identity is required.

No canonical v4.7i bytes and no full-frame-streaming-v0.1 correctness bytes were changed by this candidate.

Permanent interpretation:

**The old v4.7i idea of several photographers in one room is now re-demonstrated under a bounded streaming model: one serialized source, multiple compute workers, one deterministic ordered writer.**
