# D.RAW 44489 — LinearizationTable shadow audit v0.1 — 2026-10-11

Status: binding audit checkpoint before runtime implementation.

Base integration anchor: `c81d4430f5c82d2de83cd480452c8bd01d60878b` (PR #133 merge).
Working branch: `feat/source-capability-linearization-table-shadow-v01`.

## Why this audit exists

The current radiometric chain validates Raw-IFD domain, radiometric semantics, encoding consistency, DNG version compatibility, spatial BlackLevel dependencies and Effective Black / normalization as read-only diagnostics. Complete DNG raw-to-linear validity is still intentionally false because `LinearizationTable` has not yet been given its own parser-bound contract.

This step must not create a second TIFF parser, second source read, new admission path, pixel consumer, Scientific Master writeback or Appearance effect.

## Existing parser finding

On base `c81d4430...`, `DngContainerMetadataParser` already generically records every classic-TIFF IFD entry with exact occurrence provenance, type/count, decode status, inline/external storage, byte range and decoded value. TIFF `SHORT` is already supported.

However tag `50712` is not present in the parser tag-name map and is not projected as a named IFD semantic field. Therefore a physical 50712 occurrence can exist in the generic `entries` array, but today it is exposed as an unnamed tag rather than as `LinearizationTable`.

No second parser is required. The minimal parser extension is to name tag 50712 `LinearizationTable` and project its already-decoded value onto the parsed IFD object. The physical occurrence remains the same existing entry and must not be duplicated or regenerated.

## Normative DNG contract

Normative anchor: Adobe Digital Negative Specification 1.7.1.0, with the Adobe DNG SDK validation behavior used as an implementation cross-check.

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

## Planned shadow validator v0.1

Proposed object: `SourceCapabilityRadiometricLinearizationTableValidatorV01`.

The validator will consume only the already-parsed metadata object and existing Raw-IFD / encoding validators. It will not read image samples.

For each relevant IFD it will distinguish:

- absent tag -> specification identity default, zero physical occurrences minted;
- one explicit valid physical occurrence -> validated metadata contract;
- duplicate physical occurrences -> unresolved fail-closed, no silent selection;
- wrong TIFF type, invalid count, failed decode, floating-point raw encoding or BitsPerSample >16 -> contradicted/fail-closed as appropriate.

It must explicitly report that:

- table application to image samples has not occurred;
- full raw-to-linear mapping has not been applied or scientifically admitted;
- validator success is not admission or use;
- record lifecycle/admission/consumer state is unchanged;
- ledger and observation-evidence identity remain governed by the original physical occurrences;
- defaults do not mint source occurrences;
- source values are not modified;
- no MEASURED evidence is created;
- no Scientific Master or Appearance pixel effect occurs;
- scientific writeback remains false.

## Implementation order

1. Minimal parser naming/projection for tag 50712 using the existing occurrence entry.
2. Standalone shadow validator + focused unit tests.
3. Regression proving explicit/default/duplicate/type/count/floating-point/>16-bit behavior and closed firewalls.
4. Only after standalone tests are green: decide whether the diagnostic is exposed read-only through `SourceCapabilityRuntimeEvidenceEnvelope/0.1`.
5. No claim of complete raw-to-linear application until a later explicit contract proves the remaining pixel-domain application/clipping behavior.

## Permanent boundary

This audit changes no scientific state by itself. `VALIDATED != ADMITTED != USED`; absence defaults are specification dependencies, not physical evidence; and no `LinearizationTable` metadata may be used to rewrite sealed CFA evidence.
