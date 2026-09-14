# TruthRaw v4.7i Multithreading History + vNext Runtime Plan — 2026-09-14

**Status: ACTIVE DESIGN HISTORY / SOURCE-GROUNDED EXECUTION ANALYSIS / NO CANONICAL BYTE CHANGE**

This document records an important fact that had become easy to overlook during the later bounded-streaming work:

> **The older full canonical v4.7i in-memory processor already contains real multithreaded tile workers and parallel SDR/HDR post-processing.**

That behavior is not hypothetical and is not newly invented by the later FotoGraaf metaphor. The metaphor **"one room, multiple photographers"** is a clearer architectural interpretation of an execution capability that already existed in canonical v4.7i.

This document also records the improvements required before carrying that model into the current bounded Android streaming runtime.

---

## 1. Source of truth

The relevant canonical implementation is:

`canonical/reconstruction/v4.7i/native/src/core.cpp`

with interface/options in:

`canonical/reconstruction/v4.7i/native/include/truthraw/core.h`

The canonical bytes remain frozen. This research layer documents and tests them without rewriting them.

The interface exposes:

`ProcessOptions::threads`

with default `1`, plus `hdrEnabled`, tile policy and appearance settings.

Therefore thread count was already an explicit execution parameter of the full v4.7i processor.

---

## 2. What the old full processor actually parallelizes

### 2.1 Tile reconstruction workers

The processor first builds the tile list and derives:

`workerCount = clamp(options.threads, 1, tileCount)`

For more than one worker it requires an even tile core. The stated reason in code is that parallel half-gain scheduling must preserve ownership of the 2x2 HDR cells.

Each worker owns private reusable scratch/state:

- Stage-2 tile buffer;
- camera-RGB reconstruction buffer;
- XYZ buffer;
- neutral RGB buffer;
- appearance/look buffer;
- private display histogram;
- private scene histogram;
- private over-1 and clipped counters;
- private peak-workspace tracking;
- private status/timing accumulation.

Work distribution uses an atomic `nextTile` counter. Workers repeatedly claim the next tile until the tile queue is exhausted.

The scientific tile chain inside each worker is:

`source RAW tile`
`-> Stage-2 black/white/gain interpretation`
`-> measured-preserving reconstruction`
`-> camera RGB`
`-> XYZ D50`
`-> neutral linear RGB`
`-> appearance tile`
`-> output/core statistics`

The workers are then joined at a barrier before the global scene/exposure plan is finalized.

### 2.2 Deterministic global reduction

After all tile workers complete, their private histogram/counter state is reduced into one global display histogram, one global scene histogram, one over-1 count and one clipped count.

This matters: parallel workers are an **execution optimization**, while the global exposure/scene plan is still derived once from the complete frame statistics.

The scientific dependency remains:

`parallel local evidence processing`
`-> barrier`
`-> one global ExposurePlan`

not:

`each worker invents its own global scene interpretation`.

### 2.3 Parallel SDR application

After the one global `ExposurePlan` and monotone LUT are created, the full processor calls the generic `parallel_ranges(...)` helper across image rows.

This means the SDR application step is also already multithreaded.

Every worker handles a disjoint row range, so this stage is naturally data-parallel once the shared global plan is immutable.

### 2.4 Parallel HDR gain calculation

The full processor also calculates the half-resolution HDR gain field in parallel with the same thread count.

After the censor mask is expanded and the global exposure plan is fixed, `parallel_ranges(...)` divides the half-resolution rows between workers. Each worker computes independent `halfLogGain` cells from:

- local SDR luminance;
- local half-resolution scene maximum;
- the one shared `sceneToDisplayScalar`;
- HDR gate start/full thresholds;
- evidence confidence;
- maximum allowed HDR gain;
- censor state.

The result is stored as `log2(gain)`.

Therefore the old processor already demonstrates the principle that **HDR projection work does not need to be serialized through ISO/gain interpretation once the scene/exposure state has been finalized**.

ISO/gain belongs upstream to measurement interpretation. HDR gain calculation is a later scene/display operation.

---

## 3. Why the even tile-core rule matters

The full processor writes shared half-resolution state (`halfSceneMax` and censor ownership) while tile workers are active.

With even tile cores aligned on even tile origins, a 2x2 source-pixel cell is not split between adjacent core tiles. That makes one tile the owner of the corresponding half-resolution HDR cell and avoids two workers updating the same half cell.

