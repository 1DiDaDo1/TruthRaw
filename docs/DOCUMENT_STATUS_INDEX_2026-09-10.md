# DOCUMENT STATUS INDEX — 2026-09-10

Status: **AUTHORITATIVE DOCUMENT-GOVERNANCE INDEX**

Purpose: prevent historical README/state/audit files from being mistaken for the current TruthRaw project state while preserving scientific provenance.

## Authoritative current entry points

- `README.md` — stable project overview and navigation.
- `START_HERE_NEW_CHAT.md` — authoritative current bootstrap.
- `docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md` — current whole-house runtime/memory architecture.
- `state/CURRENT_CANONICAL_STATE_2026-09-10.json` — current state snapshot for this architecture revision.
- this file — current documentation classification.

## Current domain authorities

These remain authoritative for the scientific domain they define; they are not substitutes for current whole-project state:

- `docs/CORE_VISION_SEALED_HOUSE_ARCHITECTURE.md`
- `docs/CORE_VISION_ZERO_LINE_TRUTHRANGE_ARCHITECTURE.md`
- `docs/CORE_VISION_UNCERTAINTY_AWARE_APPEARANCE.md`
- canonical module README/STATUS/VALIDATION files under `canonical/<module>/<version>/`

A canonical module README describes that exact frozen/versioned module. It does not automatically describe later orchestration, mobile memory policy or current project-wide status.

## Historical snapshots — preserve, never bootstrap

Earlier dated state/audit files remain historical provenance and must not be rewritten merely to make them look current.

## Research README rule

Every `docs/research/**/README*.md` is **module-local research documentation** unless explicitly classified otherwise by this current index.

It may be scientifically useful, promoted in part, rejected/superseded in part, or dependent on later work. It is never a global current-state authority by filename alone.

## Branch-local research candidate: RGB / LinearRaw DNG restoration — 2026-09-13

On branch `research/restore-rgb-linearraw-output-v0.2-workbase-2026-09-13`, the module-local restoration authority includes:

- `docs/research/linear-dng-projection-v0.2/README.md`
- `docs/research/linear-dng-projection-v0.2/evidence/MODERN_FINALIZED_PREVIEW_ARCHITECTURE_2026-09-13.md`
- `docs/research/linear-dng-projection-v0.2/evidence/WORK_APK_FORENSIC_2026-09-13.md`
- `docs/research/linear-dng-projection-v0.2/evidence/HOST_CI_34780375681_2026-09-13.md`
- `docs/research/linear-dng-projection-v0.2/evidence/ANDROID_CI_34780463331_2026-09-13.md`
- `docs/research/linear-dng-projection-v0.2/evidence/HISTORICAL_ANDROID_PREVIEW_RESTORE_SOURCE_2026-09-13.md`

Classification: **research candidate / branch-local continuation**, not a global canonical-state promotion.

### Preview authority within this branch-local work

For preview restoration, the later 2026-09-11 modules are architecturally newer than the old v0.7 DNG preview packaging fix:

- `docs/research/preview-representation-v0.1/README.md`
- `docs/research/preview-representation-v0.1/DNG_EMBEDDED_PREVIEW_NOTE.md`
- `docs/research/preview-representation-v0.1/ANDROID_JPEG_NOTE.md`
- `docs/research/finalized-scientific-preview-release-v0.2/README.md`
- `docs/research/reconstructed-color-preview-v0.1/README.md`

The leading rule is **finalized preview first, representation/container second**.

A live `ARGB_8888/sRGB` surface, standalone JPEG and embedded DNG JPEG may all represent the same finalized downstream preview without becoming scientific evidence.

The historical v0.7 IFD layout is classified as **container/interoperability evidence only**. It is not the definition of Scientific Preview and is not automatically the final v0.2/vNext DNG topology.

The exact historical phase label remembered as approximately `1.3` / `1.4` is not assigned because no exact source has yet proved that label.

### Current green gates

- GCC Release host build/test;
- Clang Release host build/test;
- Clang ASan/UBSan host build/test;
- Android arm64 debug APK assembly;
- JNI/v0.2 native-path verification;
- exact APK and artifact hashes recorded.

Exact pre-embedded-preview Android restoration APK:

- version `0.6-linear-dng-restore`;
- bytes `4335889`;
- SHA-256 `93e51245e0950c5c3140b83f2f3429d2f52ad48adcd5630409134e4c430b5654`.

Still-independent gates:

- physical Honor BKQ-N49 execution;
- newly produced DNG parsing/LibRaw/DNG SDK interoperability;
- Lightroom open/edit behavior;
- embedded finalized-JPEG DNG integration and Android/piex discovery;
- portable-preview size/performance policy above the low-memory live UI surface.

This branch-local section does not alter the authority of `state/CURRENT_CANONICAL_STATE_2026-09-10.json`.

## Other README classes

- `capture/**/README*.md` — capture experiment/module-local.
- `docs/calibration/**/README*.md` — calibration campaign/module-local.
- `tests/**/README*.md` — test/regression documentation.
- `canonical/**/README*.md` — exact canonical module/version authority only.

None is a global bootstrap source.

## Current supersession map

The renewed house architecture supersedes older project-wide navigation/orchestration assumptions, not their scientific evidence.

- Building Runtime v0.1 supersedes the assumption that modules are unrelated standalone execution islands.
- Room Capsule v0.1 establishes the local-domain/tile memory pattern for relighting.
- current mobile work generalizes resource ownership across rooms;
- Technical Backplane generalizes one-copy immutable shared state;
- the streaming-frame migration supersedes full-frame ownership only where equivalence is proven.

No historical evidence file is deleted by this supersession.

## PTC naming guard

`canonical/ptc/v1.1` = **Pure Truth Certificate v1.1**.

It is not a photon-transfer-curve calibration directory.

## Machine-enforced rules

`tools/verify_documentation_governance.py` checks the current bootstrap/state/index relationship and ensures research README files are not silently promoted to global authority.
