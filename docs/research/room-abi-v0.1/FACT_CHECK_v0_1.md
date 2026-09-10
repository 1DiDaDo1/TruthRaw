# Fact check — Room ABI v0.1

## What the current repository actually supports

1. Canonical reconstruction v4.7i exposes tile-core/halo/thread policy and pointer-based tile backends, while its historical outer frame/result ownership was `std::vector` based.
2. Full-Frame Streaming v0.1 and Tile-Native DNG Source v0.1 now provide separate validated streaming/output and tile-native input work, but Room ABI v0.1 does not yet claim ownership of those handles.
3. Manifold Conditioning v1 is scalar/span based and has no heap ownership in its public ABI.
4. Local Illumination Room Capsule v0.1 computes packed-geometry, boundary, tile-workspace, peak, and budget byte counts.
5. Building Runtime v0.1 separates truth-floor authority from execution placement and per-heavy-room memory budgets.

## Consequence

A first executable memory boundary does not require rewriting scientific kernels. A narrow adapter can translate existing resource plans into explicit leases while keeping corridor provenance and scientific authority separate.

## Hardening added before real-upstream CI

The pre-CI audit found three execution-boundary gaps and closed them without changing scientific algorithms:

- invalid `TruthFloor`, `ClaimStatus`, or `RoomId` enum payloads are rejected;
- Room Capsule memory requires a `Counterfactual` truth-floor token, so a valid-looking Scene-floor token cannot enter that room;
- lease admission is checked against the **actually granted capacity**, not merely `minimumBytes`, and the effective room budget is the minimum of total-working-set and per-heavy-room limits. A zero positive-lease budget fails closed.

## Non-claims

- This does not make all current modules allocation-free.
- This does not yet bind Tile-Native DNG Source or Full-Frame Streaming into the ABI.
- This does not prove Android allocator performance or Vulkan parity.
- This does not promote any research-only scientific room.
- Releasing a rebuildable cache is valid only when deterministic inputs/recipe remain available; the cache itself is not evidence.

## Preserved negative findings

### Stack arena
The first local syntax/runtime harness placed an 8 MiB fake lease arena on the process stack and crashed before ABI execution. It was moved to static storage. This remains recorded because mobile stack footprint is itself a production constraint.

### Planner peak under-accounting
The first draft accounted only the three concrete Room Capsule buffers. Exact upstream inspection showed that `estimatedPeakBytes` also includes fixed metadata/headroom. v0.1 therefore admits the room against the full planner peak while materializing only real buffers.

### Stale local planner stub
After the full-peak gate was introduced, the old local Room Capsule stub failed because it omitted the upstream metadata headroom. Only the stub was corrected. Stub success remains non-integration evidence.

### Host overgrant gap
The pre-CI hardening audit found that `minimumBytes <= budget < preferredBytes` could allow a host to return `preferredBytes` above the runtime budget. The contract now rejects and releases such an overgrant. The gate was strengthened; no budget was relaxed.
