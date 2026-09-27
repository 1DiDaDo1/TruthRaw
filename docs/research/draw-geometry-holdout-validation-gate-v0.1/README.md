# D.RAW Geometry HOLDOUT Validation Gate v0.1

Status: **HOST GATE FOUNDATION — NO REAL GEOMETRY CANDIDATE EVALUATED**

This layer sits after Geometry Model Freeze Gate v0.1.

Its task is to evaluate one frozen geometry candidate against the complete
sealed HOLDOUT subset using the scoring policy and acceptance thresholds that
were frozen before HOLDOUT was opened.

It does not fit, refit, tune or reinterpret the candidate.

## Core law

> **HOLDOUT may judge a frozen candidate; it may not redesign it.**

The gate accepts both PASS and FAIL as scientific outcomes.

PASS becomes:

`HOLDOUT_VALIDATED_CANDIDATE_AWAITING_ENDPOINT_APPLICABILITY`

FAIL becomes:

`HOLDOUT_REJECTED_CANDIDATE`

Both outcomes remain provenance.

## Exact HOLDOUT coverage

The gate requires the original capture set in addition to the model-freeze
artifact.

It therefore verifies that the evaluation covers every sealed HOLDOUT
pair-state identity exactly once.

A partial HOLDOUT evaluation cannot pass.

A training pair cannot be substituted for a HOLDOUT pair.

## Frozen-policy binding

The evaluation must bind exactly the frozen:

- candidate ID;
- HOLDOUT set identity;
- scoring-policy identity;
- acceptance-threshold identity.

During HOLDOUT:

- model refit is forbidden;
- model-family change is forbidden;
- parameter/hyperparameter change is forbidden;
- threshold change is forbidden;
- candidate substitution is forbidden.

Changing any of those creates a new experiment/candidate. It cannot mutate the
current frozen test.

## PASS is not a graph relation

A PASS means only that this exact frozen candidate passed its predeclared
HOLDOUT policy for this exact sealed HOLDOUT set.

PASS does **not** yet grant:

- GEOMETRY relation ADMITTED;
- coordinate-transform authority for arbitrary observations;
- calibration transfer;
- fusion;
- any non-geometry relation axis.

A later endpoint-applicability/certificate layer must prove that the validated
candidate applies to the exact observations that a graph relation intends to
connect.

## FAIL remains knowledge

A rejected candidate is not deleted or rewritten. It records:

- which frozen candidate was tested;
- which HOLDOUT set judged it;
- which scoring/threshold policy applied;
- which residual/metric artifacts were produced.

A successor candidate may later be created without modifying the failed
candidate or its evidence ancestry.

## Open-world law

A HOLDOUT result is final only for **that exact candidate test**.

It is not a claim that the model family is forever true or false.

**Seal the evidence and the test outcome, not scientific thought.**
