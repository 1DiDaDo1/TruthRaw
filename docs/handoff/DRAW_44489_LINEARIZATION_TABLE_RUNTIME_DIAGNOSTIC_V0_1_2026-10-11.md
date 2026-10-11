# D.RAW 44489 — LinearizationTable runtime diagnostic v0.1 — 2026-10-11

Status: active post-PR-#134 handoff; runtime-envelope exposure not yet implemented at this checkpoint.

## Exact integration anchor

- PR #134 `Source Capability: LinearizationTable shadow validator v0.1` is merged.
- Final PR #134 head: `c4474904ec9bd00552f1378fd50896dac0d7be61`.
- Merge commit / new base integration anchor: `f9547cfe9b04c732d5543d2b42cd4513e83ab9ab`.
- Base branch `fix/android-exact-gauge-pass-artifact-v03` is verified at that merge commit.
- New working branch: `feat/source-capability-linearization-table-runtime-diagnostic-v01`.

The PR #134 merge is an engineering/governance integration only. It does not change `VALIDATED != ADMITTED != USED`, does not apply LinearizationTable to image samples, and does not promote scientific authority.

## Integrated standalone contract

`SourceCapabilityRadiometricLinearizationTableValidatorV01` / `D.RAW/SourceCapabilityRadiometricLinearizationTable/0.1` is now integrated as a standalone metadata-only validator.

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

## Next narrow step — runtime diagnostic exposure only

The only allowed next runtime change is to expose the already-proven standalone report read-only through `SourceCapabilityRuntimeEvidenceEnvelope/0.1` using the already-parsed metadata object.

The exposure must prove, before integration, that it does **not** change:

1. `ledger_digest_sha256`;
2. `observation_evidence_digest_sha256`;
3. physical occurrence records or their semantic keys;
4. record lifecycle;
5. record admission;
6. record consumers;
7. source-read count/path;
8. parser behavior or tag naming;
9. image sample payload reads;
10. pixel route;
11. Scientific Master pixels/state;
12. Appearance pixels/state;
13. source values;
14. evidence creation;
15. scientific writeback.

The underlying tag-50712 occurrence must remain `TAG_50712`, lifecycle `PARSED`, admission `UNSUPPORTED_SEMANTICS`, zero consumers unless a later separate authority/admission migration explicitly proves otherwise.

## Required report state after exposure

Even if metadata validation is `VALIDATED_LINEARIZATION_TABLE_METADATA_CONTRACT_SHADOW_ONLY`, the runtime diagnostic must continue to state:

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
