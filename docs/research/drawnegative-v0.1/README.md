# D.RAWnegative v0.1

Status: **NEW OFFICIAL SCIENTIFIC-NEGATIVE IDENTITY — TRUTHNEGATIVE RETAINED AS LEGACY/COMPATIBILITY ANCESTRY**

## Decision

From this version forward, the public D.RAW scientific-negative concept is:

**D.RAWnegative**

Historical `TruthNegative` names remain in:

- sealed historical documentation;
- legacy schemas and wire/container identities;
- stable Android/JNI/package/class symbols;
- existing test vectors and hashes;
- existing `.tnc` native container compatibility;
- internal implementation ancestry where renaming would break reproducibility.

No historical TruthNegative byte or state identity is rewritten.

## Definition

> **D.RAWnegative is the raster-independent, observation-bound, authority-aware scientific negative of one admitted D.RAW observation lineage.**

It binds:

- one sealed Source Evidence root;
- one Scientific Master;
- one local authority field;
- one historical TruthNegative Continuous parent state;
- one D.RAW Observation ID;
- one Zero-Line / TruthRange gauge identity;
- optional admitted shared Free-World gauge relation;
- precision contract;
- provenance ancestry.

## Relationship to TruthNegative

TruthNegative Continuous v0.5 remains the validated computational parent.

D.RAWnegative v0.1 creates a new identity **above** that parent:

```text
sealed Source Evidence
 -> Scientific Master
 -> Open Scene / local authority
 -> TruthNegative Continuous v0.5       [legacy validated parent]
 -> D.RAWnegative v0.1                  [new official identity]
 -> Free World Observation Graph
 -> View / Appearance / output
```

The parent state SHA-256 is incorporated into the D.RAWnegative SHA-256.

Therefore D.RAWnegative does not erase TruthNegative history; it cryptographically
inherits it.

## Observation law

One D.RAWnegative belongs to one admitted observation lineage.

A target raster is not part of its identity.

Resampling never creates measured target pixels.

Multiple lenses do not silently merge into one D.RAWnegative. Main,
ultra-wide, telephoto and other cameras each get their own D.RAWnegative until
the Free World Observation Graph relates them.

## Zero-Line / gauge law

The coordinate family remains:

`T = log2(L/L0)`

Every D.RAWnegative requires a non-empty `scaleGaugeId`.

Source-local gauge:

- shared Free-World gauge ID must be absent;
- cross-observation radiometric equality = false;
- cross-observation radiometric fusion = false.

Shared relative/absolute gauge:

- shared Free-World gauge ID is mandatory;
- equality/fusion may be enabled by the admitted relation.

The module does not itself prove a shared gauge. It only refuses contradictory
state.

## Precision law

Branch-sensitive scientific computation remains Float64.

Canonical state storage may be validated Float32 or Float64.

Precision never upgrades authority.

## TruthRange materialization boundary

v0.1 declares the TruthRange coordinate family but does **not** pretend that a
per-sample TruthRange field has already been materialized.

`perSampleTruthRangeMaterialized=false` is therefore a valid and expected
initial state.

A future version may add that field only with explicit bounds/uncertainty
semantics.

## No scientific writeback

Permanent:

- `createsNewEvidence=false`;
- `scientificWritebackAllowed=false`;
- appearance cannot mutate D.RAWnegative;
- D.RAWnegative cannot mutate its sealed source or Scientific Master.

## Public naming

New user-facing text should say **D.RAWnegative**.

Historical/internal names may continue to say TruthNegative when required for
compatibility. They are not a second scientific object.

## Short law

> **D.RAWnegative is the developed scientific negative of one sealed D.RAW observation; richer than the source representation, never stronger than the evidence.**
