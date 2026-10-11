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

## Scientific boundary for this branch

This branch may expose the already-existing `SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01` report only as an additional **read-only runtime diagnostic**.

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

Required proof before this branch can be considered complete:

1. runtime envelope contains an `effective_black_normalization_v0_1` diagnostic field;
2. its source SHA remains bound to the envelope source SHA;
3. before/after ledger digest and observation-evidence digest are unchanged for the same parsed metadata;
4. record count/content, parsed/admitted/used capability sets and consumer state remain unchanged;
5. all explicit validator firewalls remain false;
6. runtime envelope pixel/master/Appearance/writeback firewalls remain false;
7. `LinearizationTable` and `full_raw_to_linear_mapping_validated` remain false;
8. exact-head CI passes before promotion of this branch state.

## Step 2 — runtime diagnostic exposure implemented

Implemented in this step:

- `SourceCapabilityEnvelopeV02.describe(...)` now attaches `effective_black_normalization_v0_1` only after the Observation Evidence Ledger has already been built;
- the attached value is the existing `SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.describe(...)` report, without reinterpretation or admission;
- `SourceCapabilityEnvelopeV02.unavailable(...)` remains fail-closed and does not invent a diagnostic when parsing failed;
- a focused runtime regression compares the attached diagnostic byte-for-byte at JSON level with a standalone validator run on equivalent parsed metadata;
- the regression proves the parsed metadata object is not mutated by runtime-envelope construction;
- DNG defaults still do not create BlackLevel/DeltaH/DeltaV/WhiteLevel physical ledger occurrences;
- admitted/used capability sets remain empty in the focused case and downstream consumers remain disabled;
- all validator, pixel-route, Scientific Master, Appearance, evidence-creation and writeback firewalls remain false;
- `LinearizationTable` and complete raw-to-linear validation remain explicitly false.

The existing runtime-schema integrity workflow is updated to require the diagnostic exposure and run the schema regression, the new attachment regression, and the standalone Effective Black validator regression together.

Exact implementation commit is recorded after the self-cleaning patch workflow completes.

## Next step

Record the exact implementation commit in this handoff, then run and audit exact-head CI. Do not begin `LinearizationTable` work until this exposure is fully green.
