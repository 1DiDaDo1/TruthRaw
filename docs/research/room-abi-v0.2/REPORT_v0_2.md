# TruthRaw Room ABI v0.2 — Adaptive All-Room Binding report

Status: **RESEARCH_CANDIDATE_CI_PASS**

Branch: `research/room-abi-v0.2-adaptive-all-room-2026-09-11`

Stacked base: Illumination Room v0.2 branch / PR #8 state `2bceb0596f6e57b10aeb88001467124b43c8cc20`.

Validated implementation SHA: `3daf140f3df0be0f38bdc14c82a4b791b92776ee`.

Room ABI v0.2 GitHub Actions run: `34548187649` — **SUCCESS**.

Documentation Governance run on the same SHA: `34548187651` — **SUCCESS**.

## Validated contracts

- Building Runtime v0.1 remains the only execution scheduler and truth-admission graph.
- Adaptive All-Room Binding performs resource/resident admission over the existing Runtime waves rather than replacing them.
- One validated Technical Backplane state is borrowed and shared by all room bindings in the lineage.
- Existing `IRawTileSource` and `IStreamingSink` handles are borrowed; the ABI does not own or materialize their frames.
- Source and sink `residentBytesUpperBound()` values are counted once in whole-house resident accounting.
- Per-room concrete transient/rebuildable demand is compared against Building Runtime's granted heavy-room lease ceiling.
- Non-heavy rooms cannot silently create a hidden workspace because Runtime v0.1 grants them no heavy lease.
- Rebuildable cache retention follows `ResourcePolicy::retainRebuildableCaches` and is carried conservatively into later wave resident bounds when enabled.
- Manifold Conditioning remains `ZERO_HIDDEN_ALLOCATION` and any nonzero demand fails closed.
- Every wave must fit `totalWorkingSetBudgetBytes` after source, sink, retained cache and active room demand are combined.

## Low/high device invariant

The integration test runs the same complete 12-room Building Runtime graph under two device envelopes:

- low: 256 MiB app memory class, 4 CPU threads, low-RAM; Runtime derives 32 MiB total budget, one concurrent heavy room and tile 128;
- high: 2048 MiB app memory class, 12 CPU threads, optional Vulkan; Runtime derives 256 MiB total budget, up to four concurrent heavy rooms and tile 512.

Both plans borrow the same Technical Backplane object and preserve identical truth authority. Device class changes scheduling/resources, not the scientific house.

## ISO-free all-room contract

Compile-time and runtime contracts require:

- no nominal ISO member in `RoomBindingProfile`, `RoomResourceDemand`, `AdaptiveAllRoomRequest`, `AdaptiveAllRoomPlan`, `StreamingEndpointBinding` or `SharedLineageBinding`;
- no scene ISO axis;
- no corridor ISO authority;
- no resource-policy dependence on scene ISO;
- appearance EV is not ISO;
- counterfactual illumination scale is not ISO;
- CICM may retain nominal ISO only inside its separately calibrated sensor-forward model;
- calibrated sensor-forward ISO never promotes into scene state.

Candidate room profiles allow capture ISO only as immutable provenance read access where explicitly declared. `LightingStudioCicm` uses `CALIBRATED_SENSOR_FORWARD_ONLY`.

Any profile that attempts to write scene ISO or make ISO affect scene coordinates/resource policy fails closed with `ISO_SCENE_VIOLATION`.

## Negative tests

The validated test suite rejects:

- `Colorist` attempting to write scene ISO;
- nonzero hidden workspace for Manifold Conditioning;
- a room demand larger than the Runtime lease;
- source + sink resident memory larger than the whole low-tier budget;
- a Backplane with missing zero-line identity.

## Failure history

The first CI run `34548027485` failed in GCC after upstream integrity had passed. Review identified an ambiguity in the test helper where unqualified `Status::Ok` could refer to either Room ABI v0.2 or Building Runtime v0.1. The test was changed to explicitly qualify Building Runtime status values. No scientific/resource gate was weakened.

The corrected implementation `3daf140f...` then passed upstream integrity, GCC Release, Clang Release and ASan+UBSan.

## Current proof boundary

The source/sink bridge is compiled against the actual Full-Frame Streaming v0.1 interfaces, but the v0.2 integration test currently uses bounded dummy implementations of those interfaces. Therefore this PASS does not yet prove a concrete TileNativeDngSource + production streaming sink session.

It also does not prove Android RSS, allocator fragmentation, thermal behavior, Vulkan behavior, or real-device throughput.

The candidate is execution/resource architecture only. It does not alter canonical reconstruction v4.7i, create new scene evidence, close FULL_PHYSICAL blockers or change the global zero-line.
