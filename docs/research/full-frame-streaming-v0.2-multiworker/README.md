# TruthRaw Full-Frame Streaming v0.2 Multi-Worker Candidate

**Status: RESEARCH CANDIDATE / NOT PRODUCTION / DOES NOT MODIFY v0.1 OR CANONICAL v4.7i**

This module is the first concrete implementation step after recovering the older canonical v4.7i multithreading history.

It tests a stricter runtime architecture for bounded streaming:

`serialized source access -> N compute photographers -> bounded reorder window -> single ordered sink commit`

The scientific contract remains single-frame:

`physicalFrameCount = 1`

`independentEvidenceCount = 1`

More workers are execution resources only.

## Why this candidate exists

Canonical v4.7i already proved that tile reconstruction, SDR application and half-resolution HDR gain calculation can run with multiple CPU workers. The later `full-frame-streaming-v0.1` intentionally returned to `workers=1` because it added stronger memory/source/sink contracts and had not yet proved concurrent streaming safety.

v0.2 does not simply remove that guard. It introduces the missing scheduling structure while preserving the v0.1 route unchanged as the conservative reference.

## Candidate architecture

### Pass 1

Multiple compute workers claim tiles dynamically. Source reads are serialized through one source mutex, while reconstruction and statistics run outside that mutex. Every worker owns:

- its own `Workspace` scratch;
- private integer histograms;
- private clipping/over-range counters;
- private workspace telemetry.

Workers join at a deterministic reduction barrier. Integer histograms and counters are merged before the one global `ExposurePlan` is created.

### Pass 2

Workers again read source tiles through the serialized source gate and perform the expensive reconstruction/appearance/HDR tile calculations concurrently.

Completed tiles enter a bounded reorder window. The main commit path writes to `IStreamingSink` strictly in canonical tile-index order.

Therefore neither `IRawTileSource` nor `IStreamingSink` is required to be thread-safe for this candidate.

The maximum number of tiles allowed ahead of the ordered sink is bounded by `2 * workerCount` (clamped by tile count). This prevents a slow early tile from allowing later completed tiles to accumulate without limit.

## ISO and HDR dependency rule

ISO/gain/readout/capture-domain interpretation remains upstream measurement work. HDR gain generation occurs after the global scene/exposure plan exists.

This candidate therefore preserves the architectural rule:

> **HDR may execute in parallel downstream; it must not trigger a second ISO/gain normalization.**

## Memory rule

The candidate computes a conservative resident-memory admission bound from:

- source resident upper bound;
- sink resident upper bound;
- per-worker v0.1 workspace estimate multiplied by worker count;
- bounded completed-tile reorder storage.

This is deliberately conservative. A later production planner can tighten the bound, but it may not hide worker-private scratch or queue residency.

## Validation target

The test uses the unchanged v0.1 one-worker streaming processor as the byte-level reference. The v0.2 candidate is run with 2 and 4 workers and must reproduce exactly:

- `ExposurePlan`;
- SDR float bytes;
- half-resolution HDR `halfLogGain` float bytes;
- Stage-2 diagnostic float bytes;
- clipping and Stage-2-over-1 counts;
- tile counts;
- single-frame evidence provenance.

It also requires:

- source concurrency peak exactly 1;
- sink writes in canonical tile order;
- bounded reorder-window occupancy;
- the same two-pass source-read count (no extra scientific reread caused by worker count).

The four-worker case is repeated to look for schedule-dependent drift.

## Current authority boundary

A host-CI PASS is evidence that the candidate scheduler is deterministic and bounded for the synthetic regression and tested compilers/runtimes. It is **not** yet proof that Android/Honor is faster, cooler, or production-safe.

Still OPEN after host PASS:

- TileNativeDngSource integration;
- Android JNI/runtime integration;
- physical Honor 1/2/4-worker exact artifact comparison;
- wall-time/CPU/PSS/thermal profiling;
- long-run stress/race testing on device;
- resource-governor worker admission.

## Promotion law

**One source reader may feed many photographers. Many photographers may feed one ordered writer. The number of photographers may change speed and resource use; it may never change truth.**
