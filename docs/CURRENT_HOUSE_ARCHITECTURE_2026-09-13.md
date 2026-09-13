# Current TruthRaw House Architecture — 2026-09-13

Status: current project-level house/runtime architecture on the 2026-09-13 fact-check branch.

This document supersedes `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md` as the **global current architecture description**. The 2026-09-10 file remains historical provenance and is not deleted.

## 1. Architectural purpose

The house architecture exists to make two dimensions independent:

1. **scientific authority** — what a component is allowed to observe, reconstruct, claim, modify, or export;
2. **execution resources** — RAM, CPU/GPU availability, tile size, thread count, queue depth, cache budget, and thermal budget.

A stronger device may execute the same science faster. It does not receive more evidence or stronger truth authority.

## 2. The sealed foundation

The original Direct-CFA/source RAW and capture metadata form the sealed foundation/vault.

Properties:

- immutable source identity;
- original sample/capture provenance preserved;
- source clipping remains source clipping;
- source metadata can be interpreted, but not retroactively rewritten to satisfy later modules;
- the single-frame path remains `physicalFrameCount=1`, `independentEvidenceCount=1`.

The Scientific Master is built above this foundation. It is not a replacement for the foundation.

## 3. The 12 logical rooms

### 1. Archivist

Owns sealed source identity, hashes, immutable provenance, historical evidence and failure records.

### 2. MeasurementLab

Owns source-domain interpretation: CFA topology, black/white handling, required source corrections, measurement likelihood, clipping/censoring and source-bound noise information.

### 3. Architect

Owns reconstruction/scene-construction planning and the scientific transformation from measurement-domain evidence toward the reconstructed scene state.

### 4. Restorer

Owns uncertainty-aware restoration and missing-channel reconstruction under explicit evidence/uncertainty constraints. It may reconstruct; it may not relabel reconstruction as measurement.

### 5. SceneRegistry

Owns identities/bindings for the reconstructed scene/master, zero-line/scene-scale association and versioned scene state.

### 6. Surveyor

Owns measurements/diagnostics of the reconstructed scene, confidence, uncertainty, geometry-support descriptors and other scientific summaries whose authority is explicitly declared.

### 7. ManifoldConditioning

Owns numerical conditioning/virtual-observation operations that do not create independent evidence. It may reparameterize one scene to improve numerical behavior, provided all uncertainty/evidence accounting returns to the original measurement root.

### 8. LightingStudioCicm

Owns bounded illumination/counterfactual-light calculations under the available calibration and scene-support level. Relative/appearance relighting and physically calibrated relighting are distinct claim levels.

### 9. RoomCapsule

Owns compact local/ROI scene support for lighting operations: masks, depth/normals/visibility/confidence/material/light descriptors where available. It is intentionally local and rebuildable where possible.

### 10. Colorist

Owns colorimetric and appearance color processing after the scientific scene state. Source-bound color authority remains distinct from independently calibrated physical color.

### 11. Finisher

Owns display/appearance finishing such as tone/contrast/acutance decisions. Finisher output never becomes the Scientific Master.

### 12. Exporter

Owns downstream serialization/projection such as DNG/SDR/HDR/JPEG. Exporter may package identities/provenance; it may not promote scientific authority or create evidence.

## 4. Floors / authority direction

Allowed authority direction is forward-only:

`Foundation -> Measurement -> Reconstruction -> Scene -> Counterfactual -> Appearance -> Projection`

Rooms can communicate through explicit corridor contracts, but a later floor cannot overwrite the epistemic meaning of an earlier floor.

Examples:

- Exporter cannot create `FULL_PHYSICAL`.
- Finisher cannot change the Scientific Master.
- LightingStudio cannot turn a plausible relight into observed evidence.
- ManifoldConditioning cannot count virtual EV/ISO views as extra frames.
- Restorer cannot convert an inferred channel into a measured CFA sample.

## 5. Corridors

A corridor should carry the smallest object that preserves identity and authority.

Preferred corridor payload:

`CorridorToken { sourceId, masterId, tileId/range, authorityFloor, zeroLineBindingId, sceneScaleBindingId, provenanceHandle, uncertaintyHandle, payloadLeaseHandle }`

The token is conceptual until the production ABI is implemented, but the invariant applies now: **do not copy a full image merely to cross a room boundary.**

Pixels should travel as little as possible. Identity and authority should travel exactly.

## 6. Room leases

A production `RoomLease` should define:

- resident byte budget;
- tile core + halo budget;
- scratch allocation budget;
- lifetime/start/end;
- immutable/shared handles it may access;
- disposable caches it may create;
- maximum in-flight tiles;
- CPU/GPU/backend permission;
- thermal/resource profile;
- deterministic completion/commit boundary.

When memory pressure rises, rebuildable caches may be dropped. Immutable source/master identity and scientific provenance may not be rewritten to save memory.

## 7. Execution profiles

Historical target envelopes remain useful as design examples, not universal promises:

- low-tier example: about 32 MiB runtime envelope, one heavy room active, ~128 px tile core;
- high-tier example: about 256 MiB runtime envelope, up to four compatible heavy rooms, ~512 px tile core.

Exact budgets must be measured per implementation/device. The scientific result must not depend on which profile was chosen.

## 8. Heavy/light room classification is dynamic

Do not permanently hard-code “heavy” as a scientific property of a room. It is an execution property of the operation/version.

Examples:

