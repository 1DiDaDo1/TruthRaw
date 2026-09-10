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
