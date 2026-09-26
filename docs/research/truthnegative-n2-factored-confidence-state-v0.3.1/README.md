# TruthNegative N2 Factored Confidence State v0.3.1

Status: **FACTORED RESEARCH STATE — AUDIT ONLY**

## Motivation

The v0.3 whole-frame device result showed that a single strict
FULLY_COHERENT class is too coarse to describe real photographic data.

The useful information is already present as independent facts:

- whether candidates exist;
- whether every candidate has a center-excluded predictor;
- whether any valid predictor has a center-only residual above 2 sigma;
- whether any symmetric pair was rejected;
- whether any spatial scale was rejected;
- whether structure, censor or censor-boundary protection is present;
- whether the maximum predictor-variance estimate exceeds the center-variance
  estimate.

v0.3.1 keeps those facts separate.

## No threshold relaxation

This version does not replace zero-rejection rules with 90%, 95% or any other
empirical threshold.

It also does not assign weights or collapse the facts into a scalar
probability.

The booleans are exact logical statements over already measured v0.3
quantities. The max-predictor-variance-le-center-variance flag is only the
literal comparison of the recorded ratio with 1.0. It is not a denoise
criterion and does not assume noise independence.

## Legacy class

The v0.3 support class is retained only for backwards comparison.

legacy_class_non_authoritative=true

FULLY_COHERENT therefore remains available as a historical strict label, but
v0.3.1 does not require it to describe useful support facts.

## Device oracle from the 2026-09-26 Camera-5 audit

For the admitted 4080x3072 device run that motivated this module, independent
recalculation from the v0.1 and v0.2.1 sidecars predicts:

- total tiles: 3072
- has candidate tiles: 3055
- all candidates predictable: 203
- center outlier free: 126
- all candidates predictable AND center outlier free: 28
- pair rejection free: 0
- scale rejection free: 0
- structure protection present: 3066
- censor protection present: 178
- censor-boundary protection present: 177
- max predictor variance <= center variance: 3039

These are an external device oracle, not hardcoded algorithm outputs.

## Binding

The factored state is cryptographically bound to the complete preceding chain:

Source Evidence -> Scientific Master -> authority field -> TruthNegative state
-> v0.1 candidate/audit/spatial -> v0.2.1 center-excluded audit
-> v0.3 Confidence Field -> v0.3.1 Factored Confidence State.

A mismatched v0.3 Confidence Field SHA fails closed.

## Permanent invariants

- Direct CFA immutable;
- Scientific Master immutable;
- TruthNegative immutable;
- v0.1 candidate unchanged;
- v0.2.1 predictor unchanged;
- v0.3 Confidence Field unchanged;
- no scalar confidence probability;
- support_distance_admitted=false;
- promotion_eligible=false;
- candidate_applied=false;
- creates_new_evidence=false;
- scientific_writeback_allowed=false.
