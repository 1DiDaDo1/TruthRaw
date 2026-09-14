# TruthRaw FotoGraaf Parallel Execution — code audit — 2026-09-14

**Status: CODE-BASED FINDING / DESIGN INPUT / NO ROUTE PROMOTION**

This audit records an important implementation finding discovered while evaluating the "multiple photographers in one room" idea.

## 1. The idea already has a partial canonical precedent

The byte-frozen v4.7i in-memory processor exposes `ProcessOptions::threads` and implements tile workers.

Its current pattern is structurally close to the proposed FotoGraaf worker model:

- `workerCount = min(requested threads, tile count)` with a lower bound of one;
- a separate worker object per execution lane;
- worker-local histograms and temporary reconstruction/color/appearance buffers;
- an atomic `nextTile` index distributing tiles among workers;
- disjoint writes into the final full-frame output for each tile core;
- worker-local `over1` and clipped counts;
- a barrier (`join`) before global histogram/counter reduction and exposure-plan construction;
- additional range-parallel loops for SDR application and HDR half-gain generation.

The v4.7i research reconstruction backend also uses thread-local scratch for its internal green-plane work.

This means the project does **not** need to invent the core concept from zero. The new problem is to transfer the same principle into the bounded streaming route without violating its stricter source/audit, memory and sink contracts.

## 2. Why current Android is still single-worker

The Android finalized/source-bound preview uses `full-frame-streaming-v0.1`, not the full-frame in-memory processor.

Its preview options currently set:

```text
workers = 1
hdrEnabled = true
```

and streaming v0.1 explicitly rejects any worker count other than one. Its regression test asserts that `workers=2` fails closed.

Therefore the current Android route is intentionally serial at the streaming-processor worker level even though canonical v4.7i contains a proven architectural example of tile-level threading.

## 3. What can be reused conceptually

The following v4.7i principles should be carried forward:

1. worker-local scratch/state rather than one shared mutable workspace;
2. tile ownership independent from worker identity;
3. fixed barrier before global exposure/HDR planning;
4. integer histogram/counter reduction after the worker barrier;
5. resource worker count as an execution parameter, never a truth parameter;
6. even tile core for unambiguous half-resolution HDR-cell ownership when parallel;
7. per-pixel/core output ownership so two workers never author the same scientific sample.

## 4. What cannot simply be copied

The streaming route adds constraints absent or weaker in the in-memory path:

- `TileNativeDngSource` contains mutable audit counters;
- the streaming sink has no current concurrency contract;
- bounded memory accounting must multiply worker-local workspace correctly;
- source reads and output commits must remain bounded and auditable;
- a streaming worker cannot assume a full destination frame exists;
- exact artifact/scientific digest invariance must be checked against the single-worker streaming baseline;
- Android thermal/UI behavior matters to resource policy.

The POSIX source uses `pread`, so the low-level file-descriptor read itself is offset-independent. That does not make the complete current `TileNativeDngSource` thread-safe because its audit state is mutable and non-atomic.

## 5. Resulting implementation direction

The most compact scientifically safe design is not "four rooms for one task". It is:

`one authority room -> N worker-local execution lanes -> deterministic room barrier/commit`

and, after immutable scientific barriers:

`one sealed Scientific Master -> multiple independent downstream rooms/branches concurrently`.

This gives two independent forms of parallelism:

- **intra-room parallelism**: multiple photographers work on separate tiles/ROIs;
- **inter-room parallelism**: independent downstream rooms run at the same time.

These may be combined, but a global resource scheduler must prevent every room from independently spawning the maximum number of workers and oversubscribing the device.

## 6. New scheduler requirement: one resource governor

If four CPU lanes are useful, TruthRaw must not let three simultaneous rooms each create four workers and accidentally schedule twelve heavy workers on a four-core budget.

A future `ExecutionResourceGovernor` should allocate a bounded global worker budget across runnable rooms.

Example only:

```text
available validated worker slots = 4

Surveyor          2 slots
PURE projection   1 slot
HDR Finisher      1 slot
-------------------------
total             4 slots
```

or, when one latency-critical room dominates:

```text
Restorer          4 slots
other dependent rooms wait for the master barrier
```

Allocation is dynamic and performance-oriented. It may never change the scientific algorithm or authority class.

## 7. Status

**PASS:** the "four photographers in one room" model matches an existing v4.7i tile-worker pattern.

**PASS:** HDR work is separable from ISO/capture interpretation once the required upstream state is frozen.

**PASS:** intra-room and inter-room parallelism can coexist under one global resource governor.

**OPEN:** streaming-v0.1 has not yet been evolved to multi-worker execution.

**BLOCKED:** Android promotion above one streaming worker until deterministic 1/2/4-worker equivalence, source/audit safety, bounded memory, sink ordering and real-device performance/thermal gates pass.
