# TruthRaw v0.9 — Virtual Observation Manifold implementation report

## Decision

**RESTORE_EARLY_MULTI_EV_IDEA_AS_SHARED_EVIDENCE_FIRST_CLASS_LAYER**

The early Multi-EV / virtual-camera architecture is compatible with the current TruthRange model when evidence multiplicity is explicitly prevented.

## Implemented

- deterministic exposure nodes over signed camera-scene values;
- TruthRange shift identity `T' = T + exposureEV`;
- gain/virtual-ISO encoding nodes that do not move scene TruthRange;
- exact covariance scale transform through the closed v0.6 covariance API;
- normalized multi-EV weights summing to one per evaluated signal;
- immutable evidence-root identity across all views;
- hard `1 physical frame / 1 independent evidence` ledger;
- optional source-bound virtual-sensor likelihood model with fail-closed authority checks;
- saturation represented as censor/lower-bound state in virtual sensor prediction.

## Not claimed

- virtual views are not new captures;
- virtual EV/ISO does not improve intrinsic source SNR;
- no `sqrt(N)` uncertainty reduction is permitted from view count;
- a nominal ISO value alone does not define physical sensor noise;
- the current module does not certify the open HONOR electron/PTC calibration;
- virtual DNG files produced later remain compatibility projections, not additional sealed evidence houses.

## Local validation

Strict local compilation/tests passed under:

- GCC Release with `-Wall -Wextra -Wpedantic -Werror`;
- Clang Release with the same warning policy;
- GCC AddressSanitizer + UndefinedBehaviorSanitizer.

Repository closure is only allowed after the candidate builds against the real closed v0.2/v0.6 dependency chain and the existing canonical regressions remain green.
