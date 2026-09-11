# Scientific Preview Source Binding v0.2 — two-phase admission

Status: RESEARCH CANDIDATE — 2026-09-11

## Problem closed by v0.2

v0.1 correctly required a complete Technical Backplane before scientific-preview admission. For real execution this creates a lifecycle dependency: Technical Backplane v0.1 requires a non-zero `scientificMasterHash`, while the Scientific Master can only exist after Main-House computation.

A unit test can provide a fixture hash. A real Android execution must not invent one.

v0.2 therefore separates **compute admission** from **result release** without weakening either source/color checks or the final Backplane check.

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
- `previewReleaseAllowed = false`;
- `scientificClaimAllowed = false`.

So computed preview pixels may be held transiently, but cannot yet be presented as a finalized scientific-preview result.

## Phase 2 — FINALIZED_LINEAGE

After a real Scientific Master/lineage exists, `finalize_scientific_color_lineage()` delegates to the already validated v0.1 final admission. It requires a complete valid Technical Backplane whose sourceEvidenceHash and frame/evidence counts match the prepared source.

Only this phase returns a `ScientificPreviewAdmission` carrying the final claim scope.

## Invariants

- no dummy scientificMasterHash;
- no temporary zero-line hash;
- no PreviewSentinel authority;
- no source-ID substitution between phases;
- one physical frame and one independent evidence source;
- v0.1 Backplane validation remains unchanged;
- v0.1 source/color admission remains the final authority gate.

## Next dependency

A production Android path still needs a deterministic streaming Scientific Master digest/binding so phase 2 can receive a real Backplane rather than a test fixture. Until that exists, a reconstructed color surface can be computed/held under phase 1 but must not be released as finalized scientific-preview output.
