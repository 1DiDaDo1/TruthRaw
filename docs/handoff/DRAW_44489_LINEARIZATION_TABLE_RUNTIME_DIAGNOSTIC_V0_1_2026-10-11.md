# D.RAW 44489 — LinearizationTable runtime diagnostic v0.1 — 2026-10-11

Status: **read-only runtime-envelope exposure implemented; permanent old+new LinearizationTable integrity gates and all non-skipped exact-head CI are green on `d0e2ed9921662c1bce9324115705989e8dee99d4`; documentation-head revalidation and merge still pending**.

## Exact integration anchor

- PR #134 `Source Capability: LinearizationTable shadow validator v0.1` is merged.
- Final PR #134 head: `c4474904ec9bd00552f1378fd50896dac0d7be61`.
- PR #134 merge commit / base integration anchor: `f9547cfe9b04c732d5543d2b42cd4513e83ab9ab`.
- Working branch: `feat/source-capability-linearization-table-runtime-diagnostic-v01`.
- Draft PR: #135 — `Source Capability: expose LinearizationTable read-only runtime diagnostic v0.1`.
- Initial branch/handoff checkpoint: `67f98a707ec8533405b16b1ddae48853b5165487`.
- Exposure regression test commit: `ef3e68e2b728b706e56cff5a1b844c67eec58b9a`.
- Envelope exposure code commit: `090b7330f2ca93e682bf864cb307a4fb3b75aecc`.
- Temporary deterministic patcher/workflow cleanup head: `57609d61d50a3c30ee90d7230dcccc072c854cd3`.
- Permanent runtime-diagnostic integrity workflow commit: `5a201fc0293c27f68f1cbfdbe25c8685d474faab`.
- Standalone-integrity boundary alignment + fully validated code/workflow head: `d0e2ed9921662c1bce9324115705989e8dee99d4`.

The PR #134 merge and PR #135 work remain engineering/governance diagnostics only. They do not change `VALIDATED != ADMITTED != USED`, do not apply LinearizationTable to image samples, and do not promote scientific authority.

## Integrated standalone contract

`SourceCapabilityRadiometricLinearizationTableValidatorV01` / `D.RAW/SourceCapabilityRadiometricLinearizationTable/0.1` remains the standalone metadata-only contract.

Binding constraints remain:

- physical tag 50712 is consumed by numeric tag from the existing parser `entries` occurrence;
- parser semantic key remains `TAG_50712`;
- parser/tag rename is deliberately deferred because a rename would change canonical Observation Evidence Ledger identity for unchanged source bytes;
- absent tag uses the DNG identity default but mints no physical source occurrence;
- explicit tag requires TIFF SHORT and count 2..65536;
- floating-point raw and BitsPerSample >16 with explicit table fail closed;
- count is not required to equal `2^BitsPerSample`;
- duplicate tag occurrences remain unresolved with no silent selection;
- no table application to image samples is validated or performed.

## Runtime diagnostic exposure now implemented

`SourceCapabilityEnvelopeV02.describe(...)` now adds exactly one read-only report field:

`radiometric_linearization_table_v0_1`

The field is populated by calling the already-integrated `SourceCapabilityRadiometricLinearizationTableValidatorV01.describe(...)` with the same `sourceSha256` and already-parsed metadata object used by the other diagnostics.

The new diagnostic is added **after** the physical Observation Evidence Ledger/digests are built. It does not participate in record construction, classification, lifecycle, admission, consumers, ledger canonicalization or source parsing.

No parser source file and no standalone validator source file were changed by the deterministic exposure patch.

## New runtime-exposure regression

`SourceCapabilityRadiometricLinearizationTableRuntimeDiagnosticV01Test` now proves the exposure boundary.

For an explicit physical Raw-IFD fixture containing tag 50712, fixed pre-exposure baseline identities are locked as:

- `ledger_digest_sha256 = 6740e6e04c18797e8b57604f30c1884c6e8428e9a469e53cb139c26abe59afb3`
- `observation_evidence_digest_sha256 = 2bdaa44fe3435929310b79fa38d6ce3b0aa0cc05ef68f9522b4554c6862b38e6`

The test also locks that the physical tag-50712 record remains:

- semantic key `TAG_50712`;
- lifecycle `PARSED`;
- admission `UNSUPPORTED_SEMANTICS`;
- authority `UNKNOWN_SEMANTICS`;
- zero consumers.

It verifies that the already-parsed metadata JSON is unchanged by `SourceCapabilityEnvelopeV02.describe(...)` and that no capabilities become admitted or used.

A second fixture proves that the specification identity default is visible in the diagnostic while no physical tag-50712 record is minted.

## Targeted pre-commit validation

Temporary workflow `Apply LinearizationTable Runtime Diagnostic v0.1`, run `38166856331`, completed **SUCCESS** before allowing the envelope-only code commit.

The successful sequence was:

1. deterministic envelope-only patch;
2. scope guard proving no parser or standalone-validator diff;
3. `SourceCapabilityRadiometricLinearizationTableRuntimeDiagnosticV01Test`;
4. `SourceCapabilityRadiometricLinearizationTableValidatorV01Test`;
5. `SourceCapabilityRuntimeEvidenceEnvelopeV01Test`;
6. `SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01Test`;
7. envelope-only commit/push.

