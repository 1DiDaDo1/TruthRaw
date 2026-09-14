# TruthRaw FotoGraaf Parallel Room Execution — 2026-09-14

**Status: RESEARCH EXECUTION ARCHITECTURE / RESOURCE-ONLY / NO CLAIM-AUTHORITY CHANGE**

This document answers whether TruthRaw may use multiple rooms at once and whether one room may contain multiple "photographers" when the device has multiple CPU cores.

The answer is **yes in principle**, with one strict distinction:

- a **room** owns an authority contract and a scientific responsibility;
- a **photographer/worker** is only an execution lane used to perform that room's already-authorized work faster.

Four workers do not create four measurements, four physical frames or four evidence roots.

Permanent laws remain:

**Measured where measured. Reconstructed where necessary. Never invented.**

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

---

## 1. Authority graph versus execution graph

TruthRaw needs two graphs that must never be confused.

### Authority graph

The authority graph defines what must happen before a claim is allowed to exist:

`sealed source -> measurement interpretation -> reconstruction -> Scientific Master seal -> scene analysis -> counterfactual / appearance -> projection`

These dependencies cannot be removed merely because more CPU cores are available.

### Execution graph

The execution graph decides which already-valid operations can run concurrently.

Examples:

- different source tiles can be reconstructed by different workers after the measurement model is frozen;
- independent scene-analysis products may fan out after the Scientific Master is sealed;
- HDR appearance/export may run in parallel with other downstream outputs once their shared scientific dependency is immutable;
- selected RoomCapsule analysis can run beside an unrelated export branch when neither writes back into the Scientific Master.

Resource scheduling therefore changes **when and where computation happens**, never what is scientifically true.

---

## 2. "Four photographers in one room"

The preferred metaphor is:

`one room / one authority contract / N photographer workers`

Each worker receives an immutable work packet such as a tile, ROI or independent diagnostic job. Each worker has its own scratch workspace. The room supervisor merges/commits only outputs that pass the same contract as the single-worker path.

A worker may not:

- invent a new authority class;
- alter source evidence;
- increment physical/evidence counts;
- choose a different scientific algorithm because it runs on a different core;
- feed appearance or counterfactual results back into the Scientific Master;
- publish a partial result as finalized state before the room barrier succeeds.

This is execution parallelism, not evidence parallelism.

---

## 3. Why HDR does not have to be serialized with ISO

ISO/gain/readout belongs to capture-domain measurement interpretation. Display HDR belongs downstream to appearance/projection. Scientific scene-range diagnostics belong downstream to the reconstructed scene and are also distinct from capture ISO.

Therefore the correct dependency is not:

`ISO -> HDR -> reconstruction`

but approximately:

`source -> ISO/gain/exposure interpretation -> reconstruction -> Scientific Master seal`

followed by a fan-out such as:

```text
Scientific Master seal
  |-- Surveyor / FotoGraaf scene-range + light diagnostics
  |-- TRUTHRAW PURE projection
  |-- Finisher HDR/JPG appearance
  |-- RoomCapsule local analysis -> LightingStudioCicm counterfactual
  `-- other evidence-preserving diagnostics
