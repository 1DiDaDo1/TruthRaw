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

## Branch-local continuation: RGB / LinearRaw DNG restoration — 2026-09-13

When the active branch is:

`research/restore-rgb-linearraw-output-v0.2-workbase-2026-09-13`

read this module immediately after the mandatory architecture documents:

`docs/research/linear-dng-projection-v0.2/README.md`

then read its evidence files:

- `docs/research/linear-dng-projection-v0.2/evidence/WORK_APK_FORENSIC_2026-09-13.md`
- `docs/research/linear-dng-projection-v0.2/evidence/HOST_CI_34780375681_2026-09-13.md`
- `docs/research/linear-dng-projection-v0.2/evidence/ANDROID_CI_34780463331_2026-09-13.md`
- `docs/research/linear-dng-projection-v0.2/evidence/HISTORICAL_ANDROID_PREVIEW_RESTORE_SOURCE_2026-09-13.md`

This branch starts from the actually built 2026-09-13 Android Linear-DNG line (`7c040218f6c5d43c6faeff6ab95d9fec763a1046`) and restores compatibility behavior that existed in the older RGB / LinearRaw work without redesigning the Scientific Master.

Current branch-local restoration rules:

- retain finalized Scientific Preview admission and exact source re-verification;
- retain bounded streaming and do not materialize a full RGB Scientific Master merely for DNG export;
- encode the first restoration candidate through the historically exercised finite `2x` window;
- write `BaselineExposure=+1 EV` for that finite representation;
- preserve source `Make`, `Model` and `UniqueCameraModel` so copied source-bound color metadata remains tied to the source camera identity;
- reject output if original reconstructed RGB still exceeds the finite `2x` window;
- never let export alter Scientific Master hash, zero-line, Backplane, frame/evidence counts or color authority;
- preserve the old v0.1 writer as historical/negative evidence rather than rewriting its history.

Validated on this branch so far:

- GCC Release host test;
- Clang Release host test;
- Clang ASan/UBSan host test;
- Android arm64 debug APK build and v0.2 JNI/native-path verification.

Exact built restoration APK:

- version `0.6-linear-dng-restore`;
- bytes `4335889`;
- SHA-256 `93e51245e0950c5c3140b83f2f3429d2f52ad48adcd5630409134e4c430b5654`.

Do **not** promote those build results into claims of physical Honor success or Lightroom compatibility. The next independent gates are physical device execution, production of an exact v0.2 DNG, Lightroom/reader interoperability and restoration of the historically validated `IFD0 thumbnail + LinearRaw SubIFD0 + JPEG preview SubIFD1` container pattern.

## One-sentence definition

**TruthRaw preserves one sealed RAW observation as immutable evidence, reconstructs a separate uncertainty-aware Scene Master, and executes specialized rooms through a resource-adaptive but scientifically invariant building runtime.**
