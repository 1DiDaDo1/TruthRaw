# TruthRaw Room ABI v0.2 — Adaptive All-Room Binding report

Status: **RESEARCH_CANDIDATE_CI_PASS**

Branch: `research/room-abi-v0.2-adaptive-all-room-2026-09-11`

Stacked base: Illumination Room v0.2 branch / PR #8 state `2bceb0596f6e57b10aeb88001467124b43c8cc20`.

Validated implementation SHA: `6649aa9e5683eaccafaefd70b26ab8879bfbb10a`.

Room ABI v0.2 GitHub Actions run: `34548564916` — **SUCCESS**.

Documentation Governance run on the same SHA: `34548567536` — **SUCCESS**.

The validated implementation now includes a concrete `TileNativeDngSource` binding test in addition to the original whole-house dummy-endpoint admission test.

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

## Concrete TileNativeDngSource bridge

The second integration test constructs a small valid synthetic DNG container and opens it through the repository's real `TileNativeDngSource` implementation.

It verifies:

- concrete DNG parser/source construction succeeds;
- metadata geometry is bound correctly;
- opening the source does not materialize the complete file or complete RAW;
- a bounded 4x4 RAW tile is read and maps exactly to the source samples;
- RAW payload I/O is recorded while `fullRawMaterialized` remains false;
- the exact concrete `TileNativeDngSource` object is borrowed by Room ABI v0.2;
- its actual `residentBytesUpperBound()` is copied exactly into the endpoint binding;
- source resident memory is counted once together with sink resident memory and active room demand;
- the concrete source bridge cannot reintroduce a scene ISO axis.

The sink in this test is intentionally a bounded non-materializing test implementation of `IStreamingSink`. No production streaming sink has yet been established in the repository, so this result must not be described as a complete production source-to-sink session.

## Low/high device invariant

The all-room integration test runs the same complete 12-room Building Runtime graph under two device envelopes:

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

The corrected pre-source implementation `3daf140f...` passed all compilers/sanitizers. The later concrete-source implementation `6649aa9e...` also passed upstream integrity, GCC Release, Clang Release and ASan+UBSan with both all-room tests enabled.

## Current proof boundary

The real `TileNativeDngSource` source side is now integrated and CI-tested. The sink side remains a bounded test implementation of the real `IStreamingSink` interface because no production sink has yet been identified in the repository.

This result still does not prove Android RSS, allocator fragmentation, thermal behavior, Vulkan behavior, or real-device throughput.

The candidate is execution/resource architecture only. It does not alter canonical reconstruction v4.7i, create new scene evidence, close FULL_PHYSICAL blockers or change the global zero-line.
