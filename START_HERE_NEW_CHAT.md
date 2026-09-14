# START HERE — TruthRaw current bootstrap

This is the authoritative session/bootstrap entry point for the audited TruthRaw research lineage, with the active 2026-09-14 implementation overlay called out explicitly below.

## Active 2026-09-14 implementation overlay

The project-wide audited architecture remains the 2026-09-13 baseline. The current Android/output implementation work lives on:

`research/ui-output-modes-certificate-v0.1-2026-09-14`

Draft PR:

`#23 — Integrate four-mode output policy + TRUTHRAW PURE float32 DNG`

Before changing the current output/UI/certificate work, read:

- `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-14.md`
- `docs/implementation/OUTPUT_MODES_CERTIFICATE_IMPLEMENTATION_2026-09-14.md`
- `docs/research/physical-dng-test-set-v0.1/TRUTHRAW_PHYSICAL_TESTSET_2026-09-14.json`

The handoff records the four-mode product contract, float32 PURE DNG route, certificate state, multilingual/icon work, the five-file physical test set, the last fully green implementation snapshot, and the exact remaining validation gates. Re-fetch the live PR head/workflows before claiming the handoff SHA is still current.

## Mandatory reading order

1. `docs/TRUTHRAW_PROJECT_MAP_2026-09-13.md`
2. `docs/audit/PROJECT_FACT_CHECK_2026-09-13.md`
3. `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-13.md`
4. `docs/CORE_VISION_HOUSE_GENEALOGY_BACKPLANE_GATEHOUSE_2026-09-13.md`
5. `docs/CORE_VISION_SEALED_HOUSE_ARCHITECTURE.md`
6. `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md`
7. `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_EVOLUTION_2026-09-13.md`
8. `state/CURRENT_CANONICAL_STATE_2026-09-13.json`
9. `docs/DOCUMENT_STATUS_INDEX_2026-09-13.md`
10. `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-14.md` when continuing the active output/UI/certificate branch
11. only then: the canonical/research module documents, manifests, tests and evidence relevant to the task

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

## Recovered house rationale is active design context

The current house architecture is not only a performance metaphor. It combines four recovered historical concepts:

- **gezegelde woning / sealed house** — immutable original RAW evidence;
- **alle vrijheid / new house** — richer reconstructed scene representation without stronger evidence claims;
- **achterkant van de foto / Technical Backplane** — compact source/master/zero-line/provenance binding behind the visible image;
- **tussenwoning / Gatehouse** — isolated external RAW decode followed by a sealed handoff and detach before heavy Main-House work.

Read `docs/CORE_VISION_HOUSE_GENEALOGY_BACKPLANE_GATEHOUSE_2026-09-13.md` before changing house/runtime boundaries.

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

The active branch also records a five-file physical regression set in `docs/research/physical-dng-test-set-v0.1/TRUTHRAW_PHYSICAL_TESTSET_2026-09-14.json`. Those DNGs are test evidence, not automatically calibration evidence.

## Current output roles

- Direct CFA = `MEASURED_EVIDENCE`.
- Scientific Master = reconstructed scientific scene state.
- reconstructed CFA DNG = `RECONSTRUCTED_CFA_PROJECTION`.
- 16-bit Linear DNG compatibility writer = `COMPATIBILITY_PROJECTION` from reconstructed RGB Scientific Master.
- TRUTHRAW PURE float32 LinearRaw DNG = high-fidelity scientific **projection** of the same verified Scientific Master into float32 XYZ-D50; it remains downstream representation, not new measurement evidence.
- JPEG/ARGB/HDR = appearance/presentation projection.
- rawsensor = nonstandard internal payload until an explicit ABI/provenance definition makes its role precise.

Do not remosaic reconstructed RGB and call it original sensor RAW. A direct-CFA evidence repack is a separate measured-preserving class.

## Active four-mode UI contract

The current implementation branch exposes:

- `JPG`
- `JPG XL`
- `TRUTHRAW PURE`
- `TRUTHRAW ADVANCED`

`Colourful`, `Detailed`, `Soft`, and `HDR` are downstream appearance controls for JPG, JPG XL and TRUTHRAW ADVANCED only. PURE stays appearance-neutral. JPEG XL stays fail-closed until an encoder is separately validated.

English is the canonical fallback language, with current Android resources for English, Dutch, German and French.

