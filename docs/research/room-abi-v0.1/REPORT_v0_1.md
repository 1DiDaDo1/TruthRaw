# Room ABI v0.1 — Engineering Report

## Goal

Turn the promoted 12-room Building Runtime into an executable memory/lifetime boundary without changing canonical scientific algorithm bytes.

## Implemented adapters

### v4.7i resource bridge
Copies an existing `truthraw::ProcessOptions` and changes only `tile.core` and `threads` from Building Runtime `ResourcePolicy`. Halo, HDR flag, appearance profile, diagnostics and LUT size remain unchanged.

### Manifold Conditioning
Calls the real upstream `choose_best_conditioning_gauge`, `condition_exact`, and `decondition_exact` sequence. It receives no memory host and therefore cannot allocate through Room ABI.

### Local Illumination Room Capsule
Consumes the real upstream `RoomCapsulePlan`. Packed geometry is `RebuildableCache`; vector-boundary and tile-workspace storage are `TransientScratch`. The full `estimatedPeakBytes` planner reservation is checked before any lease is requested.

## Fail-closed execution rules

- one physical frame and one independent evidence source are required;
- an already modified scientific master is rejected;
- malformed floor/claim/room enum values are rejected;
- Room Capsule requires Counterfactual-floor authority;
- positive leases require non-zero effective runtime budget;
- actual host-granted capacity must fit the effective room budget;
- owner/class/alignment/capacity/lease-id mismatches are rejected and active bad leases are released;
- planner peak under-accounting is rejected before allocation.

No failure path changes evidence counts, truth floor, claim status, or scientific method.

## Local pre-CI validation

Using deliberately minimal local test doubles only for syntax/runtime hardening:

- GCC Release warning gate: PASS;
- Clang Release warning gate: PASS;
- Clang ASan/UBSan: PASS;
- textual output identical across all three;
- wrong-floor Room Capsule request: PASS fail-closed;
- zero-budget positive lease: PASS fail-closed;
- host overgrant above runtime budget: PASS rejected and released;
- malformed enum payloads: PASS fail-closed;
- planner full-peak reservation: PASS;
- `leaseRequests=5`, `leaseReleases=5` in the frozen local harness.

These local stubs are **not** repository integration evidence. The promotion gate remains real-source GitHub CI against the exact current upstream blobs.

## Preserved negative findings

1. Initial 8 MiB automatic stack arena: FAIL before ABI execution; corrected to static backing storage.
2. Old Room Capsule stub after planner-peak gate: expected FAIL because it omitted upstream metadata headroom; stub corrected only after exact upstream inspection.
3. Pre-CI host-overgrant audit: contract gap found and closed by validating actual granted capacity against effective runtime budget.

## Current upstream state

The candidate is rebased conceptually from the old local base `d7987e2...` to current `main` `c5a51610fbb7750c1e8bf95e97c4b1349fbaec7e`. The exact Building Runtime, Manifold Conditioning, Room Capsule and v4.7i blobs used by the original adapter have not drifted.

## Open production work after v0.1

The next ABI extension should bind the already promoted Tile-Native DNG Source and Full-Frame Streaming handles so source/sink lifetime, transient tile workspace and output ownership use the same external-memory contract. S-curve/output-acutance image buffers, Android allocator backend and optional Vulkan resource parity remain separate future gates.

## Status

`RESEARCH_CANDIDATE_PENDING_REAL_UPSTREAM_CI`
