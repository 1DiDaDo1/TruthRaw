# D.RAW 44489 — Source Capability Traversal Status v0.1 checkpoint — 2026-10-11

Status: **IMPLEMENTED / CI-PROVEN / SHADOW-ONLY / ZERO-PIXEL-CHANGE**

This checkpoint records the continuation of the Source Capability Evidence Ledger work on PR #131. It does not promote scientific authority and does not change sealed CFA evidence, Scientific Master pixels, reconstruction, Appearance or downstream output behavior.

## Exact runtime checkpoint

Runtime commit:

`ca8530404fed9fd02ce6b9e2d8d7fd30a638826e`

Commit:

`android: report fail-closed TIFF traversal evidence v0.1`

PR #131 remains open, draft, unmerged and based on frozen PR #130 / `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f`.

## What v0.56 adds

The existing classic-TIFF/DNG parser remains the single parser. No second source-hash route was added.

Parser identifier:

`truthraw.classic-tiff-dng-container-metadata.v0.56-traversal-status`

The parser now reports traversal attempts that were previously silent skips:

- `IFD_OFFSET_OUT_OF_BOUNDS_FAIL_CLOSED`;
- `REVISITED_IFD_OFFSET_FAIL_CLOSED`;
- `RECURSION_LIMIT_EXCEEDED_FAIL_CLOSED`;
- `NON_POSITIVE_IFD_OFFSET_FAIL_CLOSED` where such traversal is attempted.

These are passive structural observations only. They do not repair source bytes and do not infer a replacement IFD.

The Source Capability Envelope binds traversal anomalies to the same sealed `source_sha256` and source byte length and classifies them as source-structure evidence with `CONTRADICTED_FAIL_CLOSED`. They cannot become ADMITTED or USED scientific evidence.

Hard structural truncation remains a hard parse failure. On that path the profiler now emits a source-SHA-bound `SourceCapabilityEnvelopeV02.unavailable(...)` object with `PARSE_FAILED_FAIL_CLOSED` rather than silently omitting the capability envelope.

## Earlier physical-occurrence ledger retained

The preceding occurrence-ledger runtime remains intact inside this checkpoint:

- exact classic-TIFF IFD entry offset;
- exact value/offset-field offset;
- original four value/offset bytes;
- inline/external/unknown storage kind;
- safe external byte range where provable;
- separate locator / parse / decode status;
- repeated tags retained as distinct physical occurrences;
- big-endian source bytes preserved verbatim;
- BigTIFF magic 43 rejected fail-closed rather than interpreted with the classic 12-byte layout.

The compatibility projection may still expose first-found values for legacy consumers; it is not the scientific evidence ledger and it does not collapse the physical occurrence records.

## CI evidence

Dedicated successful gate:

- workflow: `D.RAW Source Capability Evidence Ledger v0.2`;
- run: `38098724974`;
- result: **SUCCESS**;
- passive occurrence patch: SUCCESS;
- passive traversal-status patch: SUCCESS;
- evidence-ledger invariants: SUCCESS;
- sealed Full-Frame Streaming integrity: SUCCESS;
- full Android `:app:testDebugUnitTest`: SUCCESS;
- validated runtime commit/push: SUCCESS.

A later concurrent run `38098782191` also reached `BUILD SUCCESSFUL` for the complete Android unit-test suite. Its final push alone failed non-fast-forward because another already-queued instance of the same gate had committed the validated runtime first. Do **not** classify that run as a code or unit-test regression.

## Scientific firewall

Permanent state remains:

- `creates_new_evidence=false`;
- `source_values_modified=false`;
- `scientific_writeback_allowed=false`;
- `candidate_applied=false`;
- `pixel_route_effect=false`;
- `scientific_master_pixel_effect=false`;
- `appearance_pixel_effect=false`;
- no camera/vendor identity may promote scientific authority.

`MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`

## Next safe step

Continue the zero-pixel-change milestone with **one standardized semantic field family at a time**. The next validator should remain shadow-only and must separate:

`physical occurrence -> structural decode -> specification-semantic validation -> admission/authority`

Specification-semantic validation is not scientific admission. A structurally valid DNG tag may become `VALIDATED` with respect to its file-format contract while still remaining `PRESENT_UNVALIDATED` for scientific use.

First recommended family: DNG radiometric boundary metadata (`BlackLevelRepeatDim`, `BlackLevel`, `WhiteLevel`, plus only the local dependencies required to validate their type/count relationships). Do not use these values to alter samples or Scientific Master in this milestone.
