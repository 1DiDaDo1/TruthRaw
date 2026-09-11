# TruthRaw Room ABI v0.2 — Adaptive All-Room Binding

Status: **RESEARCH_CANDIDATE_CI_PASS — STACKED ON ILLUMINATION ROOM v0.2**

Validated implementation: `7d2e1a3829117cde559eef38dad07ca7a8ecc0e8`.

Validation on that implementation:

- Room ABI v0.2 push run `34550111870` — **SUCCESS**;
- Room ABI v0.2 PR run `34550113743` — **SUCCESS**;
- Documentation Governance push run `34550111956` — **SUCCESS**;
- Documentation Governance PR run `34550113839` — **SUCCESS**.

This module generalizes the resource/lifetime strategy first proven by the photographer-selected Illumination Room to the whole TruthRaw house without changing scientific authority.

Stacked base: Illumination Room v0.2 PR #8 branch state. Canonical reconstruction v4.7i is not modified.

## Core rule

A cheaper phone and a stronger phone may execute the house differently, but they may not inhabit different scientific realities.

Resource tier may change:

- tile size;
- CPU thread count;
- number of concurrently open heavy rooms;
- transient workspace size;
- whether rebuildable caches are retained;
- optional acceleration backend.

Resource tier may not change:

- source evidence identity;
- scientific-master identity;
- TruthRange zero-line identity;
- scene-scale identity;
- physical-frame count;
- independent-evidence count;
- room truth authority;
- the presence/absence of scientific evidence;
- ISO semantics of the new scene.

## ISO-free scene contract

The new TruthRaw house has no ISO scene axis.

ISO is allowed only as:

1. immutable source-capture provenance where a room explicitly needs acquisition context; or
2. `CALIBRATED_SENSOR_FORWARD_ONLY` inside CICM's separately calibrated hypothetical sensor-mode model.

ISO may never become:

- a Scene Master coordinate;
- a TruthRange coordinate;
- a zero-line input;
- corridor authority;
- a resource-policy input;
- appearance EV;
- counterfactual illumination scale;
- room identity.

The all-room ABI has compile-time/tests plus per-room profiles that fail closed if scene ISO authority is reintroduced.

## One shared backside / lineage

Every room in one execution lineage borrows the same validated Technical Backplane v0.1 state. The ABI does not duplicate source/master/zero-line/scene-scale identities per room and carries no pixel payload in the Backplane binding.

## Streaming bridge

Room ABI v0.1 did not own the Tile-Native Source / Full-Frame Streaming source-sink bridge. v0.2 closes that gap by borrowing the existing `IRawTileSource` and `IStreamingSink` interfaces and recording their `residentBytesUpperBound()` values.

Source and sink resident memory are counted **once** in the whole-house admission calculation, not once per room.

The ABI does not own or materialize source/sink frames.

### Concrete source-side validation

The validated v0.2 test suite opens a synthetic valid DNG through the repository's real `TileNativeDngSource`, reads bounded RAW tiles and binds that exact source object into Room ABI v0.2.

The test confirms that the concrete source does not materialize the full DNG file or full RAW, and that its actual reported resident upper bound is counted exactly once in whole-house admission.

### Concrete bounded streaming sink

v0.2 now includes `BoundedRecordFileSink`, a caller-owned-file-descriptor implementation of the real `IStreamingSink` interface.

It writes an append-only research transport directly as typed records and does not retain a full SDR frame, half-resolution gain frame, or scientific-diagnostic frame in memory. Its declared resident upper bound is fixed at **65,536 bytes (64 KiB)**.

The validated end-to-end host path is now:

`TileNativeDngSource -> StreamingTruthRawProcessor -> BoundedRecordFileSink`

The processor is linked against frozen canonical reconstruction v4.7i for this integration test. Canonical v4.7i bytes are not modified.

Measured validation output on the validated implementation includes:

- concrete source resident upper bound: `4812` bytes for the synthetic fixture;
- concrete sink resident upper bound: `65536` bytes;
- emitted transport bytes: `14336`;
- SDR records: `12`;
- half-log-gain records: `12`;
- Stage-2 diagnostic records: `12`;
- adapter-owned full-frame buffers: `0`;
- scene ISO axis: `0` / absent;
- physical frame count remains `1`;
- independent evidence count remains `1`.

The research transport is **not** a DNG/JPEG/HEIF/AVIF media encoder and must not be presented as one. It proves a concrete bounded source-to-processor-to-sink streaming contract, not final user-facing export format support.

## Existing scheduler remains authoritative

v0.2 does not invent a second scheduler. Building Runtime v0.1 still determines:

- which rooms execute;
- dependency order;
- execution waves;
- maximum concurrent heavy rooms;
- backend preference;
- per-heavy-room lease ceiling.

Adaptive All-Room Binding then checks whether the concrete room demands plus source/sink resident bounds actually fit that execution plan.

This closes a v0.1 gap where `perHeavyRoomBudgetBytes` existed but source/sink resident memory was not admitted together with all active room demand.

## Room workspace classes

Candidate profiles declare one of:

- `NONE`;
- `BOUNDED_TRANSIENT`;
- `REBUILDABLE_CACHE`;
- `MIXED_BOUNDED`;
- `ZERO_HIDDEN_ALLOCATION`.

Manifold Conditioning preserves its zero-hidden-allocation contract. A nonzero demand for such a room fails closed.

Room Capsule remains mixed bounded/rebuildable workspace. Architect, Restorer, Surveyor, Colorist and Finisher are modelled as bounded heavy rooms whose exact upstream adapters must report concrete demand before execution.

## Conservative wave memory accounting

For each Building Runtime wave, v0.2 accounts:

`source resident + sink resident + retained rebuildable caches + active transient workspace + active rebuildable workspace`

When `retainRebuildableCaches=false`, rebuildable room state is assumed releasable at the room/wave boundary.

When `retainRebuildableCaches=true`, prior rebuildable caches remain in the conservative resident upper bound until the plan ends.

Any wave above `totalWorkingSetBudgetBytes` is rejected.

Any individual heavy-room demand above its Runtime lease is rejected.

A non-heavy room cannot silently allocate workspace because Building Runtime v0.1 grants it no heavy lease.

## Dependencies

- Building Runtime v0.1
- Full-Frame Streaming v0.1
- Tile-Native DNG Source v0.1
- Technical Backplane v0.1
- Room ABI v0.1 upstream contracts
- Illumination Room v0.2 ISO-free scene contract
- CICM v1
- Room Capsule v0.1
- Manifold Conditioning v1
- canonical reconstruction v4.7i linked unchanged in the end-to-end host test

## Scientific and validation boundary

This module is execution/resource architecture. It does not improve reconstruction truth, invent scene information, calibrate ISO, close physical colour/light calibration, or promote counterfactual observations to evidence.

Host CI now proves the declared C++/memory-accounting contracts and a concrete bounded source-to-processor-to-file-sink path under GCC, Clang and ASan+UBSan. It still does **not** prove:

- Android RSS or allocator fragmentation;
- Android thermal behavior;
- real-device throughput;
- Vulkan behavior;
- a user-facing production DNG/JPEG/HEIF/AVIF encoder;
- FULL_PHYSICAL calibration.

The global zero-line remains unchanged and shared through the Technical Backplane; the new scene remains ISO-free by contract.
