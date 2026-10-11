# D.RAW 44489 — Raw-IFD Format-Domain Validation v0.1 checkpoint — 2026-10-11

Status: **IMPLEMENTED / CI-PROVEN / SHADOW-ONLY / ZERO-PIXEL-CHANGE / NOT SCIENTIFICALLY ADMITTED**

Purpose: bind the first DNG radiometric semantic-validation family to an explicit TIFF/DNG IFD format domain without treating a tag name, vendor identity, filename, or a single PhotometricInterpretation value as scientific RAW authority.

## Exact runtime checkpoint

Validated runtime commit:

`02b611b246fb4db1ad3c3261ded7a7a484f7ae2f`

Commit message:

`android: add shadow DNG raw-IFD format-domain validator v0.1`

Gated input head:

`10df5dd20c4730bb581f805ace83244906d14de3`

PR #131 remains open, draft, mergeable and unmerged. Frozen PR #130 / `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f` remains unchanged.

## New runtime layer

New file:

`suite_android/app/src/main/java/com/truthraw/adaptiveui/SourceCapabilityRawIfdDomainValidatorV01.kt`

Schema:

`D.RAW/SourceCapabilityRawIfdDomainValidation/0.1`

Status:

`SHADOW_ONLY_FORMAT_DOMAIN_VALIDATION`

`SourceCapabilityEnvelopeV02` now attaches:

`raw_ifd_domain_validation_v0_1`

The radiometric validator now consumes only this shadow format-domain classification to decide whether its BlackLevel/WhiteLevel semantic result belongs to a DNG Raw-IFD format domain. This does not change Source Capability record lifecycle or scientific admission.

## Specification boundary

Format semantics are anchored to Adobe DNG 1.7.1.0 and TIFF Revision 6.0.

The v0.1 domain binder deliberately requires more than a single PhotometricInterpretation value.

### NewSubfileType

- TIFF tag 254;
- LONG, count 1;
- TIFF default 0 when physically absent;
- the default may resolve a format dependency but does not mint a source occurrence;
- explicit non-zero values such as reduced-resolution (`1`) or enhanced-image (`16`) prevent classification as the full-resolution raw format domain in this v0.1.

### PhotometricInterpretation

- TIFF tag 262;
- SHORT, count 1;
- no default is invented for Raw-IFD classification;
- DNG CFA value `32803` and LinearRaw value `34892` are the only raw-domain photometric values admitted by this narrow format validator.

### CFA same-IFD dependencies

For `PhotometricInterpretation=32803` (CFA), v0.1 additionally requires exactly one physical occurrence each of:

- `CFARepeatPatternDim` / tag 33421;
- `CFAPattern` / tag 33422;

in the same IFD.

Their detailed semantic contracts are **not** validated by this v0.1; their current state is explicitly:

`PRESENT_UNIQUE_SAME_IFD_NOT_SEMANTICALLY_VALIDATED`

Missing or duplicate CFA dependencies fail closed rather than selecting or inventing a value.

### LinearRaw caution

`PhotometricInterpretation=34892` alone is not considered sufficient. DNG permits LinearRaw in reduced-resolution and enhanced IFDs as well. Therefore v0.1 also requires the full-resolution `NewSubfileType=0` domain, including the TIFF default 0 when the physical tag is absent.

## Domain statuses

Per IFD:

- `VALIDATED_RAW_IFD_FORMAT_DOMAIN_SHADOW_ONLY`;
- `CONTRADICTED_NOT_RAW_IFD_FORMAT_DOMAIN`;
- `UNRESOLVED_RAW_IFD_FORMAT_DOMAIN_FAIL_CLOSED`.

These are **format-domain classifications only**.

`VALIDATED_RAW_IFD_FORMAT_DOMAIN_SHADOW_ONLY` must never be read as:

- Direct-CFA evidence admitted;
- sensor truth proven;
- metadata scientifically calibrated;
- Scientific Master authority established;
- reconstruction permitted;
- source pixels measured by this validator.

The runtime explicitly records:

`domain_validation_is_scientific_admission=false`

## Duplicate and default law