```

Once the capture interpretation and Scientific Master needed by these consumers are immutable, the downstream branches need not wait for each other.

HDR may therefore run on another core without changing ISO, and ISO interpretation does not have to be recomputed merely because HDR output is requested.

---

## 4. Current implementation audit

The current Android finalized/source-bound preview bridge deliberately configures the streaming processor with:

```text
workers = 1
hdrEnabled = true
```

The current `full-frame-streaming-v0.1` correctness contract explicitly rejects any `workers != 1` configuration with the reason that multi-worker streaming remains open. Its regression test also verifies that `workers=2` fails closed.

Therefore:

- the user's multi-core proposal is a real optimization opportunity;
- changing the Android constant from 1 to 4 today would **not** enable safe parallelism; it would make planning fail;
- v0.1 must remain unchanged as the validated single-worker baseline while a separate multi-worker evolution is proved.

---

## 5. Where streaming v0.1 can be parallelized

### Pass 1 — reconstruction + global statistics

Pass 1 processes tiles sequentially and accumulates:

- scene histogram;
- display histogram;
- over-1 count;
- clipped count;
- tile count.

A safe multi-worker design can give every worker its own:

- `Workspace`;
- scene histogram;
- display histogram;
- integer counters;
- status.

After all assigned tiles complete, a deterministic barrier merges worker histograms and counters into the canonical tile/index order or another explicitly fixed exact reduction order.

The histogram bins and counts are integer quantities, so exact schedule-independent merging is achievable. Only after this merge may the global exposure plan/LUT be chosen.

### Pass 2 — tile output

Pass 2 recomputes each tile using the now-fixed global exposure plan, applies appearance, calculates the tile-owned half-resolution HDR-gain cells and writes output.

The current even tile-core contract already gives each 2x2 HDR cell one owner. This makes disjoint tile computation a strong candidate for worker parallelism.

However, the sink interface is not currently declared thread-safe or schedule-independent. A future version should therefore either:

1. compute tiles concurrently but commit them to the sink in canonical tile-index order; or
2. introduce a sink contract with explicitly proven disjoint concurrent writes and deterministic finalization.

The first option is the safer initial implementation.

---

## 6. Current blockers that prevent simply turning on four workers

### B1 — shared workspace

v0.1 uses one mutable `Workspace`. Parallel workers require one workspace per worker.

### B2 — source audit state

`TileNativeDngSource` updates mutable audit counters such as bytes read and tile-read calls. Concurrent calls on the same source would race unless the source/audit model is evolved.

The underlying Android/POSIX byte source uses positional `pread`, which is suitable for independent-offset reads, but that alone does not make the whole `TileNativeDngSource` object thread-safe.

Preferred solution: immutable parsed source state + worker-local read/audit views, followed by deterministic audit reduction.

### B3 — reconstruction backend concurrency contract

The streaming interface does not currently certify that an arbitrary `IReconstructionBackend` instance is thread-safe and schedule-independent.

Initial multi-worker evolution should use worker-local backend instances or an explicit `cloneForWorker`/deterministic-worker contract rather than sharing mutable backend state by assumption.

### B4 — sink concurrency

The current sink API has no concurrency guarantee. Parallel compute must not imply unordered publication.

### B5 — memory accounting

One worker's bounded workspace is not the same as four workers' workspace. The planner must account for approximately `N * workerWorkspace` plus fixed shared state and bounded commit buffers before allowing N workers.

More cores may therefore reduce wall time while increasing peak resident memory.

### B6 — thermals and heterogeneous phone cores

Four workers are not automatically faster than two. Mobile CPUs can contain cores with different performance/efficiency characteristics, share memory bandwidth/cache and reduce frequency under thermal load.

TruthRaw must measure rather than assume the optimum.

---

## 7. Resource Invariance Contract

A multi-worker path is acceptable only if execution resources do not change scientific authority or result identity.

For a fixed:

- sealed source;
- parser/source semantics;
- calibration bindings;
- reconstruction backend/schema;
- scientific configuration;
- tile geometry;
- appearance/output configuration where relevant;

changing worker count or scheduling must not change:

- physical frame count;
- independent evidence count;
- source identity;
- calibration authority;
- Scientific Master identity;
- zero-line/scene-scale identity;
- censoring/support semantics;
- reconstructed values;
- uncertainty/provenance semantics;
- finalized projection contents, except explicitly non-scientific timing/resource telemetry.

**Target: exact authoritative digest equality across 1/2/4 workers.**

Where an implementation would normally introduce floating-point reduction-order differences, TruthRaw must use a fixed deterministic reduction rather than accepting unexplained epsilon drift in scientific identity.

If exact resource invariance cannot be demonstrated, the scientific route fails closed to the validated worker count.

---

## 8. Parallel room scheduling

Inter-room parallelism uses dependency barriers, not unrestricted shared access.

### Barrier A — sealed capture interpretation

MeasurementLab may perform internal parallel diagnostics, but downstream reconstruction starts only after the measurement packet required by reconstruction is finalized.

### Barrier B — Scientific Master seal

Anything claiming to consume the finalized master waits for the exact Scientific Master identity/seal.

After Barrier B, independent read-only consumers may run concurrently.

### Barrier C — counterfactual separation

RoomCapsule/LightingStudioCicm output is never allowed to become an upstream input to the captured-world Scientific Master. Parallel scheduling does not weaken this one-way authority boundary.

### Barrier D — export finalization

Each exported artifact is released only when the exact dependencies and its own certificate/provenance gates have completed. Another concurrent branch failing does not automatically contaminate a scientifically independent artifact, but shared prerequisite failure does.

---

## 9. Adaptive worker policy

Do not hard-code "4" as a scientific constant.

A future `ExecutionResourcePolicy` may choose the worker count from execution-only facts such as:

- available/effective CPU capacity;
- tile count;
- memory budget;
- per-worker workspace bound;
- current thermal state;
- foreground/UI responsiveness budget;
- measured device-specific throughput history.

A conservative form is:

```text
workerCapByMemory = floor((memoryBudget - fixedResident) / perWorkerWorkspace)
workerCount = clamp(resourcePolicyChoice, 1, min(tileCount, workerCapByMemory, validatedWorkerCap))
```

This policy may alter speed and resource consumption only. It may not choose a scientifically weaker reconstruction on a slower phone.

If thermal or memory pressure rises, TruthRaw can reduce workers for subsequent work or future phases. It must not silently alter already-computed scientific semantics.

---

## 10. Initial benchmark matrix

Before live Android multi-worker promotion, benchmark the exact same representative RAW on:

- 1 worker — canonical baseline;
- 2 workers;
- 4 workers;
- optionally the highest validated worker count allowed by the device/resource policy.

For every run record:

- exact output/master digest;
- source/master/zero-line/scene-scale identities;
- wall time;
- worker CPU time;
- peak PSS / logical resident bound;
- bytes read / tile read calls;
- pass-1/pass-2 tile counts;
- UI frame pacing;
- thermal state before/during/after;
- error/fallback state.

Promotion requires exact scientific equivalence first. Speedup is evaluated second.

A configuration that is 1.8x faster but changes the scientific digest is a FAIL. A configuration that is exactly equivalent but slower/thermally worse is scientifically PASS but performance-REJECTED for that resource policy.

---

## 11. Proposed evolution path

Keep `full-frame-streaming-v0.1` frozen as the known single-worker baseline.

Develop a separate multi-worker candidate with these gates:

1. worker-local workspace;
2. worker-local source/audit view;
3. worker-local or explicitly certified reconstruction backend;
4. deterministic pass-1 reduction;
5. parallel pass-2 compute with canonical ordered commit;
6. worker-aware memory planning;
7. 1/2/4-worker exact invariance tests;
8. sanitizer/thread-race tests;
9. actual Android Honor performance/thermal benchmark;
10. only then adaptive worker selection in the app.

No app route changes merely because the prototype compiles.

---

## 12. Decision

**PASS — architecture:** multiple rooms may execute simultaneously after their dependency barriers are satisfied.

**PASS — architecture:** one room may use multiple photographer workers over disjoint/immutable work packets.

**PASS — separation:** HDR appearance does not need to be serialized with ISO interpretation after the required scientific dependencies are frozen.

**OPEN — implementation:** `full-frame-streaming-v0.1` intentionally supports only one worker.

**BLOCKED — production multi-worker route:** until source/audit safety, backend concurrency, deterministic reduction, sink commit ordering, memory accounting and exact 1/2/4-worker invariance are proved.

The intended principle is:

**One truth, one room contract, as many validated workers as the hardware can use safely.**
