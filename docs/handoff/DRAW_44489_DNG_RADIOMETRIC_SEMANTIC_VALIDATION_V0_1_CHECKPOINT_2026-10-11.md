# D.RAW 44489 — DNG Radiometric Semantic Validation v0.1 checkpoint — 2026-10-11

Status: **IMPLEMENTED / CI-PROVEN / SHADOW-ONLY / ZERO-PIXEL-CHANGE / NOT SCIENTIFICALLY ADMITTED**

Purpose: record the first standardized semantic-validation family layered on top of the physical Source Capability Evidence Ledger without changing scientific authority, pixels, Scientific Master, Appearance, reconstruction or output.

## Exact runtime checkpoint

Validated runtime commit:

`1534d321f56a7f57b1b34e309282b1d9ead4579b`

Commit message:

`android: add shadow DNG radiometric semantic validator v0.1`

Parent/gated input head:

`17d5de89cfc1366e21ea21f942bce825bf2d6451`

PR #131 remains open, draft, mergeable and unmerged. Frozen PR #130 / `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f` is unchanged.

## New runtime layer

New file:

`suite_android/app/src/main/java/com/truthraw/adaptiveui/SourceCapabilityRadiometricSemanticValidatorV01.kt`

Schema:

`D.RAW/SourceCapabilityRadiometricSemanticValidation/0.1`

Status:

`SHADOW_ONLY_SPECIFICATION_SEMANTIC_VALIDATION`

The existing `SourceCapabilityEnvelopeV02.describe(...)` now attaches the result under:

`radiometric_semantic_validation_v0_1`

No second TIFF/DNG parser and no second source-hash route were introduced.

## Standardized field family — deliberately narrow

This v0.1 validates only file-format semantic contracts for:

- DNG `BlackLevelRepeatDim` / tag 50713;
- DNG `BlackLevel` / tag 50714;
- DNG `WhiteLevel` / tag 50717;
- TIFF `SamplesPerPixel` / tag 277 only as the local dependency needed to validate counts.

Specification anchors:

- Adobe Digital Negative (DNG) Specification 1.7.1.0;
- TIFF Revision 6.0.

The validator uses these format contracts:

- `BlackLevelRepeatDim`: SHORT, count 2, specification default 1 x 1;
- `BlackLevel`: SHORT / LONG / RATIONAL, count = repeat rows x repeat cols x SamplesPerPixel;
- `WhiteLevel`: SHORT / LONG, count = SamplesPerPixel;
- `SamplesPerPixel`: SHORT, count 1, TIFF default 1.

These are **file-format semantic constraints only**. They do not prove that a metadata value is the scientifically correct calibration for a particular sealed observation.

## Defaults do not become evidence occurrences

If `SamplesPerPixel` is absent, the TIFF default 1 may be used only as a validation dependency.

If `BlackLevelRepeatDim` is absent, the DNG default 1 x 1 may be used only as a validation dependency.

Permanent v0.1 invariant:

`defaults_mint_source_occurrence=false`

A specification default is therefore not inserted into the physical occurrence ledger and does not pretend to be bytes that existed in the sealed source.

## Duplicate handling

Physical duplicates remain preserved by the occurrence ledger.

The semantic validator does not silently choose a winner. For dependencies such as duplicate `BlackLevelRepeatDim`, status becomes:

`AMBIGUOUS_MULTIPLE_OCCURRENCES_FAIL_CLOSED`

and family selection reports:

`MULTIPLE_PHYSICAL_OCCURRENCES_NO_SELECTION_FAIL_CLOSED`

Dependent validation may then become:

`DEPENDENCY_UNRESOLVED_FAIL_CLOSED`

Permanent invariant:

`duplicate_occurrence_selection_allowed=false`

This deliberately preserves the distinction between physical occurrence evidence and later semantic interpretation.

## Validation states

Occurrence-level semantic states introduced here:

- `VALIDATED_FORMAT_SEMANTICS_SHADOW_ONLY`;
- `CONTRADICTED_FORMAT_SEMANTICS_FAIL_CLOSED`;
- `DEPENDENCY_UNRESOLVED_FAIL_CLOSED`.

Family-level states:

- `VALIDATED_SHADOW_ONLY`;
- `CONTRADICTED_FAIL_CLOSED`;
- `UNRESOLVED_FAIL_CLOSED`.

