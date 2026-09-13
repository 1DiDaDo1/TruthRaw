# START HERE — TruthRaw current bootstrap

This is the authoritative session/bootstrap entry point for the audited TruthRaw research lineage as of 2026-09-13.

## Mandatory reading order

1. `docs/TRUTHRAW_PROJECT_MAP_2026-09-13.md`
2. `docs/audit/PROJECT_FACT_CHECK_2026-09-13.md`
3. `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-13.md`
4. `docs/CORE_VISION_SEALED_HOUSE_ARCHITECTURE.md`
5. `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md`
6. `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_EVOLUTION_2026-09-13.md`
7. `state/CURRENT_CANONICAL_STATE_2026-09-13.json`
8. `docs/DOCUMENT_STATUS_INDEX_2026-09-13.md`
9. only then: the canonical/research module documents, manifests, tests and evidence relevant to the task

Do **not** bootstrap from older `CURRENT_CANONICAL_STATE_*.json`, the 2026-09-10 house/index, or `docs/PROJECT_STATE_AUDIT_2026-09-08.md`. They remain preserved historical snapshots/evidence.

## Non-negotiable scientific rules

The source RAW/CFA + capture metadata are immutable sealed evidence. The Scientific/Scene Master is a separate reconstructed state.

**Measured where measured. Reconstructed where necessary. Never invented.**

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

Measured, reconstructed, censored/unknown, counterfactual, appearance and projection data remain distinct. `physicalFrameCount=1` and `independentEvidenceCount=1` remain the normal single-frame evidence invariants.

For positive physical light, TruthRange may use `T = log2(L/L0)`. The zero-line `L0` is a gauge/reference; it is not sensor black, absolute darkness, DNG BlackLevel, clipping or display middle grey.

The TruthRange address space may be mathematically unbounded while sensor evidence remains finite/noisy/quantized/censored. Never convert this into a claim of infinite physical sensor dynamic range.

Source ISO/shutter remain immutable capture provenance and may remain relevant to sensor/noise/forward models. Relative `SELF_GAUGE` TruthRange need not use them inside its coordinate definition. Virtual EV/ISO reparameterization does not create information or extra evidence.

Appearance must never modify the Scientific Master. DNG/LinearRaw/JPEG/HDR output is a downstream compatibility/presentation projection unless an explicit contract says otherwise.

APK/GCam/computational-RAW content must not determine TruthRaw evidence, calibration, topology, color, noise model or architecture.

`canonical/ptc/v1.1` is **Pure Truth Certificate**. Do not infer photon-transfer calibration from that path name.

No silent LICENSE. Preserve failed/rejected experiments and their provenance.

## Scientific authority direction

The global authority order is:

`Foundation -> Measurement -> Reconstruction -> Scene -> Counterfactual -> Appearance -> Projection`

A downstream floor may not silently strengthen or rewrite an upstream claim.

Examples:

- reconstruction is not measurement;
- a preview is not source evidence;
- a virtual EV/ISO view is not an extra frame;
- export cannot create `FULL_PHYSICAL`;
- a structurally valid DNG does not automatically prove scientific correctness;
- a digest proves identity under its serialization, not physical truth by itself.

## Current source-bound physical reference

The physically exercised finalized-preview/read-optimization reference remains the HONOR BKQ-N49 MotionCam DNG:

- `IMG_260830_143012_297_014.dng`
- 4080 x 3072
- SHA-256 `fac842110cfd5b527aae2fa46237824d444aa5701017c2b68021962ebce47da3`

The finalized source-bound scientific preview may be release-allowed while `scientificClaimAllowed=false`. Source metadata color is not automatically independent physical calibration.

## Current output roles

