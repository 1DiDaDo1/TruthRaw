# START HERE — TruthRaw current bootstrap

This is the authoritative session/bootstrap entry point for the renewed TruthRaw house as of 2026-09-10.

## Mandatory reading order

1. `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md`
2. `docs/CORE_VISION_SEALED_HOUSE_ARCHITECTURE.md`
3. `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md`
4. `state/CURRENT_CANONICAL_STATE_2026-09-10.json`
5. `docs/DOCUMENT_STATUS_INDEX_2026-09-10.md`
6. only then: the canonical/research module documents relevant to the task

Do **not** bootstrap from `CURRENT_CANONICAL_STATE_2026-09-06.json`, `...2026-09-08.json`, `...2026-09-09.json`, or `docs/PROJECT_STATE_AUDIT_2026-09-08.md`. Those are preserved historical snapshots.

## Non-negotiable scientific rules

The source RAW/CFA + capture metadata are immutable sealed evidence. The scientific Scene Master is a separate reconstructed state.

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

Measured, reconstructed, censored/unknown, counterfactual and appearance data remain distinct. `physicalFrameCount=1` and `independentEvidenceCount=1` remain the single-frame evidence invariants.

For positive physical light, TruthRange may use `T = log2(L/L0)`. The zero-line `L0` is a gauge/reference; it is not sensor black, absolute darkness, DNG BlackLevel or clipping. Signed scene-linear estimates remain distinct from the positive-light log coordinate.

Source ISO/shutter remain immutable capture provenance. Virtual EV/ISO reparameterization does not create information. Counterfactual illumination/capture may create hypothetical measurements only inside the explicitly counterfactual world; it never retroactively creates evidence.

Appearance must never modify the scientific master. DNG/LinearRaw/export is a compatibility/presentation projection, not the scientific master.

APK/GCam/computational-RAW content must not determine TruthRaw evidence, calibration, topology, color, noise model or architecture.

`canonical/ptc/v1.1` is **Pure Truth Certificate**. Do not infer photon-transfer calibration from that path name.

No silent LICENSE. Preserve failed/rejected experiments and their provenance.

## Renewed house execution rules

The Building Runtime owns orchestration; algorithms retain their own scientific contracts.

- **Truth floor** controls epistemic permission.
- **Resource profile** controls only RAM/CPU/GPU/tile/cache execution.
- Cheap phones may run one heavy room at a time with small tiles and disposable caches.
- Strong phones may run more compatible rooms in parallel, use larger tiles/caches and optional acceleration.
- Hardware capability never upgrades scientific claims.
- Corridors carry handles/provenance rather than duplicate full-frame payloads.
- Zero-line/source/master/scene-scale identity belongs to one immutable shared backplane binding, not per-pixel duplication.
- Rebuildable caches may be evicted; immutable evidence/master identity may not be rewritten to satisfy memory pressure.
- Resource policy may be re-derived between room operations, never halfway through an indivisible scientific operation.

## Current implementation boundary

Building Runtime v0.1 and Room Capsule v0.1 are integrated on the current repository lineage. CICM v1 and Manifold Conditioning v1 remain bounded research components in that lineage.

Technical Backplane v0.1 and the all-room adaptive resource layer are active research candidates; do not describe them as main-promoted until their own repository CI/promotion gates pass.

The canonical v4.7i public API still owns full-frame `DecodedDngFrame.raw` and `ProcessResult.sdrRgb` vectors. The next production-memory migration is a validated streaming/tile-source/sink adapter. Do not silently rewrite canonical v4.7i before equivalence and integrity gates pass.

## One-sentence definition

**TruthRaw preserves one sealed RAW observation as immutable evidence, reconstructs a separate uncertainty-aware Scene Master, and executes specialized rooms through a resource-adaptive but scientifically invariant building runtime.**