- a scalar CICM forward calculation may be light;
- a full local relighting pass may be heavy;
- ManifoldConditioning can be nearly allocation-free for some operations;
- Exporter can be light when streaming and heavy when a writer incorrectly materializes a full frame.

The resource governor classifies the actual operation, not the room name.

## 9. Full-frame streaming pattern

Preferred large-image route:

`TileSource -> Stage/measurement -> reconstruct -> statistics -> release tile`

then after global plan/statistics are finalized:

`TileSource -> reconstruct -> downstream appearance/projection -> StreamedSink -> release tile`

The key rule is that global statistics may persist compactly, but full-frame scratch images should not persist merely to bridge passes.

The historical full-frame-streaming work showed that very large output dimensions do not inherently require equally large algorithmic working memory.

## 10. Gatehouse

External/proprietary RAW decode is isolated from the Main House.

Route:

`sealed source -> Gatehouse -> decode + audit + topology + provenance + resource check -> sealed persisted handoff -> Gatehouse detach/free -> Main House`

Rules:

- native/direct supported formats keep the shortest path;
- external decoder version is recorded as environment provenance;
- decoder output does not acquire scientific authority merely because the decoder supports a camera name;
- unsupported/ambiguous topology fails closed;
- Gatehouse memory/threads/cache are released before heavy Main House work where practical.

## 11. Scientific Master streaming

The Scientific Master must be consumable as a bounded/tiled scientific state rather than forcing every downstream module to own a full RGB frame.

A digest/identity is computed under a defined canonical ordering/serialization. Optimizations may change execution schedule, but they must not change the master identity when the contract requires bit-exact equivalence.

The validated read optimization is an example: the median schedule changed, but the canonical sample set/median definition and master identity did not.

## 12. Execution fusion without authority fusion

Adjacent operations may share one resident tile to reduce memory traffic/cache misses when all of these hold:

- their scientific order is unchanged;
- intermediate semantics remain auditable;
- each room’s authority remains separately recorded;
- required negative/failure diagnostics are preserved;
- deterministic output remains equal to the reference path where the contract requires equality.

This is **execution fusion**, not permission to merge Measurement, Reconstruction, Appearance, or Projection authority.

## 13. Deterministic parallelism

Before wider parallel execution, define:

- canonical tile IDs/order;
- deterministic reduction order or a mathematically proven order-independent reduction;
- explicit floating-point policy;
- deterministic digest serialization;
- repeatability tests across thread counts/tile sizes where required.

A performance backend is accepted only after it matches the scientific contract of the reference path.

## 14. Per-room profiler

The next runtime implementation should record at minimum:

- wall time;
- CPU time;
- queue wait/stall time;
- tile count;
- source tile reads;
- bytes read/written;
- allocations and peak resident lease;
- cache hit/reuse count;
- backend/thread count/tile core/halo;
- thermal state where available;
- source/master/output identities before and after the operation.

This will allow optimization to target measured bottlenecks instead of guessing.

## 15. Buffer pools and arenas

For repeatable tile operations, prefer bounded pools/arenas:

- pre-size common tile/scratch classes;
- return leases immediately after room completion;
- avoid unbounded per-tile heap churn;
- never allow a shared mutable buffer to blur room ownership or source immutability;
- zero/poison/debug buffers in validation builds when useful for lifetime defects.

## 16. Room Capsule performance contract

Room Capsule should keep local relighting/geometry support compact:

- operate only on selected ROI/zone where possible;
- use lower-resolution geometry only when the declared operation tolerates it;
- retain confidence/uncertainty with the reduced representation;
- keep full-resolution final image/output independent from geometry resolution;
- use boundary illumination descriptors instead of storing a complete outside-world geometry when that approximation is the declared model.

Historical examples such as ~1.1 MB for a particular 12-MP/25%-ROI/quarter-resolution setup and ~4.7 MB for a particular 50-MP setup are design examples, not guarantees.

## 17. CPU reference and acceleration

The C++/CPU route remains the scientific reference floor for current production validation.

A future Vulkan/GPU path may improve execution only. It must not:

- change authority;
- create evidence;
- silently change precision semantics;
- change canonical master/output where bit-exactness is required;
- bypass failed CPU/reference gates.

## 18. DNG export as a sink

Linear DNG and reconstructed-CFA output belong downstream of a finalized master/source binding.

Exporter should consume a streaming/tile interface and should not force full master materialization. Export admission must bind:

- sealed source identity;
- finalized Scientific Master identity;
- zero-line/scene-scale binding;
- Technical Backplane/provenance identity where applicable;
- color-authority level;
- projection role.

Writer validation is separate from scientific validation.

## 19. Technical Backplane

The Backplane is the compact digital backside of the house. It binds identity/provenance/authority without duplicating image payloads.

It may contain compact records such as:

- source identity/seal;
- master identity/digest;
- zero-line/scene-scale binding;
- frame/evidence counts;
- color authority;
- room/projection roles;
- validation/release state.

It is metadata/control-plane state, not hidden new image evidence.

## 20. Current implementation priority

The highest-value general optimization after this audit is:

**Building Runtime vNext = RoomLease + CorridorToken + deterministic tile scheduler + per-room profiler + bounded buffer pools.**

This should be implemented first around already-validated scientific algorithms. Do not rewrite reconstruction/color mathematics merely to fit the runtime; adapt ownership and scheduling around the existing reference outputs and prove equivalence.
