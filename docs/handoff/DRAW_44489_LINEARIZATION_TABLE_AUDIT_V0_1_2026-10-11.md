# D.RAW 44489 — LinearizationTable shadow audit v0.1 — 2026-10-11

Status: **standalone validator validated on clean exact head; not runtime-envelope-exposed, not admitted, not used, no pixel application**.

Base integration anchor: `c81d4430f5c82d2de83cd480452c8bd01d60878b` (PR #133 merge).
Working branch: `feat/source-capability-linearization-table-shadow-v01`.
Draft PR: #134 — `Source Capability: LinearizationTable shadow validator v0.1`.
Validated standalone code/CI checkpoint: `488a20065a9ef3f11695fa0167c70bc12bc7dac4`.
Validated clean post-documentation/post-cleanup head: `aefbf6c62a0caddb5de0acf4bed511dbd7abd9a6`.
Targeted integrity run on the standalone code checkpoint: `38165699523` — SUCCESS.

## Why this audit exists

The current radiometric chain validates Raw-IFD domain, radiometric semantics, encoding consistency, DNG version compatibility, spatial BlackLevel dependencies and Effective Black / normalization as read-only diagnostics. Complete DNG raw-to-linear application is still intentionally false because `LinearizationTable` has not yet been given a proven pixel-domain application contract.

This step must not create a second TIFF parser, second source read, new admission path, pixel consumer, Scientific Master writeback or Appearance effect.

## Existing parser finding

On base `c81d4430...`, `DngContainerMetadataParser` already generically records every classic-TIFF IFD entry with exact occurrence provenance, type/count, decode status, inline/external storage, byte range and decoded value. TIFF `SHORT` is already supported.

Tag `50712` is not present in the parser tag-name map and is not projected as a named IFD semantic field. Therefore a physical 50712 occurrence can exist in the generic `entries` array, but today it is exposed as `TAG_50712` rather than as a named `LinearizationTable` parser field.

### Binding implementation refinement

The initial audit proposed renaming/projecting tag 50712 in the parser first. That is now deliberately **deferred**.

Reason: `SourceCapabilityRuntimeEvidenceEnvelope/0.1` canonicalizes the parser-provided semantic key into the Observation Evidence Ledger digest. Renaming an already-present physical occurrence from `TAG_50712` to `LinearizationTable` would therefore change the ledger canonical line/digest for sources carrying tag 50712, even though the physical source bytes did not change.

The standalone validator does not need that migration: it binds the existing exact physical occurrence by numeric tag `50712` from the already-parsed `entries` array. This preserves ledger identity while proving the metadata contract. Any future parser/ledger semantic rename must be a separate explicit provenance/schema migration, never a hidden side effect of this validator.

No second parser is required and no parser source file is changed in the current standalone step.

## Normative DNG contract

Normative anchor: Adobe Digital Negative Specification 1.7.1.0, with Adobe DNG SDK validation behavior used as an implementation cross-check.

`LinearizationTable`:

- tag: `50712` (`0xC618`);
- TIFF type: `SHORT`;
- count: `N`;
- usage: Raw IFD;
- default when absent: identity table;
- if `SamplesPerPixel != 1`, one table applies to every sample plane;
- processing order: `Linearization -> Black Subtraction -> Rescaling/Normalization -> Clipping`;
- raw values outside the represented table range map to the last table entry according to the DNG processing model.

Adobe DNG SDK validation additionally establishes for an explicit table:

- count must be in `2..65536`;
- floating-point raw encoding is not compatible with LinearizationTable;
- `BitsPerSample > 16` is not compatible with LinearizationTable.

Important non-rule: the SDK does **not** require count to equal `2^BitsPerSample`. D.RAW must not invent that restriction.

## Standalone validator implemented

Object: `SourceCapabilityRadiometricLinearizationTableValidatorV01`.
Schema: `D.RAW/SourceCapabilityRadiometricLinearizationTable/0.1`.
Status: `SHADOW_ONLY_LINEARIZATION_TABLE_METADATA_CONTRACT`.
Implementation commit: `78fe7360e59c67bc039a819d20442949a000d5c3`.
Focused regression commit: `038a585a36f2fd3a8daf0f66440ec41e23b178bc`.
Integrity-workflow commit / validated exact code head: `488a20065a9ef3f11695fa0167c70bc12bc7dac4`.

The validator consumes only the already-parsed metadata object and the existing Raw-IFD and radiometric encoding validators. It does not read image samples.

For each relevant IFD it distinguishes:

- absent tag -> specification identity default, zero physical occurrences minted;
- one explicit valid physical occurrence -> validated metadata contract;
- duplicate physical occurrences -> unresolved fail-closed, no silent selection;
- wrong TIFF type or count outside `2..65536` -> contradicted fail-closed;
- decode/cardinality/value-domain uncertainty -> unresolved fail-closed;
- floating-point raw or BitsPerSample >16 with explicit table -> contradicted fail-closed;
- one explicit table may serve all sample planes;
- table count is not forced to `2^BitsPerSample`.

The report explicitly keeps false:

- `table_application_to_image_samples_validated`;
- `linearization_applied_to_image_samples`;
- `full_raw_to_linear_mapping_validated`;
- `image_sample_payload_read`;
- `record_lifecycle_effect`;
- `scientific_admission_effect`;
- `pixel_route_effect`;
- `scientific_master_pixel_effect`;
- `appearance_pixel_effect`;
- `source_values_modified`;
- `creates_new_evidence`;
- `scientific_writeback_allowed`.

It also reports `parser_tag_rename_performed=false` and `observation_evidence_ledger_identity_changed=false`.

## Focused regression scope

`SourceCapabilityRadiometricLinearizationTableValidatorV01Test` proves:

- absent identity default without minting tag 50712 evidence;
- explicit valid SHORT table shorter than `2^BitsPerSample`;
- duplicate occurrences -> unresolved;
- wrong TIFF type -> contradicted;
- count below 2 or above 65536 -> contradicted;
- floating-point raw + explicit table -> contradicted;
- unsigned raw above 16 bits + explicit table -> contradicted;
- one table shared across multi-sample raw;
- undecoded explicit occurrence -> unresolved;
- ledger digest and observation-evidence digest unchanged by standalone validation;
- the underlying ledger record remains `TAG_50712`, lifecycle `PARSED`, admission `UNSUPPORTED_SEMANTICS`, zero consumers;
- all scientific/pixel/writeback firewalls remain false.

## Exact-head validation result

The dedicated workflow `Source Capability LinearizationTable v0.1 Integrity` completed SUCCESS in run `38165699523` on exact code checkpoint `488a20065a9ef3f11695fa0167c70bc12bc7dac4`. It enforces the standalone boundary and executes the LinearizationTable validator tests plus the Raw-IFD and radiometric-encoding dependency regressions.

After the canonical 44489 refresh and removal of all temporary refresh tooling, clean head `aefbf6c62a0caddb5de0acf4bed511dbd7abd9a6` was also checked as an exact head: no check remained queued or in progress, and no check concluded failure or action-required. All triggered non-skipped checks completed successfully; Deep-Censor and Free Raster were path-scope skips rather than failures.

This proves code/contract compatibility only. It does **not** establish scientific admission, use, pixel-domain table application, complete raw-to-linear validity or physical real-device acceptance.

## Next order

1. Validate the present documentation-only status commit as an exact head.
2. If that exact head is green, merge PR #134 only as the standalone validator/integrity boundary; merge status must not promote scientific authority.
3. After that merge, create a fresh branch/PR for optional read-only exposure through `SourceCapabilityRuntimeEvidenceEnvelope/0.1`.
4. Any exposure must prove ledger digest, observation-evidence digest, records, lifecycle, admission, consumers, source reads, pixels, Scientific Master, Appearance and writeback are unchanged.
5. Parser semantic renaming of 50712 remains a separate explicit provenance/schema migration and is not required for validation or exposure.
6. No claim of complete raw-to-linear pixel application until a later explicit contract proves table application together with downstream black subtraction/normalization/clipping behavior.

## Permanent boundary

`VALIDATED != ADMITTED != USED`; absence defaults are specification dependencies, not physical evidence; and no `LinearizationTable` metadata may rewrite sealed CFA evidence or become a pixel consumer by implication.
