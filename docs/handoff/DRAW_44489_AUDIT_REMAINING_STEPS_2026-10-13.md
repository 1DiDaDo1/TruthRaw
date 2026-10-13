# D.RAW / TruthRaw — 44489 — Remaining Audit Steps — 2026-10-13

Continuation code: **44489**

Permanent motto: **One Free World. Many sealed observations. One evidence law.**

## Purpose

This document restores the audit roadmap that was already established in the preceding chat and prevents recent cable/regression tests from being mistaken for completion of the active scientific audit.

The recent Source Capability cable checks are valuable regression/foundation evidence. They prove that the existing parser/evidence/firewall wiring remains coherent under the tested conditions. They do **not** by themselves prove that the real RAW sample-domain execution path performs the complete raw-to-linear mapping, and they do not authorize Scientific Master writeback or scientific promotion.

## Authority anchors

Integrated scientific/runtime anchor:

`96be8a23c0d43f5dec304bdeb635824868cc80a7`

Active audit branch:

`audit/source-capability-raw-to-linear-contract-v01`

Draft PR:

`#136 — Source Capability: raw-to-linear ordering contract v0.1`

Current live branch head when this roadmap was restored:

`30d68684df0e347a8ae2bf52bf50650da8bfd25c`

PR #136 is open, draft and unmerged. New audit/governance SHAs are not integrated scientific authority.

## What is already established

### Metadata/specification side

The existing Source Capability route already has read-only contracts for:

- Raw-IFD domain identity;
- `WhiteLevel` and associated radiometric metadata semantics;
- spatial/effective black-level dependencies;
- Effective Black / Normalization;
- LinearizationTable metadata semantics;
- raw-to-linear ordering/composition;
- the narrow DNG post-normalization upper-clipping specification rule.

The represented ordering is:

`Linearization -> Black Subtraction -> Rescaling / Normalization -> Clipping`

The current single-cable rule remains binding: reuse the existing `SourceCapabilityRuntimeEvidenceEnvelope/0.1`; do not create a second parser, second source read/hash, second Observation Evidence Ledger, second Raw-IFD detector, duplicate validator route or second pixel pipeline.

### What the recent cable tests prove

They are regression/foundation evidence for the existing route and firewalls. In particular they can prove things such as:

- dependency identity remains source-bound;
- malformed dependency material fails closed as `UNRESOLVED`;
- duplicate dependency identity fails closed as `CONTRADICTED`;
- source-SHA mismatch fails closed;
- the one-cable/static-boundary contract is preserved;
- metadata/specification composition can validate without creating pixel effects.

They do **not** prove actual numerical execution on RAW image samples.

## Audit steps that are still open

### A. Trace the real sample-domain RAW execution path

Read-only trace from parsed/container source into the actual scientific numerical processing route. Identify the concrete callers and data objects that carry real RAW sample values. Do not infer this from metadata validators or preview/output code.

### B. LinearizationTable execution

Determine whether the real sample path already applies LinearizationTable semantics to sample values.

Required distinction:

- metadata table parsed/validated;
- table actually applied numerically to the correct source-bound sample domain.

If execution already exists, bind/prove that existing cable. Do not duplicate it.

### C. Effective-black subtraction execution

Determine whether the real sample path subtracts the already-defined effective black relation, including spatial/delta dependencies where applicable.

Verify domain/CFA/sample indexing and source binding. Metadata validation alone is insufficient.

### D. Normalization execution

Determine whether the real sample path normalizes using the correct source-bound denominator derived from `WhiteLevel - effective_black` and whether the numerical order relative to Linearization and black subtraction is correct.

Again, reuse existing execution if present.

### E. Upper clipping execution and authority

Trace whether the real numerical path clips post-normalization values above the relevant upper bound and under which exact contract.

Keep these authorities separate:

1. DNG post-normalization clipping rule;
2. WhiteLevel metadata / encoding boundary;
3. observed clipped/censored sample pattern;
4. physical sensor saturation/full-well/scene-radiance censoring;
5. Appearance/output white, tone mapping and highlight roll-off.

A metadata/specification clipping rule is not observed-sample censor validation and is not physical saturation calibration.

### F. Sample-domain provenance and CFA/domain consistency

For any discovered existing numerical execution, prove that it operates on the intended source-bound RAW/CFA domain and preserves:

- source SHA / observation identity;
- IFD/domain identity;
- CFA/sample indexing where applicable;
- Observation Evidence Ledger identity;
- immutable sealed evidence;
- no hidden source mutation.

### G. Malformed/duplicate dependency behavior in the real execution route

The metadata contracts already define fail-closed semantics. The audit must confirm that real sample-domain execution cannot bypass those semantics or silently select a malformed/duplicate dependency.

### H. Only after A-G: decide whether anything is actually missing

If every required operation already exists in the real sample path, the correct work is to bind/prove that existing path, not build another one.

Only when an operation is demonstrably absent may a new isolated candidate be designed, with exact provenance/authority and target-independent regression tests before any pixel effect.

## Hard fields that remain false until the sample-domain audit proves otherwise

- `table_application_to_image_samples_validated=false`
- `full_raw_to_linear_mapping_validated=false`
- `image_sample_payload_read=false` for the current metadata-only contract
- `linearization_applied_to_image_samples=false` for the current metadata-only contract
- `black_subtraction_applied_to_image_samples=false` for the current metadata-only contract
- `normalization_applied_to_image_samples=false` for the current metadata-only contract
- `clipping_applied_to_image_samples=false` for the current metadata-only contract
- `observed_sample_censoring_validated=false`
- `physical_sensor_saturation_calibrated=false`
- `scientific_admission_effect=false`
- `downstream_scientific_consumer_enabled=false`
- `scientific_master_pixel_effect=false`
- `appearance_pixel_effect=false`
- `source_values_modified=false`
- `creates_new_evidence=false`
- `scientific_writeback_allowed=false`

Permanent relations:

`VALIDATED != ADMITTED != USED`

`MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`

## Completion criterion for this audit phase

This phase is complete only when the existing real RAW sample-domain route has been traced end-to-end for the four ordered operations, each operation is either proven present and correctly bound or explicitly proven absent, authority/provenance boundaries are preserved, malformed/duplicate dependencies cannot bypass fail-closed behavior, and exact-head regressions confirm the resulting candidate without scientific promotion.

Until then, regression tests of older cables remain supporting evidence, not a substitute for the audit.

## Prohibited shortcuts

- no second RAW-to-linear pipeline;
- no second parser/read/hash/ledger route;
- no reinterpretation of preview, JPEG, PRO, Free Raster or Appearance behavior as scientific RAW execution evidence;
- no use of Deep-Censor/TruthRange/Appearance as Source Capability authority without an explicit identical evidence contract;
- no Scientific Master writeback;
- no promotion of candidate state;
- no new `MEASURED` evidence;
- no AI/ML scientific inference.
