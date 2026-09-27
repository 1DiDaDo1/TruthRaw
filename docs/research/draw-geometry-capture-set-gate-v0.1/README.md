# D.RAW Geometry Capture-Set Gate v0.1

Status: **CAPTURE-SET GATE CANDIDATE**

This layer sits above Geometry Direct RAW Pair Intake v0.1.

Its job is narrow:

- collect admission-bound MAIN + ULTRA_WIDE pose pairs;
- preserve TRAINING/HOLDOUT separation;
- prove minimum pair counts;
- seal which evidence belongs to which subset;
- open training-only model selection.

It does **not** choose a projection model and it does not admit a geometry
relation.

## Open-world rule

> **Seal the evidence split, not the model hypothesis.**

The capture set may seal:

- target identity;
- member pair identities;
- TRAINING membership;
- HOLDOUT membership;
- deterministic set hashes.

It does not seal:

- pinhole vs fisheye vs omnidirectional;
- distortion model family;
- feature detector;
- robust loss;
- optimizer;
- relation threshold;
- final scientific interpretation.

Those belong to later versioned model-selection/certificate layers.

## Input

Every pair must already be:

`ADMISSION_BOUND_PAIR_READY_FOR_FEATURE_EXTRACTION`

from Geometry Direct RAW Pair Intake v0.1.

The gate also reads the current Geometry Relation Campaign contract for the
minimum required counts.

For the current MAIN ↔ ULTRA_WIDE campaign:

- training minimum: 12;
- holdout minimum: 4.

## Output state

A complete set becomes:

`CAPTURE_SET_COMPLETE_TRAINING_MODEL_SELECTION_OPEN`

This means:

- training feature extraction allowed;
- training model selection allowed;
- holdout feature extraction may be prepared;
- holdout may **not** influence model selection;
- final holdout scoring remains closed until a later model-freeze artifact
  exists.

Even a complete capture set still has:

- GEOMETRY relation = UNKNOWN;
- coordinate transform authority = false;
- fusion = false;
- calibration transfer = false.

## Membership law

All member pairs must:

- belong to the same campaign;
- bind the same metric target geometry identity;
- have unique pose IDs;
- have unique MAIN source hashes;
- have unique ULTRA_WIDE source hashes;
- preserve disjoint TRAINING/HOLDOUT membership;
- remain source-local admitted.

A source capture may not silently appear in both training and holdout.

## Deterministic identities

The gate emits:

- training-set SHA-256;
- holdout-set SHA-256;
- complete capture-set SHA-256.

Set identities are calculated from canonical sorted pair-state identities, so
file ordering does not alter the scientific set identity.
