# D.RAW 44489 — Source Capability Raw-to-Linear Contract Audit v0.1 — 2026-10-12

Status: **BINDING READ-ONLY AUDIT / DESIGN CONTRACT — DOCUMENTATION ONLY**  
Continuation code: **44489**  
Exact base integration anchor: `96be8a23c0d43f5dec304bdeb635824868cc80a7`  
Audit branch: `audit/source-capability-raw-to-linear-contract-v01`

Permanent motto: **One Free World. Many sealed observations. One evidence law.**

---

## 1. Why this audit exists

PR #135 is fully integrated at merge commit:

`96be8a23c0d43f5dec304bdeb635824868cc80a7`

At that anchor, LinearizationTable is visible as a read-only runtime diagnostic while remaining **not ADMITTED, not USED and not applied to RAW image samples**. Parser identity remains `TAG_50712`; the Observation Evidence Ledger identity remains unchanged; the fixed pre-exposure digests remain:

- `ledger_digest_sha256 = 6740e6e04c18797e8b57604f30c1884c6e8428e9a469e53cb139c26abe59afb3`
- `observation_evidence_digest_sha256 = 2bdaa44fe3435929310b79fa38d6ce3b0aa0cc05ef68f9522b4554c6862b38e6`

The current Source Capability radiometric layer already contains separate contracts for:

- Raw-IFD domain;
- radiometric semantics;
- radiometric encoding consistency;
- DNG version compatibility;
- spatial BlackLevel dependencies;
- Effective Black / normalization;
- LinearizationTable metadata semantics and read-only runtime exposure.

The missing architectural layer is a **combined ordering/composition contract** that binds the already-existing validator outcomes into the normative DNG raw-to-linear sequence without reading or changing image samples.

This audit is deliberately narrower than a pixel implementation.

---

## 2. Normative DNG ordering boundary

Normative specification anchor: **Adobe Digital Negative (DNG) Specification 1.7.1.0**.

For the radiometric mapping discussed here, the normative conceptual order is:

`Linearization -> Black Subtraction -> Rescaling / Normalization -> Clipping`

That order is a specification dependency. It does **not** prove that D.RAW has already validated or executed the complete sample-domain mapping.

The current D.RAW state therefore remains:

- LinearizationTable metadata contract: validated/read-only diagnostic where applicable;
- Effective Black / Normalization metadata contract: validated/read-only diagnostic where applicable;
- complete ordered sample-domain raw-to-linear mapping: **not validated**;
- LinearizationTable sample application: **not validated and not performed**;
- scientific admission/use: **not implied by validation**.

Permanent relation:

`VALIDATED != ADMITTED != USED`

---

## 3. Candidate contract name and role

Candidate object:

`SourceCapabilityRadiometricRawToLinearContractValidatorV01`

Candidate schema identity:

`D.RAW/SourceCapabilityRadiometricRawToLinearContract/0.1`

Candidate status while this remains metadata/read-only:

`SHADOW_ONLY_RAW_TO_LINEAR_ORDERING_COMPOSITION_CONTRACT`

Purpose:

- bind existing validator reports;
- prove that required metadata dependencies are mutually compatible enough to describe the normative ordering contract;
- preserve source/IFD identity;
- expose contradictions and unresolved dependencies fail-closed;
- explicitly distinguish metadata-order validation from pixel-domain execution.

It must **not** become a second parser, a second source read, a second hash path, a pixel consumer or a scientific promotion path.

---

## 4. Allowed inputs only

The candidate validator may consume only already-existing, source-bound reports/metadata from the current Source Capability path, including where applicable:

1. `SourceCapabilityRawIfdDomainValidatorV01`
2. `SourceCapabilityRadiometricSemanticValidatorV01`
3. `SourceCapabilityRadiometricEncodingConsistencyValidatorV01`
4. current DNG-version compatibility result
5. `SourceCapabilityRadiometricSpatialBlackLevelValidatorV01`
6. `SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01`
7. `SourceCapabilityRadiometricLinearizationTableValidatorV01`
8. existing source SHA-256 / IFD identity / occurrence provenance already carried by the runtime evidence envelope

No image-sample payload may be read by this contract.

No parser source file needs to change merely to create this ordering/composition audit.

