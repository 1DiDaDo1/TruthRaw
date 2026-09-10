# TruthRaw Building Runtime v0.1

Research orchestration layer for the TruthRaw **building** architecture.

The runtime is the building's corridor, floor-permission and resource-leasing system. It does **not** reconstruct pixels, invent evidence, perform relighting, change uncertainty, or promote scientific claims. Those responsibilities remain inside their own rooms.

## Building model

- **Foundation** — sealed capture evidence and immutable provenance.
- **Rooms** — specialized processing/research workers.
- **Corridors** — typed, forward-only hand-offs carrying handles/authority rather than copied full-frame image payloads.
- **Floors** — epistemic/claim boundaries; they do not represent hardware power.
- **Technical room** — resource governor for RAM, CPU, thermal state and optional acceleration.

## Current 12-room graph

`Archivist -> MeasurementLab -> Architect`

From the Architect the graph branches into the scientific Scene/Surveyor path and later into counterfactual and appearance/output rooms:

1. Archivist
2. Measurement Lab
3. Architect
4. Restorer
5. Scene Registry
6. Surveyor
7. Manifold Conditioning
8. Lighting Studio / CICM
9. Room Capsule
10. Colorist
11. Finisher
12. Exporter

The room name is an execution role, not a claim that the corresponding algorithm is scientifically promoted. `RoomStatus` remains authoritative.

## Two independent axes

### Truth/authority axis

`Foundation -> Measurement -> Reconstruction -> Scene -> Counterfactual -> Appearance -> Projection`

A corridor may move forward to a higher output/claim domain. It may not route Appearance or Counterfactual state backward into Scientific rooms. More phone power never grants a room more truth authority.

### Resource axis

A `DeviceEnvelope` controls execution only:

- low tier: one heavy room, 128 px tiles, CPU baseline;
- mid tier: up to two compatible heavy rooms, 256 px tiles;
- high tier: up to four compatible heavy rooms, 512 px tiles, optional Vulkan;
- severe/critical thermal state collapses heavy concurrency to one and the CPU baseline.

The same admissible room decisions are used on all tiers. Stronger phones may execute more rooms concurrently or retain more rebuildable cache, but scientific semantics do not change.

## Hard invariants

- `physicalFrameCount == 1`.
- `independentEvidenceCount == 1`.
- `scientificMasterModified == false`.
- Physical Capture EV, Best Conditioning EV and Appearance EV are distinct fields.
- Unknown sigma is never interpreted as zero uncertainty.
- Scientific rooms cannot read/write Appearance or Counterfactual state.
- The runtime cannot increase evidence, mutate the input scientific master, or auto-promote a claim.
- Dependency graph must be acyclic and floor-monotone.
- Corridor tokens contain identity/authority plus an external artifact handle; no image-sized payload lives in the scheduler.
- Fixed-size hot-path structures avoid heap allocation and dynamic module discovery.

## Scope

v0.1 proves the orchestration/resource contracts implemented here. It does not prove photographic superiority, physical relighting, calibrated sensor noise, missing-channel truth, Android/JNI integration or GPU parity.
