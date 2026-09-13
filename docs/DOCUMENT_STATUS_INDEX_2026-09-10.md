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

The following are historical records:

- `state/CURRENT_CANONICAL_STATE_2026-09-06.json`
- `state/CURRENT_CANONICAL_STATE_2026-09-08.json`
- `state/CURRENT_CANONICAL_STATE_2026-09-09.json`
- `docs/PROJECT_STATE_AUDIT_2026-09-08.md`
- `state/REPOSITORY_MIGRATION_STATUS.json` — migration-era status record; not current project status.

Do not edit these merely to make them look current. Their old content is part of provenance.

## Research README rule

Every `docs/research/**/README*.md` is **module-local research documentation**.

It may still be scientifically useful, promoted in part, rejected/superseded in part, or dependent on later work. It is never a global current-state authority by filename alone. Check the current state/index and the module's state/report/integrity evidence.

This applies to, among others: zero-line/TruthRange studies, camera-RGB covariance v0.6, XYZ D50 uncertainty v0.7, missing-channel topology v0.8, virtual observation manifold v0.9, uncertainty-aware S-curve/color v1, high-ISO Item153, Manifold Conditioning v1, CICM v1, Room Capsule v0.1 and Building Runtime v0.1.

## Branch-local research candidate: RGB / LinearRaw DNG restoration — 2026-09-13

On branch `research/restore-rgb-linearraw-output-v0.2-workbase-2026-09-13`, the following files are the module-local authority for the active RGB / LinearRaw restoration candidate:

- `docs/research/linear-dng-projection-v0.2/README.md`
- `docs/research/linear-dng-projection-v0.2/evidence/WORK_APK_FORENSIC_2026-09-13.md`
- `docs/research/linear-dng-projection-v0.2/evidence/HOST_CI_34780375681_2026-09-13.md`
- `docs/research/linear-dng-projection-v0.2/evidence/ANDROID_CI_34780463331_2026-09-13.md`
- `docs/research/linear-dng-projection-v0.2/evidence/HISTORICAL_ANDROID_PREVIEW_RESTORE_SOURCE_2026-09-13.md`

Classification: **research candidate / branch-local continuation**, not a global canonical-state promotion.

The branch preserves the historical v0.1 writer and its build path as provenance while adding a v0.2 downstream compatibility restoration. Current v0.2 work restores the historically exercised finite `2x` RGB LinearRaw representation with `BaselineExposure=+1 EV`, source camera identity binding, fail-closed over-window behavior, and Android wiring on top of the finalized source-bound Scientific Preview admission path.

Recorded green gates:

- GCC Release host build/test;
- Clang Release host build/test;
- Clang ASan/UBSan host build/test;
- Android arm64 debug APK assembly;
- JNI/v0.2 native-path verification;
- exact APK SHA/size and artifact ZIP SHA/size binding.

Exact Android restoration APK recorded by evidence:

- version `0.6-linear-dng-restore`;
- bytes `4335889`;
- SHA-256 `93e51245e0950c5c3140b83f2f3429d2f52ad48adcd5630409134e4c430b5654`.

Still-independent gates:

- physical Honor BKQ-N49 execution;
- newly produced v0.2 DNG parsing/LibRaw/DNG SDK interoperability;
- Lightroom open/edit behavior;
- Android/piex embedded-preview discovery;
- native reimplementation of the historically validated multi-IFD/JPEG preview container.

The historical preview evidence now explicitly records the previous successful container pattern: reduced IFD0 thumbnail, full LinearRaw SubIFD0 and JPEG/sRGB preview SubIFD1, with unchanged LinearRaw payload and DNG SDK 1.7.1.2724 PASS for the old exact files.

This branch-local section does not alter the authority of `state/CURRENT_CANONICAL_STATE_2026-09-10.json` and does not make the v0.2 research README a global bootstrap authority.

## Other README classes

- `capture/**/README*.md` — capture experiment/module-local.
- `docs/calibration/**/README*.md` — calibration campaign/module-local.
- `tests/**/README*.md` — test/regression documentation.
- `canonical/**/README*.md` — exact canonical module/version authority only.

None is a global bootstrap source.

## Current supersession map

The renewed house architecture supersedes older *project-wide navigation/orchestration assumptions*, not their scientific evidence.

- Building Runtime v0.1 supersedes the assumption that modules are unrelated standalone execution islands.
- Room Capsule v0.1 establishes the local-domain/tile memory pattern for relighting.
- Current mobile work generalizes resource ownership across all rooms.
- Technical Backplane research generalizes one-copy immutable shared state, including the zero-line binding.
- The streaming-frame migration will supersede full-frame public buffer ownership where equivalence is proven.

No historical evidence file is deleted by this supersession.

## PTC naming guard

`canonical/ptc/v1.1` = **Pure Truth Certificate v1.1**.

It is not a photon-transfer-curve calibration directory. References to missing electron/PTC calibration elsewhere mean physical sensor calibration and remain an independent blocker.

## Machine-enforced rules

`tools/verify_documentation_governance.py` checks that:

1. root bootstrap points to the 2026-09-10 state/index;
2. root bootstrap does not point to earlier dated current-state snapshots in its mandatory list;
3. all earlier dated current-state files still exist as historical snapshots;
4. the current state file declares the earlier snapshots historical;
5. README-like path classes are covered by this governance policy;
6. no research README is declared as a global current bootstrap by the current state file.