This is an important historical implementation trick, but it should not be copied blindly into vNext. vNext should make ownership explicit in the scheduler/contract rather than rely only on tile parity as an implicit concurrency safety mechanism.

---

## 4. What has already been re-validated in 2026-09-14

The research harness:

`docs/research/worker-resource-invariance-v0.1/test_worker_resource_invariance_v0_1.cpp`

runs the byte-frozen v4.7i processor at:

`threads = 1, 2, 4`

using the same synthetic BGGR frame, same reconstruction backend, same appearance backend, HDR enabled and scientific diagnostics enabled.

It requires **exact byte equality** for:

- complete `ExposurePlan`;
- dimensions/orientation;
- provenance fields;
- `sdrRgb`;
- `halfLogGain`;
- `stage2Diagnostic`.

The four-worker case is repeated additional times to look for scheduling-dependent drift.

Timing/resource telemetry is intentionally excluded from scientific identity: faster/slower execution is allowed; different truth is not.

The corresponding CI matrix compiles/runs the test under:

- GCC Release;
- Clang Release;
- Clang ASan/UBSan.

This is the correct regression gate for the phrase:

**One truth, one room contract, as many validated workers as the hardware can safely use.**

---

## 5. Why the current bounded streaming path is still single-worker

The newer `full-frame-streaming-v0.1` route was built for bounded resident memory and explicit source/sink contracts.

Its validation deliberately contains:

`if (options.workers != 1) -> fail closed`

with the reason that multi-worker streaming remains open.

That restriction does **not** mean TruthRaw lost or disproved multithreading. It means the newer route introduced additional concurrency questions that the older full-frame in-memory processor did not need to solve:

- is the tile source safe for simultaneous reads?
- can decoder state be shared safely?
- can output tiles arrive out of order?
- can the sink commit concurrently?
- how is deterministic output ordering guaranteed?
- how is memory bounded when every worker owns scratch?
- how are per-worker histograms reduced reproducibly?
- how does thermal throttling change worker admission without changing science?

Keeping v0.1 single-worker was therefore scientifically conservative.

---

## 6. Improvement plan: do not simply copy the old thread model

The old v4.7i design proves that the mathematics can be parallelized. The production vNext runtime should preserve that strength while improving execution control.

### Improvement A — persistent room worker pool

The old full processor creates/joins thread groups for multiple phases.

vNext should use a persistent worker pool owned by the Building Runtime / RoomLease layer.

Benefits:

- less thread creation/join overhead;
- simpler thermal/core admission changes between operations;
- reusable per-worker scratch arenas;
- one place to profile queue wait and CPU utilization;
- easier CPU-affinity/performance-core policy later if validated.

This must remain a resource optimization only.

### Improvement B — explicit tile ownership instead of parity-only safety

Keep the proven rule that no two workers may write the same authoritative tile/cell, but encode ownership explicitly:

`TileTask { tileId, coreRect, haloRect, halfStateRect, ownerSequence }`

A scheduler can then prove that output and half-resolution HDR cells have one writer.

Even tile cores can remain a simple/fast admissible configuration, but concurrency correctness should be a scheduler invariant rather than an undocumented side effect of parity.

### Improvement C — producer / compute-worker / ordered-commit streaming

For sources or sinks that are not concurrently safe, do not force them to become multithreaded.

Use a compact pipeline:

`single/sealed source reader`
`-> bounded tile queue`
`-> N compute photographers`
`-> deterministic reorder/commit queue`
`-> single ordered sink writer`

This allows 2-4 reconstruction workers even when the file decoder and DNG writer remain serialized.

The queue depth must be bounded by the RoomLease memory budget.

### Improvement D — per-worker scratch leases

The old processor already uses worker-private buffers, which is the correct direction.

vNext should formalize that as:

`WorkerScratchLease`

with a known byte upper bound derived from tile core + halo + reconstruction/appearance requirements.

The memory governor should choose:

`workerCount <= floor(availableScratchBudget / scratchPerWorker)`

in addition to CPU/thermal constraints.

A high-end phone may therefore run four photographers while a low-memory device may run one or two, without changing any scientific decision.

### Improvement E — deterministic reductions

Integer histogram bins/counters are naturally mergeable, but vNext must still define the canonical reduction order.

Recommended:

