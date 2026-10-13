# D.RAW 44489 — Source Capability Raw-to-Linear Contract Audit v0.1 — 2026-10-12

Status: **BINDING AUDIT / DESIGN CONTRACT + IMPLEMENTED READ-ONLY CANDIDATE CHECKPOINT (2026-10-13)**  
Continuation code: **44489**  
Exact integrated base anchor: `96be8a23c0d43f5dec304bdeb635824868cc80a7`  
Audit/candidate branch: `audit/source-capability-raw-to-linear-contract-v01`  
Draft PR: **#136 — open / unmerged**  
Last exact code+CI checkpoint before the 2026-10-13 documentation updates: `062a1dd58cd6c4e220d1a4a66525a7152d413b54`

Permanent motto: **One Free World. Many sealed observations. One evidence law.**

---

## 0. 2026-10-13 implementation checkpoint — read this before the original design sections

The audit boundary below has now been implemented as one isolated read-only candidate without creating a second Source Capability cable.

At exact code checkpoint:

`062a1dd58cd6c4e220d1a4a66525a7152d413b54`

all of the following are true:

- `SourceCapabilityRadiometricRawToLinearContractValidatorV01` exists;
- its schema is `D.RAW/SourceCapabilityRadiometricRawToLinearContract/0.1`;
- its status is `SHADOW_ONLY_RAW_TO_LINEAR_ORDERING_COMPOSITION_CONTRACT`;
- it consumes only the existing LinearizationTable report and existing Effective Black / Normalization report;
- it never receives parsed source metadata and never invokes `DngContainerMetadataParser`;
- it never reads image sample payload;
- it is exposed through the **existing** `SourceCapabilityRuntimeEvidenceEnvelope/0.1` only;
- the runtime envelope computes the existing Effective Black / Normalization report once;
- the runtime envelope computes the existing LinearizationTable report once;
- the compositor receives those exact already-computed report objects;
- there is one compositor call and no duplicate parser/read/hash/ledger/Raw-IFD/component-validator path;
- malformed dependency-report IFD identity is explicit `UNRESOLVED` fail-closed;
- duplicate dependency-report IFD identity is `CONTRADICTED` fail-closed;
- source-SHA mismatch is `CONTRADICTED` fail-closed;
- the valid metadata prefix may be described, but complete raw-to-linear composition remains unresolved because clipping has no separately validated Source Capability dependency contract;
- parser semantic key for tag 50712 remains `TAG_50712`;
- the Observation Evidence Ledger identity is unchanged;
- no scientific authority, admission, use, pixel route, Scientific Master, Appearance, source mutation, evidence creation or writeback is enabled.

Exact dedicated and broad compatibility checks on `062a1dd58...` were SUCCESS:

- Source Capability Raw-to-Linear Contract v0.1 Integrity;
- Source Capability Runtime Evidence Envelope v0.1 Integrity;
- Source Capability LinearizationTable v0.1 Integrity;
- Source Capability LinearizationTable Runtime Diagnostic v0.1 Integrity;
- Canonical Integrity;
- Documentation Governance;
- Shared Scientific Context v0.1 Integrity;
- D.RAW Suite Universal Intake v0.1;
- D.RAW Universal Physical Capture v0.3 Live Preview Macro;
- Tile-Native DNG Source v0.2 Android Compatibility;
- D.RAW Android DngCreator Compatibility v0.1;
- TruthRange v0.4 / v0.5 Integrity;
- XYZ D50 Uncertainty v0.7 Integrity;
- Camera RGB Covariance v0.6 Integrity.

Path-filtered/skipped and therefore **not failures**:

- D.RAW Free Raster v0.3 Finish APK;
- D.RAW Deep-Censor Chroma Guard v0.1.

This checkpoint is still an **unintegrated candidate** on PR #136. It does not replace integrated scientific/runtime anchor `96be8a23...` merely because the branch is newer or green.

---

## 1. Why this audit exists

PR #135 is integrated at:

`96be8a23c0d43f5dec304bdeb635824868cc80a7`