---

## 5. Required dependency binding

For each relevant Raw IFD, the contract should bind the following conceptual stages:

### Stage A — encoded RAW domain

Confirm that the existing Raw-IFD and encoding validators identify a compatible sample domain and fail closed where sample type, bit depth, sample count, planar configuration or required structure is unresolved/contradicted.

This stage does not decode a second time and does not read pixels.

### Stage B — Linearization dependency

Bind the existing `LinearizationTable` report.

Valid metadata cases may include:

- explicit physical tag 50712 validated under the existing standalone contract; or
- the DNG specification identity default when the tag is absent.

The absence default is a specification-defined dependency value. It is **not** a physical source occurrence and may not mint a new ledger record.

Parser semantic key remains `TAG_50712` until a separate provenance/schema migration explicitly changes it.

### Stage C — Black subtraction dependency

Bind existing BlackLevel / BlackLevelRepeatDim / BlackLevelDeltaH / BlackLevelDeltaV / ActiveArea semantics through the already-existing radiometric/spatial-black contracts.

Current effective-black metadata relation remains:

`effective_black = BlackLevel[row mod repeatRows][col mod repeatCols][sample] + BlackLevelDeltaH[col] + BlackLevelDeltaV[row]`

with repeat origin at the top-left of ActiveArea under the existing contract.

This audit does not subtract that value from image samples.

### Stage D — Rescaling / normalization dependency

Bind the existing Effective Black / Normalization result, including the current denominator validity condition:

`normalization_denominator = WhiteLevel[sample] - max_effective_black[sample]`

which must remain finite and strictly positive where the existing validator claims a valid normalization dependency.

This audit does not normalize image samples.

### Stage E — Clipping dependency

The combined contract may record that clipping is the next normative stage in the DNG ordering, but it must not claim complete clipping validation unless an explicit existing Source Capability clipping/censoring dependency contract proves the required metadata/sample-domain semantics for that IFD.

Therefore:

- “clipping exists elsewhere in D.RAW” is not equivalent to
- “the complete Source Capability raw-to-linear clipping stage is jointly validated here”.

If the required Source Capability clipping dependency is absent or insufficient, the combined contract must report that limitation explicitly and keep complete mapping false.

---

## 6. Required fail-closed states

The candidate contract must not silently compose a valid raw-to-linear mapping when any required dependency is:

- absent where no specification default applies;
- unresolved;
- contradicted;
- bound to a different source SHA;
- bound to a different Raw-IFD identity;
- based on duplicate unresolved occurrences;
- inconsistent in SamplesPerPixel/sample-plane interpretation;
- incompatible with floating-point raw or unsupported bit depth where the existing LinearizationTable validator forbids it;
- incompatible with WhiteLevel/effective-black denominator validity;
- semantically valid only in isolation but not provably composable in the required order.

A valid response may be `UNKNOWN`, `UNRESOLVED_DEPENDENCY`, `CONTRADICTED_DEPENDENCY` or an equivalent fail-closed status. It must never silently substitute zero, identity, clipping or guessed radiometric behavior beyond a specification-defined default already represented by an existing validator.

---

## 7. Mandatory report fields / safety assertions

The combined read-only report should make the ordering visible and machine-auditable. At minimum it should record:

- schema/version;
- source SHA-256;
- Raw-IFD path/identity;
- ordered stages `[LINEARIZATION, BLACK_SUBTRACTION, RESCALING_NORMALIZATION, CLIPPING]`;
- dependency report identities/versions;
- per-stage status;
- whether a stage uses an explicit physical occurrence or a specification default;
- contradiction/unresolved reasons;
- whether all dependency reports bind to the same source/IFD;
- whether the metadata ordering/composition contract is internally valid;
- whether complete sample-domain execution has been validated.

The following safety fields must remain false at this audit stage:

- `image_sample_payload_read=false`
- `linearization_applied_to_image_samples=false`
- `black_subtraction_applied_to_image_samples=false`
- `normalization_applied_to_image_samples=false`
- `clipping_applied_to_image_samples=false`
- `table_application_to_image_samples_validated=false`
- `full_raw_to_linear_mapping_validated=false`
- `record_lifecycle_effect=false`
- `scientific_admission_effect=false`
- `downstream_scientific_consumer_enabled=false`
- `pixel_route_effect=false`
- `scientific_master_pixel_effect=false`
- `appearance_pixel_effect=false`
- `source_values_modified=false`
- `creates_new_evidence=false`
- `scientific_writeback_allowed=false`
- `parser_tag_rename_performed=false`
- `observation_evidence_ledger_identity_changed=false`