- Direct CFA = `MEASURED_EVIDENCE`.
- Scientific Master = reconstructed scientific scene state.
- reconstructed CFA DNG = `RECONSTRUCTED_CFA_PROJECTION`.
- Linear DNG = `COMPATIBILITY_PROJECTION` from reconstructed RGB Scientific Master.
- JPEG/ARGB/HDR = appearance/presentation projection.
- rawsensor = nonstandard internal payload until an explicit ABI/provenance definition makes its role precise.

Do not remosaic reconstructed RGB and call it original sensor RAW. A direct-CFA evidence repack is a separate measured-preserving class.

## Renewed house execution rules

The Building Runtime owns orchestration; algorithms retain their scientific contracts.

The 12 logical rooms are Archivist, MeasurementLab, Architect, Restorer, SceneRegistry, Surveyor, ManifoldConditioning, LightingStudioCicm, RoomCapsule, Colorist, Finisher and Exporter.

- **Truth floor** controls epistemic permission.
- **Resource profile** controls RAM/CPU/GPU/tile/cache execution only.
- Cheap phones may run fewer heavy operations simultaneously and use smaller tiles/caches.
- Strong phones may run more compatible operations in parallel and use larger bounded caches.
- Hardware capability never upgrades scientific claims or changes canonical science.
- Corridors should carry handles/provenance/authority rather than duplicate full-frame payloads.
- Zero-line/source/master/scene-scale identity belongs to compact immutable shared state/backplane, not per-pixel duplication.
- Rebuildable caches may be evicted; immutable evidence/master identity may not be rewritten to satisfy memory pressure.
- Resource policy may be re-derived between indivisible scientific operations, never midway through one in a way that changes its result.

## Next production optimization

The highest-value general runtime step is:

**RoomLease + CorridorToken + deterministic tile scheduler + per-room profiler + bounded buffer pools.**

Implement this around already-validated algorithms first. Measure wall/CPU/stall time, tile/source reads, bytes, allocation/peak resident lease, queue wait, cache reuse, tile size/thread count/backend and source/master/output identities.

Execution fusion is allowed only when it keeps adjacent tile data resident without merging their scientific authority roles.

CPU/reference behavior remains the validation floor. Vulkan/GPU may later be an optional execution backend only.

## Read-optimization fact

The physical read-optimization run reduced exact tile reads from 13,824 to 7,680 and improved source-read payload and runtime. It did **not** prove a memory improvement because peak PSS rose slightly. Do not repeat the performance result as a RAM claim.

## Gatehouse / external RAW

External/proprietary RAW decode is isolated:

`sealed source -> Gatehouse decode/audit/topology/provenance/resource check -> sealed handoff -> detach/free Gatehouse -> Main House`

A decoder's camera support/version is tooling provenance, not scientific authority. Unsupported or ambiguous topology fails closed.

## DNG/export boundary

DNG writer correctness and scientific correctness are separate gates.

Linear DNG should be a bounded/streaming downstream sink from the finalized RGB Scientific Master. Reconstructed CFA must remain explicitly reconstructed. Export may not create photons, extra frames/evidence, a stronger color claim, or a different master/zero-line merely through serialization.

Android `DngCreator` is appropriate for RAW_SENSOR/Bayer-style carriage; it is not by itself proof of the intended RGB LinearRaw semantics. Native RGB LinearRaw writer validation therefore remains a separate responsibility.

## Current repository lineage warning

The observed 2026-09-13 Android-DNG-export and RGB-LinearRaw-restoration research heads were divergent in Git ancestry during this audit. Do not infer completeness from branch names. Compare actual trees and preserve unique commits before any promotion/merge.

The audit branch itself was created from verified base:

`research/restore-rgb-linearraw-output-v0.2-2026-09-13`

at:

`0e8291f3300b5a1aa1ffe7652be5908da57a889e`

## One-sentence definition

**TruthRaw preserves one sealed RAW observation as immutable evidence, reconstructs a separate uncertainty-aware Scientific/Scene Master, represents that scene without inheriting arbitrary source-container limits, and executes specialized rooms through a resource-adaptive but scientifically invariant runtime.**