At that anchor, LinearizationTable is visible as a read-only runtime diagnostic while remaining **not ADMITTED, not USED and not applied to RAW image samples**. Parser identity remains `TAG_50712`; Observation Evidence Ledger identity remains unchanged; locked digests remain:

- `ledger_digest_sha256 = 6740e6e04c18797e8b57604f30c1884c6e8428e9a469e53cb139c26abe59afb3`
- `observation_evidence_digest_sha256 = 2bdaa44fe3435929310b79fa38d6ce3b0aa0cc05ef68f9522b4554c6862b38e6`

The Source Capability radiometric layer already contains separate contracts for:

- Raw-IFD domain;
- radiometric semantics;
- radiometric encoding consistency;
- DNG version compatibility;
- spatial BlackLevel dependencies;
- Effective Black / normalization;
- LinearizationTable metadata semantics and read-only runtime exposure.

The purpose of this audit was to add a combined ordering/composition contract without reading or changing image samples. That metadata-only candidate now exists on PR #136; real sample-domain mapping remains out of scope and unvalidated.

---

## 2. Normative DNG ordering boundary

Normative specification anchor: **Adobe Digital Negative (DNG) Specification 1.7.1.0**.

Relevant conceptual order:

`Linearization -> Black Subtraction -> Rescaling / Normalization -> Clipping`

That order is a specification dependency. It does **not** prove complete D.RAW sample-domain execution.

Current state:

- LinearizationTable metadata contract: validated/read-only diagnostic where applicable;
- Effective Black / Normalization metadata contract: validated/read-only diagnostic where applicable;
- combined prefix ordering/composition candidate: implemented read-only on PR #136;
- clipping Source Capability dependency: unresolved/not separately validated;
- complete ordered sample-domain raw-to-linear mapping: **not validated**;
- LinearizationTable sample application: **not validated and not performed**;
- scientific admission/use: **not implied by validation**.

Permanent relation:

`VALIDATED != ADMITTED != USED`

---

## 3. Contract identity and role

Object:

`SourceCapabilityRadiometricRawToLinearContractValidatorV01`

Schema:

`D.RAW/SourceCapabilityRadiometricRawToLinearContract/0.1`

Status:

`SHADOW_ONLY_RAW_TO_LINEAR_ORDERING_COMPOSITION_CONTRACT`

Role:

- bind existing validator reports;
- preserve source/IFD identity;
- describe the normative dependency order;
- expose contradictions and unresolved dependencies fail-closed;
- distinguish metadata-order composition from pixel-domain execution.

It is **not**:

- a parser;
- a second source-read or hash path;
- a new Observation Evidence Ledger;
- a pixel consumer;
- a clipping implementation;
- a scientific promotion path.

---

## 4. Allowed inputs — single-cable rule

The current v0.1 compositor takes only:

1. the already-existing `SourceCapabilityRadiometricLinearizationTableValidatorV01` report;
2. the already-existing `SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01` report;
3. the expected source SHA used to prove common source binding.

It does not receive the parser output itself.

The surrounding existing runtime envelope remains responsible for the single component-validator calls. Conceptually:

`existing parsed metadata`
`-> existing LinearizationTable validator (once)`
`-> existing Effective Black / Normalization validator (once)`
`-> same report objects`
`-> Raw-to-Linear compositor (once)`
`-> same SourceCapabilityRuntimeEvidenceEnvelope/0.1`

This is the required architecture. Do not replace it with a compositor that re-runs the parser, re-runs the component validators, recomputes source hashes or creates a second ledger/envelope route.

---

## 5. Dependency binding

### Stage A — encoded RAW domain

Raw-IFD and encoding semantics remain owned by existing validators and their transitive dependencies. The compositor does not build a second Raw-IFD detector.

### Stage B — Linearization

Bind the existing LinearizationTable report.

Valid metadata cases may include:

- explicit physical tag 50712 validated under the standalone contract; or
- DNG specification identity default when the tag is absent.

The absent identity default is a specification dependency, not a physical occurrence. It may not mint a ledger record.

Parser semantic key remains `TAG_50712` unless a separate explicit provenance/schema migration is ever approved.

### Stage C — Black subtraction dependency