The TruthRaw certificate belongs inside the file/technical backside, not visibly on the image. Until a trusted issuer key exists, certificate status must remain `UNSIGNED DEVELOPMENT`; no private signing key may be committed to the repository or APK.

Read the 2026-09-14 handoff/implementation contract for exact current behavior and remaining blockers.

## Historical/current Scene Master distinction

Do not flatten the project's terminology across time. Early Latent/Scene Master experiments explored richer scene representations and at times colorimetric/device-independent state. The current project-level Scientific Master is specifically reconstructed **camera-native RGB before the normal `camera_to_xyz()` route and before appearance**.

History is provenance, not permission to silently redefine the current Scientific Master.

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

The highest-value general runtime step remains:

**RoomLease + CorridorToken + deterministic tile scheduler + per-room profiler + bounded buffer pools.**

Implement this around already-validated algorithms first. Measure wall/CPU/stall time, tile/source reads, bytes, allocation/peak resident lease, queue wait, cache reuse, tile size/thread count/backend and source/master/output identities.

Execution fusion is allowed only when it keeps adjacent tile data resident without merging their scientific authority roles.

CPU/reference behavior remains the validation floor. Vulkan/GPU may later be an optional execution backend only.

For the active output/UI branch, however, the immediate continuation priority is the validation sequence in `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-14.md`: real-device five-DNG PURE export, certificate extraction/integrity, interoperability, appearance-profile semantics, JPEG XL encoder validation, then trusted signing.

## Read-optimization fact

The physical read-optimization run reduced exact tile reads from 13,824 to 7,680 and improved source-read payload and runtime. It did **not** prove a memory improvement because peak PSS rose slightly. Do not repeat the performance result as a RAM claim.

## Gatehouse / external RAW

External/proprietary RAW decode is isolated:

`sealed source -> Gatehouse decode/audit/topology/provenance/resource check -> sealed handoff -> detach/free Gatehouse -> Main House`

Lifecycle shorthand:

`ATTACHED -> SEALED_HANDOFF -> DETACHED -> MAIN_HOUSE_ACTIVE`

A decoder's camera support/version is tooling provenance, not scientific authority. Unsupported or ambiguous topology fails closed. The Gatehouse is a **tussenwoning**, not a second scientific source or alternate Main House.

## Technical Backplane / achterkant van de foto

The visible image is the front-facing projection. The Technical Backplane is the compact digital backside that binds source identity, master identity, zero-line/scene-scale, evidence counts, authority and projection status.

It is not hidden image evidence and must not duplicate or secretly replace the Scientific Master.

> **Preview is a window onto TruthRaw, never the source of TruthRaw.**

## DNG/export boundary

DNG writer correctness and scientific correctness are separate gates.

Linear DNG should be a bounded/streaming downstream sink from the finalized RGB Scientific Master. Reconstructed CFA must remain explicitly reconstructed. Export may not create photons, extra frames/evidence, a stronger color claim, or a different master/zero-line merely through serialization.

Android `DngCreator` is appropriate for RAW_SENSOR/Bayer-style carriage; it is not by itself proof of the intended RGB LinearRaw semantics. Native RGB LinearRaw writer validation therefore remains a separate responsibility.

The current PURE route intentionally uses the native float32 Scientific Master LinearRaw writer and preserves negative and greater-than-one components. The older 16-bit route remains compatibility-only.

## Current repository lineage warning

The observed 2026-09-13 Android-DNG-export and RGB-LinearRaw-restoration research heads were divergent in Git ancestry during this audit. Do not infer completeness from branch names. Compare actual trees and preserve unique commits before any promotion/merge.

The audit branch itself was created from verified base:

`research/restore-rgb-linearraw-output-v0.2-2026-09-13`

at:

`0e8291f3300b5a1aa1ffe7652be5908da57a889e`

The active four-mode/certificate branch is layered on the audited project branch rather than redefining canonical science. PR `#23` remains draft until the remaining real-device/interoperability/signing/profile gates are satisfied.

## One-sentence definition

**TruthRaw preserves one sealed RAW observation as immutable evidence, optionally passes unsupported external formats through an isolated Gatehouse, reconstructs a separate uncertainty-aware Scientific/Scene Master, binds that state through a compact Technical Backplane and in-file provenance certificate, represents the scene without inheriting arbitrary source-container limits, and exposes JPG/JPG XL/TRUTHRAW PURE/TRUTHRAW ADVANCED as downstream product surfaces without allowing presentation choices to rewrite scientific authority.**
