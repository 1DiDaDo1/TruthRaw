# D.RAW / TruthRaw — Document Status Index — 2026-10-13 — 44489

Continuation code: **44489**

## Current operational entry points

Read in this order:

1. `START_HERE_NEW_CHAT.md`
2. `docs/handoff/DRAW_44489_MASTER_CAPSULE_CURRENT.md`
3. `docs/handoff/DRAW_44489_NEXT_CHAT_2026-10-13.md`
4. resolve live head of `audit/source-capability-raw-to-linear-contract-v01`
5. `state/DRAW_PROJECT_STATE_2026-10-13.json`
6. this file
7. `docs/handoff/DRAW_44489_RAW_TO_LINEAR_CONTRACT_AUDIT_V0_1_2026-10-12.md`
8. `docs/handoff/DRAW_44489_INTERDISCIPLINARY_OBSERVATION_AUDIT_2026-10-10.md`

## Current authority split

### Integrated scientific/runtime authority

`96be8a23c0d43f5dec304bdeb635824868cc80a7`

This remains the integrated PR #135 anchor.

### Verified unintegrated candidate code

`e6bdbce11c02ccac0e3edf31f39f0eb869105d51`

Dedicated exact-head Raw-to-Linear integrity workflow: **SUCCESS**.

PR #136 remains draft/open/unmerged. This candidate SHA does not replace the integrated anchor.

### Documentation/governance heads

Documentation commits after `e6bdbce1...` may be newer. They describe the verified candidate; they do not create new scientific authority. Always resolve the live branch before making a new code decision.

## Binding current documents

- `START_HERE_NEW_CHAT.md` — bootstrap/router.
- `docs/handoff/DRAW_44489_MASTER_CAPSULE_CURRENT.md` — broad project architecture, Evidence Law, subsystem map and historical navigation.
- `docs/handoff/DRAW_44489_NEXT_CHAT_2026-10-13.md` — newest raw-to-linear/clipping-rule delta and immediate next action; it supersedes only older current-checkpoint wording where the master still stops at `062a1dd58...`.
- `state/DRAW_PROJECT_STATE_2026-10-13.json` — machine-readable current candidate/integration/authority split.
- `docs/handoff/DRAW_44489_RAW_TO_LINEAR_CONTRACT_AUDIT_V0_1_2026-10-12.md` — original binding design audit plus earlier implemented checkpoint.
- `docs/handoff/DRAW_44489_INTERDISCIPLINARY_OBSERVATION_AUDIT_2026-10-10.md` — broad scientific foundation.

## Current Source Capability raw-to-linear files

### Existing reused cable/components

- `suite_android/app/src/main/java/com/truthraw/adaptiveui/SourceCapabilityEnvelopeV02.kt`
- `suite_android/app/src/main/java/com/truthraw/adaptiveui/SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.kt`
- `suite_android/app/src/main/java/com/truthraw/adaptiveui/SourceCapabilityRadiometricLinearizationTableValidatorV01.kt`

### Current candidate composition/specification contracts

- `suite_android/app/src/main/java/com/truthraw/adaptiveui/SourceCapabilityRadiometricRawToLinearContractValidatorV01.kt`
- `suite_android/app/src/main/java/com/truthraw/adaptiveui/SourceCapabilityRadiometricRawToLinearClippingRuleValidatorV01.kt`

### Regressions / integrity

- `suite_android/app/src/test/java/com/truthraw/adaptiveui/SourceCapabilityRadiometricRawToLinearContractValidatorV01Test.kt`
- `suite_android/app/src/test/java/com/truthraw/adaptiveui/SourceCapabilityRadiometricRawToLinearClippingRuleValidatorV01Test.kt`
- `suite_android/app/src/test/java/com/truthraw/adaptiveui/SourceCapabilityRawToLinearRuntimeExposureV01Test.kt`
- `.github/workflows/source-capability-raw-to-linear-contract-v01-integrity.yml`

## Single-cable rule

The current verified candidate keeps:

- one existing runtime Source Capability envelope;
- one Effective Black/Normalization computation in that envelope;
- one LinearizationTable computation in that envelope;
- one Raw-to-Linear compositor call in that envelope;
- zero clipping-rule calls in the envelope;
- one clipping-rule derivation inside the compositor from the already supplied normalization report.

Do not add a second parser, source read, hash, ledger, Raw-IFD detector, normalization route, Linearization route or pixel route.

## Clipping authority status

The current clipping-rule candidate is only a DNG processing/specification dependency. Keep separate:

- DNG post-normalization clipping rule;
- WhiteLevel metadata/encoding boundary;
- observed sample censoring;
- physical sensor saturation/full-well/scene-radiance censoring;
- Appearance/output white and highlight roll-off.

No existing Deep-Censor, TruthRange or Appearance code may be treated as Source Capability clipping authority merely because its name or visual role is related.

## Hard false state

Even when the four-stage metadata ordering validates for a qualifying IFD:

`full_raw_to_linear_mapping_validated=false`

and all sample application, observed-censor, physical-saturation, admission/use, pixel, Scientific Master, Appearance, mutation, new-evidence and writeback effects remain false.

## Historical documents

All older dated state files, status indexes and handoffs remain provenance. In particular:

- `docs/DOCUMENT_STATUS_INDEX_2026-10-06.md` is historical and must not be interpreted as current PR #131 authority;
- `state/CURRENT_PROJECT_STATE_2026-10-02.json` remains historical 44488 provenance;
- `docs/handoff/START_HERE_NEW_CHAT_LEGACY_2026-10-06.md` remains the preserved old bootstrap;
- the master capsule's `062a1dd58...` checkpoint remains valid historical evidence for the earlier three-stage-plus-unresolved-clipping state, but the newer 2026-10-13 delta is the current raw-to-linear continuation.

## Immediate next task

Perform a read-only sample-domain execution audit before adding any new pixel processing. Trace actual existing execution for LinearizationTable, effective-black subtraction, normalization and clipping, and reuse existing routes wherever they already exist.