Bind existing BlackLevel / BlackLevelRepeatDim / BlackLevelDeltaH / BlackLevelDeltaV / ActiveArea semantics through the already-existing Effective Black path.

Current metadata relation remains:

`effective_black = BlackLevel[row mod repeatRows][col mod repeatCols][sample] + BlackLevelDeltaH[col] + BlackLevelDeltaV[row]`

No image sample is subtracted by this contract.

### Stage D — Rescaling / normalization dependency

Bind the existing normalization report and denominator rule:

`normalization_denominator = WhiteLevel[sample] - max_effective_black[sample]`

The denominator must remain finite and strictly positive where the existing validator claims a valid dependency.

No image sample is normalized by this contract.

### Stage E — Clipping dependency

Clipping is the next normative stage, but the current Source Capability layer has no separately validated clipping/censoring dependency contract sufficient to complete this composition.

Therefore current state is explicitly:

`UNRESOLVED_CLIPPING_COMPONENT_NOT_VALIDATED`

and:

`clipping_component_validator_present=false`

Do not equate any of these:

- DNG post-normalization clipping/order boundary;
- WhiteLevel metadata / encoding boundary;
- observed clipping/censoring pattern;
- physical sensor saturation / scene-radiance censoring;
- display/output white or highlight roll-off.

“Clipping exists elsewhere in D.RAW” is not proof that this Source Capability stage is jointly validated.

WhiteLevel must not silently become a claim of exact physical sensor saturation truth.

---

## 6. Fail-closed identity and dependency semantics

Current implemented rules:

- source-SHA mismatch -> `CONTRADICTED`;
- duplicate component report IFD identity -> `CONTRADICTED` rather than silent selection;
- malformed component report IFD identity -> explicit `UNRESOLVED` identity-integrity state;
- missing required report -> `UNRESOLVED`;
- schema/status mismatch -> unresolved or contradicted according to the named condition;
- unresolved component remains unresolved;
- contradicted component makes composition contradicted;
- a valid local IFD prefix does not erase an envelope-level malformed identity condition;
- DNG identity default remains a dependency value and never a physical occurrence.

Why malformed is unresolved rather than automatically contradicted:

malformed report structure proves that the dependency identity cannot be trusted/completely composed, but it is not automatically physical counter-evidence about the source scene or sensor. Duplicate identities or explicit source-binding mismatches are stronger contradictions because they prevent unique trustworthy binding.

---

## 7. Mandatory report fields / safety assertions

The report carries the ordered stages:

`[LINEARIZATION, BLACK_SUBTRACTION, RESCALING_NORMALIZATION, CLIPPING]`

and explicit source/report/IFD binding state.

All of the following remain false:

- `metadata_ordering_composition_contract_validated=false`
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

A successful prefix composition must never flip complete mapping true.

---

## 8. Integrity contract — implemented CI

Workflow:

`.github/workflows/source-capability-raw-to-linear-contract-v01-integrity.yml`

The dedicated gate enforces at least:

- exactly one Effective Black / Normalization `.describe(...)` call in the existing runtime envelope;
- exactly one LinearizationTable `.describe(...)` call in the existing runtime envelope;
- exactly one Raw-to-Linear compositor `.describe(...)` call in that envelope;
- compositor source does not invoke `DngContainerMetadataParser`;
- parser tag 50712 is not renamed to `LinearizationTable`;
- clipping remains explicitly unresolved;
- malformed/duplicate identity fail-closed constants remain present;
- all pixel/authority/writeback safety fields remain false.

Its regression set includes:

- `SourceCapabilityRadiometricRawToLinearContractValidatorV01Test`;
- `SourceCapabilityRawToLinearRuntimeExposureV01Test`;
- `SourceCapabilityRadiometricLinearizationTableValidatorV01Test`;
- `SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01Test`;
- `SourceCapabilityRuntimeEvidenceEnvelopeV01Test`.

The runtime-exposure regression proves that the existing parent envelope remains non-consuming and pixel/master/appearance/writeback inert after the compositor report is added.

Exact checkpoint `062a1dd58...` passed this dedicated gate and the broad compatibility checks listed in section 0.

---

## 9. What is required later for real sample-domain validation