Permanent v0.1 invariants:

- `defaults_mint_source_occurrence=false`;
- `duplicate_occurrence_selection_allowed=false`;
- duplicate PhotometricInterpretation or NewSubfileType occurrences do not get a first-found winner;
- unresolved dependencies remain fail-closed.

The existing compatibility projection may still expose first-found metadata for legacy product/intake compatibility, but this validator does not use that projection as evidence truth.

## Radiometric binding

`SourceCapabilityRadiometricSemanticValidatorV01` now records a `raw_ifd_domain` block and one of:

- `VALIDATED_RAW_IFD_FORMAT_USAGE_SHADOW_ONLY`;
- `CONTRADICTED_NON_RAW_IFD_USAGE_FAIL_CLOSED`;
- `UNRESOLVED_RAW_IFD_USAGE_FAIL_CLOSED`.

A BlackLevel or WhiteLevel occurrence can therefore satisfy its type/count contract and still fail closed for use in this radiometric semantic family if its IFD is not a validated Raw-IFD **format** domain.

This still does not scientifically admit BlackLevel or WhiteLevel.

## Tests added / updated

The dedicated domain tests prove at least:

1. CFA Raw-IFD domain with absent NewSubfileType uses TIFF default 0 only as a dependency and does not mint a tag-254 occurrence;
2. full-resolution LinearRaw with default NewSubfileType 0 validates as shadow format domain;
3. LinearRaw with `NewSubfileType=1` is contradicted as reduced/non-full-resolution raw domain;
4. LinearRaw with `NewSubfileType=16` is contradicted as enhanced/non-full-resolution raw domain;
5. missing PhotometricInterpretation remains unresolved fail-closed;
6. duplicate PhotometricInterpretation never selects a winner;
7. CFA without required same-IFD CFAPattern / CFARepeatPatternDim fails closed;
8. radiometric metadata in a reduced/non-raw IFD yields `CONTRADICTED_NON_RAW_IFD_USAGE_FAIL_CLOSED` while the underlying Evidence Ledger records remain `PARSED / PRESENT_UNVALIDATED`;
9. the earlier radiometric semantic fixtures were upgraded to explicit valid CFA Raw-IFD format fixtures before they may assert `VALIDATED_SHADOW_ONLY`.

## CI evidence

Workflow:

`D.RAW Source Capability Evidence Ledger v0.2`

Successful run:

`38100003379`

Result: **SUCCESS**.

The run proved:

- existing physical occurrence ledger preserved;
- existing v0.56 traversal state preserved;
- radiometric semantic validator preserved;
- new Raw-IFD domain validator generated cleanly;
- evidence/firewall invariants passed;
- sealed Full-Frame Streaming integrity passed;
- complete Android `:app:testDebugUnitTest` passed;
- Gradle: `BUILD SUCCESSFUL in 1m 55s`;
- `23 actionable tasks: 23 executed`;
- guarded commit/push produced exact runtime `02b611b246fb4db1ad3c3261ded7a7a484f7ae2f`.

## Scientific firewall — unchanged

The new domain validator reports and preserves:

- `scientific_admission_effect=false`;
- `pixel_route_effect=false`;
- `scientific_master_pixel_effect=false`;
- `appearance_pixel_effect=false`;
- `creates_new_evidence=false`;
- `scientific_writeback_allowed=false`;
- `source_values_modified=false` on per-IFD result objects.

No pixel values, sealed CFA, Scientific Master, Appearance, candidate state or promotion state are changed.

Permanent law remains:

`MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`

## Next safe step

Remain inside the zero-pixel-change Source Capability milestone.

The Raw-IFD format domain is now explicit, but **format-domain validation is still not scientific consistency validation**. Before BlackLevel or WhiteLevel can influence normalization, the next step must compare source-bound radiometric metadata against independent structural/sample-domain facts under a fail-closed contradiction contract, without yet consuming the values in Scientific Master computation.

In particular, future work must not jump from:

`valid DNG type/count + valid Raw-IFD format domain`

to:

`scientifically admitted BlackLevel/WhiteLevel`.