- worker-local integer histograms;
- fixed worker/tile-ID reduction order;
- no atomic floating-point accumulation for scientific summaries;
- canonical serialization/digest after the reduction barrier.

For any future floating-point reduction, use a fixed reduction tree or another explicitly proven deterministic policy.

### Improvement F — phase-aware concurrency

Do not send every task through one giant serial corridor.

Respect dependencies instead:

`Capture/Measurement interpretation`
`-> parallel reconstruction tiles`
`-> deterministic global reduction/barrier`
`-> finalized Scientific Master / Scene plan`
`-> independent downstream branches where allowed`

After the Scientific Master and required shared scene plan are sealed, independent consumers may execute concurrently when resource budgets allow, for example:

- PURE projection/export;
- FotoGraaf scene-metrology analysis;
- HDR/appearance branch;
- RoomCapsule preparation;
- certificate/provenance preparation that only reads finalized identities.

This is **room-level concurrency** in addition to **worker-level concurrency inside one room**.

### Improvement G — HDR is downstream from ISO interpretation

The scheduler must preserve the architectural distinction:

- ISO/gain/readout/capture-domain interpretation belongs to MeasurementLab / capture metrology;
- reconstruction uses that interpreted evidence;
- HDR scene-range analysis and presentation gain are downstream consumers of the scene state.

Therefore HDR need not wait for unrelated repeated ISO work, and it must never cause ISO normalization to be re-applied.

### Improvement H — thermal/resource adaptation only at safe barriers

Worker count may be changed between indivisible phases/tilesets based on:

- available cores;
- thermal state;
- RAM budget;
- battery/performance policy;
- queue pressure.

It must not change halfway through a mathematical reduction or in any way that changes the authoritative answer.

Safe adaptation points are explicit barriers between operations.

---

## 7. Recommended production architecture

The compact vNext model is:

`BuildingRuntime`
`  -> ResourceGovernor`
`  -> RoomLease`
`  -> persistent WorkerPool`
`  -> deterministic TileScheduler`
`  -> bounded WorkerScratchLease[N]`
`  -> deterministic ReductionBarrier`
`  -> ordered Commit/Sink`

This supports both forms of concurrency discussed in the FotoGraaf model:

1. **multiple photographers in one room** — many workers executing the same scientific contract on disjoint tiles;
2. **multiple compatible rooms at once** — independent downstream consumers operating concurrently after their shared prerequisites are sealed.

Neither form creates another physical frame or another evidence root.

---

## 8. Promotion gates before Android multi-worker streaming

Do not promote bounded Android streaming beyond one worker until all of these are demonstrated:

- exact 1/2/4-worker scientific equivalence;
- race-free sanitizer runs;
- deterministic output ordering/digests;
- source-reader concurrency contract or serialized producer contract;
- sink concurrency contract or ordered single-writer commit;
- bounded resident-memory proof for N workers;
- physical Honor run at 1/2/4 workers;
- wall/CPU/PSS/thermal measurements;
- no regression in source reads or output artifact identity;
- no change to `physicalFrameCount=1` / `independentEvidenceCount=1`;
- PURE Scientific Master digest unchanged across worker counts where the contract requires exact identity.

Performance claims require device measurements. Host CI proves equivalence/safety under tested host runtimes, not phone speedup.

---

## 9. Project decision

The project should no longer describe multithreading as merely a future possibility.

The correct historical/current statement is:

> **Canonical v4.7i already implemented true multithreaded tile reconstruction, parallel SDR application and parallel half-resolution HDR-gain calculation. The later bounded streaming route intentionally returned to one worker as a conservative correctness/memory baseline. TruthRaw vNext should recover multi-worker execution through explicit RoomLease, worker-scratch, ownership, deterministic-reduction and ordered-streaming contracts rather than by modifying canonical v4.7i.**

Status:

- canonical v4.7i multithreaded tile workers: **EXISTS / HISTORICAL PASS**;
- canonical v4.7i parallel SDR apply: **EXISTS**;
- canonical v4.7i parallel HDR half-gain: **EXISTS**;
- 1/2/4 host exact-equivalence regression: **ACTIVE CI GATE**;
- bounded streaming multi-worker production route: **OPEN**;
- physical Android speed/thermal proof: **OPEN**;
- scientific authority increase from more workers: **REJECTED**.

Permanent law:

**More cores may give more photographers. They may never give more truth.**