A **separate later scientific/runtime contract** would be required for actual ordered application to RAW samples. It would need independent evidence/tests for at least:

- exact sample-domain LinearizationTable lookup or identity-default behavior;
- coordinate/sample-plane binding for effective black subtraction;
- normalization/rescaling behavior;
- clipping/censoring semantics;
- numeric precision and boundary behavior;
- transformed-value provenance;
- authority transition of transformed samples;
- true fixtures and/or independently calculable sample vectors;
- no mutation of sealed source evidence;
- explicit downstream consumer identity.

Only such later evidence could even make `full_raw_to_linear_mapping_validated=true` a candidate claim.

PR #136 does not authorize that automatically.

---

## 10. Explicitly out of scope

This contract does **not** authorize:

- image-sample reads;
- LinearizationTable application to samples;
- black subtraction on pixels;
- normalization on pixels;
- clipping implementation or clipping recolouring;
- parser semantic rename of tag 50712;
- ledger canonicalization changes;
- scientific admission/use;
- Scientific Master writeback;
- Appearance changes;
- camera/vendor-specific truth selection;
- AI/ML scientific inference;
- new MEASURED evidence;
- merge/promotion merely because CI is green.

---

## 11. External scientific / normative anchors

These sources guide semantics, models and tests but do not create observation-specific evidence:

- Adobe — Digital Negative (DNG) Specification 1.7.1.0 / DNG SDK resources;
- ISO 12233:2024 — camera resolution / spatial frequency response;
- EMVA 1288 Release 4.0 — camera/image-sensor characterization;
- CIE 248:2022 — CIECAM16 for downstream colour appearance;
- CIE 249:2022-Cor1 — time-modulated lighting systems;
- PBRT 4th edition — radiometry/light transport;
- Hartley & Zisserman / Oxford VGG — projective and multi-view geometry.

General scientific knowledge may shape contracts and tests; it never mints source-specific evidence by itself.

---

## 12. Exact next action after the 2026-10-13 checkpoint

The old instruction “create the read-only compositor” is complete on PR #136 and must not be repeated by a new chat.

The next scientific edge is **not** to make the current compositor read pixels and **not** to add a second cable.

The next task is to audit the exact semantics of a possible future Source Capability clipping/censoring dependency, beginning from existing parser/semantic/WhiteLevel/censoring structures and proving whether any existing cable can be reused.

That audit must keep separate:

1. DNG post-normalization clipping/order semantics;
2. WhiteLevel as source metadata/encoding dependency;
3. observed sample clipping/censoring patterns;
4. physical sensor saturation and scene-radiance censoring;
5. downstream display/output white and highlight roll-off.

If no existing Source Capability dependency proves the needed semantics, clipping stays unresolved. Do not create a validator merely to make the ordering graph look complete.

Until separately proven:

`full_raw_to_linear_mapping_validated=false`

remains the correct scientific result.

PR #136 remains draft/unmerged pending an explicit later integration decision.

---

## 13. Exact implementation checkpoints

Key commits in the PR #136 candidate line:

- `215d86864ec002787604eb4e3055e7ce10fd3578` — initial fail-closed metadata compositor;
- `6dc59b89f66846d1dd2dd67e8a91b6ebdd8b8b4b` — compositor hardening;
- `0fd46f284a7bec67f490eb08395d283a1056299f` — initial compositor tests;
- `efe0b95a...` — malformed component identity becomes explicit unresolved fail-closed;
- `9da66b5b...` — malformed/duplicate identity regression coverage;
- `98234f2c9b00590e159e7a99f4fa47b689e4e426` — single-cable integrity gate checkpoint;
- `131ab30217af0aa27bd8282ae6317e2d6c646ae2` — compositor exposed through the existing runtime envelope using reused report objects;
- `416227e8d8f743d13ebf148ca4d21ee4810b9438` — runtime exposure regression;
- `062a1dd58cd6c4e220d1a4a66525a7152d413b54` — tightened exact single-cable gate and last fully green code checkpoint before the 44489 documentation update.

Documentation commits after `062a1dd58...` must not be reinterpreted as new scientific/runtime integration anchors.
