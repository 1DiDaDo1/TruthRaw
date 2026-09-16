# Scientific Preview Source Binding v0.2 — two-phase admission

Status: RESEARCH CANDIDATE — 2026-09-11

## Problem closed by v0.2

v0.1 correctly required a complete Technical Backplane before scientific-preview admission. For real execution this creates a lifecycle dependency: Technical Backplane v0.1 requires a non-zero `scientificMasterHash`, while the Scientific Master can only exist after Main-House computation.

A unit test can provide a fixture hash. A real Android execution must not invent one.

v0.2 therefore separates **compute/appearance admission** from **final scientific release** without weakening either source/color checks or the final Backplane check.

## Phase 1 — PREPARED_SOURCE

`prepare_scientific_color_source()` requires:

- canonical non-empty source SHA-256 seal;
- authorized validated color authority;
- exact color→sourceEvidenceId match;
- non-empty binding ID and finite matrix;
- physicalFrameCount=1;
- independentEvidenceCount=1.

It emits the `TileNativeDngSource::OpenOptions` needed to start Main-House computation.

It explicitly reports:

- `mainHouseComputeAllowed = true`;
- `sourceBoundAppearanceReleaseAllowed = true`;
- `scientificPreviewReleaseAllowed = false`;
- `scientificClaimAllowed = false`.

This means a reconstructed result may be shown or exported only as a clearly labeled **source-bound appearance preview**. It is not yet a finalized Scientific Preview and is not a Scientific Master claim.

## Phase 2 — FINALIZED_LINEAGE

After a real Scientific Master/lineage exists, `finalize_scientific_color_lineage()` delegates to the already validated v0.1 final admission. It requires a complete valid Technical Backplane whose sourceEvidenceHash and frame/evidence counts match the prepared source.

Only this phase returns a `ScientificPreviewAdmission` carrying the final claim scope.

## Invariants

- no dummy scientificMasterHash;
- no temporary zero-line hash;
- no PreviewSentinel authority;
- no source-ID substitution between phases;
- appearance preview never becomes evidence;
- phase-1 visibility never implies Scientific Master finalization;
- one physical frame and one independent evidence source;
- v0.1 Backplane validation remains unchanged;
- v0.1 source/color admission remains the final authority gate.

## Next dependency

The Android path may now render a bounded source-bound appearance preview from the prepared source, while a deterministic streaming Scientific Master digest/binding is still required before phase 2 can receive a real Backplane. Until that exists, `scientificPreviewReleaseAllowed` and `scientificClaimAllowed` remain false.
