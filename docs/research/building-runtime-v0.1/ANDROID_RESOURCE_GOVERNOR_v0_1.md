# Android Resource Governor v0.1

The Building Runtime separates **what is scientifically admissible** from **how much execution space the phone can afford**.

## DeviceEnvelope inputs

The future Android adapter supplies:

- app memory class or an explicit stricter job working-set ceiling;
- currently available memory when known;
- low-RAM classification;
- CPU thread budget;
- optional Vulkan capability;
- foreground/background state;
- thermal state.

The native v0.1 core consumes only these abstract capability values and therefore has no Android SDK dependency.

## Resource tiers

### Low

Typical policy: one heavy-room lease, 128 px tile, no speculative prefetch, no rebuildable-cache retention, CPU correctness baseline.

### Mid

Up to two dependency-compatible heavy-room leases and 256 px tiles. Optional acceleration is allowed only as an execution backend; output semantics remain the same.

### High

Up to four compatible heavy-room leases, 512 px tiles, optional Vulkan preference and rebuildable-cache retention while foreground/thermal state permits.

### Thermal/background pressure

Background execution disables speculative prefetch/cache retention and collapses heavy concurrency to one. Severe/critical thermal state also collapses concurrency to one, forces 128 px tiles and returns preference to the CPU baseline.

## Budget rule

The default Building Runtime budget is at most one eighth of app memory class, optionally tightened by an explicit working-set ceiling and by half of currently available memory. A budget below 4 MiB fails closed with `BudgetTooSmall`.

This is a scheduler lease, not permission for a room to allocate the whole amount unnecessarily. Individual rooms may impose stricter ceilings; Room Capsule already has its own tighter mobile planner.

## Eviction priority

1. UI/preview cache;
2. counterfactual rendered cache;
3. reproducible Room Capsule geometry;
4. appearance intermediates;
5. deterministic scientific derived cache;
6. immutable evidence/master references are retained or safely persisted, never rewritten to satisfy memory pressure.
