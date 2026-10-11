# D.RAW 44489 — Effective Black Runtime Diagnostic v0.1 — live step handoff

Date: 2026-10-11

Permanent motto: **One Free World. Many sealed observations. One evidence law.**

## Step 1 — governance boundary closed before scientific diagnostic work

Completed:

- PR #132 — `Architecture: separate runtime Source Capability evidence-envelope schema v0.1` — is merged/closed.
- Final PR #132 user-authored green head: `6b8fec8da1c9ef67c5db904663a122e9cec8652f`.
- PR #132 merge commit: `4c27255ee1f5f95f4b623289d3785e5038c8cbe6`.
- Historical Exact Gauge checkpoint `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f` remains the frozen narrow historical reference; live PR #130 remains closed superseded/unmerged.
- Runtime parser-bound envelope identity is now unambiguous: `D.RAW/SourceCapabilityRuntimeEvidenceEnvelope/0.1`.
- Canonical historical Physical Knowledge Map remains `D.RAW/SourceCapabilityEnvelope/0.2`.
- Observation Evidence Ledger remains `D.RAW/ObservationEvidenceLedger/0.1`.
- New branch created from exact PR #132 merge commit: `feat/source-capability-effective-black-runtime-diagnostic-v01`.
- Draft PR #133 is the isolated review unit for this diagnostic exposure.
- Initial branch-state commit: `012df2e35da25e7fe50d662c4d5ad10d2da0b89d`.

## Scientific boundary for this branch

This branch exposes the already-existing `SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01` report only as an additional **read-only runtime diagnostic**.

It must not:

- alter ledger digest or ledger records;
- alter lifecycle or admission state;
- enable a downstream scientific consumer;
- add a parser, source read, hash path or image-sample read;
- normalize pixels;
- alter pixel routing, Scientific Master or Appearance;
- create evidence;
- permit scientific writeback;
- imply `LinearizationTable` validation;
- claim complete raw-to-linear validity, admission or use;
- promote any scientific candidate.

## Step 2 — runtime diagnostic exposure implemented and focused-regression validated

Implementation code/test checkpoint:

`1d0fe23779285d2e07bce5bd45e56fd291a599cd`

Implemented and pre-commit validated:

- `SourceCapabilityEnvelopeV02.describe(...)` attaches `effective_black_normalization_v0_1` only after the Observation Evidence Ledger has already been built;
- the attached value is the existing `SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.describe(...)` report, without reinterpretation, admission or consumer binding;
- `SourceCapabilityEnvelopeV02.unavailable(...)` remains fail-closed and does not invent the diagnostic after parser failure;
- the attached diagnostic is regression-compared with a standalone validator run on equivalent parsed metadata;
- the parsed metadata object is unchanged by envelope construction;
- DNG defaults do not create BlackLevel/DeltaH/DeltaV/WhiteLevel physical ledger occurrences;
- admitted/used capability sets remain empty in the focused fixture and downstream consumers remain disabled;
- validator, pixel-route, Scientific Master, Appearance, evidence-creation and writeback firewalls remain false;
- `linearization_table_processing_validated=false` and `full_raw_to_linear_mapping_validated=false` remain explicit.

The deterministic patch runner completed all three focused Gradle regressions with `BUILD SUCCESSFUL` before the implementation commit was accepted. A first bot push attempt failed only because a GitHub App without workflow permission attempted to update a workflow file; no scientific/test failure occurred. The commit route was then narrowed so workflow files were excluded from the bot commit.

Permanent integrity-workflow update:

`dffc32c39d475bfd8893cff4817939851d9eab50`

It now requires the Effective Black diagnostic exposure and runs together:

- `SourceCapabilityRuntimeEvidenceEnvelopeV01Test`;
- `SourceCapabilityEffectiveBlackRuntimeDiagnosticV01Test`;
- `SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01Test`.

The one-time patch workflow and script were subsequently removed from the branch; they are not part of the final PR #133 diff.

## Step 2.1 — evidence-identity proof strengthened

Digest-lock regression commit:

`6fcbe1ca25d449d2eac887661fb54a033d3ed07e`

The focused runtime regression independently constructs the exact expected physical occurrence records for its pre-existing TIFF/DNG fixture and verifies:

- `ledger_digest_sha256` equals `SourceCapabilityEnvelopeV02.ledgerDigest(expectedOccurrenceRecords)`;
- `observation_evidence_digest_sha256` equals the exact digest derived from that unchanged ledger digest plus the empty traversal-anomaly digest;
- record count remains exactly the expected physical occurrence count;
- default radiometric dependencies still mint no physical occurrences.

This converts the evidence-identity constraint from an architectural inference into an executable regression: adding the diagnostic may add a report field, but it may not change the underlying Observation Evidence Ledger identity.

## Step 3 — exact-head CI acceptance for the diagnostic line

Exact green code/test/handoff checkpoint:

`bd87bf5ceb1698fd1b4214a638d60b5e43f6420a`

All triggered non-skipped workflows observed on that exact head completed **SUCCESS**:

- Source Capability Runtime Evidence Envelope v0.1 Integrity;
- Canonical Integrity;
- Documentation Governance;
- D.RAW Suite Universal Intake v0.1;
- D.RAW Android DngCreator Compatibility v0.1;
- Tile-Native DNG Source v0.2 Android Compatibility;
- D.RAW Universal Physical Capture v0.3 Live Preview Macro;
- Shared Scientific Context v0.1 Integrity;
- Camera RGB Covariance v0.6 Integrity;
- XYZ D50 Uncertainty v0.7 Integrity;
- TruthRange v0.4 Integrity;
- TruthRange v0.5 Integrity.

Path-scope `SKIPPED`, not failed:

- D.RAW Deep-Censor Chroma Guard v0.1;
- D.RAW Free Raster v0.3 Finish APK.

Meaning of this acceptance is deliberately narrow: the runtime envelope can now expose the metadata-only Effective Black / Normalization report without changing observation-evidence identity or any scientific/pixel authority. CI success does **not** turn the report into admission, use, a pixel normalization operation, a calibration promotion or real-device radiometric proof.

## Current state / next order

PR #133 remains draft/unmerged until this step handoff is committed and its final review state is checked. No scientific promotion has occurred.

After PR #133 is cleanly integrated, the next scientific branch may implement a separate `LinearizationTable` **shadow validator** anchored to DNG 1.7.1.0. It must remain metadata-only/read-only and must not silently apply linearization to pixels or make complete raw-to-linear admission/use true.

Even after this diagnostic integration:

`VALIDATED != ADMITTED != USED`

and complete raw-to-linear validity/admission/use remains false until the later separate `LinearizationTable` contract and the full ordered raw-to-linear dependency chain are jointly validated.
