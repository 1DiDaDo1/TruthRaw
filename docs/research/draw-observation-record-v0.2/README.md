# D.RAW Observation Record v0.2 — D.RAWnegative

Status: **CURRENT OBSERVATION-RECORD SUCCESSOR**

v0.1 remains historical provenance.

v0.2 makes the current scientific-negative identity explicit:

- `legacy_truthnegative_parent` = validated TruthNegative Continuous ancestry;
- `drawnegative` = current D.RAWnegative state.

The record does not rewrite the parent state.

## Required relation

```text
sealed Source Evidence
 -> Scientific Master
 -> authority field
 -> legacy TruthNegative parent
 -> D.RAWnegative
 -> Free World Observation Graph
```

The D.RAWnegative state SHA is recomputable from:

- parent TruthNegative SHA-256;
- Observation ID;
- scale-gauge ID;
- optional shared Free-World gauge ID;
- gauge relation;
- canonical storage;
- TruthRange declaration/materialization flags.

The v0.2 validator recomputes the state digest and fails closed on mismatch.

## Lamp-scene migration

The existing Camera-5 lamp-scene observation is migrated without changing its
physical evidence.

Existing parent TruthNegative state:

`73c908b06d42b57d07461c8d3d6aa84f7496e041ac87c8ce7170479ca3c002c5`

D.RAWnegative v0.1 state derived from that parent and the source-local gauge:

`7fabfd66dd9c2dd334ffe7798e34fd63e01e49d344b67ff455ad919bac13e9a5`

This is a new derived state identity, not new evidence.

## Fail-closed gauge rule

The current lamp-scene record is source-local:

- no shared Free-World gauge;
- cross-observation radiometric equality = false;
- cross-observation radiometric fusion = false.

Main and ultra-wide must receive independent records and independent local
gauges until a separate calibration relation is admitted.
