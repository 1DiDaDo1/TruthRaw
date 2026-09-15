# START HERE — TruthRaw current bootstrap

This file originated as the authoritative renewed-house bootstrap on 2026-09-10. A **2026-09-15 continuity layer** has now been added so that recent FotoGraaf/Camera2 work is not mistaken for the origin or whole direction of TruthRaw.

## Mandatory reading order

1. `docs/PROJECT_HISTORY_AND_FOTOGRAAF_CONTEXT_2026-09-15.md` — historical/project-wide continuity; this does not replace module-local scientific authority
2. `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md`
3. `docs/CORE_VISION_SEALED_HOUSE_ARCHITECTURE.md`
4. `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md`
5. `state/CURRENT_CANONICAL_STATE_2026-09-10.json`
6. `docs/DOCUMENT_STATUS_INDEX_2026-09-10.md`
7. only then: the current claim/status documents and canonical/research module documents relevant to the task

Do **not** bootstrap from `CURRENT_CANONICAL_STATE_2026-09-06.json`, `...2026-09-08.json`, `...2026-09-09.json`, or `docs/PROJECT_STATE_AUDIT_2026-09-08.md`. Those are preserved historical snapshots.

Do **not** infer global project direction merely from the last recoverable Camera2/FotoGraaf experiment. The recent camera-heavy conversations are execution history inside the newer acquisition/metrology layer. Their interruption points are not architectural milestones. Use `docs/PROJECT_HISTORY_AND_FOTOGRAAF_CONTEXT_2026-09-15.md` to place them in the larger lineage.

## Non-negotiable scientific rules

The source RAW/CFA + capture metadata are immutable sealed evidence. The scientific Scene Master is a separate reconstructed state.

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

Measured, reconstructed, censored/unknown, counterfactual and appearance data remain distinct. `physicalFrameCount=1` and `independentEvidenceCount=1` remain the single-frame evidence invariants.

For positive physical light, TruthRange may use `T = log2(L/L0)`. The zero-line `L0` is a gauge/reference; it is not sensor black, absolute darkness, DNG BlackLevel or clipping. Signed scene-linear estimates remain distinct from the positive-light log coordinate.

Source ISO/shutter remain immutable capture provenance. Virtual EV/ISO reparameterization does not create information. Counterfactual illumination/capture may create hypothetical measurements only inside the explicitly counterfactual world; it never retroactively creates evidence.

Appearance must never modify the scientific master. DNG/LinearRaw/export is a compatibility/presentation projection, not the scientific master.

APK/GCam/computational-RAW content must not determine TruthRaw evidence, calibration, topology, color, noise model or architecture. GCam/MotionCam material may be retained as **historical precursor/provenance**, but it has no authority to redefine Direct-CFA evidence or physical calibration.

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

## FotoGraaf naming rule

Do not conflate these three layers:

1. **Fotograafkamer** — historical/counterfactual photographic-light concept; mature descendants include CICM/Room Capsule/Lighting Studio/appearance floors.
2. **Broad FotoGraaf** — photography + metrology + physical calibration trajectory connecting controlled capture to TruthRaw.
3. **Android FotoGraaf** — concrete Camera2 acquisition/metrology application before reconstruction; proves physical route/sample-domain evidence and collects data for calibration.

Android FotoGraaf is a subsystem of the larger trajectory, not a replacement for the Scientific Master, House architecture or calibration programme.

## Current implementation boundary

Building Runtime v0.1 and Room Capsule v0.1 are integrated on the renewed-house lineage. CICM v1 and Manifold Conditioning v1 remain bounded research components in that lineage.

The 2026-09-10 text below historically described Technical Backplane v0.1 and the all-room adaptive resource layer as active research candidates. **Do not use that dated sentence alone to infer present promotion state.** Resolve current authority from the module's own later CI/promotion/status evidence.

The canonical v4.7i public API on the renewed-house baseline still owns full-frame `DecodedDngFrame.raw` and `ProcessResult.sdrRgb` vectors. Any later branch that changes ownership/streaming must be evaluated against its own equivalence and integrity evidence; do not silently rewrite canonical v4.7i history.

Recent Android acquisition work has stronger physical-route and raw-buffer evidence than this 2026-09-10 bootstrap originally contained. That later acquisition evidence does not by itself promote a calibration domain or redefine the reconstruction core. In particular, maximum-resolution capability/session support is not equivalent to proof of an actually delivered 16320x12288 Direct-CFA frame.

## One-sentence definition

**TruthRaw preserves one sealed RAW observation as immutable evidence, reconstructs a separate uncertainty-aware Scene Master, and executes specialized rooms through a resource-adaptive but scientifically invariant architecture; FotoGraaf is the later acquisition/metrology layer used to prove and calibrate the physical origin of that evidence.**