`VALIDATED_FORMAT_SEMANTICS_SHADOW_ONLY` means only that the occurrence fits the currently encoded DNG/TIFF type/count contract. It does **not** mean scientifically admitted, measured, calibrated, trustworthy for reconstruction, or safe for writeback.

Raw-IFD usage is deliberately not promoted in this milestone and remains:

`NOT_INDEPENDENTLY_VALIDATED_V0_1`

## Authority firewall

This validator explicitly reports:

- `record_lifecycle_effect=false`;
- `scientific_admission_effect=false`;
- `pixel_route_effect=false`;
- `scientific_master_pixel_effect=false`;
- `appearance_pixel_effect=false`;
- `creates_new_evidence=false`;
- `scientific_writeback_allowed=false`.

Existing Source Capability records for BlackLevel/WhiteLevel therefore remain, at most, their previous passive state such as:

`lifecycle=PARSED`

and

`admission=PRESENT_UNVALIDATED`.

No record is upgraded to `VALIDATED`, `ADMITTED` or `USED` merely because this shadow validator recognizes a valid DNG/TIFF shape.

## CI evidence

Dedicated workflow:

`D.RAW Source Capability Evidence Ledger v0.2`

Successful run:

`38099516269`

Result: **SUCCESS**.

Proven in that run:

1. existing v0.56 occurrence/traversal runtime recognized without downgrade or rewrite;
2. shadow radiometric semantic validator generated successfully;
3. evidence-ledger/firewall invariants passed;
4. sealed Full-Frame Streaming integrity passed;
5. complete Android `:app:testDebugUnitTest` passed;
6. Gradle result: `BUILD SUCCESSFUL in 1m 23s`;
7. `23 actionable tasks: 23 executed`;
8. guarded commit/push produced exact runtime `1534d321...`.

### Earlier gate-only failure retained as provenance

Run `38099417951` failed before the new validator or unit tests ran. The old occurrence-ledger patcher expected the obsolete v0.54 parser version and did not yet recognize v0.56 as an already-applied superset.

That was a CI replay/idempotence defect, not a runtime, parser, validator or test regression.

The workflow was corrected at:

`17d5de89cfc1366e21ea21f942bce825bf2d6451`

It now treats the proven v0.56 parser as already containing the v0.55 occurrence-ledger work and uses workflow concurrency to prevent same-branch source-evidence gates from racing each other.

## Tests added for v0.1

The dedicated radiometric semantic tests prove at least:

- absent `SamplesPerPixel` and `BlackLevelRepeatDim` use specification defaults only as validation dependencies and mint no source occurrence;
- explicit 2 x 2 `BlackLevelRepeatDim` plus `SamplesPerPixel=1` requires four BlackLevel values;
- BlackLevel count mismatch fails closed;
- WhiteLevel encoded as RATIONAL is contradicted by the encoded DNG type contract;
- duplicate `BlackLevelRepeatDim` occurrences never select a winner and leave dependent BlackLevel count validation unresolved;
- Source Capability ledger lifecycle/admission remains unchanged by shadow semantic validation.

## Permanent epistemic interpretation

The chain is now explicitly:

`sealed source bytes -> physical occurrence -> structural decode -> specification-semantic validation -> [future independent scientific validation] -> [future admission] -> [future use]`

Never collapse this to:

`DNG tag present + type/count valid -> scientific truth`.

In particular:

- BlackLevel format validity does not yet prove the scientifically correct zero-light calibration for the observation;
- WhiteLevel format validity does not prove every finite sample below it has trustworthy uncensored chromaticity;
- a specification default is not measured evidence;
- duplicate or contradictory metadata is evidence about uncertainty/structure and must not be hidden by first-found compatibility projection.

Permanent project law remains:

`MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`

## Next safe step

Stay inside the zero-pixel-change Source Capability milestone.

Before any radiometric metadata can influence normalization or reconstruction, add **independent semantic/consistency validation** that remains source-bound and fail-closed. The next useful work should first settle IFD/domain binding and contradiction rules for the radiometric family, including whether an occurrence belongs to the actual admitted raw image domain, without yet consuming BlackLevel or WhiteLevel in Scientific Master computation.