The temporary patch script and temporary apply workflow were then removed from the branch. They are not part of the intended final PR #135 surface.

## Permanent exact-head validation

The first permanent runtime-diagnostic head exposed one intentional governance mismatch: the older standalone `Source Capability LinearizationTable v0.1 Integrity` workflow still contained the pre-exposure rule that the validator must not be called from `SourceCapabilityEnvelopeV02`. Its boundary step failed before Gradle, while the new runtime-diagnostic gate was already green. This was not treated as a validator or pixel-path failure.

Commit `d0e2ed9921662c1bce9324115705989e8dee99d4` replaces only that obsolete negative assertion with a stronger post-exposure contract:

- parser tag 50712 still must not be renamed to `LinearizationTable`;
- the runtime envelope must contain exactly one `radiometric_linearization_table_v0_1` report key;
- it must contain exactly one call to `SourceCapabilityRadiometricLinearizationTableValidatorV01.describe(...)`;
- all standalone metadata-only false/firewall assertions remain required;
- the standalone LinearizationTable + encoding + Raw-IFD regression set still runs.

On exact head `d0e2ed9921662c1bce9324115705989e8dee99d4`:

- `Source Capability LinearizationTable v0.1 Integrity`, run `38167611304`: **SUCCESS**;
- `Source Capability LinearizationTable Runtime Diagnostic v0.1 Integrity`, run `38167611297`: **SUCCESS**;
- `Source Capability Runtime Evidence Envelope v0.1 Integrity`: **SUCCESS**;
- `D.RAW Suite Universal Intake v0.1`: **SUCCESS**;
- `D.RAW Universal Physical Capture v0.3 Live Preview Macro`: **SUCCESS**;
- `D.RAW Android DngCreator Compatibility v0.1`: **SUCCESS**;
- `Canonical Integrity`: **SUCCESS**;
- `Documentation Governance 2026-09-10`: **SUCCESS**;
- `Shared Scientific Context v0.1 Integrity`: **SUCCESS**;
- `Tile-Native DNG Source v0.2 Android Compatibility`: **SUCCESS**;
- `TruthRange v0.4 Integrity`: **SUCCESS**;
- `TruthRange v0.5 Integrity`: **SUCCESS**;
- `Camera RGB Covariance v0.6 Integrity`: **SUCCESS**;
- `XYZ D50 Uncertainty v0.7 Integrity`: **SUCCESS**.

`D.RAW Free Raster v0.3 Finish APK` and `D.RAW Deep-Censor Chroma Guard v0.1` were path-scope **SKIPPED** on this head. At completion there were no queued or in-progress runs and no non-skipped failure/action-required result.

This proves engineering compatibility of the read-only diagnostic exposure. It is not scientific admission, not `USED`, and not proof of raw-sample linearization.

## Required report state remains closed

Even when metadata validation is `VALIDATED_LINEARIZATION_TABLE_METADATA_CONTRACT_SHADOW_ONLY`, the runtime diagnostic continues to state:

- `parser_tag_rename_performed=false`;
- `observation_evidence_ledger_identity_changed=false`;
- `table_application_to_image_samples_validated=false`;
- `linearization_applied_to_image_samples=false`;
- `full_raw_to_linear_mapping_validated=false`;
- `image_sample_payload_read=false`;
- `record_lifecycle_effect=false`;
- `scientific_admission_effect=false`;
- `pixel_route_effect=false`;
- `scientific_master_pixel_effect=false`;
- `appearance_pixel_effect=false`;
- `source_values_modified=false`;
- `creates_new_evidence=false`;
- `scientific_writeback_allowed=false`.

The runtime envelope itself also remains `SHADOW_READ_ONLY_ZERO_PIXEL_CHANGE`, with downstream consumers disabled and no Scientific Master or Appearance pixel effect.

## Remaining order

1. Update canonical 44489 with the validated `d0e2ed99...` checkpoint and governance-boundary correction.
2. Run exact-head CI again on the resulting documentation head.
3. Recheck PR #135 mergeability, changed-file scope and review state.
4. Only if that final exact head is clean may PR #135 be taken out of draft and merged as a read-only diagnostic integration boundary.
5. Merge status must not be interpreted as scientific admission or pixel-domain application.
6. The next scientific step after integration is a separate read-only audit of the complete `Linearization -> Black Subtraction -> Normalization -> Clipping` contract; no pixel application is authorized by this handoff.

## Explicitly out of scope

This branch must not:

- rename tag 50712 in `DngContainerMetadataParser`;
- change ledger canonicalization;
- admit or consume LinearizationTable scientifically;
- apply the lookup table to source samples;
- claim complete raw-to-linear validity;
- change black subtraction, normalization or clipping behavior;
- modify Scientific Master, reconstruction, Free Raster, JPEG or Appearance.

Complete raw-to-linear application remains a later scientific/runtime contract. Parser semantic renaming remains a separate provenance/schema migration.
