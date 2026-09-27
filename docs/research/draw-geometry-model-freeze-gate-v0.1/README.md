# D.RAW Geometry Model Freeze Gate v0.1

Status: **HOST GATE FOUNDATION — NO REAL GEOMETRY MODEL PROMOTED**

This layer sits after Geometry Capture-Set Gate v0.1 and before final HOLDOUT
scoring.

Its purpose is reproducibility, not ontology.

> **Freeze one test candidate, not scientific thought.**

A complete capture set opens TRAINING-only model selection. This gate then
binds one selected geometry candidate, its training-only decision record and
its pre-declared HOLDOUT acceptance policy into one immutable test candidate.

After this gate, final HOLDOUT scoring may begin for that candidate.

The gate does **not**:

- claim the selected model is the final or uniquely correct lens model;
- admit a GEOMETRY relation;
- grant coordinate-transform authority;
- grant fusion;
- transfer calibration;
- rewrite source evidence, pair membership or the capture set.

## Required input state

The capture set must be:

`CAPTURE_SET_COMPLETE_TRAINING_MODEL_SELECTION_OPEN`

and must contain valid deterministic TRAINING and HOLDOUT set identities.

The training selection record must be:

`TRAINING_ONLY_SELECTION_COMPLETE`

and must bind the exact capture set.

HOLDOUT measurements or metrics may not have been accessed during model
selection.

## Candidate model vocabulary is open

Model families are strings, not a closed scientific enum.

A candidate can therefore describe, for example, a pinhole, fisheye,
omnidirectional, spline, learned correction field or a future model family,
provided the exact model specification and parameters are content-addressed.

The gate requires a stable identity for:

- model specification;
- fitted parameters;
- training result;
- training metric summary;
- validity domain.

It does not prescribe the internal mathematics of those artifacts.

## Pre-declared HOLDOUT policy

Before HOLDOUT is opened, the selection record must also bind:

- a HOLDOUT scoring policy;
- acceptance thresholds.

These receive immutable SHA-256 identities for this one candidate test.

After freeze, the candidate cannot use HOLDOUT to:

- refit parameters;
- change model family;
- change hyperparameters;
- change the selected candidate;
- alter acceptance thresholds.

If a scientist wants to change any of those, the current candidate remains
historical provenance and a **new versioned candidate** must be created and
tested against an appropriate holdout policy.

## Output state

A valid freeze becomes:

`MODEL_CANDIDATE_FROZEN_HOLDOUT_SCORING_OPEN`

At that point:

- final HOLDOUT scoring is allowed for this candidate;
- model refit/change is forbidden inside this candidate;
- threshold changes are forbidden inside this candidate;
- GEOMETRY relation remains UNKNOWN;
- coordinate-transform authority remains false;
- fusion remains false.

A later held-out validation/certificate gate is required before any graph
relation can become ADMITTED.

## Open-world law

The frozen scope is intentionally narrow:

`ONE_REPRODUCIBLE_CERTIFICATE_CANDIDATE`

The project remains free to create future successors or competing model
families from new evidence or a separately governed experiment.

Sealed evidence protects history. It does not terminate scientific inquiry.