A successful metadata composition result must never flip `full_raw_to_linear_mapping_validated` to true.

---

## 8. Integrity requirements for any later implementation

Before a future code implementation of this read-only contract may be considered clean, regressions should prove:

1. same source bytes -> same parser occurrence records;
2. same physical Observation Evidence Ledger digest;
3. same observation-evidence digest;
4. tag 50712 remains `TAG_50712` unless a separate explicit migration is performed;
5. no new physical occurrence is minted for a DNG default;
6. no new source read/hash path is introduced;
7. no image sample is read;
8. no lifecycle/admission/consumer state changes;
9. no Scientific Master/Appearance pixel changes;
10. every dependency is source-SHA/Raw-IFD bound;
11. contradictory or unresolved dependencies fail closed;
12. `full_raw_to_linear_mapping_validated=false` remains locked.

The existing fixed digest regression for the LinearizationTable runtime exposure is a required preservation anchor, not a value to recompute opportunistically.

---

## 9. What would be required later for real sample-domain validation

A separate later scientific/runtime contract would be needed to prove actual ordered application to RAW samples. That future work would need its own evidence and tests for at least:

- exact sample-domain lookup behavior of LinearizationTable or identity default;
- coordinate/sample-plane binding for effective black subtraction;
- normalization/rescaling behavior;
- clipping/censoring semantics;
- numeric precision and boundary behavior;
- provenance of transformed values;
- authority transition of transformed samples;
- true fixtures and/or independently calculable sample vectors;
- no mutation of sealed source evidence;
- explicit downstream consumer identity.

Only such a later contract could even be considered for setting `full_raw_to_linear_mapping_validated=true`.

This audit does not authorize that work automatically.

---

## 10. Explicitly out of scope

This audit does **not** authorize:

- pixel implementation;
- image-sample reads;
- LinearizationTable application;
- black subtraction on pixels;
- normalization on pixels;
- clipping changes;
- parser semantic rename of tag 50712;
- ledger canonicalization changes;
- scientific admission or use;
- Scientific Master writeback;
- Appearance changes;
- camera/vendor-specific truth selection;
- AI/ML inference;
- new MEASURED evidence;
- merge/promotion merely because a documentation contract exists.

---

## 11. External scientific / normative anchors verified 2026-10-12

These sources guide semantics, models and tests but do not themselves create observation-specific evidence:

- Adobe — Digital Negative (DNG) Specification 1.7.1.0; Adobe DNG SDK resources. Current Adobe resource page also lists DNG SDK 1.7.1 Build 2724, 2026-09-08.  
  https://www.adobe.com/support/downloads/dng/dng_sdk.html
- ISO 12233:2024 — Digital cameras — Resolution and spatial frequency responses.  
  https://www.iso.org/standard/88626.html
- EMVA 1288 Release 4.0 — objective camera/image-sensor characterization.  
  https://www.emva.org/standards-technology/emva-1288/
- CIE 248:2022 — CIECAM16, for viewing-condition-specific colour appearance downstream of scientific scene formation.  
  https://www.cie.co.at/publications/cie-2016-colour-appearance-model-colour-management-systems-ciecam16
- CIE 249:2022-Cor1 — Visual Aspects of Time-Modulated Lighting Systems, Corrigendum 1, released 2026-10-05.  
  https://www.cie.co.at/publications/visual-aspects-time-modulated-lighting-systems-incl-corrigendum-1

---

## 12. Exact next action

The immediate next implementation step, if work continues on this audit branch, is **not** pixel processing.

It is to turn this document into one isolated read-only `SourceCapabilityRadiometricRawToLinearContractValidatorV01` that only composes existing validator outcomes, accompanied by integrity tests proving zero change to parser/ledger/source/pixels/Scientific Master/Appearance/admission/use.

Until such a validator exists and passes its own exact-head checks, this document remains the current audit boundary.
