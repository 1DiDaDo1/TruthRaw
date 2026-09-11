# TruthRaw Room ABI v0.2 — Adaptive All-Room Binding report

Status: **RESEARCH_CANDIDATE_CI_PASS**

Branch: `research/room-abi-v0.2-adaptive-all-room-2026-09-11`

Stacked base: Illumination Room v0.2 branch / PR #8 state `2bceb0596f6e57b10aeb88001467124b43c8cc20`.

Validated implementation SHA: `7d2e1a3829117cde559eef38dad07ca7a8ecc0e8`.

Validation on that exact implementation:

- Room ABI v0.2 push run `34550111870` — **SUCCESS**;
- Room ABI v0.2 PR run `34550113743` — **SUCCESS**;
- Documentation Governance push run `34550111956` — **SUCCESS**;
- Documentation Governance PR run `34550113839` — **SUCCESS**.

The validated implementation includes the whole-house admission tests, the concrete `TileNativeDngSource` bridge, and a concrete bounded file-backed `IStreamingSink` exercised through the real Full-Frame Streaming processor.

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
- The new TruthRaw scene has no ISO axis; capture ISO remains provenance/sensor-forward context only where explicitly permitted.

## Concrete TileNativeDngSource bridge

The concrete-source integration test constructs a small valid synthetic DNG container and opens it through the repository's real `TileNativeDngSource` implementation.

It verifies:

- concrete DNG parser/source construction succeeds;
- metadata geometry is bound correctly;
- opening the source does not materialize the complete file or complete RAW;
- bounded RAW tile reads map exactly to the source samples;
- RAW payload I/O is recorded while `fullRawMaterialized` remains false;
- the exact concrete `TileNativeDngSource` object is borrowed by Room ABI v0.2;
- its actual `residentBytesUpperBound()` is copied exactly into endpoint binding;
- source resident memory is counted once together with sink resident memory and active room demand;
- the concrete source bridge cannot reintroduce a scene ISO axis.

## Concrete bounded file streaming sink

The candidate now includes `BoundedRecordFileSink`, a concrete `IStreamingSink` implementation that borrows a caller-owned POSIX file descriptor and writes an append-only typed research transport.

It has a fixed declared resident upper bound of `65536` bytes and does not retain complete SDR, half-log-gain or Stage-2 diagnostic images in memory.

The transport begins with the research magic `TRSINK01`, followed by a frame header and typed tile/block records. It is an integration transport, **not** a production DNG/JPEG/HEIF/AVIF encoder and **not** a scientific-master container.

## End-to-end streaming validation

The validated host path is:

`synthetic valid DNG -> TileNativeDngSource -> StreamingTruthRawProcessor -> BoundedRecordFileSink`

The test links frozen canonical reconstruction v4.7i unchanged plus the existing streaming implementation. It runs with bounded tiles and scientific diagnostics enabled.

Observed test output on the validated implementation:

- `source_resident_bound=4812`;
- `sink_resident_bound=65536`;
- `output_bytes=14336`;
- `sdr_records=12`;
- `gain_records=12`;
- `diagnostic_records=12`;
- `adapter_full_frame_buffers=0`;
- `scene_iso_axis=0`;
- `physicalFrameCount=1`;
- `independentEvidenceCount=1`.

The source remains tile-native after the full two-pass processing session. The sink output is checked without reading the complete output back into memory: the test verifies file size and only the fixed 8-byte magic.

The same end-to-end test passes under:

- GCC Release;
- Clang Release;
- ASan + UBSan.

## Low/high device invariant

The all-room integration test runs the same complete 12-room Building Runtime graph under two device envelopes:

- low: Runtime derives a 32 MiB total working-set budget, one concurrent heavy room and tile 128;
- high: Runtime derives a 256 MiB total working-set budget, up to four concurrent heavy rooms and tile 512.

The validated test reports:

- `low_peak_bytes=10485760`;
- `high_peak_bytes=22020096`;
- `shared_backplane=TRUE`;
- `source_sink_counted_once=TRUE`;
- `scene_iso_axis=ABSENT`.

Both plans borrow the same Technical Backplane identity and preserve identical truth authority. Device class changes scheduling/resources, not the scientific house.

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
- a Backplane with missing zero-line identity;
- invalid sink lifecycle/geometry/count states through the concrete streaming sink contract.

## Failure history

The original all-room candidate first failed on run `34548027485` because an unqualified `Status::Ok` was ambiguous between Room ABI v0.2 and Building Runtime v0.1. That was fixed by explicit qualification; no gate was weakened.

When the concrete file-backed streaming sink was added, run `34549927676` at head `82164be04c9550334c6a372fa989204af63f0a50` again exposed a namespace ambiguity in the new end-to-end test: both canonical `truthraw::Status` and Room ABI v0.2 `Status` were visible. Upstream integrity had passed, GCC failed, and Clang/sanitizers were skipped as intended.

The correction at `7d2e1a3829117cde559eef38dad07ca7a8ecc0e8` fully qualified `truthraw::room_abi::v0_2::Status::Ok`. The verification was also tightened so the test no longer materializes its entire output file merely to validate the stream. No evidence, ISO, zero-line, budget or scientific gate was relaxed.

Both push run `34550111870` and PR run `34550113743` then passed upstream integrity, GCC, Clang and ASan+UBSan.

## Current proof boundary

The host-side source-to-processor-to-bounded-file-sink path is now concrete and CI-tested.

This result still does **not** prove:

- Android RSS;
- allocator fragmentation on device;
- Android thermal behavior;
- real-device throughput;
- Vulkan behavior;
- a production media encoder or final export container;
- FULL_PHYSICAL calibration.

The candidate is execution/resource architecture only. It does not alter canonical reconstruction v4.7i, create new scene evidence, close FULL_PHYSICAL blockers or change the global zero-line.
